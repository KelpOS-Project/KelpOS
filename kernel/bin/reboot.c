/* kernel/bin/reboot.c */
#include <stdio.h>
#include "io.h"
#include "commands.h"

int cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("Reboot...\n");
    __asm__ volatile("cli");
    outb(0x64, 0xFE); /* 8042 Fast Reset */
    for (;;)
        __asm__ volatile("hlt");
    return 0;
}
