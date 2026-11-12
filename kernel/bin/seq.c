#include "commands.h"

#include "../../drivers/vga_terminal.h"
#include "number.h"

void cmd_seq(const char *args)
{
    const char *cursor = args != NULL ? args : "";
    int32_t first;
    int32_t last;
    if (!number_parse_i32(&cursor, &first)) {
        terminal_write_string("Usage: seq END | seq START END (maximum 64 values)\n");
        return;
    }
    number_skip_spaces(&cursor);
    if (*cursor == '\0') {
        last = first;
        first = 1;
    } else if (!number_parse_i32(&cursor, &last)) {
        terminal_write_string("Usage: seq END | seq START END (maximum 64 values)\n");
        return;
    }
    number_skip_spaces(&cursor);
    if (*cursor != '\0') {
        terminal_write_string("Usage: seq END | seq START END (maximum 64 values)\n");
        return;
    }

    const int64_t distance = (int64_t)last - first;
    if (distance > 63 || distance < -63) {
        terminal_write_string("seq: range exceeds 64 values\n");
        return;
    }
    const int32_t step = first <= last ? 1 : -1;
    for (int32_t value = first;; value += step) {
        number_write_i64(value);
        terminal_write_string("\n");
        if (value == last) {
            break;
        }
    }
}