/* kernel/bin/help.c — package-aware command listing */
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "commands.h"

static void print_padded(const char *s, int width)
{
    size_t len = strlen(s);
    kprintf("%s", s);
    for (size_t i = len; i < (size_t)width; i++)
        kputc(' ');
}

int cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("Available commands:\n");
    for (size_t i = 0; i < package_count; i++) {
        const struct package *p = &packages[i];
        if (!p->compiled || !p->enabled)
            continue; /* disabled packages are shown by 'pkg list' */
        for (size_t c = 0; c < p->cmd_count; c++) {
            kprintf("  ");
            print_padded(p->cmds[c].name, 10);
            kprintf("%s\n", p->cmds[c].help);
        }
    }
    kprintf("Use 'kpm list' to see not-installed and not-built packages.\n");
    return 0;
}
