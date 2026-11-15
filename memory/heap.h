#ifndef KELP_HEAP_H
#define KELP_HEAP_H

#include <stddef.h>
#include <stdint.h>

void   heap_init(uintptr_t base, size_t size);
void  *kmalloc(size_t n);
void   kfree(void *p);
void   heap_stats(size_t *used, size_t *total);

#endif
