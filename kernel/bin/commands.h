#ifndef KELP_BIN_COMMANDS_H
#define KELP_BIN_COMMANDS_H

#include <stddef.h>
#include <stdbool.h>
#include <pkgconf.h> /* generated: which optional packages are built in */

struct builtin {
    const char *name;
    int (*fn)(int argc, char **argv);
    const char *help;
};

/* A package is a group of shell commands. "core" is always compiled in
 * and enabled. Optional packages can be left out of the kernel image at
 * build time (make PKGS=...) and can be enabled/disabled at run time
 * with the `pkg` command (runtime state lives in RAM and resets on
 * reboot). */
struct package {
    const char *name;
    const char *desc;
    const struct builtin *cmds;
    size_t cmd_count;
    bool compiled; /* false: optional package known but not built in */
    bool enabled;
};

extern struct package packages[];
extern const size_t package_count;

struct package *pkg_find(const char *name);
const struct builtin  *pkg_find_cmd(const char *name); /* honors enabled */
bool pkg_set_enabled(const char *name, bool enabled);  /* false if not compiled */

int cmd_help(int argc, char **argv);
int cmd_echo(int argc, char **argv);
int cmd_clear(int argc, char **argv);
int cmd_uptime(int argc, char **argv);
int cmd_pkg(int argc, char **argv);
int cmd_reboot(int argc, char **argv);
int cmd_shutdown(int argc, char **argv);
int cmd_desktop(int argc, char **argv); /* kernel/desktop.c */
int cmd_crash(int argc, char **argv);   /* kernel/bin/crash.c */

#endif
