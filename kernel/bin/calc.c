#include "commands.h"

#include <stdint.h>

#include "../../drivers/vga_terminal.h"
#include "number.h"

void cmd_calc(const char *args)
{
    const char *cursor = args != NULL ? args : "";
    int32_t left;
    int32_t right;
    number_skip_spaces(&cursor);
    if (!number_parse_i32(&cursor, &left)) {
        terminal_write_string("Usage: calc INTEGER OP INTEGER\n");
        return;
    }
    number_skip_spaces(&cursor);
    const char operation = *cursor++;
    number_skip_spaces(&cursor);
    if (!number_parse_i32(&cursor, &right)) {
        terminal_write_string("Usage: calc INTEGER OP INTEGER\n");
        return;
    }
    number_skip_spaces(&cursor);
    if (*cursor != '\0') {
        terminal_write_string("Usage: calc INTEGER OP INTEGER\n");
        return;
    }

    int64_t result;
    switch (operation) {
    case '+': result = (int64_t)left + right; break;
    case '-': result = (int64_t)left - right; break;
    case '*': result = (int64_t)left * right; break;
    case '/':
        if (right == 0) {
            terminal_write_string("calc: division by zero\n");
            return;
        }
        if (left == INT32_MIN && right == -1) {
            terminal_write_string("calc: result out of range\n");
            return;
        }
        result = left / right;
        break;
    case '%':
        if (right == 0) {
            terminal_write_string("calc: division by zero\n");
            return;
        }
        if (left == INT32_MIN && right == -1) {
            terminal_write_string("calc: result out of range\n");
            return;
        }
        result = left % right;
        break;
    default:
        terminal_write_string("calc: supported operators are + - * / %\n");
        return;
    }
    number_write_i64(result);
    terminal_write_string("\n");
}