/* kernel/gfx/stb/stb_image_impl.c — stb_image compiled for KelpOS.
 *
 * Vendored copy of https://github.com/nothings/stb (stb_image.h,
 * public domain / MIT). KelpOS changes:
 *   - STBI_NO_STDIO: images are decoded from memory only
 *   - STBI_NO_SIMD: scalar decoder, no XMM helpers outside kernel/gfx
 *   - STBI_REALLOC_SIZED: klibc's realloc does not copy, so the
 *     decoder gets a copying wrapper (the bump allocator never frees)
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STBI_NO_STDIO
#define STBI_NO_SIMD
#define STBI_NO_THREAD_LOCALS
#define STBI_ASSERT(x) assert(x)
#define STBI_MALLOC(sz) malloc(sz)
#define STBI_FREE(p) free(p)

static void *stbi_realloc_sized(void *p, int oldsz, int newsz)
{
    if (newsz <= 0)
        return NULL;
    void *n = malloc((size_t)newsz);
    if (!n)
        return NULL;
    if (p && oldsz > 0)
        memcpy(n, p, (size_t)(oldsz < newsz ? oldsz : newsz));
    free(p);
    return n;
}

#define STBI_REALLOC_SIZED(p, oldsz, newsz) stbi_realloc_sized(p, oldsz, newsz)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
