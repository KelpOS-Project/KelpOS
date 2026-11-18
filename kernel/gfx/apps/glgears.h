#ifndef KELP_GLGEARS_H
#define KELP_GLGEARS_H

#include <stdint.h>

/* GLGears — the classic rotating-gears OpenGL demo, built on the
 * vendored TinyGL port (kernel/gfx/tinygl/). */

int glgears_init(int w, int h);
/* milliseconds since boot (integer: the kernel is built -mno-sse) */
void glgears_render(unsigned long long ms);

const uint32_t *glgears_pixels(void);
int glgears_width(void);
int glgears_height(void);

#endif
