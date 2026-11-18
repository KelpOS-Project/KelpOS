#ifndef KELP_TGL_SURFACE_H
#define KELP_TGL_SURFACE_H

#include <stdint.h>

/* KelpOS glue for TinyGL: one render surface per GL window.
 * TinyGL renders into its own 16-bit ZBuffer; tgl_surface_present()
 * converts the frame into an RGB32 buffer that the desktop blits. */

typedef struct tgl_surface tgl_surface;

/* w must be a multiple of 4 (TinyGL's RGB32 copy requirement) */
tgl_surface *tgl_surface_open(int w, int h);
void tgl_surface_make_current(tgl_surface *s);
void tgl_surface_present(tgl_surface *s);
const uint32_t *tgl_surface_pixels(const tgl_surface *s);
int tgl_surface_width(const tgl_surface *s);
int tgl_surface_height(const tgl_surface *s);

#endif
