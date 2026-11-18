#ifndef KELP_GLTEXTOBJ_H
#define KELP_GLTEXTOBJ_H

#include <stdint.h>

/* GLTexobj — TinyGL's texture-object demo (rotating textured quads),
 * built on the vendored TinyGL port (kernel/gfx/tinygl/). */

int gltexobj_init(int w, int h);
/* milliseconds since boot (integer: the kernel is built -mno-sse) */
void gltexobj_render(unsigned long long ms);

const uint32_t *gltexobj_pixels(void);
int gltexobj_width(void);
int gltexobj_height(void);

#endif
