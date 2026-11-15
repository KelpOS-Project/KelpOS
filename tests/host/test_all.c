/* tests/host/test_all.c — unit tests for klibc + heap (native, no QEMU needed) */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "heap.h"

static char heapbuf[1 << 20];

static void test_string(void)
{
    char b[32];

    assert(strlen("abc") == 3);
    assert(strcmp("abc", "abc") == 0);
    assert(strcmp("abc", "abd") < 0);
    assert(strncmp("abcdef", "abcxyz", 3) == 0);

    memset(b, 'x', 5);
    b[5] = 0;
    assert(strcmp(b, "xxxxx") == 0);

    memcpy(b, "hello", 6);
    assert(strcmp(b, "hello") == 0);

    memcpy(b, "abcdef", 6);
    memmove(b + 2, b, 4);
    b[6] = 0;
    assert(strcmp(b, "ababcd") == 0);

    assert(strchr("hello", 'l') && *strchr("hello", 'l') == 'l');
    assert(strchr("hello", 'z') == 0);
    assert(strrchr("hello", 'l') == strchr("hello", 'l') + 1);
    assert(strstr("haystack", "stack") != 0);
    assert(strstr("short", "longer") == 0);

    assert(memchr("hello", 'e', 5) == strchr("hello", 'e'));
    assert(memchr("hello", 'z', 5) == 0);

    strcpy(b, "ab");
    strncat(b, "cdef", 2);
    assert(strcmp(b, "abcd") == 0);

    assert(strcasecmp("HeLLo", "hello") == 0);
    assert(strcasecmp("hello", "help") < 0);
    assert(strncasecmp("WORLD", "worm", 3) == 0);
}

static void test_ctype(void)
{
    assert(isdigit('5') && !isdigit('x'));
    assert(isalpha('q') && !isalpha('3'));
    assert(isalnum('Z') && !isalnum('-'));
    assert(isspace(' ') && isspace('\t') && !isspace('a'));
    assert(toupper('a') == 'A' && tolower('Q') == 'q');
    assert(isxdigit('f') && isxdigit('B') && !isxdigit('g'));
}

static void test_printf(void)
{
    char out[64];

    snprintf(out, sizeof(out), "%d %s %x %u", -42, "hi", 255, 7u);
    assert(strcmp(out, "-42 hi ff 7") == 0);

    snprintf(out, sizeof(out), "%llx", 0xdeadbeefULL);
    assert(strcmp(out, "deadbeef") == 0);

    snprintf(out, sizeof(out), "%p", (void *)0x1234);
    assert(strcmp(out, "0x1234") == 0);

    snprintf(out, sizeof(out), "%c%c", 'o', 'k');
    assert(strcmp(out, "ok") == 0);

    snprintf(out, sizeof(out), "%03d|%5d|%08x", 7, 42, 0xBEEF);
    assert(strcmp(out, "007|   42|0000beef") == 0);

    snprintf(out, sizeof(out), "%6s!", "hi");
    assert(strcmp(out, "    hi!") == 0);

    snprintf(out, sizeof(out), "%o|%03o", 8, 5);
    assert(strcmp(out, "10|005") == 0);

    snprintf(out, sizeof(out), "[%-9s]", "crc");
    assert(strcmp(out, "[crc      ]") == 0);

    snprintf(out, sizeof(out), "[%0-5d]", 42);
    assert(strcmp(out, "[42   ]") == 0);
}

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return x < y ? -1 : x > y;
}

static void test_stdlib_heap(void)
{
    assert(atoi("123") == 123);
    assert(atoi("-17") == -17);
    assert(atoi("  42abc") == 42);
    assert(abs(-5) == 5 && abs(5) == 5);

    char b[16];
    itoa(255, b, 10);
    assert(strcmp(b, "255") == 0);
    itoa(255, b, 16);
    assert(strcmp(b, "ff") == 0);

    heap_init((uintptr_t)heapbuf, sizeof(heapbuf));
    void *p = malloc(100);
    assert(p != 0);
    memset(p, 1, 100);
    free(p);

    size_t used, total;
    heap_stats(&used, &total);
    assert(used >= 112 && total > 0);
}

static void test_stdlib_more(void)
{
    char *end = 0;
    assert(strtol("42", 0, 10) == 42);
    assert(strtol("-42x", &end, 10) == -42 && *end == 'x');
    assert(strtol("0x1f", 0, 0) == 31);
    assert(strtol("0755", 0, 0) == 493);
    assert(strtol("ff", 0, 16) == 255);
    assert(strtol("  +7", 0, 10) == 7);
    assert(labs(-100L) == 100L && labs(3L) == 3L);

    int arr[] = { 5, 3, 9, 1, 7, 3, 0 };
    qsort(arr, 7, sizeof(int), cmp_int);
    int want[] = { 0, 1, 3, 3, 5, 7, 9 };
    for (int i = 0; i < 7; i++)
        assert(arr[i] == want[i]);

    int key = 7;
    int *hit = bsearch(&key, arr, 7, sizeof(int), cmp_int);
    assert(hit && *hit == 7);
    key = 4;
    assert(bsearch(&key, arr, 7, sizeof(int), cmp_int) == 0);

    srand(1234);
    unsigned r1 = rand(), r2 = rand();
    srand(1234);
    assert(rand() == r1 && rand() == r2); /* deterministic */
    assert(r1 != r2);                     /* ... but not constant */
}

static void test_heap_strdup(void)
{
    heap_init((uintptr_t)heapbuf, sizeof(heapbuf));
    char *s = strdup("kelp");
    assert(s && strcmp(s, "kelp") == 0);
    free(s);
}

int main(void)
{
    test_string();
    test_ctype();
    test_printf();
    test_stdlib_heap();
    test_stdlib_more();
    test_heap_strdup();
    kputs("ALL TESTS PASSED");
    return 0;
}
