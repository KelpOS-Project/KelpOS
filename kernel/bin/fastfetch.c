/* kernel/bin/fastfetch.c — package "fastfetch": system info, neofetch-style */
#include "commands.h"

#ifdef KELPOS_PKG_FASTFETCH
#include <stdint.h>
#include <stdio.h>
#include "version.h"
#include "timer.h"
#include "heap.h"
#include "users.h"
#include "shell.h"
#include "console.h"

static int cmd_fastfetch(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uint64_t ms = timer_ms();
    size_t used = 0, total = 0;
    heap_stats(&used, &total);

    size_t built = 0, enabled = 0;
    for (size_t i = 0; i < package_count; i++) {
        if (packages[i].compiled) {
            built++;
            if (packages[i].enabled)
                enabled++;
        }
    }

    char l1[96], l2[96], l3[96], l4[96], l5[96];
    snprintf(l1, sizeof(l1), "%s@kelpos", user_current()->name);
    snprintf(l2, sizeof(l2), "OS: KelpOS %s %s", KELPOS_VERSION, KELPOS_ARCH);
    snprintf(l3, sizeof(l3), "Shell: ksh | Boot: Limine");
    snprintf(l4, sizeof(l4), "Uptime: %llu.%03llu s",
             (unsigned long long)(ms / 1000),
             (unsigned long long)(ms % 1000));
    snprintf(l5, sizeof(l5), "Packages: %llu built, %llu enabled",
             (unsigned long long)built, (unsigned long long)enabled);

    const char *info[7] = {
        l1, l2, l3, l4, l5, "", ""
    };
    char memline[96];
    snprintf(memline, sizeof(memline), "Memory: %llu / %llu MiB",
             (unsigned long long)(used >> 20),
             (unsigned long long)(total >> 20));
    info[5] = memline;

    for (int i = 0; i < 7; i++) {
        console_set_colors(0x3fa34du, KELP_BG); /* kelp green art */
        console_set_colors(KELP_FG, KELP_BG);
        kputs(info[i]);
        kputc('\n');
    }
    return 0;
}

const struct builtin fastfetch_cmds[] = {
    { "fastfetch", cmd_fastfetch, "system info with kelp art" },
};
#endif
