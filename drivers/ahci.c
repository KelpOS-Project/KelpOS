/* drivers/ahci.c — minimal AHCI (SATA) block driver for KelpOS.
 *
 * One command slot and one PRDT entry per port: transfers up to 64 KiB.
 * LBA48 commands are used, so large disks work for the low sectors we
 * touch. The driver is deliberately small: no hotplug, no NCQ, no
 * interrupts (polled completion), no ATAPI.
 *
 * DMA buffers come from the bump heap, whose addresses are physical
 * (Limine maps RAM identity), so no virt->phys translation is needed.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "io.h"
#include "pci.h"
#include "block.h"
#include "heap.h"
#include "ahci.h"

#define HBA_CAP   0x00
#define HBA_GHC   0x04
#define HBA_PI    0x0C

#define PX_CLB    0x00
#define PX_CLBU   0x04
#define PX_FB     0x08
#define PX_FBU    0x0C
#define PX_IS     0x10
#define PX_CMD    0x18
#define PX_TFD    0x20
#define PX_SIG    0x24
#define PX_SSTS   0x28
#define PX_SERR   0x30
#define PX_CI     0x38

#define CMD_ST    (1u << 0)
#define CMD_SUD   (1u << 1)
#define CMD_POD   (1u << 2)
#define CMD_FRE   (1u << 4)
#define CMD_FR    (1u << 14)
#define CMD_CR    (1u << 15)
#define GHC_AE    (1u << 31)

#define TFD_BSY   (1u << 7)
#define TFD_DRQ   (1u << 3)
#define TFD_ERR   (1u << 0)

#define SATA_SIG_ATA 0x00000101u

#define MAX_PORTS     32
#define DMA_MAX       65536u   /* one PRDT entry */

struct ahci_port {
    volatile uint8_t *regs;
    uint8_t *clb;      /* command list (1 KiB, 1 KiB aligned) */
    uint8_t *fb;       /* FIS receive area (256 B) */
    uint8_t *ct;       /* command table (128 B + PRDT) */
    uint8_t *dma;      /* bounce buffer, DMA_MAX bytes */
    uint64_t sectors;
    int used;
};

static volatile uint8_t *hba;
static struct ahci_port ports[MAX_PORTS];

static int port_flush(struct ahci_port *p);

static uint32_t hba_rd(uint32_t off) { return *(volatile uint32_t *)(hba + off); }
static void hba_wr(uint32_t off, uint32_t v) { *(volatile uint32_t *)(hba + off) = v; }

static uint32_t px_rd(struct ahci_port *p, uint32_t off)
{
    return *(volatile uint32_t *)(p->regs + off);
}
static void px_wr(struct ahci_port *p, uint32_t off, uint32_t v)
{
    *(volatile uint32_t *)(p->regs + off) = v;
}

static void *alloc_aligned(size_t n, size_t align)
{
    uintptr_t raw = (uintptr_t)kmalloc(n + align);
    if (!raw)
        return 0;
    return (void *)((raw + align - 1) & ~(uintptr_t)(align - 1));
}

static int wait_clear(volatile uint32_t *reg, uint32_t mask, int loops)
{
    while (loops-- > 0) {
        if ((*reg & mask) == 0)
            return 0;
        __asm__ volatile("pause");
    }
    return -1;
}

static int port_start(struct ahci_port *p)
{
    /* stop the port before reprogramming the DMA areas */
    uint32_t cmd = px_rd(p, PX_CMD);
    cmd &= ~(uint32_t)(CMD_ST | CMD_FRE);
    px_wr(p, PX_CMD, cmd);
    if (wait_clear((volatile uint32_t *)(p->regs + PX_CMD), CMD_CR, 1000000) < 0)
        return -1;

    px_wr(p, PX_CLB, (uint32_t)(uintptr_t)p->clb);
    px_wr(p, PX_CLBU, (uint32_t)((uint64_t)(uintptr_t)p->clb >> 32));
    px_wr(p, PX_FB, (uint32_t)(uintptr_t)p->fb);
    px_wr(p, PX_FBU, (uint32_t)((uint64_t)(uintptr_t)p->fb >> 32));

    px_wr(p, PX_IS, 0xFFFFFFFFu);
    px_wr(p, PX_SERR, 0xFFFFFFFFu);
    px_wr(p, PX_CMD, px_rd(p, PX_CMD) | CMD_FRE | CMD_ST);
    return 0;
}

static void port_stop(struct ahci_port *p)
{
    uint32_t cmd = px_rd(p, PX_CMD);
    px_wr(p, PX_CMD, cmd & ~(uint32_t)CMD_ST);
    wait_clear((volatile uint32_t *)(p->regs + PX_CMD), CMD_FR, 1000000);
    px_wr(p, PX_CMD, px_rd(p, PX_CMD) & ~(uint32_t)CMD_FRE);
    wait_clear((volatile uint32_t *)(p->regs + PX_CMD), CMD_CR, 1000000);
}

/* Build a Register H2D FIS and start one command. `write` selects the
 * direction, `count` is a sector count (1..128). */
