/* kernel/gfx/image.c — stb_image wrapper for KelpOS.
 *
 * stb_image returns RGBA bytes. The framebuffer and the wallpaper cache
 * use packed 0xAARRGGBB, so the four bytes of each pixel are repacked in
 * place into the same four-byte slot (no second buffer needed).
 */
#include <stdlib.h>
#define STBI_NO_STDIO
#include "stb_image.h"
#include "image.h"

int image_load_rgb32(const void *data, size_t len, image_rgb32 *out)
{
    if (!data || !len || !out || len > 0x7FFFFFFF)
        return -1;

    int w = 0, h = 0, n = 0;
    unsigned char *px = stbi_load_from_memory((const stbi_uc *)data,
                                              (int)len, &w, &h, &n, 4);
    if (!px || w <= 0 || h <= 0)
        return -1;

    uint32_t *pix = (uint32_t *)px;
    size_t count = (size_t)w * (size_t)h;
    for (size_t i = 0; i < count; i++) {
        unsigned char *p = px + i * 4;
        pix[i] = 0xFF000000u | ((uint32_t)p[0] << 16) |
                 ((uint32_t)p[1] << 8) | (uint32_t)p[2];
    }

    out->pixels = pix;
    out->w = w;
    out->h = h;
    return 0;
}

void image_free(image_rgb32 *img)
{
    if (!img)
        return;
    free(img->pixels); /* no-op on the bump heap */
    img->pixels = NULL;
    img->w = img->h = 0;
}
