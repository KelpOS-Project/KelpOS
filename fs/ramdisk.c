/* fs/ramdisk.c — memory-backed block device. */
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include "block.h"
#include "ramdisk.h"

#define SECTOR 512

struct ramdisk_ctx {
    uint8_t *base;
    size_t size;
};

static int ramdisk_read(struct blockdev *bd, uint64_t lba, uint32_t count,
                        void *buf)
{
    struct ramdisk_ctx *c = bd->ctx;
    uint64_t off = lba * SECTOR;
    if (off + (uint64_t)count * SECTOR > c->size)
        return BLOCK_ERR_RANGE;
    memcpy(buf, c->base + off, (size_t)count * SECTOR);
    return BLOCK_ERR_OK;
}

static int ramdisk_write(struct blockdev *bd, uint64_t lba, uint32_t count,
                         const void *buf)
{
    struct ramdisk_ctx *c = bd->ctx;
    uint64_t off = lba * SECTOR;
    if (off + (uint64_t)count * SECTOR > c->size)
        return BLOCK_ERR_RANGE;
    memcpy(c->base + off, buf, (size_t)count * SECTOR);
    return BLOCK_ERR_OK;
}

int ramdisk_register(const char *name, uintptr_t addr, size_t size)
{
    if (!name || !addr || size < SECTOR)
        return -1;

    static struct blockdev devs[4];
    static struct ramdisk_ctx ctxs[4];
    static size_t next;

    if (next >= 4)
        return -1;

    struct blockdev *bd = &devs[next];
    struct ramdisk_ctx *c = &ctxs[next];
    next++;

    c->base = (uint8_t *)addr;
    c->size = size;

    memset(bd, 0, sizeof(*bd));
    bd->name = name;
    bd->blocks = size / SECTOR;
    bd->read = ramdisk_read;
    bd->write = ramdisk_write;
    bd->ctx = c;
    return block_register(bd);
}
