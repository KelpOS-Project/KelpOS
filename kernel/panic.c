/* kernel/panic.c — Kernel-Panic */
#include <stdarg.h>
#include <stdio.h>
#include "panic.h"
#include "console.h"
#include "bin/commands.h"

void panicf(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    kprintf("\n*** KERNEL PANIC ***\n%s\nSystem halted.\n", buf);
    for (;;)
        __asm__ volatile("cli; hlt");
}
