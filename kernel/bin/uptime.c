/* kernel/bin/uptime.c */
#include <stdint.h>
#include <stdio.h>
#include "commands.h"
#include "timer.h"

int cmd_uptime(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    uint64_t ms = timer_ms();
    kprintf("Uptime: %llu.%03llu s (%llu Ticks)\n",
            (unsigned long long)(ms / 1000),
            (unsigned long long)(ms % 1000),
            (unsigned long long)(ms / 10));
    return 0;
}
