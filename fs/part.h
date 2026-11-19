#ifndef KELP_FS_PART_H
#define KELP_FS_PART_H

#include "block.h"

/* MBR partition scanner: registers the primary partitions of a disk as
 * Unix-style device names ("sda1", "sda2", ...). Each partition is a
 * block device offset into its parent, so the existing filesystem
 * drivers can mount them unchanged.
 *
 * Returns the number of partitions registered (0 if the disk has no
 * MBR or no non-empty primary partition). */

int part_scan(struct blockdev *disk);

#endif
