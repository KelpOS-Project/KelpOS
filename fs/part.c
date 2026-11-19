/* fs/part.c — MBR partition block-device wrappers. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "block.h"
#include "heap.h"
#include "part.h"

struct part_bd {
    struct blockdev bd;
    struct blockdev *parent;
    uint64_t start;   /* first sector on the parent */
    char name[16];
};

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int part_read(struct blockdev *bd, uint64_t lba, uint32_t count,
                     void *buf)
{
    struct part_bd *p = bd->ctx;
    if (lba + count > bd->blocks)
        return BLOCK_ERR_RANGE;
    return p->parent->read(p->parent, p->start + lba, count, buf);
}

static int part_write(struct blockdev *bd, uint64_t lba, uint32_t count,
                      const void *buf)
{
    struct part_bd *p = bd->ctx;
    if (!p->parent->write)
        return BLOCK_ERR_IO;
    if (lba + count > bd->blocks)
        return BLOCK_ERR_RANGE;
    return p->parent->write(p->parent, p->start + lba, count, buf);
}

static int part_flush(struct blockdev *bd)
{
    struct part_bd *p = bd->ctx;
    return p->parent->flush ? p->parent->flush(p->parent) : BLOCK_ERR_OK;
}

int part_scan(struct blockdev *disk)
{
    if (!disk || disk->blocks < 2)
        return 0;

    uint8_t mbr[512];
    if (disk->read(disk, 0, 1, mbr) != BLOCK_ERR_OK)
        return 0;
    if (mbr[510] != 0x55 || mbr[511] != 0xAA)
        return 0;

    int found = 0;
    for (int i = 0; i < 4; i++) {
        const uint8_t *e = mbr + 446 + i * 16;
        uint8_t type = e[4];
        uint32_t start = le32(e + 8);
        uint32_t count = le32(e + 12);
        if (!type || !count)
            continue;
        if ((uint64_t)start + count > disk->blocks)
            continue;

        struct part_bd *p = kmalloc(sizeof(*p));
        if (!p)
            break;
        memset(p, 0, sizeof(*p));
        p->parent = disk;
        p->start = start;
        snprintf(p->name, sizeof(p->name), "%s%d", disk->name, i + 1);
        p->bd.name = p->name;
        p->bd.blocks = count;
        p->bd.read = part_read;
        p->bd.write = disk->write ? part_write : 0;
        p->bd.flush = disk->flush ? part_flush : 0;
        p->bd.ctx = p;
        if (block_register(&p->bd) == 0)
            found++;
    }
    return found;
}
