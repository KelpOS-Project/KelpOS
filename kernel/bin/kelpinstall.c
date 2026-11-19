/* kernel/bin/kelpinstall.c — install KelpOS onto a SATA disk.
 *
 * The installer is part of the running system. The ISO carries a
 * complete, bootable disk image (/boot/install.img, built by
 * scripts/geninstallimg.sh) as a Limine module; the installer streams
 * that image to a disk sector by sector, flushes the cache, rescans the
 * partition table and verifies the read-back MBR.
 *
 * On the framebuffer terminal it runs a full-screen menu-driven UI in
 * the spirit of Linux `make menuconfig` (target disk, fixed layout
 * options, one confirmation, progress bar). Serial-only machines fall
 * back to the plain non-interactive install: no flags, ever.
 */
#include <stdbool.h>
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
#include "tui.h"
#include "version.h"

extern const void *boot_module(const char *needle, size_t *size);

#define CHUNK_SECTORS 128u /* 64 KiB per write */
#define MAX_DISKS     8

/* ── block device helpers ──────────────────────────────────────────── */

/* whole disks only: "sda", "sdb", ... (never partitions like "sda1") */
static bool is_disk(const struct blockdev *bd)
{
    return bd && strlen(bd->name) == 3 && bd->name[0] == 's' &&
           bd->name[1] == 'd';
}

static size_t collect_disks(struct blockdev **out, size_t max)
{
    size_t n = 0;
    for (size_t i = 0; i < block_count() && n < max; i++) {
        struct blockdev *bd = block_get(i);
        if (is_disk(bd))
            out[n++] = bd;
    }
    return n;
}

static struct blockdev *first_disk(uint64_t need_sectors)
{
    struct blockdev *disks[MAX_DISKS];
    size_t n = collect_disks(disks, MAX_DISKS);
    for (size_t i = 0; i < n; i++)
        if (disks[i]->write && disks[i]->blocks >= need_sectors)
            return disks[i];
    return 0;
}

/* ── the install itself ────────────────────────────────────────────── */

/* result codes for install_image() */
#define INSTALL_OK       0
#define INSTALL_WRITE    1
#define INSTALL_VERIFY   2
#define INSTALL_MEMORY   3
#define INSTALL_FLUSH    4

static int install_image(struct blockdev *bd, const uint8_t *img,
                         uint64_t sectors, int use_tui)
{
    uint64_t image_mib = (sectors + 2047) / 2048;
    uint8_t *buf = malloc(CHUNK_SECTORS * 512u);
    if (!buf) {
        kprintf("kelpinstall: out of memory\n");
        return INSTALL_MEMORY;
    }

    kprintf("kelpinstall: writing %llu MiB to %s (%llu MiB)\n",
            (unsigned long long)image_mib, bd->name,
            (unsigned long long)(bd->blocks / 2048));

    uint64_t done = 0;
    int last_print = -1, last_draw = -1;
    int rc = INSTALL_OK;
    char detail[80];

    while (done < sectors) {
        uint32_t n = (uint32_t)(sectors - done);
        if (n > CHUNK_SECTORS)
            n = CHUNK_SECTORS;
        memcpy(buf, img + done * 512u, (size_t)n * 512u);
        if (bd->write(bd, done, n, buf) != BLOCK_ERR_OK) {
            kprintf("\nkelpinstall: write error at sector %llu\n",
                    (unsigned long long)done);
            rc = INSTALL_WRITE;
            break;
        }
        done += n;

        int pct = (int)((done * 100) / sectors);
        if (use_tui) {
            if (pct != last_draw && (pct % 2 == 0 || pct == 100)) {
                snprintf(detail, sizeof(detail), "%llu of %llu MiB on %s",
                         (unsigned long long)(done / 2048),
                         (unsigned long long)image_mib, bd->name);
                tui_progress("Installing KelpOS", "Writing the system image",
                             (unsigned)pct, detail);
                last_draw = pct;
            }
            /* keep the serial log useful even though the screen is the UI */
            if (pct / 10 != last_print / 10 || pct == 100) {
                kprintf("kelpinstall: %3d%% (%llu/%llu MiB)\n", pct,
                        (unsigned long long)(done / 2048),
                        (unsigned long long)image_mib);
                last_print = pct;
            }
        } else if (pct / 10 != last_print / 10 || pct == 100) {
            kprintf("kelpinstall: %3d%% (%llu/%llu MiB)\n", pct,
                    (unsigned long long)(done / 2048),
                    (unsigned long long)image_mib);
            last_print = pct;
        }
    }

    if (rc == INSTALL_OK) {
        if (bd->flush && bd->flush(bd) != BLOCK_ERR_OK) {
            kprintf("kelpinstall: cache flush failed\n");
            rc = INSTALL_FLUSH;
        }
        /* make the new partition table visible in this session (sda1) */
        part_scan(bd);

        uint8_t verify[512];
        if (bd->read(bd, 0, 1, verify) != BLOCK_ERR_OK ||
            memcmp(verify, img, sizeof(verify)) != 0) {
            kprintf("kelpinstall: verification failed (read-back mismatch)\n");
            rc = INSTALL_VERIFY;
        }
    }

    free(buf);
    return rc;
}

