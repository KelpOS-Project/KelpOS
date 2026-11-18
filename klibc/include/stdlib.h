#ifndef KELP_KLIB_STDLIB_H
#define KELP_KLIB_STDLIB_H

#include <stddef.h>

void *malloc(size_t n);   /* -> memory/heap.c (kmalloc) */
void *calloc(size_t n, size_t size);
void  free(void *p);
void *realloc(void *p, size_t n);
int   abs(int x);
long  labs(long x);
int   atoi(const char *s);
long  strtol(const char *s, char **end, int base);
char *itoa(int value, char *buf, int base);

void qsort(void *base, size_t n, size_t size,
           int (*cmp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t n, size_t size,
              int (*cmp)(const void *, const void *));

/* deterministic PRNG (same sequence everywhere: good for tests) */
unsigned int rand(void);
void         srand(unsigned int seed);
#define RAND_MAX 0x7fffffffu

#endif
