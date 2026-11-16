/* kernel/bin/adduser.c — package "users": user management commands */
#include "commands.h"

#ifdef KELPOS_PKG_USERS
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "users.h"
#include "ramfs.h"
#include "keyboard.h"

static int cmd_adduser(int argc, char **argv)
{
    if (argc < 3) {
        kprintf("usage: adduser <name> <password>\n");
        return 1;
    }
    if (user_add(argv[1], argv[2], 100) != 0) {
        kprintf("adduser: cannot create user '%s' (exists or invalid)\n", argv[1]);
        return 1;
    }
    /* give the user a home directory */
    char path[64];
    snprintf(path, sizeof(path), "/home/%s", argv[1]);
    ramfs_mkdir(path);
    group_add_member("users", argv[1]);
    users_sync();
    kprintf("adduser: created user '%s'\n", argv[1]);
    return 0;
}

static int cmd_groupmod(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "-a") == 0) {
        /* groupmod -a <user> <group>: add member */
        if (group_add_member(argv[3], argv[2]) != 0) {
            kprintf("groupmod: cannot add '%s' to '%s'\n", argv[2], argv[3]);
            return 1;
        }
        users_sync();
        kprintf("groupmod: '%s' added to '%s'\n", argv[2], argv[3]);
        return 0;
    }
    if (argc == 3) {
        /* groupmod <name> <gid>: create or show a group */
        if (group_find(argv[1])) {
            const struct kelp_group *g = group_find(argv[1]);
            kprintf("%s: gid %d, members: %s\n", g->name, g->gid,
                    *g->members ? g->members : "(none)");
            return 0;
        }
        int gid = atoi(argv[2]);
        if (group_add(argv[1], gid) != 0) {
            kprintf("groupmod: cannot create group '%s'\n", argv[1]);
            return 1;
        }
        users_sync();
        kprintf("groupmod: created group '%s' (gid %d)\n", argv[1], gid);
        return 0;
    }
    kprintf("usage: groupmod <group> <gid> | groupmod -a <user> <group>\n");
    return 1;
}

static int cmd_whoami(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("%s\n", user_current()->name);
    return 0;
}

static int cmd_id(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const struct kelp_user *u = user_current();
    kprintf("uid=%d(%s) gid=%d groups=%d(%s)\n",
            u->uid, u->name, u->gid, u->gid, u->name);
    return 0;
}

/* doas/sudo: check the password, then run the command.
 * NOTE: the kernel is single-privilege for now — passing the password
 * check is the gate, real privilege separation comes with userspace. */
static int do_runas(int argc, char **argv)
{
    const struct kelp_user *u = user_current();

    /* root needs no password (matches classic sudo behaviour) */
    if (u->uid != 0) {
        char pw[32];
        kprintf("password for %s: ", u->name);
        size_t n = 0;
        for (;;) {
            int c = kbd_getchar();
            if (c == '\n')
                break;
            if (c == '\b') {
                if (n > 0)
                    n--;
                continue;
            }
            if (c >= 32 && c < 127 && n < sizeof(pw) - 1)
                pw[n++] = (char)c;
        }
        pw[n] = 0;
        kputc('\n');
        if (!user_check_password(u->name, pw)) {
            kprintf("doas: authentication failed\n");
            return 1;
        }
    }

    if (argc < 2) {
        kprintf("usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    const struct builtin *cmd = pkg_find_cmd(argv[1]);
    if (!cmd) {
        kprintf("%s: command not found: %s\n", argv[0], argv[1]);
        return 1;
    }
    return cmd->fn(argc - 1, argv + 1);
}

const struct builtin users_cmds[] = {
    { "adduser", cmd_adduser, "create a user (adduser name password)" },
    { "groupmod", cmd_groupmod, "create/show groups, add members" },
    { "whoami",  cmd_whoami, "print the current user" },
    { "id",      cmd_id, "print uid/gid of the current user" },
    { "doas",    do_runas, "run a command as another user (auth check)" },
    { "sudo",    do_runas, "alias of doas" },
};
#endif
