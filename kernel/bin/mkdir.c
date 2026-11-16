/* kernel/bin/mkdir.c — package "fs": create directories */
#include "commands.h"

#ifdef KELPOS_PKG_FS
#include <string.h>
#include <stdio.h>
#include "ramfs.h"

int cmd_mkdir(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("usage: mkdir <directory>\n");
        return 1;
    }
    int rc = ramfs_mkdir(argv[1]);
    switch (rc) {
    case RAMFS_ERR_OK:
        return 0;
    case RAMFS_ERR_EXIST:
        kprintf("mkdir: already exists: %s\n", argv[1]);
        return 1;
    case RAMFS_ERR_NOTDIR:
        kprintf("mkdir: parent is not a directory: %s\n", argv[1]);
        return 1;
    default:
        kprintf("mkdir: cannot create: %s\n", argv[1]);
        return 1;
    }
}

#endif
