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

    terminal_write_string("OS: KelpOS development\n");
    terminal_write_string("Architecture: x86_64\n");
    terminal_write_string("Kernel: freestanding, single-core prototype\n");
    terminal_write_string("Boot: GRUB Multiboot v1\n");
    terminal_write_string("Filesystem: not implemented\n");
    terminal_write_string("Network: raw Ethernet driver; no IP stack\n");
    terminal_write_string("KPM: host-side Python prototype\n");
    terminal_write_string("CISWN: host-side C prototype\n");
}
