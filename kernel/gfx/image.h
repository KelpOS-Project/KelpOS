#ifndef KELP_IMAGE_H
#define KELP_IMAGE_H

#include <stddef.h>
#include <stdint.h>

/* RGB32 image decoding for the desktop (stb_image, kernel/gfx/stb/).
 * The buffer is 0xAARRGGBB with A = 0xFF, i.e. the format
 * console_blit_rgb32() expects. Decoded buffers live on the bump heap;
 * image_free() therefore only exists for symmetry. */

typedef struct {
    uint32_t *pixels;
    int w, h;
} image_rgb32;

/* decode PNG, JPEG, BMP, GIF, TGA, ... from memory; 0 on success */
int image_load_rgb32(const void *data, size_t len, image_rgb32 *out);

void image_free(image_rgb32 *img);

#endif
