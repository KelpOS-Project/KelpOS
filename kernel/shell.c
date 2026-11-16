/* kernel/shell.c — interactive in-kernel shell */
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "shell.h"
#include "keyboard.h"
#include "console.h"
#include "ramfs.h"
#include "bin/commands.h"

#define LINE_MAX 256

/* shell working directory (fs package provides cd/ls on top of it) */
static char cwd[96] = "/";

const char *shell_cwd(void)
{
    return cwd;
}

int shell_chdir(const char *path)
{
    if (!path || !*path)
        return -1;
    struct ramfs_node *n = ramfs_resolve_at(ramfs_root(), path);
    if (!n || !ramfs_is_dir(n))
        return -1;
    char newcwd[96];
    if (!ramfs_path_of(n, newcwd, sizeof(newcwd)))
        return -1;
    strcpy(cwd, newcwd);
    return 0;
}

static void exec_line(char *line)
{
    char *argv[16];
    int argc = 0;

    char *p = line;
    while (*p && argc < 16) {
        while (*p == ' ')
            p++;
        if (!*p)
            break;
        argv[argc++] = p;
        while (*p && *p != ' ')
            p++;
        if (*p)
            *p++ = 0;
    }

    if (argc == 0)
        return;

    const struct builtin *cmd = pkg_find_cmd(argv[0]);
    if (cmd) {
        cmd->fn(argc, argv);
        return;
    }

    /* helpful hints for known-but-off commands */
    for (size_t i = 0; i < package_count; i++) {
        const struct package *pkg = &packages[i];
        if (!pkg->compiled)
            continue;
        for (size_t c = 0; c < pkg->cmd_count; c++) {
            if (strcmp(argv[0], pkg->cmds[c].name) == 0) {
                kprintf("'%s' is disabled (package '%s') — run 'pkg enable %s'.\n",
                        argv[0], pkg->name, pkg->name);
                return;
            }
        }
    }
    kprintf("Unknown command: '%s' — 'help' lists everything.\n", argv[0]);
}

void shell_run(void)
{
    char buf[LINE_MAX];

    for (;;) {
        console_set_colors(0x3fa34du, KELP_BG); /* green prompt */
        kprintf("kelp");
        console_set_colors(KELP_FG, KELP_BG);
        kprintf("%s~$ ", cwd);

        int n = 0;
        for (;;) {
            int c = kbd_getchar();
            if (c == '\n') {
                kputc('\n');
                break;
            }
            if (c == '\b') {
                if (n > 0) {
                    n--;
                    kprintf("\b \b");
                }
                continue;
            }
            if (c >= 32 && c < 127 && n < LINE_MAX - 1) {
                buf[n++] = (char)c;
                kputc(c);
            }
        }
        buf[n] = 0;
        exec_line(buf);
    }
}
