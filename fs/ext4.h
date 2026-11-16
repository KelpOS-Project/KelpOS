#ifndef KELP_FS_EXT4_H
#define KELP_FS_EXT4_H

#include "vfs.h"

/* ext4 backend — probe-only skeleton for now: mounting and listing
 * succeed far enough to report the fs type, but file access needs the
 * extent/journal reader, which lands with the M5 fs milestone. */
extern const struct fs_ops ext4_fs_ops;

#endif
