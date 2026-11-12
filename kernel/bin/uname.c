#include "commands.h"
#include "../../drivers/vga_terminal.h"

void cmd_uname(const char *args)
{
    (void)args;
    terminal_write_string("KelpOS kelpos development x86_64 freestanding Multiboot-v1\n");
}