static int port_cmd(struct ahci_port *p, uint8_t command, int write,
                    uint64_t lba, uint16_t count, void *buf)
{
    if (count == 0 || count > 128)
        return -1;

    if (wait_clear((volatile uint32_t *)(p->regs + PX_TFD),
                   TFD_BSY | TFD_DRQ, 1000000) < 0)
        return -1;

    memset(p->clb, 0, 1024);
    memset(p->ct, 0, 128 + 16 * 16);
    memset(p->fb, 0, 256);

    uint8_t *fis = p->ct;
    fis[0] = 0x27;                 /* host-to-device FIS */
    fis[1] = 0x80;                 /* command */
    fis[2] = command;
    fis[4] = (uint8_t)(lba);
    fis[5] = (uint8_t)(lba >> 8);
    fis[6] = (uint8_t)(lba >> 16);
    fis[7] = 0x40;                 /* LBA mode */
    fis[8] = (uint8_t)(lba >> 24);
    fis[9] = (uint8_t)(lba >> 32);
    fis[10] = (uint8_t)(lba >> 40);
    fis[12] = (uint8_t)count;
    fis[13] = (uint8_t)(count >> 8);

    uint8_t *prdt = p->ct + 128;
    uint32_t bytes = (uint32_t)count * 512u;
    uint64_t dba = (uint64_t)(uintptr_t)buf;
    prdt[0] = (uint8_t)dba;
    prdt[1] = (uint8_t)(dba >> 8);
    prdt[2] = (uint8_t)(dba >> 16);
    prdt[3] = (uint8_t)(dba >> 24);
    prdt[4] = (uint8_t)(dba >> 32);
    prdt[5] = (uint8_t)(dba >> 40);
    prdt[6] = (uint8_t)(dba >> 48);
    prdt[7] = (uint8_t)(dba >> 56);
    uint32_t dbc = bytes - 1u;
    prdt[12] = (uint8_t)dbc;
    prdt[13] = (uint8_t)(dbc >> 8);
    prdt[14] = (uint8_t)(dbc >> 16);
    prdt[15] = (uint8_t)(dbc >> 24);

    uint32_t *hdr = (uint32_t *)p->clb;
    hdr[0] = 5u | (write ? (1u << 6) : 0) | (1u << 16); /* 1 PRDT entry */
    hdr[1] = 0;
    hdr[2] = (uint32_t)(uintptr_t)p->ct;
    hdr[3] = (uint32_t)((uint64_t)(uintptr_t)p->ct >> 32);

    px_wr(p, PX_IS, 0xFFFFFFFFu);
    px_wr(p, PX_CI, 1u);

    for (int i = 0; i < 2000000; i++) {
        uint32_t ci = px_rd(p, PX_CI);
        uint32_t is = px_rd(p, PX_IS);
        uint32_t tfd = px_rd(p, PX_TFD);
        if (is & (1u << 30)) /* task file error */
            return -1;
        if ((ci & 1u) == 0 && (tfd & TFD_BSY) == 0)
            return (tfd & TFD_ERR) ? -1 : 0;
        __asm__ volatile("pause");
    }
    return -1;
}

static int port_identify(struct ahci_port *p, uint64_t *sectors)
{
    if (port_cmd(p, 0xEC, 0, 0, 1, p->dma) < 0)
        return -1;
    uint16_t *w = (uint16_t *)p->dma;
    uint64_t lba28 = (uint32_t)w[60] | ((uint32_t)w[61] << 16);
    uint64_t lba48 = (uint64_t)((uint32_t)w[100] | ((uint32_t)w[101] << 16)) |
                     ((uint64_t)((uint32_t)w[102] |
                                 ((uint32_t)w[103] << 16)) << 32);
    *sectors = lba48 ? lba48 : lba28;
    return *sectors ? 0 : -1;
}

/* ── block device glue ─────────────────────────────────────────────── */

struct ahci_bd {
    struct blockdev bd;
    struct ahci_port *port;
    char name[8];
};

static int bd_read(struct blockdev *bd, uint64_t lba, uint32_t count, void *buf)
{
    struct ahci_port *p = ((struct ahci_bd *)bd)->port;
    uint8_t *dst = buf;
    if (!count)
        return BLOCK_ERR_OK;
    if (lba + count > p->sectors)
        return BLOCK_ERR_RANGE;
    while (count > 0) {
        uint16_t n = count > 128 ? 128 : (uint16_t)count;
        if (port_cmd(p, 0x25, 0, lba, n, p->dma) < 0)
            return BLOCK_ERR_IO;
        memcpy(dst, p->dma, (size_t)n * 512);
        dst += (size_t)n * 512;
        lba += n;
        count -= n;
    }
    return BLOCK_ERR_OK;
}

