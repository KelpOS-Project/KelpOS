#include "commands.h"

#include "../../drivers/vga_terminal.h"

void cmd_arch(const char *args)
{
    (void)args;
    terminal_write_string("x86_64\n");
}