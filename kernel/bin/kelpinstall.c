/* kernel/bin/kelpinstall.c — install KelpOS onto the first SATA disk.
 *
 * The installer is part of the running system: no flags, no prompts, no
 * dry run. The ISO carries a complete, bootable disk image
 * (/boot/install.img, built by scripts/geninstallimg.sh) as a Limine
 * module; this command streams that image to the first AHCI disk
 * sector by sector and flushes the cache. Reboot afterwards and the
 * machine starts from the hard disk.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "commands.h"
#include "block.h"
#include "console.h"
#include "ahci.h"
#include "part.h"

extern const void *boot_module(const char *needle, size_t *size);

#define CHUNK_SECTORS 128u   /* 64 KiB */

static struct blockdev *install_target(uint64_t need_sectors)
{
    for (size_t i = 0; i < block_count(); i++) {
        struct blockdev *bd = block_get(i);
        if (!bd || !bd->write || bd->blocks < need_sectors)
            continue;
        /* only whole disks (sda, sdb, ...), never partitions (sda1) */
        if (strncmp(bd->name, "sd", 2) != 0 || strlen(bd->name) != 3)
            continue;
        return bd;
    }
    return 0;
}

int cmd_kelpinstall(int argc, char **argv)
{
    (void)argv;
    if (argc > 1) {
        kprintf("kelpinstall: this command takes no options\n");
        return 1;
    }

    size_t size = 0;
    const uint8_t *img = boot_module("install.img", &size);
    if (!img || size < 512 || (size & 511)) {
        kprintf("kelpinstall: install image not found "
                "(boot the ISO with boot/install.img as a module)\n");
        return 1;
    }

    uint64_t sectors = size / 512;
    struct blockdev *bd = install_target(sectors);
    if (!bd) {
        if (!ahci_present())
            kprintf("kelpinstall: no SATA disk found\n");
        else
            kprintf("kelpinstall: no SATA disk with at least %llu MiB free "
                    "for the install image\n",
                    (unsigned long long)((sectors + 2047) / 2048));
        kprintf("kelpinstall: run 'lsblk' to list the block devices\n");
        return 1;
    }

    uint64_t target_mib = bd->blocks / 2048;
    uint64_t image_mib = (sectors + 2047) / 2048;
    kprintf("kelpinstall: writing KelpOS %llu MiB to %s (%llu MiB)\n",
            (unsigned long long)image_mib, bd->name,
            (unsigned long long)target_mib);
    kprintf("kelpinstall: this overwrites the first %llu MiB of %s\n",
            (unsigned long long)image_mib, bd->name);

    uint8_t *buf = malloc(CHUNK_SECTORS * 512u);
    if (!buf) {
        kprintf("kelpinstall: out of memory\n");
        return 1;
    }

    uint64_t done = 0;
    int last_pct = -1;
    while (done < sectors) {
        uint32_t n = (uint32_t)(sectors - done);
        if (n > CHUNK_SECTORS)
            n = CHUNK_SECTORS;
        memcpy(buf, img + done * 512u, (size_t)n * 512u);
        if (bd->write(bd, done, n, buf) != BLOCK_ERR_OK) {
            kprintf("\nkelpinstall: write error at sector %llu\n",
                    (unsigned long long)done);
            free(buf);
            return 1;
        }
        done += n;
        int pct = (int)((done * 100) / sectors);
        if (pct / 10 != last_pct / 10 || pct == 100) {
            kprintf("kelpinstall: %3d%% (%llu/%llu MiB)\n", pct,
                    (unsigned long long)(done / 2048),
                    (unsigned long long)image_mib);
            last_pct = pct;
        }
    }

    if (bd->flush)
        bd->flush(bd);
    free(buf);

    /* make the new partition table visible in this session (sda1, ...) */
    part_scan(bd);

    /* read back the MBR: proves the data actually reached the disk */
    uint8_t verify[512];
    if (bd->read(bd, 0, 1, verify) != BLOCK_ERR_OK ||
        memcmp(verify, img, sizeof(verify)) != 0) {
        kprintf("kelpinstall: verification failed (read-back mismatch)\n");
        return 1;
    }

    kprintf("kelpinstall: done - KelpOS installed on %s\n", bd->name);
    kprintf("kelpinstall: remove the CD and reboot to start from the disk\n");
    return 0;
}
