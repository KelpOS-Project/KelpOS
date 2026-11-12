#include "commands.h"

#include "../../drivers/vga_terminal.h"

void cmd_true(const char *args)
{
    (void)args;
    terminal_write_string("true\n");
}