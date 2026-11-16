/* kernel/users.c — minimal user/group database (see users.h).
 * Non-cryptographic FNV-1a "hashes": good enough to demo the flow,
 * replaced by real hashing once a proper fs + userspace exist. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "users.h"
#include "../fs/ramfs.h"

#define MAX_USERS 16
#define MAX_GROUPS 16

static struct kelp_user users_db[MAX_USERS];
static size_t users_n;
static struct kelp_group groups_db[MAX_GROUPS];
static size_t groups_n;
static int current_user = -1;

static uint32_t fnv1a(const char *s)
{
    uint32_t h = 2166136261u;
    for (; *s; s++) {
        h ^= (uint8_t)*s;
        h *= 16777619u;
    }
    return h;
}

static void hash_password(const char *pw, char *out, size_t cap)
{
    if (!pw || !*pw) {
        out[0] = 0;
        return;
    }
    uint32_t h = fnv1a(pw);
    snprintf(out, cap, "$fnv$%08x", h);
}

void users_init(void)
{
    users_n = groups_n = 0;
    user_add("root", "kelp", 0);
    group_add("users", 100);
    group_add_member("users", "root");
    current_user = 0;
    users_sync();
}

size_t user_count(void) { return users_n; }

const struct kelp_user *user_get(size_t i)
{
    return i < users_n ? &users_db[i] : 0;
}

const struct kelp_user *user_find(const char *name)
{
    for (size_t i = 0; i < users_n; i++)
        if (strcmp(users_db[i].name, name) == 0)
            return &users_db[i];
    return 0;
}

const struct kelp_user *user_current(void)
{
    if (current_user < 0)
        current_user = 0;
    return &users_db[current_user];
}

int user_add(const char *name, const char *password, int gid)
{
    if (!name || !*name || strlen(name) >= sizeof(users_db[0].name))
        return -1;
    if (users_n >= MAX_USERS || user_find(name))
        return -1;

    struct kelp_user *u = &users_db[users_n];
    memset(u, 0, sizeof(*u));
    strcpy(u->name, name);
    u->uid = users_n == 0 ? 0 : 1000 + (int)users_n - 1;
    u->gid = gid;
    hash_password(password, u->phash, sizeof(u->phash));
    users_n++;
    return 0;
}

int user_set_password(const char *name, const char *password)
{
    const struct kelp_user *u = user_find(name);
    if (!u)
        return -1;
    hash_password(password, users_db[u - users_db].phash, sizeof(users_db[0].phash));
    return 0;
}

int user_check_password(const char *name, const char *password)
{
    const struct kelp_user *u = user_find(name);
    if (!u)
        return 0;
    char want[32];
    hash_password(password, want, sizeof(want));
    return strcmp(u->phash, want) == 0;
}

void user_switch(const char *name)
{
    for (size_t i = 0; i < users_n; i++)
        if (strcmp(users_db[i].name, name) == 0)
            current_user = (int)i;
}

size_t group_count(void) { return groups_n; }

const struct kelp_group *group_get(size_t i)
{
    return i < groups_n ? &groups_db[i] : 0;
}

const struct kelp_group *group_find(const char *name)
{
    for (size_t i = 0; i < groups_n; i++)
        if (strcmp(groups_db[i].name, name) == 0)
            return &groups_db[i];
    return 0;
}

int group_add(const char *name, int gid)
{
    if (!name || !*name || strlen(name) >= sizeof(groups_db[0].name))
        return -1;
    if (groups_n >= MAX_GROUPS || group_find(name))
        return -1;
    struct kelp_group *g = &groups_db[groups_n];
    memset(g, 0, sizeof(*g));
    strcpy(g->name, name);
    g->gid = gid;
    groups_n++;
    return 0;
}

int group_add_member(const char *group, const char *user)
{
    struct kelp_group *g = 0;
    for (size_t i = 0; i < groups_n; i++)
        if (strcmp(groups_db[i].name, group) == 0)
            g = &groups_db[i];
    if (!g || !user_find(user))
        return -1;
    if (strstr(g->members, user))
        return 0; /* already a member */
    size_t len = strlen(g->members);
    if (len && len + 1 < sizeof(g->members))
        g->members[len++] = ',';
    if (len + strlen(user) >= sizeof(g->members))
        return -1;
    strcpy(g->members + len, user);
    return 0;
}

/* ── mirror to /etc ─────────────────────────────────────────────────── */

void users_sync(void)
{
    struct ramfs_node *etc = ramfs_resolve_at(ramfs_root(), "/etc");
    if (!etc)
        return;

    struct ramfs_node *pw = ramfs_resolve_at(etc, "passwd");
    if (!pw) {
        ramfs_mkfile("/etc/passwd");
        pw = ramfs_resolve_at(etc, "passwd");
    }
    struct ramfs_node *gr = ramfs_resolve_at(etc, "group");
    if (!gr) {
        ramfs_mkfile("/etc/group");
        gr = ramfs_resolve_at(etc, "group");
    }
    if (!pw || !gr)
        return;

    char line[256];
    ramfs_write(pw, "", 0);
    for (size_t i = 0; i < users_n; i++) {
        const struct kelp_user *u = &users_db[i];
        snprintf(line, sizeof(line), "%s:x:%d:%d::/home/%s:/bin/ksh\n",
                 u->name, u->uid, u->gid, u->name);
        ramfs_append(pw, line, strlen(line));
    }
    ramfs_write(gr, "", 0);
    for (size_t i = 0; i < groups_n; i++) {
        const struct kelp_group *g = &groups_db[i];
        snprintf(line, sizeof(line), "%s:x:%d:%s\n", g->name, g->gid, g->members);
        ramfs_append(gr, line, strlen(line));
    }
}
