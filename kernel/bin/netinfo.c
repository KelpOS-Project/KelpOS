#include "commands.h"

#include "../../drivers/e1000.h"
#include "../../drivers/vga_terminal.h"

void cmd_netinfo(const char *args)
{
    (void)args;
    terminal_write_string("Ethernet: ");
    terminal_write_string(e1000_is_initialized() ? "Intel E1000 initialized\n"
        : "no supported Intel E1000 detected\n");
    terminal_write_string("Network layer: raw Ethernet only; no IP, DHCP, or sockets\n");
}