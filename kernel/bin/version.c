#include "commands.h"

#include "../../drivers/vga_terminal.h"

void cmd_version(const char *args)
{
    (void)args;
    terminal_write_string("KelpOS development (x86_64, Multiboot v1)\n");
}