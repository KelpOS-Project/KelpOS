/* kernel/bin/crash.c — package "crash": force a kernel panic on
 * purpose. Handy to verify the panic handler / stack trace works.
 * Enable with `pkg enable crash` at runtime, or build with PKGS=crash. */
#include "commands.h"

#ifdef KELPOS_PKG_CRASH
#include "../panic.h"

int cmd_crash(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    panicf("User requested crash");
    return 0;
}

const struct builtin crash_cmds[] = {
    { "crash", cmd_crash, "(debug) force a kernel panic" },
};
#endif
