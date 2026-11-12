#include "commands.h"

#include "../../drivers/vga_terminal.h"

_Noreturn void cmd_halt(const char *args)
{
    (void)args;
    terminal_write_string("KelpOS halted.\n");
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}