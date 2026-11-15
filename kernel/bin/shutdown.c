/* kernel/bin/shutdown.c — sauberer Shutdown unter QEMU (isa-debug-exit),
 * auf echter Hardware nur Halten. */
#include <stdio.h>
#include "io.h"
#include "commands.h"

int cmd_shutdown(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("Shutdown (QEMU isa-debug-exit)...\n");
    outw(0x604, 0x2000); /* QEMU: exit status 0 */
    kprintf("Not running under QEMU — halting the system.\n");
    __asm__ volatile("cli");
    for (;;)
        __asm__ volatile("hlt");
    return 0;
}
