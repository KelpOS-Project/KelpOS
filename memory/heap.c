/* memory/heap.c — simpler linear (bump) allocator.
 * Prep work for PMM/VMM: kmalloc stays stable, kfree is a no-op for now. */
#include <stddef.h>
#include <stdint.h>
#include "heap.h"

static uint8_t *heap_base;
static size_t   heap_size, heap_used;

void heap_init(uintptr_t base, size_t size)
{
    /* 16-byte alignment */
    heap_base = (uint8_t *)((base + 15) & ~(uintptr_t)15);
    size -= (size_t)((uint8_t *)base - heap_base);
    heap_size = size & ~(size_t)15;
    heap_used = 0;
}

void *kmalloc(size_t n)
{
    if (!heap_base)
        return 0;
    n = (n + 15) & ~(size_t)15;
    if (heap_used + n > heap_size)
        return 0;
    void *p = heap_base + heap_used;
    heap_used += n;
    return p;
}

void kfree(void *p)
{
    (void)p; /* bump allocator: real free comes with PMM/VMM */
}

void heap_stats(size_t *used, size_t *total)
{
    if (used)
        *used = heap_used;
    if (total)
        *total = heap_size;
}
