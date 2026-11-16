#ifndef KELP_FS_RAMFS_H
#define KELP_FS_RAMFS_H

#include <stddef.h>

/* Tiny in-memory filesystem (M4 preview). Everything lives in RAM and
 * disappears on reboot; it gives the shell real directories and files
 * (ls, cd, mkdir, tree, cat, ...) and a home for /etc/passwd. */

struct ramfs_node {
    char *name;
    int is_dir;
    struct ramfs_node *parent;
    struct ramfs_node *first_child;
    struct ramfs_node *next_sibling;
    /* file only */
    char *data;
    size_t size, cap;
};

#define RAMFS_ERR_OK       0
#define RAMFS_ERR_NOENT   -1  /* path (or parent) not found */
#define RAMFS_ERR_EXIST   -2  /* node already exists */
#define RAMFS_ERR_NOTDIR  -3  /* parent or target is not a directory */
#define RAMFS_ERR_ISDIR   -4  /* target is a directory (file op) */
#define RAMFS_ERR_NOMEM   -5
#define RAMFS_ERR_INVAL   -6  /* malformed path */

void ramfs_init(void);
struct ramfs_node *ramfs_root(void);

/* Resolve a path ("/a/b" or relative to `base`, supports "." and ".."). */
struct ramfs_node *ramfs_resolve_at(struct ramfs_node *base, const char *path);

int ramfs_mkdir(const char *path);
int ramfs_mkfile(const char *path);
int ramfs_rm(const char *path);          /* files and empty dirs */

/* File contents (text-oriented; always NUL-terminated for convenience). */
int  ramfs_write(struct ramfs_node *f, const void *data, size_t len);
int  ramfs_append(struct ramfs_node *f, const void *data, size_t len);
const char *ramfs_data(struct ramfs_node *f);
size_t ramfs_size(struct ramfs_node *f);

/* Children of a directory; returns count, fills `out` (caller provides
 * an array big enough) sorted by name. */
size_t ramfs_children(struct ramfs_node *dir, struct ramfs_node **out,
                      size_t max);

/* "/a/b" path of node into buf; returns buf. */
char *ramfs_path_of(struct ramfs_node *node, char *buf, size_t cap);

/* convenience accessors */
int         ramfs_is_dir(struct ramfs_node *n);
const char *ramfs_node_name(struct ramfs_node *n);

#endif
