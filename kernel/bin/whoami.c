#include "commands.h"

#include "../../drivers/vga_terminal.h"

void cmd_whoami(const char *args)
{
    (void)args;
    terminal_write_string("kernel (ring 0; users and processes are not implemented)\n");
}