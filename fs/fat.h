#ifndef KELP_FS_FAT_H
#define KELP_FS_FAT_H

#include "vfs.h"

/* Read-only FAT12/16/32 + exFAT backend over a block device.
 * Long file names are read; writes are not supported yet. */
extern const struct fs_ops fat_fs_ops;

#endif
