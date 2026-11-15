/* kernel/bin/clear.c — clear the framebuffer terminal + serial screen */
#include <stdio.h>
#include "console.h"
#include "commands.h"

int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    console_clear();
    return 0;
}
