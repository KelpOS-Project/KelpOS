/* kernel/bin/mem.c — package "mem": heap usage statistics */
#include "commands.h"

#ifdef KELPOS_PKG_MEM
#include <stdio.h>
#include "heap.h"

static int cmd_mem(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    size_t used, total;
    heap_stats(&used, &total);
    kprintf("Heap: %llu / %llu KiB used (bump allocator)\n",
            (unsigned long long)(used / 1024),
            (unsigned long long)(total / 1024));
    return 0;
}

const struct builtin mem_cmds[] = {
    { "mem", cmd_mem, "heap usage statistics" },
};
#endif
