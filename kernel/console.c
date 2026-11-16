/* kernel/console.c — Early console: serial COM1 + framebuffer text terminal.
 *
 * The framebuffer terminal is a color cell grid (8x16 cells, glyphs from
 * the embedded bitmap font kernel/font.c). It supports scrolling, cursor
 * rendering, backspace and tab, and exposes a small draw API that the
 * desktop and GUI apps build on. Output is always mirrored to the serial
 * port so boot logs remain capturable with -serial stdio / file:... */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <limine.h>
#include "console.h"
#include "serial.h"

extern const uint8_t font8x16[256][16];

/* ── Serial (always on) ─────────────────────────────────────────────── */

void console_init(void)
{
    serial_init();
}

/* ── Framebuffer text terminal ──────────────────────────────────────── */

#define CELL_W 8
#define CELL_H 16
#define TERM_MAX_COLS 160
#define TERM_MAX_ROWS 100

static struct {
    volatile uint32_t *px;
    uint32_t stride;
    uint64_t width, height;
    unsigned cols, rows;
    unsigned col, row;
    char text[TERM_MAX_COLS * TERM_MAX_ROWS];
    uint32_t fg[TERM_MAX_COLS * TERM_MAX_ROWS];
    uint32_t bg[TERM_MAX_COLS * TERM_MAX_ROWS];
    bool active;
} fb;

static uint32_t cur_fg = KELP_FG;
static uint32_t cur_bg = KELP_BG;

unsigned console_cols(void) { return fb.active ? fb.cols : 0; }
unsigned console_rows(void) { return fb.active ? fb.rows : 0; }
bool console_fb_active(void) { return fb.active; }

static void fb_draw_glyph(unsigned col, unsigned row, char ch,
                          uint32_t fgc, uint32_t bgc, bool cursor)
{
    const uint8_t *g = font8x16[(uint8_t)ch];
    uint32_t x0 = col * CELL_W, y0 = row * CELL_H;
    for (unsigned r = 0; r < CELL_H; r++) {
        volatile uint32_t *line = fb.px + (uint64_t)(y0 + r) * fb.stride + x0;
        uint8_t bits = g[r];
        for (unsigned c = 0; c < CELL_W; c++) {
            bool on = (bits & (0x80u >> c)) != 0;
            uint32_t colr = on ? fgc : bgc;
            if (cursor && r >= CELL_H - 2)
                colr = ~colr | 0xff000000u; /* underline cursor */
            line[c] = colr;
        }
    }
}

/* repaint cell (col,row) from the cell buffers */
static void fb_paint(unsigned col, unsigned row, bool cursor)
{
    unsigned idx = row * fb.cols + col;
    fb_draw_glyph(col, row, fb.text[idx], fb.fg[idx], fb.bg[idx], cursor);
}

static void fb_set_cell(unsigned col, unsigned row, char ch,
                        uint32_t fgc, uint32_t bgc)
{
    unsigned idx = row * fb.cols + col;
    fb.text[idx] = ch;
    fb.fg[idx] = fgc;
    fb.bg[idx] = bgc;
    fb_draw_glyph(col, row, ch, fgc, bgc, false);
}

void console_draw_cell(unsigned col, unsigned row, char ch,
                       uint32_t fgc, uint32_t bgc)
{
    if (!fb.active || col >= fb.cols || row >= fb.rows)
        return;
    fb_set_cell(col, row, ch, fgc, bgc);
}

void console_fill(unsigned col0, unsigned row0, unsigned col1, unsigned row1,
                  uint32_t bgc)
{
    if (!fb.active)
        return;
    if (col1 >= fb.cols)
        col1 = fb.cols - 1;
    if (row1 >= fb.rows)
        row1 = fb.rows - 1;
    for (unsigned r = row0; r <= row1 && r < fb.rows; r++)
        for (unsigned c = col0; c <= col1 && c < fb.cols; c++)
            fb_set_cell(c, r, ' ', bgc, bgc);
}

void console_set_colors(uint32_t fgc, uint32_t bgc)
{
    cur_fg = fgc;
    cur_bg = bgc;
}

