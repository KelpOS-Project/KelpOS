/* kernel/bin/pkg.c — the `pkg` command: kernel package manager.
 *
 * Packages group shell commands. Optional packages can be compiled into
 * the image or left out at build time (make PKGS="crc cowsay") and can
 * be enabled/disabled at run time. Runtime state lives in RAM and
 * resets on reboot — persistence comes with a real filesystem (M3). */
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "commands.h"

static void pkg_status(const struct package *p, char *out, size_t cap)
{
    if (!p->compiled)
        snprintf(out, cap, "not built");
    else if (p->enabled)
        snprintf(out, cap, "enabled");
    else
        snprintf(out, cap, "disabled");
}

static int pkg_list(void)
{
    char status[16];
    kprintf("Packages:\n");
    for (size_t i = 0; i < package_count; i++) {
        pkg_status(&packages[i], status, sizeof(status));
        kprintf("  %-9s %-11s %s\n", packages[i].name, status, packages[i].desc);
    }
    return 0;
}

static int pkg_info(const char *name)
{
    const struct package *p = pkg_find(name);
    if (!p) {
        kprintf("pkg: no such package: '%s' (try 'pkg list')\n", name);
        return 1;
    }
    kprintf("%s: %s\n", p->name, p->desc);
    if (!p->compiled) {
        kprintf("  status: not built (rebuild with: make PKGS+=%s)\n", p->name);
        return 0;
    }
    kprintf("  status: %s\n", p->enabled ? "enabled" : "disabled");
    for (size_t c = 0; c < p->cmd_count; c++)
        kprintf("  command: %s — %s\n", p->cmds[c].name, p->cmds[c].help);
    return 0;
}

static int pkg_set(const char *name, bool enabled)
{
    const struct package *p = pkg_find(name);
    if (!p) {
        kprintf("pkg: no such package: '%s'\n", name);
        return 1;
    }
    if (!enabled && strcmp(name, "core") == 0) {
        kprintf("pkg: refusing to disable 'core' — you would lose the 'pkg' command itself.\n");
        return 1;
    }
    if (!pkg_set_enabled(name, enabled)) {
        kprintf("pkg: package '%s' is not built into this image\n", name);
        return 1;
    }
    kprintf("pkg: %s %s\n", enabled ? "enabled" : "disabled", name);
    return 0;
}

int cmd_pkg(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "list") == 0)
        return pkg_list();
    if (strcmp(argv[1], "info") == 0 && argc > 2)
        return pkg_info(argv[2]);
    if (strcmp(argv[1], "enable") == 0 && argc > 2)
        return pkg_set(argv[2], true);
    if (strcmp(argv[1], "disable") == 0 && argc > 2)
        return pkg_set(argv[2], false);

    kprintf("usage: pkg list | pkg info <pkg> | pkg enable <pkg> | pkg disable <pkg>\n");
    return 1;
}
