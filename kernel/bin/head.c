/* kernel/bin/head.c — package "fs": first lines of a file */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stddef.h>
#include <stdio.h>
#include "ramfs.h"

int cmd_head(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: head <file>\n");
        return 1;
    }
    struct ramfs_node *f = ramfs_resolve_at(ramfs_root(), argv[1]);
    if (!f || ramfs_is_dir(f)) {
        kprintf("head: no such file: %s\n", argv[1]);
        return 1;
    }
    const char *d = ramfs_data(f);
    int lines = 10, shown = 0;
    bool last_nl = true;
    for (size_t i = 0; d[i] && shown < lines; i++) {
        kputc(d[i]);
        last_nl = d[i] == '\n';
        if (last_nl)
            shown++;
    }
    if (!last_nl)
        kputc('\n');
    return 0;
}
#endif
