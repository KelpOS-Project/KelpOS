#include "mouse.h"

#include "x86_io.h"

enum {
    DATA_PORT = 0x60,
    STATUS_PORT = 0x64,
    COMMAND_PORT = 0x64,
    OUTPUT_READY = 1,
    INPUT_BUSY = 2,
    AUXILIARY_DATA = 0x20,
    POLL_LIMIT = 100000,
};

static struct ps2_mouse_state mouse_state;
static size_t screen_width;
static size_t screen_height;
static uint8_t packet[3];
static unsigned int packet_index;
static bool initialized;

static bool wait_input_ready(void)
{
    for (unsigned int attempt = 0; attempt < POLL_LIMIT; attempt++) {
        if ((x86_port_read8(STATUS_PORT) & INPUT_BUSY) == 0) {
            return true;
        }
    }
    return false;
}

static bool read_output(uint8_t *value, bool auxiliary)
{
    for (unsigned int attempt = 0; attempt < POLL_LIMIT; attempt++) {
        const uint8_t status = x86_port_read8(STATUS_PORT);
        if ((status & OUTPUT_READY) != 0) {
            const uint8_t data = x86_port_read8(DATA_PORT);
            if (((status & AUXILIARY_DATA) != 0) == auxiliary) {
                *value = data;
                return true;
            }
        }
    }
    return false;
}

static bool write_controller(uint8_t command)
{
    if (!wait_input_ready()) {
        return false;
    }
    x86_port_write8(COMMAND_PORT, command);
    return true;
}

static bool write_auxiliary(uint8_t value)
{
    if (!write_controller(0xD4) || !wait_input_ready()) {
        return false;
    }
    x86_port_write8(DATA_PORT, value);
    uint8_t response;
    return read_output(&response, true) && response == 0xFA;
}

bool ps2_mouse_initialize(size_t width, size_t height)
{
    initialized = false;
    if (width == 0 || height == 0 || !write_controller(0xA8)
            || !write_controller(0x20)) {
        return false;
    }

    uint8_t configuration;
    if (!read_output(&configuration, false)
            || !write_controller(0x60)
            || !wait_input_ready()) {
        return false;
    }
    configuration &= (uint8_t)~0x22;
    x86_port_write8(DATA_PORT, configuration);

    if (!write_auxiliary(0xF6) || !write_auxiliary(0xF4)) {
        return false;
    }

    screen_width = width;
    screen_height = height;
    mouse_state.x = width / 2;
    mouse_state.y = height / 2;
    mouse_state.buttons = 0;
    packet_index = 0;
    initialized = true;
    return true;
}

bool ps2_mouse_poll(struct ps2_mouse_state *state)
{
    if (!initialized || state == NULL) {
        return false;
    }

    bool changed = false;
    for (;;) {
        const uint8_t status = x86_port_read8(STATUS_PORT);
        if ((status & OUTPUT_READY) == 0) {
            break;
        }
        if ((status & AUXILIARY_DATA) == 0) {
            break;
        }

        const uint8_t data = x86_port_read8(DATA_PORT);
        if (packet_index == 0 && (data & 0x08) == 0) {
            continue;
        }
        packet[packet_index++] = data;
        if (packet_index != 3) {
            continue;
        }
        packet_index = 0;

        const size_t old_x = mouse_state.x;
        const size_t old_y = mouse_state.y;
        const uint8_t old_buttons = mouse_state.buttons;
        if ((packet[0] & 0xC0) == 0) {
            int dx = packet[1];
            int dy = packet[2];
            if ((packet[0] & 0x10) != 0) {
                dx -= 256;
            }
            if ((packet[0] & 0x20) != 0) {
                dy -= 256;
            }
            int x = (int)mouse_state.x + dx;
            int y = (int)mouse_state.y - dy;
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if ((size_t)x >= screen_width) x = (int)screen_width - 1;
            if ((size_t)y >= screen_height) y = (int)screen_height - 1;
            mouse_state.x = (size_t)x;
            mouse_state.y = (size_t)y;
        }
        mouse_state.buttons = packet[0] & 0x07;
        changed |= old_x != mouse_state.x || old_y != mouse_state.y
            || old_buttons != mouse_state.buttons;
    }

    *state = mouse_state;
    return changed;
}