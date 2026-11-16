/* kernel/bin/ls.c — package "fs": list directory contents */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ramfs.h"
#include "shell.h"
#include "console.h"

int cmd_ls(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : shell_cwd();
    struct ramfs_node *d = ramfs_resolve_at(ramfs_root(), path);
    if (!d) {
        kprintf("ls: no such file or directory: %s\n", path);
        return 1;
    }
    if (!ramfs_is_dir(d)) {
        kprintf("%s\n", ramfs_node_name(d));
        return 0;
    }

    struct ramfs_node *kids[64];
    size_t n = ramfs_children(d, kids, 64);
    if (n == 0)
        return 0;
    for (size_t i = 0; i < n; i++) {
        const char *name = ramfs_node_name(kids[i]);
        if (ramfs_is_dir(kids[i])) {
            console_set_colors(0x7fb3d5u, KELP_BG); /* dirs in blue */
            kprintf("%s/", name);
            console_set_colors(KELP_FG, KELP_BG);
        } else {
            size_t sz = ramfs_size(kids[i]);
            kprintf("%-14s %llu B", name, (unsigned long long)sz);
        }
        kputc('\n');
    }
    return 0;
}

#endif
