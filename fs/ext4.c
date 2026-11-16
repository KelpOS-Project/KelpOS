/* fs/ext4.c — ext4 probe skeleton (see ext4.h).
 *
 * Detects a valid ext2/3/4 superblock (magic 0xEF53 at byte 1024 of the
 * volume) and reports volume info. File data access is NOT implemented
 * yet: that needs the block group descriptor + extent reader, coming
 * with the M5 fs milestone. Being honest about that in `mounts`. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ext4.h"
#include "block.h"

#define SB_LBA 2        /* superblock sits 1024 bytes in = LBA 2 */
#define SB_MAGIC_OFF 56 /* magic within the superblock */
#define EXT2_MAGIC 0xEF53u

struct ext4_ctx {
    uint32_t inodes;       /* total inode count */
    uint64_t blocks;       /* total block count */
    uint32_t block_size;
    char     label[17];
    int      compat_ro_features_present; /* just honest flagging */
};

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int ext4_probe(struct blockdev *bd)
{
    uint8_t sb[512];
    if (bd->blocks < SB_LBA + 1)
        return 0;
    if (bd->read(bd, SB_LBA, 1, sb))
        return 0;
    if (rd16(sb + SB_MAGIC_OFF) != EXT2_MAGIC)
        return 0;
    /* sanity: log block size must be sane (0..6) */
    uint32_t log_bs = rd32(sb + 24);
    return log_bs <= 6;
}

static int ext4_mount(struct vfs_fs *fs)
{
    uint8_t sb[512];
    if (fs->bd->read(fs->bd, SB_LBA, 1, sb))
        return -1;

    struct ext4_ctx *c = malloc(sizeof(*c));
    if (!c)
        return -1;
    memset(c, 0, sizeof(*c));
    fs->ctx = c;

    c->inodes = rd32(sb + 0);
    uint64_t lo = rd32(sb + 4), hi = rd32(sb + 92);
    c->blocks = lo | (hi << 32);
    uint32_t log_bs = rd32(sb + 24);
    c->block_size = 1024u << log_bs;
    memcpy(c->label, sb + 120, 16); /* s_volume_name */
    c->label[16] = 0;
    return 0;
}

static void ext4_info(struct vfs_fs *fs, char *buf, size_t cap)
{
    struct ext4_ctx *c = fs->ctx;
    snprintf(buf, cap, "ext4 \"%s\" (%llu blocks, RO skeleton)",
             c->label, (unsigned long long)c->blocks);
}

/* listings/files are not possible yet */
static int ext4_list(struct vfs_fs *fs, const char *dir,
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

static long ext4_read_file(struct vfs_fs *fs, const char *path, char *buf,
                           size_t cap)
{
    (void)fs;
    (void)path;
    (void)buf;
    (void)cap;
    return -1;
}

const struct fs_ops ext4_fs_ops = {
    .name = "ext4",
    .probe = ext4_probe,
    .mount = ext4_mount,
    .list = ext4_list,
    .read_file = ext4_read_file,
    .info = ext4_info,
};
