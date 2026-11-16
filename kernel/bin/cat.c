/* kernel/bin/cat.c — package "fs": print file contents */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stddef.h>
#include <stdio.h>
#include "ramfs.h"

int cmd_cat(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: cat <file>\n");
        return 1;
    }
    struct ramfs_node *f = ramfs_resolve_at(ramfs_root(), argv[1]);
    if (!f) {
        kprintf("cat: no such file: %s\n", argv[1]);
        return 1;
    }
    if (ramfs_is_dir(f)) {
        kprintf("cat: is a directory: %s\n", argv[1]);
        return 1;
    }
    kputs(ramfs_data(f));
    return 0;
}

#endif
