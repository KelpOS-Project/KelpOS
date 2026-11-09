#include "commands.h"
#include "../../drivers/vga_terminal.h"

void cmd_echo(const char *args)
{
    if (args != NULL) {
        terminal_write_string(args);
    }
    terminal_write_string("\n");
}
