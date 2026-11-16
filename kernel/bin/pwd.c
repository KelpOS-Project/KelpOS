/* kernel/bin/pwd.c — package "fs": print working directory */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stdio.h>
#include "shell.h"

int cmd_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("%s\n", shell_cwd());
    return 0;
}

#endif
