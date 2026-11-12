#include "commands.h"

#include "../../drivers/vga_terminal.h"

void cmd_hostname(const char *args)
{
    (void)args;
    terminal_write_string("kelpos\n");
}