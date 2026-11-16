/* kernel/bin/rm.c — package "fs": remove files or empty directories */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <string.h>
#include <stdio.h>
#include "ramfs.h"

int cmd_rm(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: rm <path>\n");
        return 1;
    }
    int rc = ramfs_rm(argv[1]);
    switch (rc) {
    case RAMFS_ERR_OK:
        return 0;
    case RAMFS_ERR_NOENT:
        kprintf("rm: no such file or directory: %s\n", argv[1]);
        return 1;
    case RAMFS_ERR_NOTDIR:
        kprintf("rm: directory not empty: %s\n", argv[1]);
        return 1;
    default:
        kprintf("rm: cannot remove: %s\n", argv[1]);
        return 1;
    }
}
#endif
