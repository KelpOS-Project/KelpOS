/* kernel/bin/uname.c — core: print system information */
#include "commands.h"
#include <stdio.h>
#include "version.h"

int cmd_uname(int argc, char **argv)
{
    if (argc > 1 && (argv[1][0] == '-' && argv[1][1] == 'a')) {
        kprintf("KelpOS kelpos " KELPOS_VERSION " x86_64 "
                "Limine Boot Protocol\n");
        return 0;
    }
    kprintf("KelpOS\n");
    return 0;
}
