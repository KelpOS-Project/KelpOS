/* kernel/bin/grep.c — package "fs": print lines matching a pattern */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ramfs.h"
#include "console.h"

int cmd_grep(int argc, char **argv)
{
    if (argc < 3) {
        kprintf("usage: grep <pattern> <file>\n");
        return 1;
    }
    struct ramfs_node *f = ramfs_resolve_at(ramfs_root(), argv[2]);
    if (!f || ramfs_is_dir(f)) {
        kprintf("grep: no such file: %s\n", argv[2]);
        return 1;
    }
    const char *d = ramfs_data(f);
    size_t line_start = 0;
    bool found = false;
    for (size_t i = 0;; i++) {
        if (d[i] == '\n' || d[i] == 0) {
            size_t len = i - line_start;
            if (len >= strlen(argv[1])) {
                for (size_t j = 0; j + strlen(argv[1]) <= len; j++) {
                    if (strncmp(d + line_start + j, argv[1],
                                strlen(argv[1])) == 0) {
                        char buf[128];
                        if (len >= sizeof(buf))
                            len = sizeof(buf) - 1;
                        memcpy(buf, d + line_start, len);
                        buf[len] = 0;
                        kputs(buf);
                        kputc('\n');
                        found = true;
                        break;
                    }
                }
            }
            if (d[i] == 0)
                break;
            line_start = i + 1;
        }
    }
    return found ? 0 : 1;
}
#endif
