/* kernel/bin/tree.c — package "fs": print the directory tree */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ramfs.h"
#include "shell.h"

static void walk(struct ramfs_node *d, int depth, int *files, int *dirs)
{
    struct ramfs_node *kids[64];
    size_t n = ramfs_children(d, kids, 64);
    for (size_t i = 0; i < n; i++) {
        for (int s = 0; s < depth; s++)
            kprintf("  ");
        if (ramfs_is_dir(kids[i])) {
            kprintf("%s/\n", ramfs_node_name(kids[i]));
            (*dirs)++;
            walk(kids[i], depth + 1, files, dirs);
        } else {
            kprintf("%s\n", ramfs_node_name(kids[i]));
            (*files)++;
        }
    }
}

int cmd_tree(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : shell_cwd();
    struct ramfs_node *d = ramfs_resolve_at(ramfs_root(), path);
    if (!d || !ramfs_is_dir(d)) {
        kprintf("tree: no such directory: %s\n", path);
        return 1;
    }

    int files = 0, dirs = 0;
    kprintf("%s\n", path);
    walk(d, 1, &files, &dirs);
    kprintf("\n%d directories, %d files\n", dirs, files);
    return 0;
}

#endif
