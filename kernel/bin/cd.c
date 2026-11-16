/* kernel/bin/cd.c — package "fs": change the shell's working directory */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <string.h>
#include <stdio.h>
#include "shell.h"

int cmd_cd(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/";
    if (shell_chdir(path) != 0) {
        kprintf("cd: no such directory: %s\n", path);
        return 1;
    }
    return 0;
}

#endif
