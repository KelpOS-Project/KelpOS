#ifndef KELP_CONSOLE_H
#define KELP_CONSOLE_H

#include <stddef.h>

struct limine_framebuffer;

/* Early console: serial (COM1) + framebuffer text terminal (8x16 font).
 * All output goes to serial; the framebuffer terminal is used when
 * console_fb_init() has been given a usable RGB32 framebuffer. */

void console_init(void);
void console_splash(struct limine_framebuffer *fb);
void console_fb_init(struct limine_framebuffer *fb);
void console_clear(void);

#endif
