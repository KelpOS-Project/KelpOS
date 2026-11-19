/* fs/vfs.c — minimal virtual filesystem layer (see vfs.h). */
#include <stddef.h>
#include <string.h>
#include "vfs.h"

#define MAX_OPS 8
#define MAX_MOUNTS 8

static const struct fs_ops *ops_tab[MAX_OPS];
static size_t ops_n;
static struct vfs_fs mounts_tab[MAX_MOUNTS];
static size_t mounts_n;

void vfs_init(void)
{
    ops_n = mounts_n = 0;
}

int vfs_register_ops(const struct fs_ops *ops)
{
    if (!ops || !ops->name || !ops->probe || !ops->mount)
        return -1;
    if (ops_n >= MAX_OPS)
        return -1;
    for (size_t i = 0; i < ops_n; i++)
        if (strcmp(ops_tab[i]->name, ops->name) == 0)
            return -1;
    ops_tab[ops_n++] = ops;
    return 0;
}

int vfs_mount(struct blockdev *bd, const char *mountpoint)
{
    if (!bd || !mountpoint || mounts_n >= MAX_MOUNTS)
        return -1;
    if (vfs_find(mountpoint))
        return -1; /* already mounted there */

    for (size_t i = 0; i < ops_n; i++) {
        if (!ops_tab[i]->probe(bd))
            continue;
        struct vfs_fs *m = &mounts_tab[mounts_n];
        memset(m, 0, sizeof(*m));
        strncpy(m->mp, mountpoint, sizeof(m->mp) - 1);
        m->mp[sizeof(m->mp) - 1] = 0;
        m->mountpoint = m->mp;
        m->bd = bd;
        m->ops = ops_tab[i];
        if (ops_tab[i]->mount(m) == 0) {
            mounts_n++;
            return 0;
        }
    }
    return -1;
}

int vfs_unmount(const char *mountpoint)
{
    for (size_t i = 0; i < mounts_n; i++) {
        if (strcmp(mounts_tab[i].mountpoint, mountpoint) == 0) {
            memmove(&mounts_tab[i], &mounts_tab[i + 1],
                    (mounts_n - i - 1) * sizeof(mounts_tab[0]));
            mounts_n--;
            return 0;
        }
    }
    return -1;
}

size_t vfs_mount_count(void)
{
    return mounts_n;
}

struct vfs_fs *vfs_mount_get(size_t i)
{
    return i < mounts_n ? &mounts_tab[i] : 0;
}

struct vfs_fs *vfs_find(const char *mountpoint)
{
    for (size_t i = 0; i < mounts_n; i++)
        if (strcmp(mounts_tab[i].mountpoint, mountpoint) == 0)
            return &mounts_tab[i];
    return 0;
}

int vfs_list(struct vfs_fs *fs, const char *dir,
             int (*cb)(const char *name, int is_dir, uint64_t size, void *ud),
             void *ud)
{
    if (!fs || !fs->ops->list)
        return -1;
    return fs->ops->list(fs, dir, cb, ud);
}

long vfs_read_file(struct vfs_fs *fs, const char *path, char *buf, size_t cap)
{
    if (!fs || !fs->ops->read_file)
        return -1;
    return fs->ops->read_file(fs, path, buf, cap);
}
