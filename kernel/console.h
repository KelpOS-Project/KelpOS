#ifndef KELP_CONSOLE_H
#define KELP_CONSOLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct limine_framebuffer;

/* Early console: serial (COM1) + framebuffer text terminal (8x16 font).
 * All output goes to serial; the framebuffer terminal is used when
 * console_fb_init() has been given a usable RGB32 framebuffer. The
 * terminal is a color cell grid: everything the shell, the desktop and
 * the GUI apps draw goes through the same cell model so scrolling and
 * the cursor stay consistent. */

void console_init(void);
void console_splash(struct limine_framebuffer *fb);
void console_fb_init(struct limine_framebuffer *fb);
void console_clear(void);

/* geometry of the text terminal (0 when fb terminal is inactive) */
unsigned console_cols(void);
unsigned console_rows(void);
bool console_fb_active(void);

/* color cell API (col/row in cells) */
void console_draw_cell(unsigned col, unsigned row, char ch,
                       uint32_t fg, uint32_t bg);
void console_fill(unsigned col0, unsigned row0,
                  unsigned col1, unsigned row1, uint32_t bg);

/* colors for subsequent kputc/kprintf output on the fb terminal */
void console_set_colors(uint32_t fg, uint32_t bg);

/* mouse cursor overlay (pixel position); show() saves the covered
 * pixels and draws the arrow, hide() restores them */
void console_cursor_show(int px, int py);
void console_cursor_hide(void);
void console_cursor_move(int px, int py);
void console_cursor_repaint(void);

/* default scheme */
#define KELP_FG 0xd8d8d8u
#define KELP_BG 0x10141au

#endif
