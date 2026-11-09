#include "commands.h"
#include "../../drivers/vga_terminal.h"

void cmd_clear(const char *args)
{
    (void)args;
    terminal_clear();
}
