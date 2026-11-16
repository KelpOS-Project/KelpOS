/* tests/host/test_fs.c — host tests for the fs stack: ramfs, block,
 * vfs and the real FAT backend (reads the generated boot/disk.img). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "ramfs.h"
#include "block.h"
#include "vfs.h"
#include "fat.h"
#include "ext4.h"
#include "btrfs.h"

/* ── ramfs ──────────────────────────────────────────────────────────── */

static void test_ramfs(void)
{
    ramfs_init();
    assert(ramfs_root());
    assert(ramfs_mkdir("/etc") == RAMFS_ERR_OK);
    assert(ramfs_mkdir("/etc") == RAMFS_ERR_EXIST);
    assert(ramfs_mkfile("/etc/motd") == RAMFS_ERR_OK);
    assert(ramfs_mkdir("/etc/motd") == RAMFS_ERR_EXIST);
    assert(ramfs_mkdir("/a/b/c") == RAMFS_ERR_NOTDIR); /* /a missing */
    assert(ramfs_mkdir("relative") == RAMFS_ERR_INVAL);

    struct ramfs_node *motd = ramfs_resolve_at(ramfs_root(), "/etc/motd");
    assert(motd && !ramfs_is_dir(motd));
    assert(ramfs_write(motd, "hi\n", 3) == RAMFS_ERR_OK);
    assert(ramfs_size(motd) == 3);
    assert(strcmp(ramfs_data(motd), "hi\n") == 0);
    assert(ramfs_append(motd, "more", 4) == RAMFS_ERR_OK);
    assert(ramfs_size(motd) == 7);

    /* resolve: absolute, relative, dot-dot, missing */
    assert(ramfs_resolve_at(ramfs_root(), "/etc/../etc/motd") == motd);
    assert(ramfs_resolve_at(motd, "motd") == motd); /* file base: use parent */
    assert(ramfs_resolve_at(ramfs_root(), "/etc/./motd") == motd);
    assert(ramfs_resolve_at(ramfs_root(), "/etc/nope") == 0);

    /* children sorted */
    ramfs_mkdir("/etc/zoo");
    ramfs_mkfile("/etc/apple");
    struct ramfs_node *kids[8];
    assert(ramfs_children(ramfs_resolve_at(ramfs_root(), "/etc"),
                          kids, 8) == 3);
    assert(strcmp(ramfs_node_name(kids[0]), "apple") == 0);
    assert(strcmp(ramfs_node_name(kids[1]), "motd") == 0);
    assert(strcmp(ramfs_node_name(kids[2]), "zoo") == 0);

    /* rm: files + empty dirs, not non-empty ones */
    assert(ramfs_rm("/etc/zoo") == RAMFS_ERR_OK);
    assert(ramfs_rm("/etc") == RAMFS_ERR_NOTDIR);
    assert(ramfs_rm("/etc/apple") == RAMFS_ERR_OK);
    assert(ramfs_rm("/etc/motd") == RAMFS_ERR_OK);
    assert(ramfs_resolve_at(ramfs_root(), "/etc/motd") == 0);

    /* path_of */
    ramfs_mkdir("/x");
    ramfs_mkdir("/x/y");
    char buf[64];
    assert(ramfs_path_of(ramfs_resolve_at(ramfs_root(), "/x/y"),
                         buf, sizeof(buf)) == buf);
    assert(strcmp(buf, "/x/y") == 0);
    assert(ramfs_path_of(ramfs_root(), buf, sizeof(buf)) == buf);
    assert(strcmp(buf, "/") == 0);
}

/* ── block + vfs + FAT (real image from boot/disk.img) ──────────────── */

struct membd {
    uint8_t *base;
    size_t size;
};

static int membd_read(struct blockdev *bd, uint64_t lba, uint32_t count,
                      void *buf)
{
    struct membd *m = bd->ctx;
    if (lba * 512 + (uint64_t)count * 512 > m->size)
        return BLOCK_ERR_RANGE;
    memcpy(buf, m->base + lba * 512, (size_t)count * 512);
    return BLOCK_ERR_OK;
}

