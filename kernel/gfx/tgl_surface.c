/* kernel/gfx/tgl_surface.c — TinyGL render target for KelpOS windows.
 *
 * TinyGL keeps everything in a ZBuffer (16-bit colour + 16-bit depth by
 * default) and can convert a finished frame to packed RGB32. That is
 * exactly the format of the framebuffer, so presenting is one
 * ZB_copyFrameBuffer() into a malloc'd pixel buffer that the desktop
 * blits with console_blit_rgb32().
 */
#include <stdint.h>
#include <stdlib.h>
#include "zgl.h"
#include "zbuffer.h"
#include "tgl_surface.h"

struct tgl_surface {
    ZBuffer *zb;
    GLContext *ctx;
    uint32_t *pixels;
    int w, h;
};

tgl_surface *tgl_surface_open(int w, int h)
{
    /* ZB_copyFrameBuffer()'s RGB32 path converts four pixels per
     * iteration, so the width has to be a multiple of four. */
    if (w <= 0 || h <= 0 || (w & 3) || w > 2048 || h > 2048)
        return NULL;

    tgl_surface *s = malloc(sizeof(*s));
    if (!s)
        return NULL;
    s->pixels = malloc((size_t)w * (size_t)h * sizeof(uint32_t));
    if (!s->pixels)
        return NULL;
    s->w = w;
    s->h = h;

    s->zb = ZB_open(w, h, ZB_MODE_RGBA, 0, NULL, NULL, NULL);
    if (!s->zb)
        return NULL;

    glInit(s->zb);
    s->ctx = gl_get_context();
    return s;
}

void tgl_surface_make_current(tgl_surface *s)
{
    if (s)
        gl_ctx = s->ctx;
}

void tgl_surface_present(tgl_surface *s)
{
    if (!s || !s->zb || !s->pixels)
        return;
    ZB_copyFrameBuffer(s->zb, s->pixels, s->w * (int)sizeof(uint32_t));
}

const uint32_t *tgl_surface_pixels(const tgl_surface *s)
{
    return s ? s->pixels : NULL;
}

int tgl_surface_width(const tgl_surface *s) { return s ? s->w : 0; }
int tgl_surface_height(const tgl_surface *s) { return s ? s->h : 0; }
