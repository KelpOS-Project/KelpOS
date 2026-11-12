#include "commands.h"

#include "../../drivers/vga_terminal.h"
#include "../../drivers/x86_io.h"

void cmd_reboot(const char *args)
{
    (void)args;
    terminal_write_string("Requesting reboot...\n");
    for (unsigned int attempt = 0; attempt < 100000; attempt++) {
        if ((x86_port_read8(0x64) & 0x02) == 0) {
            x86_port_write8(0x64, 0xFE);
            for (unsigned int wait = 0; wait < 1000000; wait++) {
                __asm__ volatile ("pause");
            }
            terminal_write_string("Reboot request did not reset the machine.\n");
            return;
        }
    }
    terminal_write_string("Reboot failed: keyboard controller is busy.\n");
}