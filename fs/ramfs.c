/* fs/ramfs.c — tiny in-memory filesystem.
 *
 * Plain tree of nodes: directories hold children, files hold a byte
 * buffer. All paths are slash-separated; "." and ".." are resolved.
 * Host-testable: pure C, errors as return codes. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ramfs.h"

static struct ramfs_node root_node;

void ramfs_init(void)
{
    memset(&root_node, 0, sizeof(root_node));
    root_node.name = (char *)"";
    root_node.is_dir = 1;
}

struct ramfs_node *ramfs_root(void)
{
    return &root_node;
}

/* ── helpers ────────────────────────────────────────────────────────── */

static struct ramfs_node *find_child(struct ramfs_node *dir, const char *name, size_t nlen)
{
    for (struct ramfs_node *c = dir->first_child; c; c = c->next_sibling)
        if (strlen(c->name) == nlen && strncmp(c->name, name, nlen) == 0)
            return c;
    return 0;
}

static struct ramfs_node *node_new(const char *name, size_t nlen, int is_dir)
{
    struct ramfs_node *n = malloc(sizeof(*n));
    if (!n)
        return 0;
    memset(n, 0, sizeof(*n));
    n->name = malloc(nlen + 1);
    if (!n->name) {
        free(n);
        return 0;
    }
    memcpy(n->name, name, nlen);
    n->name[nlen] = 0;
    n->is_dir = is_dir;
    return n;
}

/* Copy the next path component from `path` into buf; returns the number
 * of characters consumed, or 0 when the path is exhausted. */
struct ramfs_node *ramfs_resolve_at(struct ramfs_node *base, const char *path)
{
    if (!base || !path)
        return 0;

    struct ramfs_node *cur = base->is_dir ? base : base->parent;
    if (!cur)
        cur = &root_node;

    if (*path == '/')
        cur = &root_node;

    char comp[64];
    while (*path) {
        size_t nlen = 0;
        while (*path == '/')
            path++;
        if (!*path)
            break;
        const char *start = path;
        while (*path && *path != '/') {
            if (nlen + 1 < sizeof(comp))
                comp[nlen++] = *path;
            path++;
        }
        comp[nlen] = 0;
        if (path == start) /* no chars consumed, guard against loops */
            break;

        if (strcmp(comp, ".") == 0)
            continue;
        if (strcmp(comp, "..") == 0) {
            cur = cur->parent ? cur->parent : &root_node;
            continue;
        }
        if (!cur->is_dir)
            return 0;
        cur = find_child(cur, comp, nlen);
        if (!cur)
            return 0;
    }
    return cur;
}

/* ── create / remove ────────────────────────────────────────────────── */

