/* kernel/bin/tail.c — package "fs": last lines of a file */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ramfs.h"

int cmd_tail(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: tail <file>\n");
        return 1;
    }
    struct ramfs_node *f = ramfs_resolve_at(ramfs_root(), argv[1]);
    if (!f || ramfs_is_dir(f)) {
        kprintf("tail: no such file: %s\n", argv[1]);
        return 1;
    }
    const char *d = ramfs_data(f);
    size_t size = ramfs_size(f);
    int want = 10;

    /* find the start of the last `want` lines */
    size_t start = 0;
    int seen = 0;
    for (size_t i = size; i > 0; i--) {
        if (d[i - 1] == '\n') {
            if (seen >= want && i < size) {
                start = i;
                break;
            }
            seen++;
        }
    }
    kputs(d + start);
    return 0;
}
#endif
