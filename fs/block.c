/* fs/block.c — block device registry. */
#include <stddef.h>
#include <string.h>
#include "block.h"

#define MAX_BLOCKDEVS 8

static struct blockdev *devs[MAX_BLOCKDEVS];
static size_t devs_n;

void block_init(void)
{
    devs_n = 0;
}

int block_register(struct blockdev *bd)
{
    if (!bd || !bd->name || !bd->read)
        return -1;
    if (block_find(bd->name))
        return -1;
    if (devs_n >= MAX_BLOCKDEVS)
        return -1;
    devs[devs_n++] = bd;
    return 0;
}

struct blockdev *block_find(const char *name)
{
    for (size_t i = 0; i < devs_n; i++)
        if (strcmp(devs[i]->name, name) == 0)
            return devs[i];
    return 0;
}

struct blockdev *block_get(size_t i)
{
    return i < devs_n ? devs[i] : 0;
}

size_t block_count(void)
{
    return devs_n;
}
