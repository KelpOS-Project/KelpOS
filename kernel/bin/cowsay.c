/* kernel/bin/cowsay.c — package "cowsay": a cow says whatever you want.
 * A harmless demo of an optional package. */
#include "commands.h"

#ifdef KELPOS_PKG_COWSAY
#include <stddef.h>
#include <stdio.h>

#define BUBBLE_MAX 40

static int cmd_cowsay(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: cowsay <something worth saying>\n");
        return 1;
    }

    /* join arguments, clamp to BUBBLE_MAX */
    char msg[BUBBLE_MAX + 1];
    size_t n = 0;
    msg[0] = 0;
    for (int i = 1; i < argc; i++) {
        size_t len = 0;
        while (argv[i][len])
            len++;
        if (i > 1 && n < BUBBLE_MAX) {
            msg[n++] = ' ';
            msg[n] = 0;
        }
        for (size_t j = 0; j < len && n < BUBBLE_MAX; j++)
            msg[n++] = argv[i][j];
        msg[n] = 0;
    }

    kprintf(" ");
    for (size_t i = 0; i < n + 2; i++)
        kputc('_');
    kputc('\n');
    kprintf("< %s >\n", msg);
    kprintf(" ");
    for (size_t i = 0; i < n + 2; i++)
        kputc('-');
    kputc('\n');
    kprintf("        \\   ^__^\n");
    kprintf("         \\  (oo)\\_______\n");
    kprintf("            (__)\\       )\\/\\\n");
    kprintf("                ||----w |\n");
    kprintf("                ||     ||\n");
    return 0;
}

const struct builtin cowsay_cmds[] = {
    { "cowsay", cmd_cowsay, "a cow says whatever you want" },
};
#endif
