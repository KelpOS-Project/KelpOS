#include "commands.h"
#include "../../drivers/vga_terminal.h"

void cmd_about(const char *args)
{
    (void)args;

    terminal_write_string("  _  __     _          ___  ____  \n");
    terminal_write_string(" | |/ /___ | |_ ____  / _ \\/ ___| \n");
    terminal_write_string(" | ' // _ \\| | '_ \\ \\| | | \\___ \\ \n");
    terminal_write_string(" | . \\  __/| | |_) | | |_| |___) |\n");
    terminal_write_string(" |_|\\_\\___||_| .__/   \\___/|____/ \n");
    terminal_write_string("             |_|                  \n\n");

    terminal_write_string("OS: KelpOS x86_64 TTY\n");
    terminal_write_string("Kernel Type: Freestanding\n");
    terminal_write_string("UNIX-like: Unknown / Custom\n");
    terminal_write_string("POSIX Commands: Custom Builtins\n");
    terminal_write_string("Installable: No\n");
    terminal_write_string("Package Manager: Not finished yet\n");
    terminal_write_string("Init System: CISWN (kernel/ciswn)\n");
    terminal_write_string("Shell Registry: Table-Driven Macro Core\n");
}
