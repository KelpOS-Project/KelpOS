/* fs/btrfs.c — btrfs probe skeleton (see btrfs.h).
 *
 * Detects the btrfs superblock (magic "_BHRfS_M" at LBA 128) and reads
 * volume info. Actual data access needs the chunk-tree/B-tree reader —
 * planned for the M5 fs milestone; `mounts` reports it as RO skeleton. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "btrfs.h"
#include "block.h"

#define SB_LBA 128 /* first superblock copy: byte 65536 */

struct btrfs_ctx {
    char     label[256];
    uint32_t sectorsize;
    uint32_t nodesize;
    uint64_t bytes_used;
    uint64_t total_bytes;
};

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint64_t rd64(const uint8_t *p)
{
    return rd32(p) | (uint64_t)rd32(p + 4) << 32;
}

static int btrfs_probe(struct blockdev *bd)
{
    uint8_t sb[512];
    if (bd->blocks < SB_LBA + 1)
        return 0;
    if (bd->read(bd, SB_LBA, 1, sb))
        return 0;
    /* csum (32 B) + fsid (16 B) + bytenr/flags (16 B), then magic */
    return memcmp(sb + 64, "_BHRfS_M", 8) == 0;
}

static int btrfs_mount(struct vfs_fs *fs)
{
    uint8_t sb[512];
    if (fs->bd->read(fs->bd, SB_LBA, 1, sb))
        return -1;

    struct btrfs_ctx *c = malloc(sizeof(*c));
    if (!c)
        return -1;
    memset(c, 0, sizeof(*c));
    fs->ctx = c;

    memcpy(c->label, sb + 298, 255); /* s_label after the dev_item */
    c->label[255] = 0;
    c->sectorsize = rd32(sb + 144);
    c->nodesize = rd32(sb + 148);
    c->total_bytes = rd64(sb + 112);
    c->bytes_used = rd64(sb + 120);
    return 0;
}

static void btrfs_info(struct vfs_fs *fs, char *buf, size_t cap)
{
    struct btrfs_ctx *c = fs->ctx;
    snprintf(buf, cap, "btrfs \"%s\" (%llu/%llu B used, RO skeleton)",
             c->label, (unsigned long long)c->bytes_used,
             (unsigned long long)c->total_bytes);
}

/* listings/files are not possible yet */
static int btrfs_list(struct vfs_fs *fs, const char *dir,
                      int (*cb)(const char *name, int is_dir, uint64_t size,
                                void *ud),
                      void *ud)
{
    (void)fs;
    (void)dir;
    (void)cb;
    (void)ud;
    return -1;
}

static long btrfs_read_file(struct vfs_fs *fs, const char *path, char *buf,
                            size_t cap)
{
    (void)fs;
    (void)path;
    (void)buf;
    (void)cap;
    return -1;
}

const struct fs_ops btrfs_fs_ops = {
    .name = "btrfs",
    .probe = btrfs_probe,
    .mount = btrfs_mount,
    .list = btrfs_list,
    .read_file = btrfs_read_file,
    .info = btrfs_info,
};
