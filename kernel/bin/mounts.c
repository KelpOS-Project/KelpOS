/* kernel/bin/mounts.c — package "diskfs": real-disk commands on top of
 * the VFS (fs/vfs.h). Commands:
 *   mounts   — list mountpoints + fs type
 *   diskls   — list a directory on a mounted volume
 *   diskcat  — print a file from a mounted volume
 * Everything is read-only for now. */
#include "commands.h"

#ifdef KELPOS_PKG_DISKFS
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "vfs.h"
#include "fat.h"
#include "ext4.h"
#include "btrfs.h"
#include "console.h"

void diskfs_init(void)
{
    vfs_register_ops(&fat_fs_ops);
    vfs_register_ops(&ext4_fs_ops);
    vfs_register_ops(&btrfs_fs_ops);
}

static int cmd_mounts(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (vfs_mount_count() == 0) {
        kprintf("no filesystems mounted\n");
        return 1;
    }
    for (size_t i = 0; i < vfs_mount_count(); i++) {
        struct vfs_fs *m = vfs_mount_get(i);
        char info[128];
        info[0] = 0;
        if (m->ops->info)
            m->ops->info(m, info, sizeof(info));
        console_set_colors(0x7fb3d5u, KELP_BG);
        kprintf("%-16s", m->mountpoint);
        console_set_colors(KELP_FG, KELP_BG);
        kprintf(" %-6s %-12s %llu blocks (%s)\n",
                m->bd->name, m->ops->name,
                (unsigned long long)m->bd->blocks,
                info);
    }
    return 0;
}

/* "<mountpoint>/rest" -> fs + subpath; matches the longest prefix */
static struct vfs_fs *path_lookup(const char *path, const char **sub)
{
    for (size_t i = 0; i < vfs_mount_count(); i++) {
        struct vfs_fs *m = vfs_mount_get(i);
        size_t len = strlen(m->mountpoint);
        if (strncmp(path, m->mountpoint, len) == 0 &&
            (path[len] == '/' || path[len] == 0)) {
            const char *rest = path + len;
            while (*rest == '/')
                rest++;
            *sub = rest;
            return m;
        }
    }
    return 0;
}

static int list_print_cb(const char *name, int is_dir, uint64_t size, void *ud)
{
    (void)ud;
    if (is_dir) {
        console_set_colors(0x7fb3d5u, KELP_BG);
        kprintf("%s/", name);
        console_set_colors(KELP_FG, KELP_BG);
    } else {
        kprintf("%-20s %llu B", name, (unsigned long long)size);
    }
    kputc('\n');
    return 0;
}

static int cmd_diskls(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/media/disk0";
    const char *sub = 0;
    struct vfs_fs *fs = path_lookup(path, &sub);
    if (!fs) {
        kprintf("diskls: not mounted: %s (see 'mounts')\n", path);
        return 1;
    }

    char dirbuf[128];
    snprintf(dirbuf, sizeof(dirbuf), "/%s", sub);
    int rc = vfs_list(fs, dirbuf, list_print_cb, 0);
    if (rc != 0) {
        kprintf("diskls: cannot list '%s' on %s (rc %d)\n",
                dirbuf, fs->ops->name, rc);
        return 1;
    }
    return 0;
}

static int cmd_diskcat(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: diskcat <mountpoint>/<file>\n");
        return 1;
    }
    const char *sub = 0;
    struct vfs_fs *fs = path_lookup(argv[1], &sub);
    if (!fs) {
        kprintf("diskcat: not mounted: %s (see 'mounts')\n", argv[1]);
        return 1;
    }

    char buf[4096];
    long n = vfs_read_file(fs, sub ? sub : argv[1], buf, sizeof(buf) - 1);
    if (n < 0) {
        kprintf("diskcat: cannot read '%s' on %s (not supported or missing)\n",
                sub ? sub : argv[1], fs->ops->name);
        return 1;
    }
    buf[n] = 0;
    kputs(buf);
    if (n == 0 || buf[n - 1] != '\n')
        kputc('\n');
    return 0;
}

static struct blockdev *dev_lookup(const char *name)
{
    if (strncmp(name, "/dev/", 5) == 0)
        name += 5;
    return block_find(name);
}

static int cmd_mount(int argc, char **argv)
{
    if (argc != 3) {
        kprintf("usage: mount <device> <mountpoint>\n"
                "       mount sda1 /mnt\n");
        return 1;
    }
    struct blockdev *bd = dev_lookup(argv[1]);
    if (!bd) {
        kprintf("mount: no such block device: %s\n", argv[1]);
        return 1;
    }
    if (vfs_mount(bd, argv[2]) < 0) {
        kprintf("mount: cannot mount %s on %s "
                "(unknown fs or already mounted)\n", bd->name, argv[2]);
        return 1;
    }
    kprintf("mount: %s on %s\n", bd->name, argv[2]);
    return 0;
}

static int cmd_umount(int argc, char **argv)
{
    if (argc != 2) {
        kprintf("usage: umount <mountpoint>\n");
        return 1;
    }
    if (vfs_unmount(argv[1]) < 0) {
        kprintf("umount: not mounted: %s\n", argv[1]);
        return 1;
    }
    kprintf("umount: %s released\n", argv[1]);
    return 0;
}

static int cmd_df(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("Filesystem      1K-blocks  Device  Mounted on\n");
    for (size_t i = 0; i < vfs_mount_count(); i++) {
        struct vfs_fs *m = vfs_mount_get(i);
        kprintf("%-14s %10llu  %-6s  %s\n", m->ops->name,
                (unsigned long long)(m->bd->blocks / 2),
                m->bd->name, m->mountpoint);
    }
    return 0;
}

const struct builtin diskfs_cmds[] = {
    { "mounts",  cmd_mounts,  "list mounted filesystems" },
    { "mount",   cmd_mount,   "mount a block device (mount sda1 /mnt)" },
    { "umount",  cmd_umount,  "unmount a filesystem" },
    { "df",      cmd_df,      "show mounted filesystems and sizes" },
    { "diskls",  cmd_diskls,  "list a directory on a mounted disk" },
    { "diskcat", cmd_diskcat, "print a file from a mounted disk" },
};
#endif
