/* kernel/bin/touch.c — package "fs": create an empty file (or update
 * an existing one's length, i.e. a no-op for content) */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <stdio.h>
#include "ramfs.h"

int cmd_touch(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: touch <file>\n");
        return 1;
    }
    if (ramfs_resolve_at(ramfs_root(), argv[1]))
        return 0; /* exists */
    int rc = ramfs_mkfile(argv[1]);
    if (rc != RAMFS_ERR_OK) {
        kprintf("touch: cannot create: %s\n", argv[1]);
        return 1;
    }
    return 0;
}
#endif
