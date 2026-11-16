#ifndef KELP_USERS_H
#define KELP_USERS_H

#include <stddef.h>

/* Minimal user/group database, kept in RAM and mirrored to
 * /etc/passwd and /etc/group in the ramfs. Passwords are stored as
 * NON-CRYPTOGRAPHIC hashes for now (demo milestone); real hashing
 * arrives with userspace + a real fs. */

struct kelp_user {
    char name[32];
    int  uid;
    int  gid;
    char phash[32];   /* hash of the password ("" = no password) */
};

struct kelp_group {
    char name[32];
    int  gid;
    char members[160]; /* comma-separated user names */
};

void users_init(void);   /* seeds root (uid 0) and users group */

size_t              user_count(void);
const struct kelp_user *user_get(size_t i);
const struct kelp_user *user_find(const char *name);
const struct kelp_user *user_current(void);      /* active user (root initially) */

int user_add(const char *name, const char *password, int gid);
int user_set_password(const char *name, const char *password);
int user_check_password(const char *name, const char *password);
void user_switch(const char *name);

size_t              group_count(void);
const struct kelp_group *group_get(size_t i);
const struct kelp_group *group_find(const char *name);
int group_add(const char *name, int gid);
int group_add_member(const char *group, const char *user);

/* sync the databases into /etc/passwd and /etc/group (ramfs) */
void users_sync(void);

#endif
