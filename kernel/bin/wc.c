/* kernel/bin/wc.c — package "fs": count lines/words/chars of a file */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ramfs.h"

int cmd_wc(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: wc <file>\n");
        return 1;
    }
    struct ramfs_node *f = ramfs_resolve_at(ramfs_root(), argv[1]);
    if (!f || ramfs_is_dir(f)) {
        kprintf("wc: no such file: %s\n", argv[1]);
        return 1;
    }
    const char *d = ramfs_data(f);
    size_t lines = 0, words = 0, chars = 0;
    bool in_word = false;
    for (size_t i = 0; i < ramfs_size(f); i++) {
        chars++;
        if (d[i] == '\n')
            lines++;
        if (d[i] == ' ' || d[i] == '\n' || d[i] == '\t')
            in_word = false;
        else if (!in_word) {
            in_word = true;
            words++;
        }
    }
    kprintf("%llu %llu %llu %s\n",
            (unsigned long long)lines, (unsigned long long)words,
            (unsigned long long)chars, argv[1]);
    return 0;
}
#endif
