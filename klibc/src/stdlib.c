/* klibc/src/stdlib.c */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "heap.h"

void *malloc(size_t n)
{
    return kmalloc(n);
}

void *calloc(size_t n, size_t size)
{
    if (size && n > (size_t)-1 / size)
        return NULL;
    size_t total = n * size;
    void *p = kmalloc(total);
    if (p)
        memset(p, 0, total);
    return p;
}

void free(void *p)
{
    kfree(p);
}

int abs(int x)
{
    return x < 0 ? -x : x;
}

long labs(long x)
{
    return x < 0 ? -x : x;
}

/* bump allocator cannot grow in place and cannot know the old size:
 * realloc allocates a fresh block WITHOUT copying (documented limit;
 * callers that need the old content copy it themselves). */
void *realloc(void *p, size_t n)
{
    if (!p)
        return malloc(n);
    (void)p;
    return kmalloc(n);
}

int atoi(const char *s)
{
    int r = 0, neg = 0;
    while (isspace((unsigned char)*s))
        s++;
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (isdigit((unsigned char)*s))
        r = r * 10 + (*s++ - '0');
    return neg ? -r : r;
}

char *itoa(int value, char *buf, int base)
{
    char *p = buf;
    char tmp[33];
    int i = 0;
    unsigned v = (unsigned)value;
    bool neg = false;

    if (base == 10 && value < 0) {
        neg = true;
        v = (unsigned)(-(value + 1)) + 1U;
    }
    if (v == 0)
        tmp[i++] = '0';
    while (v) {
        unsigned d = v % (unsigned)base;
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned)base;
    }
    if (neg)
        *p++ = '-';
    while (i)
        *p++ = tmp[--i];
    *p = 0;
    return buf;
}

long strtol(const char *s, char **end, int base)
{
    long r = 0;
    int neg = 0;

    if (end)
        *end = (char *)s;
    while (isspace((unsigned char)*s))
        s++;
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    if ((base == 0 || base == 16) && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        base = 16;
    } else if (base == 0) {
        base = *s == '0' ? 8 : 10;
    }

    for (;; s++) {
        int d;
        if (isdigit((unsigned char)*s))
            d = *s - '0';
        else if (*s >= 'a' && *s < 'a' + base - 10)
            d = *s - 'a' + 10;
        else if (*s >= 'A' && *s < 'A' + base - 10)
            d = *s - 'A' + 10;
        else
            break;
        r = r * base + d;
    }
    if (end)
        *end = (char *)s;
    return neg ? -r : r;
}

/* in-place heapsort — compact, stack-free, O(n log n) */
static void sift_down(char *base, size_t size, size_t start, size_t n,
                      int (*cmp)(const void *, const void *))
{
    for (;;) {
        size_t child = 2 * start + 1;
        if (child >= n)
            return;
        if (child + 1 < n && cmp(base + child * size, base + (child + 1) * size) < 0)
            child++;
        if (cmp(base + start * size, base + child * size) >= 0)
            return;
        /* swap elements start/child byte by byte */
        char *a = base + start * size, *b = base + child * size;
        for (size_t i = 0; i < size; i++) {
            char t = a[i];
            a[i] = b[i];
            b[i] = t;
        }
        start = child;
    }
}

void qsort(void *base, size_t n, size_t size,
           int (*cmp)(const void *, const void *))
{
    char *b = base;
    if (!b || n < 2 || !size)
        return;
    for (size_t i = n / 2; i-- > 0;)
        sift_down(b, size, i, n, cmp);
    for (size_t end = n - 1; end > 0; end--) {
        for (size_t i = 0; i < size; i++) {
            char t = b[i];
            b[i] = b[end * size + i];
            b[end * size + i] = t;
        }
        sift_down(b, size, 0, end, cmp);
    }
}

void *bsearch(const void *key, const void *base, size_t n, size_t size,
              int (*cmp)(const void *, const void *))
{
    const char *b = base;
    while (n) {
        size_t mid = n / 2;
        const void *e = b + mid * size;
        int r = cmp(key, e);
        if (r == 0)
            return (void *)e;
        if (r < 0) {
            n = mid;
        } else {
            b = (const char *)e + size;
            n -= mid + 1;
        }
    }
    return 0;
}

/* LCG: x = x*1103515245 + 12345 (POSIX reference formula) */
static unsigned int rng_state = 1;

void srand(unsigned int seed)
{
    rng_state = seed;
}

unsigned int rand(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return (rng_state >> 16) & RAND_MAX;
}
