#include "../drivers/keyboard.h"
#include "../drivers/x86_io.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t current_scan_code;

uint8_t x86_port_read8(uint16_t port)
{
    if (port == 0x64) {
        return 1;
    }
    assert(port == 0x60);
    return current_scan_code;
}

void x86_port_write8(uint16_t port, uint8_t value)
{
    (void)port;
    (void)value;
}

static char read_key(uint8_t scan_code)
{
    char character = '\0';
    current_scan_code = scan_code;
    assert(keyboard_read_char(&character));
    return character;
}

static void key_event(uint8_t scan_code)
{
    char character;
    current_scan_code = scan_code;
    assert(!keyboard_read_char(&character));
}

int main(void)
{
    assert(keyboard_get_layout() == KEYBOARD_LAYOUT_ENGLISH);
    assert(read_key(0x15) == 'y');

    assert(keyboard_set_layout(KEYBOARD_LAYOUT_GERMAN));
    assert(read_key(0x15) == 'z');
    assert(read_key(0x2C) == 'y');

    assert(keyboard_set_layout(KEYBOARD_LAYOUT_FRENCH));
    assert(read_key(0x10) == 'a');
    assert(read_key(0x1E) == 'q');
    assert(read_key(0x2C) == 'w');
    assert(read_key(0x02) == '&');
    key_event(0x2A);
    assert(read_key(0x02) == '1');
    key_event(0xAA);

    assert(keyboard_set_layout(KEYBOARD_LAYOUT_GERMAN));
    key_event(0x2A);
    assert(read_key(0x03) == '"');
    key_event(0xAA);

    assert(keyboard_set_layout(KEYBOARD_LAYOUT_ENGLISH));
    assert(read_key(0x10) == 'q');
    assert(!keyboard_set_layout((enum keyboard_layout)99));
    assert(!keyboard_read_char(NULL));

    puts("Keyboard layout tests passed");
    return 0;
}