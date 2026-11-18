#ifndef KELP_GFX_ASSERT_H
#define KELP_GFX_ASSERT_H

/* TinyGL and stb_image assert on internal invariants. In the kernel
 * there is no libc abort(), so print the failure and stop: continuing
 * would mean drawing with broken state (or writing out of bounds). */

void kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#define assert(expr)                                                      \
    do {                                                                  \
        if (!(expr)) {                                                    \
            kprintf("gfx: assertion failed: %s (%s:%d)\n", #expr,         \
                    __FILE__, __LINE__);                                  \
            __builtin_trap();                                             \
        }                                                                 \
    } while (0)

#endif
