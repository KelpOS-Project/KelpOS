#ifndef KELP_GEARS_H
#define KELP_GEARS_H

#include <stdint.h>

/* GLGears — the classic rotating-gears OpenGL demo, drawn with the
 * TinyGL subset (kernel/gfx/tgl.c) into an RGB32 buffer that the
 * desktop blits into a window. */

/* allocate the colour/depth buffers for a w*h view (idempotent) */
int gears_init(int w, int h);

/* render one frame at time t in seconds */
void gears_render(float t);

const uint32_t *gears_pixels(void);
int gears_width(void);
int gears_height(void);

#endif