static void show_install_error(int rc, int use_tui)
{
    static const char *const mem[] = {
        "The kernel could not allocate the write buffer.",
        "Nothing was written.",
    };
    static const char *const write[] = {
        "The disk reported an I/O error while writing.",
        "The installation is incomplete - do not boot from this disk.",
    };
    static const char *const flush[] = {
        "The write cache could not be flushed.",
        "The data may not have reached the platter.",
    };
    static const char *const verify[] = {
        "The read-back of the boot sector does not match the image.",
        "The installation is not trustworthy - retry it.",
    };

    const char *const *lines;
    const char *title;
    int count;

    switch (rc) {
    case INSTALL_MEMORY:
        title = "Out of memory";
        lines = mem;
        count = 2;
        break;
    case INSTALL_WRITE:
        title = "Write error";
        lines = write;
        count = 2;
        break;
    case INSTALL_FLUSH:
        title = "Flush error";
        lines = flush;
        count = 2;
        break;
    default:
        title = "Verification failed";
        lines = verify;
        count = 2;
        break;
    }

    if (use_tui)
        tui_page("Install KelpOS", title, lines, count, 0);
    else
        for (int i = 0; i < count; i++)
            kprintf("kelpinstall: %s\n", lines[i]);
}

/* ── full-screen UI ────────────────────────────────────────────────── */

enum {
    ID_DISK = 1,
    ID_IMAGE,
    ID_PART,
    ID_FS,
    ID_BOOT,
    ID_INSTALL,
    ID_EXIT
};

static struct blockdev *choose_disk(struct blockdev *cur)
{
    struct blockdev *disks[MAX_DISKS];
    size_t n = collect_disks(disks, MAX_DISKS);
    static const char *const none[] = {
        "No SATA disk was found on this machine.",
        "Attach a disk (QEMU: '-drive file=disk.img,format=raw,if=ide')",
        "and open the installer again.",
    };
    if (n == 0) {
        tui_page("Target disk", "No disk", none, 3, 0);
        return cur;
    }

    struct tui_item items[MAX_DISKS];
    char labels[MAX_DISKS][16];
    char values[MAX_DISKS][48];
    int sel = 0;

    memset(items, 0, sizeof(items));
    for (size_t i = 0; i < n; i++) {
        snprintf(labels[i], sizeof(labels[i]), "%s", disks[i]->name);
        snprintf(values[i], sizeof(values[i]), "%llu MiB   %s",
                 (unsigned long long)(disks[i]->blocks / 2048),
                 disks[i]->write ? "rw" : "read-only");
        items[i].label = labels[i];
        items[i].value = values[i];
        items[i].hint = "The first 48 MiB of this disk will be overwritten.";
        items[i].id = (int)i;
        items[i].disabled = !disks[i]->write;
        if (disks[i] == cur)
            sel = (int)i;
    }

    int r = tui_menu("Target disk",
                     "Choose the disk that receives the installation",
                     items, (int)n, sel, 0);
    return r < 0 ? cur : disks[r];
}

