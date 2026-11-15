/* kernel/bin/echo.c */
#include <stdio.h>
#include "commands.h"

int cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
        kprintf("%s%s", argv[i], i + 1 < argc ? " " : "");
    kputc('\n');
    return 0;
}
