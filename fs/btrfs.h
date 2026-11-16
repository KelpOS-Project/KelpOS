#ifndef KELP_FS_BTRFS_H
#define KELP_FS_BTRFS_H

#include "vfs.h"

/* btrfs backend — probe-only skeleton for now: the superblock is
 * validated, everything else (chunk trees, checksums, CoW) needs the
 * M5 fs milestone. */
extern const struct fs_ops btrfs_fs_ops;

#endif