static int create_node(const char *path, int is_dir)
{
    if (!path || !*path)
        return RAMFS_ERR_INVAL;

    char parent_path[128];
    const char *slash = strrchr(path, '/');
    size_t plen;
    const char *name;
    if (!slash) {
        return RAMFS_ERR_INVAL; /* absolute paths only */
    } else if (slash == path) {
        plen = 1; /* parent is "/" */
        name = slash + 1;
        parent_path[0] = '/';
        parent_path[1] = 0;
    } else {
        plen = (size_t)(slash - path);
        if (plen >= sizeof(parent_path))
            return RAMFS_ERR_INVAL;
        memcpy(parent_path, path, plen);
        parent_path[plen] = 0;
        name = slash + 1;
    }
    if (!*name || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return RAMFS_ERR_INVAL;

    struct ramfs_node *parent = ramfs_resolve_at(&root_node, parent_path);
    if (!parent || !parent->is_dir)
        return RAMFS_ERR_NOTDIR;

    struct ramfs_node *existing = find_child(parent, name, strlen(name));
    if (existing)
        return RAMFS_ERR_EXIST;

    struct ramfs_node *n = node_new(name, strlen(name), is_dir);
    if (!n)
        return RAMFS_ERR_NOMEM;
    n->parent = parent;

    /* insert sorted by name */
    struct ramfs_node **link = &parent->first_child;
    while (*link && strcmp((*link)->name, name) < 0)
        link = &(*link)->next_sibling;
    n->next_sibling = *link;
    *link = n;
    return RAMFS_ERR_OK;
}

int ramfs_mkdir(const char *path)
{
    return create_node(path, 1);
}

int ramfs_mkfile(const char *path)
{
    return create_node(path, 0);
}

int ramfs_rm(const char *path)
{
    struct ramfs_node *n = ramfs_resolve_at(&root_node, path);
    if (!n || n == &root_node)
        return RAMFS_ERR_NOENT;
    if (n->is_dir && n->first_child)
        return RAMFS_ERR_NOTDIR; /* not empty */

    /* unlink */
    struct ramfs_node *p = n->parent;
    struct ramfs_node **link = &p->first_child;
    while (*link && *link != n)
        link = &(*link)->next_sibling;
    if (*link)
        *link = n->next_sibling;

    if (!n->is_dir)
        free(n->data);
    free(n->name);
    free(n);
    return RAMFS_ERR_OK;
}

/* ── file contents ──────────────────────────────────────────────────── */

static int file_grow(struct ramfs_node *f, size_t need)
{
    if (need <= f->cap)
        return RAMFS_ERR_OK;
    size_t ncap = f->cap ? f->cap : 32;
    while (ncap < need)
        ncap *= 2;
    /* grow = new block + copy (bump allocator: no in-place realloc) */
    char *nd = malloc(ncap);
    if (!nd)
        return RAMFS_ERR_NOMEM;
    if (f->data) {
        memcpy(nd, f->data, f->size);
        /* old block intentionally leaked until the PMM lands */
    }
    f->data = nd;
    f->cap = ncap;
    return RAMFS_ERR_OK;
}

int ramfs_write(struct ramfs_node *f, const void *data, size_t len)
{
    if (!f || f->is_dir)
        return RAMFS_ERR_ISDIR;
    int rc = file_grow(f, len + 1);
    if (rc)
        return rc;
    memcpy(f->data, data, len);
    f->data[len] = 0;
    f->size = len;
    return RAMFS_ERR_OK;
}

int ramfs_append(struct ramfs_node *f, const void *data, size_t len)
{
    if (!f || f->is_dir)
        return RAMFS_ERR_ISDIR;
    int rc = file_grow(f, f->size + len + 1);
    if (rc)
        return rc;
    memcpy(f->data + f->size, data, len);
    f->size += len;
    f->data[f->size] = 0;
    return RAMFS_ERR_OK;
}

const char *ramfs_data(struct ramfs_node *f)
{
    return f && f->data ? f->data : "";
}

size_t ramfs_size(struct ramfs_node *f)
{
    return f && !f->is_dir ? f->size : 0;
}

/* ── listing ────────────────────────────────────────────────────────── */

size_t ramfs_children(struct ramfs_node *dir, struct ramfs_node **out, size_t max)
{
    if (!dir || !dir->is_dir)
        return 0;
    size_t n = 0;
    for (struct ramfs_node *c = dir->first_child; c && n < max; c = c->next_sibling)
        out[n++] = c;
    /* children are kept sorted at insert time */
    return n;
}

char *ramfs_path_of(struct ramfs_node *node, char *buf, size_t cap)
{
    if (!node || !buf || cap == 0)
        return 0;
    if (node == &root_node) {
        buf[0] = '/';
        buf[1] = 0;
        return buf;
    }
    /* build from the end */
    size_t pos = cap - 1;
    buf[pos] = 0;
    for (struct ramfs_node *n = node; n && n != &root_node; n = n->parent) {
        size_t len = strlen(n->name);
        if (pos < len + 1)
            return 0; /* truncated */
        pos -= len;
        memcpy(buf + pos, n->name, len);
        if (pos)
            buf[--pos] = '/';
    }
    if (pos == cap - 1)
        buf[--pos] = '/'; /* bare name under root: prefix "/" */
    memmove(buf, buf + pos, cap - pos);
    return buf;
}

int ramfs_is_dir(struct ramfs_node *n)
{
    return n && n->is_dir;
}

const char *ramfs_node_name(struct ramfs_node *n)
{
    return n && *n->name ? n->name : "/";
}