static char list_names[32][64];
static int  list_dirs[32];
static size_t list_n;

static int list_cb(const char *name, int is_dir, uint64_t size, void *ud)
{
    (void)size;
    (void)ud;
    snprintf(list_names[list_n], sizeof(list_names[0]), "%s", name);
    list_dirs[list_n] = is_dir;
    list_n++;
    return 0;
}

static void test_vfs_fat(void)
{
    /* load the first MiB of the demo image (built by gendiskimg.sh) —
     * the klibc bump allocator used in these host tests is small */
    int fd = open("boot/disk.img", O_RDONLY);
    if (fd < 0) {
        kprintf("SKIP fat test (no boot/disk.img)\n");
        return;
    }
    long sz = 512 << 10;
    uint8_t *img = malloc((size_t)sz);
    ssize_t got = read(fd, img, (size_t)sz);
    close(fd);
    assert(img && got == sz);

    block_init();
    vfs_init();

    struct blockdev bd;
    struct membd m = { img, (size_t)sz };
    memset(&bd, 0, sizeof(bd));
    bd.name = "test0";
    bd.blocks = (uint64_t)sz / 512;
    bd.read = membd_read;
    bd.ctx = &m;
    assert(block_register(&bd) == 0);

    vfs_register_ops(&fat_fs_ops);
    vfs_register_ops(&ext4_fs_ops); /* probe must NOT match on FAT */
    vfs_register_ops(&btrfs_fs_ops);

    assert(vfs_mount(&bd, "/media/disk0") == 0);
    struct vfs_fs *fs = vfs_find("/media/disk0");
    assert(fs && strcmp(fs->ops->name, "fat") == 0);

    /* root listing: dirs + files, short + long names */
    list_n = 0;
    assert(vfs_list(fs, "/", list_cb, 0) == 0);
    assert(list_n >= 3);
    int saw_docs = 0, saw_readme = 0;
    for (size_t i = 0; i < list_n; i++) {
        if (strcasecmp(list_names[i], "docs") == 0)
            saw_docs = list_dirs[i];
        if (strcasecmp(list_names[i], "readme.txt") == 0)
            saw_readme = 1;
    }
    assert(saw_docs && saw_readme);

    /* read a file whose content we know */
    char buf[512];
    long n = vfs_read_file(fs, "docs/hello.txt", buf, sizeof(buf) - 1);
    assert(n > 0);
    buf[n] = 0;
    assert(strstr(buf, "Hello from the FAT filesystem!") == buf);
    /* size comes from the dirent, not the buffer cap */
    assert(n < 512);

    /* path resolution through a subdirectory */
    n = vfs_read_file(fs, "docs/license.txt", buf, sizeof(buf) - 1);
    assert(n > 0 && strstr(buf, "GPL-3.0"));

    /* missing file + missing dir */
    assert(vfs_read_file(fs, "no/such/file.txt", buf, sizeof(buf)) == -1);
    assert(vfs_list(fs, "/no/such", list_cb, 0) != 0);

    /* info line */
    char info[128];
    fs->ops->info(fs, info, sizeof(info));
    assert(strstr(info, "FAT16"));
    assert(strstr(info, "KELPDEMO"));

    /* ext4/btrfs probes must reject a FAT volume */
    assert(ext4_fs_ops.probe(&bd) == 0);
    assert(btrfs_fs_ops.probe(&bd) == 0);
    /* ext4_mount/btrfs_mount report the skeleton honestly */
    assert(vfs_list(fs, "/", list_cb, 0) == 0); /* still works after */

    free(img);
}

int test_fs_main(void)
{
    test_ramfs();
    test_vfs_fat();
    kprintf("FS TESTS PASSED\n");
    return 0;
}
