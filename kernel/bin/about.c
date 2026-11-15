/* kernel/bin/about.c — package "about": information about KelpOS */
#include "commands.h"

#ifdef KELPOS_PKG_ABOUT
#include <stdio.h>
#include "version.h"

static int cmd_about(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("%s %s (%s)\n", KELPOS_NAME, KELPOS_VERSION, KELPOS_ARCH);
    kprintf("A free (GPL-3.0) Unix-like hobby kernel.\n");
    kprintf("Boot: Limine Boot Protocol | Language: C + optional Rust\n");
    kprintf("Developed with AI assistance - see AI-POLICY.md in the repo.\n");
    return 0;
}

const struct builtin about_cmds[] = {
    { "about", cmd_about, "information about KelpOS" },
};
#endif