static int installer_tui(const uint8_t *img, uint64_t sectors)
{
    struct blockdev *target = first_disk(sectors);
    char disk_value[64];
    char image_value[32];
    char head[96];
    int sel = 0;

    snprintf(image_value, sizeof(image_value), "%llu MiB",
             (unsigned long long)((sectors + 2047) / 2048));

    tui_begin();
    for (;;) {
        if (target)
            snprintf(disk_value, sizeof(disk_value), "%s   %llu MiB",
                     target->name,
                     (unsigned long long)(target->blocks / 2048));
        else
            snprintf(disk_value, sizeof(disk_value), "none found");

        const struct tui_item items[] = {
            { "Target disk", disk_value,
              "Disk that receives the installation (all data on it is lost).",
              ID_DISK, false },
            { "Install image", image_value,
              "Bootable KelpOS image built into this ISO (install.img).",
              ID_IMAGE, false },
            { "Partition table", "MBR (fixed)",
              "Master boot record with one active FAT16 partition at LBA 2048.",
              ID_PART, false },
            { "Filesystem", "FAT16 (fixed)",
              "FAT16 holding the kernel, the boot loader and the wallpaper.",
              ID_FS, false },
            { "Boot loader", "Limine (fixed)",
              "Limine BIOS stage, installed into the image by the host.",
              ID_BOOT, false },
            { 0, 0, 0, 0, false },
            { "Install KelpOS", 0,
              "Write the image to the target disk and make it bootable.",
              ID_INSTALL, false },
            { "Exit", 0, "Return to the shell without changing anything.",
              ID_EXIT, false },
        };
        int count = (int)(sizeof(items) / sizeof(items[0]));

        int idx = tui_menu("KelpOS Installer",
                           "KelpOS " KELPOS_VERSION
                           " - install the running system to a disk",
                           items, count, sel, 0);
        if (idx < 0)
            break;
        sel = idx;

        switch (items[idx].id) {
        case ID_DISK:
            target = choose_disk(target);
            break;

        case ID_IMAGE: {
            static const char *const lines[] = {
                "The installer does not copy a package tree: it writes a",
                "complete, prebuilt disk image that this ISO carries as",
                "the Limine module install.img (48 MiB).",
                "",
                "The image contains the kernel, the Limine boot loader,",
                "the boot menu, the wallpaper and the demo disk, so the",
                "installed system boots on its own.",
            };
            tui_page("Install image", image_value, lines, 7, 0);
            break;
        }

        case ID_PART: {
            static const char *const lines[] = {
                "MBR (DOS) partition table with a single partition:",
                "",
                "  start LBA 2048, type 0x0e (FAT16 LBA), active",
                "",
                "The layout is fixed: the image is written verbatim, so",
                "the boot loader finds its files where it expects them.",
            };
            tui_page("Partition table", "MBR", lines, 6, 0);
            break;
        }

        case ID_FS: {
            static const char *const lines[] = {
                "FAT16, volume label KELPOS.",
                "",
                "  /boot/kelpos.elf      the kernel",
                "  /boot/limine.conf     the boot menu",
                "  /boot/limine-bios.sys Limine stage 2",
                "  /boot/disk.img        the demo disk",
                "  /wallpaper.bmp        the boot screen",
            };
            tui_page("Filesystem", "FAT16", lines, 7, 0);
            break;
        }

        case ID_BOOT: {
            static const char *const lines[] = {
                "Limine, installed by scripts/geninstallimg.sh on the host",
                "with 'limine bios-install'. The kernel is booted with the",
                "Limine boot protocol.",
                "",
                "The installed system needs no ISO and no second stage",
                "from the installation medium.",
            };
            tui_page("Boot loader", "Limine", lines, 5, 0);
            break;
        }

        case ID_INSTALL: {
            if (!target) {
                static const char *const lines[] = {
                    "There is no writable SATA disk to install to.",
                    "Attach a disk and open the installer again.",
                };
                tui_page("Install KelpOS", "No target disk", lines, 2, 0);
                break;
            }

            snprintf(head, sizeof(head), "Install KelpOS on %s (%llu MiB)?",
                     target->name,
                     (unsigned long long)(target->blocks / 2048));
            static const char *const warn[] = {
                "The first 48 MiB of the disk will be overwritten,",
                "including its partition table and every file stored there.",
                "",
                "This is the last question: the write cannot be undone.",
            };
            if (!tui_confirm("Install KelpOS", head, warn, 4,
                             "Install", "Cancel"))
                break;

            int rc = install_image(target, img, sectors, 1);
            if (rc == INSTALL_OK) {
                snprintf(head, sizeof(head), "KelpOS installed on %s",
                         target->name);
                static const char *const done[] = {
                    "The disk is bootable and needs no installation medium.",
                    "Remove the CD, then reboot into the installed system.",
                };
                if (tui_confirm("Installation complete", head, done, 2,
                                "Reboot", "Shell")) {
                    char *args[] = { "reboot", 0 };
                    tui_end();
                    return cmd_reboot(1, args);
                }
                target = first_disk(sectors);
            } else {
                show_install_error(rc, 1);
            }
            break;
        }

        case ID_EXIT:
        default:
            tui_end();
            return 0;
        }
    }

    tui_end();
    return 0;
}

/* ── line-based fallback (serial only, no framebuffer) ─────────────── */

static int installer_auto(const uint8_t *img, uint64_t sectors)
{
    uint64_t image_mib = (sectors + 2047) / 2048;

    struct blockdev *bd = first_disk(sectors);
    if (!bd) {
        if (!ahci_present())
            kprintf("kelpinstall: no SATA disk found\n"
                    "kelpinstall: this build has no framebuffer UI, "
                    "attach a disk and retry\n");
        else
            kprintf("kelpinstall: no SATA disk with at least %llu MiB free "
                    "for the install image\n",
                    (unsigned long long)image_mib);
        kprintf("kelpinstall: run 'lsblk' to list the block devices\n");
        return 1;
    }

    kprintf("kelpinstall: no framebuffer UI on this machine: installing "
            "without prompts\n");
    kprintf("kelpinstall: this overwrites the first %llu MiB of %s\n",
            (unsigned long long)image_mib, bd->name);

    int rc = install_image(bd, img, sectors, 0);
    if (rc != INSTALL_OK) {
        show_install_error(rc, 0);
        return 1;
    }

    kprintf("kelpinstall: done - KelpOS installed on %s\n", bd->name);
    kprintf("kelpinstall: remove the CD and reboot to start from the disk\n");
    return 0;
}

/* ── command entry point ───────────────────────────────────────────── */

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
    if (tui_available()) {
        kprintf("kelpinstall: text UI on the framebuffer, "
                "logging to serial\n");
        return installer_tui(img, sectors);
    }
    return installer_auto(img, sectors);
}
