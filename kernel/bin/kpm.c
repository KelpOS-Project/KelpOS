/* kernel/bin/kpm.c — `kpm`, the KelpOS package manager.
 *
 * Packages group shell commands. Optional packages are compiled into
 * the kernel image at build time (make PKGS=...) and can be installed
 * (enabled) or uninstalled (disabled) at run time. The installed set is
 * mirrored to /etc/kpm/installed (ramfs, survives shell sessions) and
 * re-applied on boot via kpm_restore() once a writable root fs can
 * carry it across reboots; today a reboot reseeds the ramfs, so the
 * DB starts empty again.
 *
 * Uninstalling compiled-in code cannot remove it from the image — kpm
 * uninstalls by disabling the commands, the honest equivalent for a
 * single-binary kernel. */
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "commands.h"
#include "../fs/ramfs.h"

#define KPM_DB "/etc/kpm/installed"

/* ── persistence (ramfs) ────────────────────────────────────────────── */

static void kpm_sync(void)
{
    struct ramfs_node *db = ramfs_resolve_at(ramfs_root(), KPM_DB);
    if (!db)
        return;
    char buf[256];
    size_t o = 0;
    buf[0] = 0;
    for (size_t i = 0; i < package_count; i++) {
        if (packages[i].compiled && packages[i].enabled) {
            snprintf(buf + o, sizeof(buf) - o, "%s\n", packages[i].name);
            o = strlen(buf);
        }
    }
    ramfs_write(db, buf, o);
}

void kpm_restore(void)
{
    struct ramfs_node *db = ramfs_resolve_at(ramfs_root(), KPM_DB);
    if (!db)
        return;
    const char *data = ramfs_data(db);
    char name[32];
    size_t n = 0;
    for (size_t i = 0;; i++) {
        char c = data[i];
        if (c == '\n' || c == 0) {
            name[n] = 0;
            if (n)
                pkg_set_enabled(name, true);
            n = 0;
            if (c == 0)
                break;
            continue;
        }
        if (n + 1 < sizeof(name))
            name[n++] = c;
    }
}

/* ── commands ───────────────────────────────────────────────────────── */

static int kpm_list(void)
{
    kprintf("Packages (installed set: " KPM_DB "):\n");
    for (size_t i = 0; i < package_count; i++) {
        const struct package *p = &packages[i];
        char status[20];
        if (!p->compiled)
            snprintf(status, sizeof(status), "not built");
        else if (p->enabled)
            snprintf(status, sizeof(status), "installed");
        else
            snprintf(status, sizeof(status), "available");
        kprintf("  %-9s %-10s %s\n", p->name, status, p->desc);
    }
    return 0;
}

static int kpm_info(const char *name)
{
    const struct package *p = pkg_find(name);
    if (!p) {
        kprintf("kpm: no such package: '%s' (try 'kpm list')\n", name);
        return 1;
    }
    kprintf("%s: %s\n", p->name, p->desc);
    if (!p->compiled) {
        kprintf("  status: not built (rebuild with: make PKGS+=%s)\n", p->name);
        return 0;
    }
    kprintf("  status: %s\n", p->enabled ? "installed" : "available");
    for (size_t c = 0; c < p->cmd_count; c++)
        kprintf("  command: %s — %s\n", p->cmds[c].name, p->cmds[c].help);
    return 0;
}

static int kpm_install(const char *name)
{
    const struct package *p = pkg_find(name);
    if (!p) {
        kprintf("kpm: no such package: '%s'\n", name);
        return 1;
    }
    if (!p->compiled) {
        kprintf("kpm: '%s' is not built into this image — "
                "rebuild with: make PKGS+=%s\n", name, name);
        return 1;
    }
    if (p->enabled) {
        kprintf("kpm: '%s' is already installed\n", name);
        return 0;
    }
    pkg_set_enabled(name, true);
    kpm_sync();
    kprintf("kpm: installed '%s' (%s)\n", name, p->desc);
    return 0;
}

static int kpm_uninstall(const char *name)
{
    const struct package *p = pkg_find(name);
    if (!p) {
        kprintf("kpm: no such package: '%s'\n", name);
        return 1;
    }
    if (strcmp(name, "core") == 0) {
        kprintf("kpm: refusing to uninstall 'core' — "
                "it provides 'kpm' itself.\n");
        return 1;
    }
    if (!p->enabled) {
        kprintf("kpm: '%s' is not installed\n", name);
        return 0;
    }
    pkg_set_enabled(name, false);
    kpm_sync();
    kprintf("kpm: uninstalled '%s' (commands disabled; the image keeps "
            "the code)\n", name);
    return 0;
}

int cmd_kpm(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "list") == 0)
        return kpm_list();
    if (strcmp(argv[1], "info") == 0 && argc > 2)
        return kpm_info(argv[2]);
    if (strcmp(argv[1], "install") == 0 && argc > 2)
        return kpm_install(argv[2]);
    if (strcmp(argv[1], "uninstall") == 0 && argc > 2)
        return kpm_uninstall(argv[2]);
    if (strcmp(argv[1], "update") == 0) {
        kpm_sync();
        kprintf("kpm: package database written to " KPM_DB "\n");
        return 0;
    }

    kprintf("usage: kpm list | kpm info <pkg> | kpm install <pkg> | "
            "kpm uninstall <pkg> | kpm update\n");
    return 1;
}
