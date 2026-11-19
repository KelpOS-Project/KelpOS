#ifndef KELP_FS_BLOCK_H
#define KELP_FS_BLOCK_H

#include <stddef.h>
#include <stdint.h>

/* Block device layer: everything the filesystem drivers talk to.
 * 512-byte sectors (LBA). Registering a device hands ownership of the
 * name to the registry. */

struct blockdev {
    const char *name;   /* e.g. "ram0" */
    uint64_t blocks;    /* total 512-byte sectors */
    int (*read)(struct blockdev *bd, uint64_t lba, uint32_t count, void *buf);
    int (*write)(struct blockdev *bd, uint64_t lba, uint32_t count,
                 const void *buf); /* may be 0: read-only device */
    int (*flush)(struct blockdev *bd); /* may be 0: nothing to flush */
    void *ctx;
};

#define BLOCK_ERR_OK     0
#define BLOCK_ERR_IO    -1
#define BLOCK_ERR_RANGE -2  /* lba out of range */

void block_init(void);
int  block_register(struct blockdev *bd);
struct blockdev *block_find(const char *name);
struct blockdev *block_get(size_t i);
size_t block_count(void);

#endif
