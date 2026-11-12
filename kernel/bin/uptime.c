#include "commands.h"

#include "../../drivers/vga_terminal.h"

void cmd_uptime(const char *args)
{
    (void)args;
    terminal_write_string("Uptime unavailable: no PIT/APIC timer is initialized.\n");
}