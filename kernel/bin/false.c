#include "commands.h"

#include "../../drivers/vga_terminal.h"

void cmd_false(const char *args)
{
    (void)args;
    terminal_write_string("false\n");
}