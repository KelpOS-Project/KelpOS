/* kernel/bin/env.c — core: print the shell environment */
#include "commands.h"

#include <stdio.h>
#include "users.h"
#include "shell.h"
#include "version.h"

int cmd_env(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("USER=%s\n", user_current()->name);
    kprintf("HOME=/home/%s\n", user_current()->name);
    kprintf("PWD=%s\n", shell_cwd());
    kprintf("SHELL=/bin/kelpsh\n");
    kprintf("KELPOS_VERSION=" KELPOS_VERSION "\n");
    return 0;
}
