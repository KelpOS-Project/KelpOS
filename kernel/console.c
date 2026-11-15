/* kernel/console.c — Early console: serial COM1 + framebuffer text terminal.
 *
 * The framebuffer terminal draws glyphs from the embedded 8x16 bitmap font
 * (kernel/font.c) and supports scrolling, cursor rendering, backspace and
 * tab. Output is always mirrored to the serial port so boot logs remain
 * capturable with -serial stdio / file:... */
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
#define FG_COLOR 0xd8d8d8u
#define BG_COLOR 0x10141au

static struct {
    volatile uint32_t *px;
    uint32_t stride;
    uint64_t width, height;
    unsigned cols, rows;
    unsigned col, row;
    char text[160 * 100]; /* screen content for cursor restore + scroll */
    bool active;
} fb;

static void fb_draw_char(unsigned col, unsigned row, char ch, bool cursor)
{
    if (!fb.active)
        return;
    const uint8_t *g = font8x16[(uint8_t)ch];
    uint32_t x0 = col * CELL_W, y0 = row * CELL_H;
    for (unsigned r = 0; r < CELL_H; r++) {
        volatile uint32_t *line = fb.px + (uint64_t)(y0 + r) * fb.stride + x0;
        uint8_t bits = g[r];
        for (unsigned c = 0; c < CELL_W; c++) {
            bool on = (bits & (0x80u >> c)) != 0;
            uint32_t colr = on ? FG_COLOR : BG_COLOR;
            if (cursor && r >= CELL_H - 2)
                colr = ~colr | 0xff000000u; /* underline cursor */
            line[c] = colr;
        }
    }
}

static void fb_clear_row(unsigned row)
{
    if (!fb.active)
        return;
    for (unsigned c = 0; c < fb.cols; c++) {
        fb.text[row * fb.cols + c] = 0;
        fb_draw_char(c, row, ' ', false);
    }
}

static void fb_scroll(void)
{
    if (!fb.active)
        return;
    /* text buffer */
    for (unsigned r = 1; r < fb.rows; r++)
        for (unsigned c = 0; c < fb.cols; c++)
            fb.text[(r - 1) * fb.cols + c] = fb.text[r * fb.cols + c];
    for (unsigned c = 0; c < fb.cols; c++)
        fb.text[(fb.rows - 1) * fb.cols + c] = 0;
    /* pixels: shift everything up by one cell row */
    for (uint64_t y = 0; y < fb.height - CELL_H; y++)
        for (uint64_t x = 0; x < fb.width; x++)
            fb.px[y * fb.stride + x] = fb.px[(y + CELL_H) * fb.stride + x];
    for (uint64_t y = fb.height - CELL_H; y < fb.height; y++)
        for (uint64_t x = 0; x < fb.width; x++)
            fb.px[y * fb.stride + x] = BG_COLOR;
    /* repaint the (r-1)-th row: it lost its pixel content */
    for (unsigned c = 0; c < fb.cols; c++)
        fb_draw_char(c, fb.rows - 1, fb.text[(fb.rows - 1) * fb.cols + c], false);
}

static void fb_putc(char c)
{
    if (!fb.active)
        return;

    /* remove cursor from current cell before moving */
    fb_draw_char(fb.col, fb.row, fb.text[fb.row * fb.cols + fb.col], false);

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
            fb.text[fb.row * fb.cols + fb.col] = 0;
            fb_draw_char(fb.col, fb.row, ' ', false);
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
        fb.text[fb.row * fb.cols + fb.col] = c;
        fb_draw_char(fb.col, fb.row, c, false);
        fb.col++;
        break;
    }

    if (fb.row >= fb.rows) {
        fb_scroll();
        fb.row = fb.rows - 1;
    }
    fb_draw_char(fb.col, fb.row, fb.text[fb.row * fb.cols + fb.col], true);
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
        for (unsigned c = 0; c < fb.cols; c++)
            fb.text[r * fb.cols + c] = 0;
}

void console_clear(void)
{
    if (fb.active) {
        for (uint64_t y = 0; y < fb.height; y++)
            for (uint64_t x = 0; x < fb.width; x++)
                fb.px[y * fb.stride + x] = BG_COLOR;
        for (unsigned r = 0; r < fb.rows; r++)
            fb_clear_row(r);
        fb.col = fb.row = 0;
        fb_draw_char(0, 0, 0, true);
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
