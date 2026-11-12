#include "../graphics/framebuffer.h"
#include "../graphics/font.h"
#include "../graphics/desktop.h"
#include "../drivers/vga_terminal.h"

static const char *test_keys;
static struct ps2_mouse_state test_mouse_events[4];
static size_t test_mouse_event_count;
static size_t test_mouse_event_index;
static unsigned int test_keyboard_idle_reads;
static bool test_mouse_enabled;

bool keyboard_read_char(char *character)
{
    if (test_keyboard_idle_reads != 0) {
        test_keyboard_idle_reads--;
        return false;
    }
    if (test_keys == NULL || *test_keys == '\0') {
        return false;
    }
    *character = *test_keys++;
    return true;
}

bool ps2_mouse_initialize(size_t width, size_t height)
{
    (void)width;
    (void)height;
    test_mouse_event_index = 0;
    return test_mouse_enabled;
}

bool ps2_mouse_poll(struct ps2_mouse_state *state)
{
    if (!test_mouse_enabled || test_mouse_event_index >= test_mouse_event_count) {
        return false;
    }
    *state = test_mouse_events[test_mouse_event_index++];
    return true;
}

void x86_port_write8(uint16_t port, uint8_t value)
{
    (void)port;
    (void)value;
}

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    uint32_t storage[4 * 3];
    memset(storage, 0, sizeof(storage));
    struct graphics_framebuffer framebuffer;

    assert(!graphics_framebuffer_initialize(&framebuffer, storage, 0, 3, 16));
    assert(!graphics_framebuffer_initialize(&framebuffer, storage, 4, 3, 15));
    assert(!graphics_framebuffer_initialize(&framebuffer, NULL, 4, 3, 16));
    assert(graphics_framebuffer_initialize(&framebuffer, storage, 3, 3, 16));

    assert(graphics_put_pixel(&framebuffer, 2, 1, 0x11223344));
    assert(storage[1 * 4 + 2] == 0x11223344);
    assert(storage[1 * 4 + 3] == 0);
    assert(!graphics_put_pixel(&framebuffer, 3, 1, 0xFFFFFFFF));

    assert(graphics_fill_rect(&framebuffer, 1, 0, 2, 2, 0xAABBCCDD));
    assert(storage[1] == 0xAABBCCDD);
    assert(storage[2] == 0xAABBCCDD);
    assert(storage[5] == 0xAABBCCDD);
    assert(storage[6] == 0xAABBCCDD);
    assert(storage[3] == 0);
    assert(!graphics_fill_rect(&framebuffer, 2, 2, 2, 1, 0));

    uint32_t font_storage[8 * 8];
    memset(font_storage, 0, sizeof(font_storage));
    assert(graphics_framebuffer_initialize(&framebuffer, font_storage, 8, 8, 8 * sizeof(uint32_t)));
    assert(graphics_draw_text(&framebuffer, 0, 0, "A", 0x12345678, 1));
    assert(font_storage[1] == 0x12345678);
    assert(font_storage[0] == 0);
    assert(graphics_draw_text(&framebuffer, 0, 0, "a", 0xABCDEF01, 1));
    assert(font_storage[1] == 0xABCDEF01);

    static uint32_t desktop_pixels[640 * 480];
    const char *lines[6] = {"READY", "", "", "", "", ""};
    assert(graphics_framebuffer_initialize(&framebuffer, desktop_pixels,
        640, 480, 640 * sizeof(uint32_t)));
    desktop_draw(&framebuffer, false, lines, "");
    assert(desktop_pixels[0] == 0x121B29); /* top panel */
    assert(desktop_pixels[40 * 640 + 1] == 0x202C3D); /* wallpaper */
    assert(desktop_pixels[200 * 640 + 300] == 0x202C3D); /* empty desktop */
    desktop_draw(&framebuffer, true, lines, "help");
    assert(desktop_pixels[200 * 640 + 300] == 0x101923); /* terminal */
    const struct ps2_mouse_state mouse = { .x = 100, .y = 100, .buttons = 0 };
    desktop_draw_pointer(&framebuffer, false, lines, "", &mouse);
    assert(desktop_pixels[100 * 640 + 100] == 0x101820);
    assert(desktop_pixels[103 * 640 + 103] == 0xFFFFFF);

    terminal_use_framebuffer(&framebuffer);
    terminal_initialize();
    terminal_write_string("A");
    assert(desktop_pixels[1] == 0xAAAAAA);
    terminal_clear();
    assert(desktop_pixels[1] == 0x000000);

    desktop_set_framebuffer(&framebuffer);
    test_mouse_events[0] = (struct ps2_mouse_state) {
        .x = 210, .y = 440, .buttons = 1,
    };
    test_mouse_events[1] = (struct ps2_mouse_state) {
        .x = 210, .y = 440, .buttons = 0,
    };
    test_mouse_events[2] = (struct ps2_mouse_state) {
        .x = 595, .y = 67, .buttons = 1,
    };
    test_mouse_events[3] = (struct ps2_mouse_state) {
        .x = 595, .y = 67, .buttons = 0,
    };
    test_mouse_event_count = 4;
    test_keyboard_idle_reads = 3;
    test_keys = "\x1b";
    assert(desktop_start());
    assert(*test_keys == '\0');
    test_keys = NULL;
    test_mouse_enabled = false;
    test_mouse_event_count = 0;
    test_keyboard_idle_reads = 0;
    test_keys = "\x1b";
    assert(desktop_start());
    test_keyboard_idle_reads = 1;
    assert(*test_keys == '\0');
    test_keys = NULL;
    terminal_use_framebuffer(NULL);

    test_mouse_enabled = false;
    puts("graphics framebuffer/font/desktop tests passed");
    return 0;
}