static int bd_write(struct blockdev *bd, uint64_t lba, uint32_t count,
                    const void *buf)
{
    struct ahci_port *p = ((struct ahci_bd *)bd)->port;
    const uint8_t *src = buf;
    if (!count)
        return BLOCK_ERR_OK;
    if (lba + count > p->sectors)
        return BLOCK_ERR_RANGE;
    while (count > 0) {
        uint16_t n = count > 128 ? 128 : (uint16_t)count;
        memcpy(p->dma, src, (size_t)n * 512);
        if (port_cmd(p, 0x35, 1, lba, n, p->dma) < 0)
            return BLOCK_ERR_IO;
        src += (size_t)n * 512;
        lba += n;
        count -= n;
    }
    return BLOCK_ERR_OK;
}

static int bd_flush(struct blockdev *bd)
{
    struct ahci_port *p = ((struct ahci_bd *)bd)->port;
    return port_flush(p) < 0 ? BLOCK_ERR_IO : BLOCK_ERR_OK;
}

/* 0xEA FLUSH CACHE EXT carries no data (no PRDT, no sector count). */
static int port_flush(struct ahci_port *p)
{
    if (wait_clear((volatile uint32_t *)(p->regs + PX_TFD),
                   TFD_BSY | TFD_DRQ, 1000000) < 0)
        return -1;
    memset(p->clb, 0, 1024);
    memset(p->ct, 0, 128 + 16 * 16);
    uint8_t *fis = p->ct;
    fis[0] = 0x27;
    fis[1] = 0x80;
    fis[2] = 0xEA;
    uint32_t *hdr = (uint32_t *)p->clb;
    hdr[0] = 5u | (1u << 16);
    hdr[2] = (uint32_t)(uintptr_t)p->ct;
    hdr[3] = (uint32_t)((uint64_t)(uintptr_t)p->ct >> 32);
    px_wr(p, PX_IS, 0xFFFFFFFFu);
    px_wr(p, PX_CI, 1u);
    for (int i = 0; i < 2000000; i++) {
        uint32_t ci = px_rd(p, PX_CI);
        uint32_t tfd = px_rd(p, PX_TFD);
        uint32_t is = px_rd(p, PX_IS);
        if (is & (1u << 30))
            return -1;
        if ((ci & 1u) == 0 && (tfd & TFD_BSY) == 0)
            return (tfd & TFD_ERR) ? -1 : 0;
        __asm__ volatile("pause");
    }
    return -1;
}

int ahci_flush_all(void)
{
    int rc = 0;
    for (int i = 0; i < MAX_PORTS; i++)
        if (ports[i].used && port_flush(&ports[i]) < 0)
            rc = -1;
    return rc;
}

int ahci_present(void)
{
    for (int i = 0; i < MAX_PORTS; i++)
        if (ports[i].used)
            return 1;
    return 0;
}

/* ── init ──────────────────────────────────────────────────────────── */

static int disk_no;

static int init_port(struct ahci_port *p, uint32_t portno)
{
    p->regs = hba + 0x100 + portno * 0x80;
    uint32_t ssts = px_rd(p, PX_SSTS);
    if ((ssts & 0x0F) != 3)        /* device detected + phy ready */
        return -1;
    if (px_rd(p, PX_SIG) != SATA_SIG_ATA)
        return -1;

    p->clb = alloc_aligned(1024, 1024);
    p->fb = alloc_aligned(256, 256);
    p->ct = alloc_aligned(512, 128);
    p->dma = alloc_aligned(DMA_MAX, 16);
    if (!p->clb || !p->fb || !p->ct || !p->dma)
        return -1;

    if (port_start(p) < 0)
        return -1;
    if (port_identify(p, &p->sectors) < 0) {
        port_stop(p);
        return -1;
    }

    struct ahci_bd *d = alloc_aligned(sizeof(*d), 16);
    if (!d)
        return -1;
    if (disk_no < 26) {
        d->name[0] = 's';
        d->name[1] = 'd';
        d->name[2] = (char)('a' + disk_no);
        d->name[3] = 0;
    } else {
        snprintf(d->name, sizeof(d->name), "sd%d", disk_no);
    }
    d->bd.name = d->name;
    d->bd.blocks = p->sectors;
    d->bd.read = bd_read;
    d->bd.write = bd_write;
    d->bd.flush = bd_flush;
    d->bd.ctx = p;
    d->port = p;
    if (block_register(&d->bd) < 0)
        return -1;

    p->used = 1;
    disk_no++;
    return 0;
}

int ahci_init(void)
{
    uint8_t bus, slot, func;
    if (pci_find_class(PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_SATA, 1,
                       &bus, &slot, &func) < 0)
        return -1;

    uintptr_t abar = pci_bar_mmio(bus, slot, func, 5);
    if (!abar)
        return -1;
    hba = (volatile uint8_t *)abar;

    if ((hba_rd(HBA_CAP) & 0xFFFF) == 0xFFFF)
        return -1; /* BAR did not decode */

    hba_wr(HBA_GHC, hba_rd(HBA_GHC) | GHC_AE);

    uint32_t pi = hba_rd(HBA_PI);
    for (uint32_t i = 0; i < MAX_PORTS; i++)
        if (pi & (1u << i))
            init_port(&ports[i], i);

    if (!ahci_present())
        return -1;
    kprintf("ahci: %d disk(s) on PCI %02x:%02x.%x\n", disk_no, bus, slot, func);
    return 0;
}
