#ifndef KELPOS_KERNEL_BIN_NUMBER_H
#define KELPOS_KERNEL_BIN_NUMBER_H

#include <stdbool.h>
#include <stdint.h>

#include "../../drivers/vga_terminal.h"

static inline void number_skip_spaces(const char **text)
{
    while (**text == ' ' || **text == '\t') {
        (*text)++;
    }
}

static inline bool number_parse_i32(const char **text, int32_t *result)
{
    number_skip_spaces(text);
    bool negative = false;
    if (**text == '+' || **text == '-') {
        negative = **text == '-';
        (*text)++;
    }
    if (**text < '0' || **text > '9') {
        return false;
    }

    const uint32_t limit = negative ? (uint32_t)INT32_MAX + 1u : (uint32_t)INT32_MAX;
    uint32_t value = 0;
    while (**text >= '0' && **text <= '9') {
        const uint32_t digit = (uint32_t)(**text - '0');
        if (value > (limit - digit) / 10u) {
            return false;
        }
        value = value * 10u + digit;
        (*text)++;
    }
    *result = negative
        ? (value == (uint32_t)INT32_MAX + 1u ? INT32_MIN : -(int32_t)value)
        : (int32_t)value;
    return true;
}

static inline void number_write_i64(int64_t value)
{
    char digits[20];
    size_t count = 0;
    uint64_t magnitude;
    if (value < 0) {
        terminal_write_string("-");
        magnitude = (uint64_t)0 - (uint64_t)value;
    } else {
        magnitude = (uint64_t)value;
    }
    do {
        digits[count++] = (char)('0' + magnitude % 10u);
        magnitude /= 10u;
    } while (magnitude != 0);
    while (count != 0) {
        terminal_write(&digits[--count], 1);
    }
}

#endif