#ifndef KELP_FS_VFS_H
#define KELP_FS_VFS_H

#include <stddef.h>
#include <stdint.h>
#include "block.h"

/* Minimal VFS: filesystems register backend ops (probe/mount/list/read)
 * and get mounted on a block device at a mountpoint. The shell exposes
 * mounts via the "diskfs" package (mounts/diskls/diskcat). */

struct vfs_fs;

struct fs_ops {
    const char *name; /* "fat", "ext4", "btrfs", ... */
    /* returns 1 if this fs type is present on the device, else 0 */
    int (*probe)(struct blockdev *bd);
    /* prepare the fs context; returns 0 on success */
    int (*mount)(struct vfs_fs *fs);
    /* list a directory: callback receives name, is_dir, size */
    int (*list)(struct vfs_fs *fs, const char *dir,
                int (*cb)(const char *name, int is_dir, uint64_t size,
                          void *ud),
                void *ud);
    /* read a file into buf (max cap bytes, returns bytes read, -1 err) */
    long (*read_file)(struct vfs_fs *fs, const char *path,
                      char *buf, size_t cap);
    /* fs-type specific stats line for `mounts` */
    void (*info)(struct vfs_fs *fs, char *buf, size_t cap);
};

struct vfs_fs {
    const char *mountpoint;  /* e.g. "/media/disk0" */
    char mp[64];             /* storage for runtime mountpoints */
    struct blockdev *bd;
    const struct fs_ops *ops;
    void *ctx;
};

void vfs_init(void);
int  vfs_register_ops(const struct fs_ops *ops);

/* mount `bd` at `mountpoint` (auto-detects fs type via registered ops);
 * 0 on success, <0 on failure (no fs found / mount failed) */
int  vfs_mount(struct blockdev *bd, const char *mountpoint);
int  vfs_unmount(const char *mountpoint);

size_t vfs_mount_count(void);
struct vfs_fs *vfs_mount_get(size_t i);
struct vfs_fs *vfs_find(const char *mountpoint);

/* convenience wrappers (first mount containing path is used) */
int  vfs_list(struct vfs_fs *fs, const char *dir,
              int (*cb)(const char *name, int is_dir, uint64_t size, void *ud),
              void *ud);
long vfs_read_file(struct vfs_fs *fs, const char *path, char *buf, size_t cap);

#endif
