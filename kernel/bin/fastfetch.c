#include "commands.h"
#include "../../drivers/vga_terminal.h"

void cmd_fastfetch(const char *args)
{
    (void)args;
    terminal_write_string("KelpOS\n");
    terminal_write_string("OS: KelpOS development\n");
    terminal_write_string("Architecture: x86_64\n");
    terminal_write_string("Boot: GRUB Multiboot v1\n");
    terminal_write_string("Kernel: freestanding, single-core prototype\n");
    terminal_write_string("Network: Intel E1000 raw-frame driver\n");
    cmd_mem(NULL);
}