/* kernel/bin/fs.c — command table for the "fs" package.
 * The command implementations live in ls.c, pwd.c, cd.c, mkdir.c,
 * tree.c, cat.c, rm.c, touch.c, wc.c, head.c, tail.c and grep.c;
 * this file only aggregates them. */
#include "commands.h"

#ifdef KELPOS_PKG_FS
int cmd_ls(int argc, char **argv);
int cmd_pwd(int argc, char **argv);
int cmd_cd(int argc, char **argv);
int cmd_mkdir(int argc, char **argv);
int cmd_tree(int argc, char **argv);
int cmd_cat(int argc, char **argv);
int cmd_rm(int argc, char **argv);
int cmd_touch(int argc, char **argv);
int cmd_wc(int argc, char **argv);
int cmd_head(int argc, char **argv);
int cmd_tail(int argc, char **argv);
int cmd_grep(int argc, char **argv);

const struct builtin fs_cmds_all[] = {
    { "ls",    cmd_ls,    "list directory contents" },
    { "pwd",   cmd_pwd,   "print working directory" },
    { "cd",    cmd_cd,    "change directory (cd .. goes up)" },
    { "mkdir", cmd_mkdir, "create a directory" },
    { "tree",  cmd_tree,  "print the directory tree" },
    { "cat",   cmd_cat,   "print a file's contents" },
    { "rm",    cmd_rm,    "remove a file or empty directory" },
    { "touch", cmd_touch, "create an empty file" },
    { "wc",    cmd_wc,    "count lines/words/chars of a file" },
    { "head",  cmd_head,  "first lines of a file" },
    { "tail",  cmd_tail,  "last lines of a file" },
    { "grep",  cmd_grep,  "print lines matching a pattern" },
};
#endif
