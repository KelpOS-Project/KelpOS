#include "vga_terminal.h"

#include "../base/string.h"
#include "../graphics/font.h"
#include "x86_io.h"

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
};

enum {
    VGA_WIDTH = 80,
    VGA_HEIGHT = 25,
    VGA_CELL_COUNT = VGA_WIDTH * VGA_HEIGHT,
};

static volatile uint16_t *const terminal_buffer = (volatile uint16_t *)0xB8000;
static struct graphics_framebuffer *graphics_console;
static size_t terminal_row;
static size_t terminal_column;
static size_t terminal_width = VGA_WIDTH;
static size_t terminal_height = VGA_HEIGHT;
static uint8_t terminal_color;

static uint32_t console_color(uint8_t color, bool background)
{
    static const uint32_t colors[8] = {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
        0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    };
    const unsigned int index = background ? (color >> 4) & 7 : color & 7;
    return colors[index];
}

static void update_cursor(size_t x, size_t y)
{
    if (graphics_console != NULL) {
        return;
    }
    uint16_t pos = y * VGA_WIDTH + x;
    x86_port_write8(0x3D4, 0x0F);
    x86_port_write8(0x3D5, (uint8_t)(pos & 0xFF));
    x86_port_write8(0x3D4, 0x0E);
    x86_port_write8(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static uint8_t vga_entry_color(enum vga_color foreground, enum vga_color background)
{
    return (uint8_t)(foreground | (uint8_t)(background << 4));
}

static uint16_t vga_entry(char character, uint8_t color)
{
    return (uint16_t)(uint8_t)character | (uint16_t)color << 8;
}

static void terminal_clear_row(size_t row)
{
    if (graphics_console != NULL) {
        graphics_fill_rect(graphics_console, 0, row * 9,
            graphics_console->width, 9, console_color(terminal_color, true));
        return;
    }
    for (size_t column = 0; column < terminal_width; column++) {
        terminal_buffer[row * VGA_WIDTH + column] = vga_entry(' ', terminal_color);
    }
}

static void terminal_scroll(void)
{
    if (graphics_console != NULL) {
        const size_t pitch = graphics_console->pitch_bytes / sizeof(uint32_t);
        for (size_t row = 0; row + 9 < graphics_console->height; row++) {
            for (size_t column = 0; column < graphics_console->width; column++) {
                graphics_console->pixels[row * pitch + column] =
                    graphics_console->pixels[(row + 9) * pitch + column];
            }
        }
        terminal_clear_row(terminal_height - 1);
        terminal_row = terminal_height - 1;
        return;
    }
    for (size_t index = 0; index < VGA_CELL_COUNT - VGA_WIDTH; index++) {
        terminal_buffer[index] = terminal_buffer[index + VGA_WIDTH];
    }

    terminal_clear_row(VGA_HEIGHT - 1);
    terminal_row = VGA_HEIGHT - 1;
}

static void terminal_put_entry(char character, size_t column, size_t row)
{
    if (graphics_console != NULL) {
        if (column < terminal_width && row < terminal_height) {
            const size_t x = column * 6;
            const size_t y = row * 9;
            graphics_fill_rect(graphics_console, x, y, 6, 9,
                console_color(terminal_color, true));
            char text[2] = { character, '\0' };
            graphics_draw_text(graphics_console, x, y,
                text, console_color(terminal_color, false), 1);
        }
    } else if (column < VGA_WIDTH && row < VGA_HEIGHT) {
        terminal_buffer[row * VGA_WIDTH + column] = vga_entry(character, terminal_color);
    }
}

static void terminal_putchar(char character)
{
    if (character == '\0') {
        return;
    }

    if (character == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else if (character == '\r') {
        terminal_column = 0;
    } else if (character == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            terminal_put_entry(' ', terminal_column, terminal_row);
        }
    } else if (character == '\t') {
        size_t spaces = 4 - terminal_column % 4;
        while (spaces-- > 0) {
            terminal_putchar(' ');
        }
    } else {
        terminal_put_entry(character, terminal_column, terminal_row);
        terminal_column++;
        if (terminal_column == terminal_width) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    if (terminal_row >= terminal_height) {
        terminal_scroll();
    }

    update_cursor(terminal_column, terminal_row);
}

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for (size_t row = 0; row < terminal_height; row++) {
        terminal_clear_row(row);
    }
    update_cursor(terminal_column, terminal_row);
}

void terminal_use_framebuffer(struct graphics_framebuffer *framebuffer)
{
    graphics_console = framebuffer;
    if (framebuffer != NULL && framebuffer->pixels != NULL) {
        terminal_width = framebuffer->width / 6;
        terminal_height = framebuffer->height / 9;
        if (terminal_width == 0) terminal_width = 1;
        if (terminal_height == 0) terminal_height = 1;
    } else {
        terminal_width = VGA_WIDTH;
        terminal_height = VGA_HEIGHT;
    }
}

void terminal_set_color(uint8_t color)
{
    terminal_color = color;
}

void terminal_write(const char *data, size_t size)
{
    if (data == NULL) {
        return;
    }

    for (size_t index = 0; index < size; index++) {
        terminal_putchar(data[index]);
    }
}

void terminal_write_string(const char *string)
{
    terminal_write(string, kstrlen(string));
}

void terminal_clear(void)
{
    for (size_t row = 0; row < terminal_height; row++) {
        terminal_clear_row(row);
    }
    terminal_row = 0;
    terminal_column = 0;
    update_cursor(terminal_column, terminal_row);
}