static void fb_scroll(void)
{
    if (!fb.active)
        return;

    /* shift cell buffers up by one row */
    for (unsigned r = 1; r < fb.rows; r++)
        for (unsigned c = 0; c < fb.cols; c++) {
            unsigned to = (r - 1) * fb.cols + c;
            unsigned from = r * fb.cols + c;
            fb.text[to] = fb.text[from];
            fb.fg[to] = fb.fg[from];
            fb.bg[to] = fb.bg[from];
        }
    for (unsigned c = 0; c < fb.cols; c++) {
        unsigned idx = (fb.rows - 1) * fb.cols + c;
        fb.text[idx] = 0;
        fb.fg[idx] = cur_fg;
        fb.bg[idx] = cur_bg;
    }

    /* pixels: shift everything up by one cell row */
    for (uint64_t y = 0; y < fb.height - CELL_H; y++)
        for (uint64_t x = 0; x < fb.width; x++)
            fb.px[y * fb.stride + x] = fb.px[(y + CELL_H) * fb.stride + x];
    for (uint64_t y = fb.height - CELL_H; y < fb.height; y++)
        for (uint64_t x = 0; x < fb.width; x++)
            fb.px[y * fb.stride + x] = cur_bg;

    /* last visible row lost its pixels — repaint from the cell buffers */
    for (unsigned c = 0; c < fb.cols; c++)
        fb_draw_glyph(c, fb.rows - 1, fb.text[(fb.rows - 1) * fb.cols + c],
                      fb.fg[(fb.rows - 1) * fb.cols + c],
                      fb.bg[(fb.rows - 1) * fb.cols + c], false);
}

static void fb_putc(char c)
{
    if (!fb.active)
        return;

    /* remove cursor from current cell before moving */
    fb_paint(fb.col, fb.row, false);

    switch (c) {
    case '\n':
        fb.col = 0;
        fb.row++;
        break;
    case '\r':
        fb.col = 0;
        break;
    case '\b':
        if (fb.col > 0) {
            fb.col--;
            fb_set_cell(fb.col, fb.row, ' ', cur_fg, cur_bg);
        }
        break;
    case '\t':
        fb.col = (fb.col + 8) & ~7u;
        if (fb.col >= fb.cols)
            fb.col = fb.cols - 1;
        break;
    default:
        if (c < 32)
            break;
        if (fb.col >= fb.cols) {
            fb.col = 0;
            fb.row++;
        }
        if (fb.row >= fb.rows) {
            fb_scroll();
            fb.row = fb.rows - 1;
        }
        fb_set_cell(fb.col, fb.row, c, cur_fg, cur_bg);
        fb.col++;
        break;
    }

    if (fb.row >= fb.rows) {
        fb_scroll();
        fb.row = fb.rows - 1;
    }
    fb_paint(fb.col, fb.row, true);
}

void console_fb_init(struct limine_framebuffer *f)
{
    if (!f || f->memory_model != LIMINE_FRAMEBUFFER_RGB || f->bpp != 32)
        return;

    fb.px = (volatile uint32_t *)(uintptr_t)f->address;
    fb.stride = (uint32_t)(f->pitch / 4);
    fb.width = f->width;
    fb.height = f->height;
    fb.cols = (unsigned)(fb.width / CELL_W);
    fb.rows = (unsigned)(fb.height / CELL_H);
    fb.col = fb.row = 0;
    fb.active = true;

    for (unsigned r = 0; r < fb.rows; r++)
        for (unsigned c = 0; c < fb.cols; c++) {
            unsigned idx = r * fb.cols + c;
            fb.text[idx] = 0;
            fb.fg[idx] = cur_fg;
            fb.bg[idx] = cur_bg;
        }
}

void console_clear(void)
{
    if (fb.active) {
        for (uint64_t y = 0; y < fb.height; y++)
            for (uint64_t x = 0; x < fb.width; x++)
                fb.px[y * fb.stride + x] = cur_bg;
        for (unsigned r = 0; r < fb.rows; r++)
            for (unsigned c = 0; c < fb.cols; c++) {
                unsigned idx = r * fb.cols + c;
                fb.text[idx] = 0;
                fb.fg[idx] = cur_fg;
                fb.bg[idx] = cur_bg;
            }
        fb.col = fb.row = 0;
        fb_paint(0, 0, true);
    }
}

void console_splash(struct limine_framebuffer *f)
{
    if (!f || f->memory_model != LIMINE_FRAMEBUFFER_RGB || f->bpp != 32)
        return;

    volatile uint32_t *px = (volatile uint32_t *)(uintptr_t)f->address;
    uint32_t stride = (uint32_t)(f->pitch / 4);

    for (uint64_t y = 0; y < f->height; y++)
        for (uint64_t x = 0; x < f->width; x++) {
            uint32_t r = (uint32_t)(y * 32 / f->height);
            uint32_t b = (uint32_t)(x * 48 / f->width);
            px[y * stride + x] = (r << 16) | (0x08 << 8) | b;
        }
}

/* ── Unified output ─────────────────────────────────────────────────── */

void kputc(int c)
{
    if (c == '\n')
        serial_putc('\r');
    serial_putc((char)c);

    if (c == '\n' || c == '\f')
        fb_putc('\n');
    else if (c != '\r')
        fb_putc((char)c);
}
