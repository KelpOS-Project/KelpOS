#ifndef KELPOS_DRIVERS_MOUSE_H
#define KELPOS_DRIVERS_MOUSE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ps2_mouse_state {
    size_t x;
    size_t y;
    uint8_t buttons;
};

bool ps2_mouse_initialize(size_t screen_width, size_t screen_height);
bool ps2_mouse_poll(struct ps2_mouse_state *state);

#endif