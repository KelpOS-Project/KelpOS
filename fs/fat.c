/* fs/fat.c — read-only FAT12/16/32 + exFAT backend (see fat.h).
 *
 * Reads BPB/directory structures straight from the block device,
 * follows FAT cluster chains (and exFAT no-fat-chain runs), assembles
 * long file names from LFN/UTF-16 entries. Writes: not yet — the
 * "diskfs" package stays read-only for now. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fat.h"
#include "block.h"

#define SECTOR 512u

enum { FAT12 = 1, FAT16, FAT32, EXFAT };

struct fat_ctx {
    int type;
    uint16_t spc;             /* sectors per cluster */
    uint16_t reserved;
    uint8_t  nfats;
    uint32_t fatsz;           /* sectors per FAT */
    uint32_t root_entries;    /* FAT12/16 */
    uint32_t totsec;
    uint32_t root_cluster;    /* FAT32/exFAT */
    uint32_t fat_start;       /* LBA of first FAT */
    uint32_t data_start;      /* LBA of cluster 2 */
    uint32_t clusters;
    uint32_t cluster_bytes;
    char     label[16];
};

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint64_t rd64(const uint8_t *p)
{
    return rd32(p) | (uint64_t)rd32(p + 4) << 32;
}

/* ── FAT access ─────────────────────────────────────────────────────── */

static uint32_t fat_entry(struct fat_ctx *f, struct blockdev *bd,
                          uint32_t cluster)
{
    static uint8_t s[SECTOR]; /* single-threaded boot kernel: fine */

    if (f->type == FAT12) {
        uint32_t off = cluster * 3 / 2;
        if (bd->read(bd, f->fat_start + off / SECTOR, 1, s))
            return 0;
        uint16_t v = s[off % SECTOR] | ((uint16_t)s[(off + 1) % SECTOR] << 8);
        return (cluster & 1) ? v >> 4 : v & 0xFFF;
    }
    if (f->type == FAT16) {
        uint32_t off = cluster * 2;
        if (bd->read(bd, f->fat_start + off / SECTOR, 1, s))
            return 0;
        return rd16(s + off % SECTOR);
    }
    uint32_t off = cluster * 4;
    if (bd->read(bd, f->fat_start + off / SECTOR, 1, s))
        return 0;
    uint32_t v = rd32(s + off % SECTOR);
    return f->type == EXFAT ? v : (v & 0x0FFFFFFFu);
}

static int is_eoc(struct fat_ctx *f, uint32_t v)
{
    if (f->type == FAT12)
        return v >= 0xFF8;
    if (f->type == FAT16)
        return v >= 0xFFF8;
    if (f->type == FAT32)
        return v >= 0x0FFFFFF8;
    return v >= 0xFFFFFFF8; /* exFAT */
}

/* ── probe & mount ──────────────────────────────────────────────────── */

static int fat_probe(struct blockdev *bd)
{
    uint8_t s[SECTOR];
    if (bd->read(bd, 0, 1, s))
        return 0;
    if (rd16(s + 510) != 0xAA55)
        return 0;
    if (memcmp(s + 3, "EXFAT   ", 8) == 0)
        return 1;
    if (rd16(s + 11) != SECTOR || s[13] == 0)
        return 0;
    uint32_t fatsz = rd16(s + 22) ? rd16(s + 22) : rd32(s + 36);
    uint32_t totsec = rd16(s + 19) ? rd16(s + 19) : rd32(s + 32);
    uint32_t rootsec = (rd16(s + 17) * 32 + SECTOR - 1) / SECTOR;
    uint32_t data = rd16(s + 14) + (uint32_t)s[16] * fatsz + rootsec;
    uint32_t clusters = totsec >= data ? (totsec - data) / s[13] : 0;
    return clusters >= 2;
}

static int fat_mount(struct vfs_fs *fs)
{
    struct blockdev *bd = fs->bd;
    uint8_t s[SECTOR];

    if (bd->read(bd, 0, 1, s))
        return -1;

    struct fat_ctx *f = malloc(sizeof(*f));
    if (!f)
        return -1;
    memset(f, 0, sizeof(*f));
    fs->ctx = f;

    if (memcmp(s + 3, "EXFAT   ", 8) == 0) {
        f->type = EXFAT;
        if ((1u << s[99]) != SECTOR)
            goto fail;
        f->spc = (uint16_t)1 << s[100];
        f->nfats = s[101];
        f->fat_start = rd32(s + 72);
        f->fatsz = rd32(s + 76);
        f->data_start = rd32(s + 80);
        f->root_cluster = rd32(s + 88);
        f->clusters = rd32(s + 84);
        f->label[0] = 0;
    } else {
        if (rd16(s + 11) != SECTOR)
            goto fail;
        f->spc = s[13];
        f->reserved = rd16(s + 14);
        f->nfats = s[16];
        f->root_entries = rd16(s + 17);
        f->totsec = rd16(s + 19) ? rd16(s + 19) : rd32(s + 32);
        f->fatsz = rd16(s + 22) ? rd16(s + 22) : rd32(s + 36);
        if (!f->spc || !f->nfats || !f->fatsz || !f->totsec)
            goto fail;
        f->fat_start = f->reserved;
        uint32_t rootsec = (f->root_entries * 32 + SECTOR - 1) / SECTOR;
        f->data_start = f->reserved + f->nfats * f->fatsz + rootsec;
        f->clusters = f->totsec > f->data_start
                          ? (f->totsec - f->data_start) / f->spc : 0;
        f->root_cluster = rd32(s + 44);
        f->type = f->clusters < 4085 ? FAT12
                : f->clusters < 65525 ? FAT16 : FAT32;
        if (f->type == FAT32 && !f->root_cluster)
            goto fail;
        memcpy(f->label, s + 43, 11); /* BPB volume label */
        f->label[11] = 0;
    }
    f->cluster_bytes = (uint32_t)f->spc * SECTOR;
    if (f->cluster_bytes > 256 * 1024)
        goto fail;
    return 0;

fail:
    free(f);
    fs->ctx = 0;
    return -1;
}

/* ── cluster chain reading ──────────────────────────────────────────── */

static int read_cluster(struct fat_ctx *f, struct blockdev *bd, uint32_t cl,
                        void *buf)
{
    uint32_t lba = f->data_start + (cl - 2) * f->spc;
    return bd->read(bd, lba, f->spc, buf);
}

/* ── directory entry stream ─────────────────────────────────────────── */

struct dirent_fat {
    char name[256];
    uint32_t first_cluster;
    uint64_t size;
    int is_dir;
};

/* entry assembler: feeds raw 32-byte entries, emits complete files */
struct asm_state {
    struct fat_ctx *f;
    /* FAT LFN assembly */
    char lfn[256];
    int  lfn_seq; /* 0 = no LFN pending */
    /* exFAT assembly */
    char ex_name[256];
    uint32_t ex_cluster;
    uint64_t ex_size;
    int ex_dir, ex_ready;
};

/* utf16le (len chars) -> ascii into dst */
static void utf16_ascii(const uint8_t *src, size_t len, char *dst, size_t cap)
{
    size_t o = strlen(dst);
    for (size_t i = 0; i + 1 < len && o + 1 < cap; i += 2) {
        uint16_t u = src[i] | ((uint16_t)src[i + 1] << 8);
        if (u == 0 || u == 0xFFFF)
            break;
        dst[o++] = u < 128 ? (char)u : '_';
    }
    dst[o] = 0;
}

static void short_name(const uint8_t *p, char *out, size_t cap)
{
    size_t o = 0;
    for (int i = 0; i < 8 && p[i] != ' ' && p[i]; i++)
        if (o + 1 < cap)
            out[o++] = (char)p[i];
    if (p[8] != ' ' && p[8]) {
        if (o + 1 < cap)
            out[o++] = '.';
        for (int i = 8; i < 11 && p[i] != ' ' && p[i]; i++)
            if (o + 1 < cap)
                out[o++] = (char)p[i];
    }
    out[o] = 0;
}

#define ENT_END    -1  /* no more entries in this directory */
#define ENT_SKIP    0  /* not a file/dir entry */
#define ENT_FILE    1  /* complete entry assembled */

/* flush a complete exFAT entry set into `out` */
static int ex_flush(struct asm_state *st, struct dirent_fat *out)
{
    strncpy(out->name, st->ex_name, sizeof(out->name));
    out->name[sizeof(out->name) - 1] = 0;
    out->first_cluster = st->ex_cluster;
    out->size = st->ex_size;
    out->is_dir = st->ex_dir;
    st->ex_name[0] = 0;
    return ENT_FILE;
}

/* feed one raw 32-byte entry */
static int feed_entry(struct asm_state *st, const uint8_t *p,
                      struct dirent_fat *out)
{
    struct fat_ctx *f = st->f;

    if (f->type == EXFAT) {
        uint8_t t = p[0];
        if (t == 0x00) {
            /* end of directory: flush a pending entry set first */
            if (st->ex_name[0])
                return ex_flush(st, out);
            return ENT_END;
        }
        if (t == 0x85) { /* new file entry set: flush the previous one */
            if (st->ex_name[0])
                return ex_flush(st, out);
            st->ex_dir = (p[4] & 0x10) != 0; /* attributes at offset 4 */
            st->ex_name[0] = 0;
            st->ex_cluster = 0;
            st->ex_size = 0;
            return ENT_SKIP;
        }
        if (t == 0xC0) { /* stream extension: cluster + size */
            st->ex_cluster = rd32(p + 20);
            st->ex_size = rd64(p + 24);
            return ENT_SKIP;
        }
        if (t == 0xC1) { /* name entry: up to 15 UTF-16 chars */
            utf16_ascii(p + 2, 30, st->ex_name, sizeof(st->ex_name));
            return ENT_SKIP;
        }
        return ENT_SKIP; /* volume label / bitmap / upcase / tex */
    }

    /* FAT family */
    if (p[0] == 0x00)
        return ENT_END;
    if (p[0] == 0xE5)
        return ENT_SKIP; /* deleted */

    uint8_t attr = p[11];
    if (attr == 0x0F) { /* LFN */
        uint8_t seq = p[0] & 0x1F;
        if (p[0] & 0x40) { /* first (highest) part of the name */
            st->lfn[0] = 0;
            st->lfn_seq = seq;
        }
        if (st->lfn_seq && seq == st->lfn_seq) {
            /* parts arrive highest-first; prepend each to the tail */
            char part[64];
            part[0] = 0;
            utf16_ascii(p + 1, 10, part, sizeof(part));
            utf16_ascii(p + 14, 12, part, sizeof(part));
            utf16_ascii(p + 28, 4, part, sizeof(part));
            size_t plen = strlen(part), old = strlen(st->lfn);
            memmove(st->lfn + plen, st->lfn, old + 1);
            memcpy(st->lfn, part, plen);
            st->lfn_seq--;
        }
        return ENT_SKIP;
    }
    if (attr & 0x08)
        return ENT_SKIP; /* volume label */

    short_name(p, out->name, sizeof(out->name));
    out->first_cluster = rd16(p + 26) | ((uint32_t)rd16(p + 20) << 16);
    out->size = rd32(p + 28);
    out->is_dir = (attr & 0x10) != 0;
    if (st->lfn[0] && !st->lfn_seq)
        strncpy(out->name, st->lfn, sizeof(out->name));
    out->name[sizeof(out->name) - 1] = 0;
    st->lfn[0] = 0;
    return ENT_FILE;
}

/* iterate a directory; cb returns nonzero to stop */
static int dir_iterate(struct asm_state *st, struct blockdev *bd,
                       uint32_t dir_cluster, long fixed_entries,
                       int (*cb)(struct dirent_fat *de, void *ud), void *ud)
{
    struct fat_ctx *f = st->f;
    uint8_t *buf = malloc(f->cluster_bytes > SECTOR ? f->cluster_bytes : SECTOR);
    if (!buf)
        return -1;

    int rc = -1;
    struct dirent_fat de;

    if (fixed_entries >= 0) {
        /* FAT12/16: fixed root region right after the FATs */
        long total = fixed_entries;
        uint32_t lba = f->reserved + f->nfats * f->fatsz;
        for (long off = 0; off < total * 32; off += SECTOR) {
            if (bd->read(bd, lba + (uint32_t)(off / SECTOR), 1, buf))
                goto out;
            for (unsigned e = 0; e < SECTOR / 32; e++) {
                int r = feed_entry(st, buf + e * 32, &de);
                if (r == ENT_END) { rc = 0; goto out; }
                if (r == ENT_FILE && cb(&de, ud)) { rc = 1; goto out; }
            }
        }
        rc = 0;
        goto out;
    }

    uint32_t cl = dir_cluster;
    for (;;) {
        if (cl < 2 || is_eoc(f, cl) || read_cluster(f, bd, cl, buf))
            goto out;
        for (unsigned e = 0; e < f->cluster_bytes / 32; e++) {
            int r = feed_entry(st, buf + e * 32, &de);
            if (r == ENT_END) { rc = 0; goto out; }
            if (r == ENT_FILE && cb(&de, ud)) { rc = 1; goto out; }
        }
        uint32_t next = fat_entry(f, bd, cl);
        if (next < 2)
            goto out;
        cl = next;
    }
out:
    free(buf);
    return rc;
}

/* ── path resolution ────────────────────────────────────────────────── */

struct find_ud {
    const char *want;
    struct dirent_fat found;
    int ok;
};

static int find_cb(struct dirent_fat *de, void *ud)
{
    struct find_ud *u = ud;
    if (strcasecmp(de->name, u->want) == 0) {
        u->found = *de;
        u->ok = 1;
        return 1;
    }
    return 0;
}

/* resolve a path inside the volume to a directory entry;
 * returns 0 ok (out filled), 1 not found, <0 error */
static int fat_resolve(struct vfs_fs *fs, const char *path,
                       struct dirent_fat *out)
{
    struct fat_ctx *f = fs->ctx;
    struct asm_state st;
    memset(&st, 0, sizeof(st));
    st.f = f;

    if (!*path || strcmp(path, "/") == 0) {
        memset(out, 0, sizeof(*out));
        strcpy(out->name, "/");
        out->is_dir = 1;
        out->first_cluster = f->type == FAT12 || f->type == FAT16
                                 ? 0 : f->root_cluster;
        return 0;
    }

    /* current directory while walking */
    uint32_t dir_cluster = f->type == FAT12 || f->type == FAT16
                               ? 0 : f->root_cluster;
    long fixed_entries = (f->type == FAT12 || f->type == FAT16)
                             ? (long)f->root_entries : -1;

    char comp[128];
    struct dirent_fat last = {0};
    const char *p = path;
    while (*p == '/')
        p++;
    while (*p) {
        size_t n = 0;
        while (*p && *p != '/' && n + 1 < sizeof(comp))
            comp[n++] = *p++;
        comp[n] = 0;
        while (*p == '/')
            p++;
        if (!*comp)
            continue;

        if (fixed_entries < 0 && dir_cluster < 2)
            return 1;

        struct find_ud u;
        memset(&u, 0, sizeof(u));
        u.want = comp;
        int rc = dir_iterate(&st, fs->bd, dir_cluster, fixed_entries,
                             find_cb, &u);
        if (rc < 0)
            return -1;
        if (!u.ok)
            return 1;

        last = u.found;
        dir_cluster = last.first_cluster;
        fixed_entries = -1; /* subdirs are chained, even on FAT12/16 */
        if (!last.is_dir && *p)
            return 1; /* file in the middle of the path */
    }

    *out = last; /* keeps the real on-disk name, is_dir and size */
    out->first_cluster = dir_cluster;
    return 0;
}

/* ── backend ops ────────────────────────────────────────────────────── */

struct list_ud {
    int (*cb)(const char *name, int is_dir, uint64_t size, void *ud);
    void *ud;
};

static int list_cb(struct dirent_fat *de, void *ud)
{
    struct list_ud *u = ud;
    u->cb(de->name, de->is_dir, de->size, u->ud);
    return 0;
}

static int fat_list(struct vfs_fs *fs, const char *dir,
                    int (*cb)(const char *name, int is_dir, uint64_t size,
                              void *ud),
                    void *ud)
{
    struct fat_ctx *f = fs->ctx;
    struct dirent_fat target;
    int rc = fat_resolve(fs, dir, &target);
    if (rc != 0)
        return rc;
    if (!target.is_dir)
        return -1;

    struct asm_state st;
    memset(&st, 0, sizeof(st));
    st.f = f;

    long fixed = (f->type == FAT12 || f->type == FAT16) &&
                 target.first_cluster < 2
                     ? (long)f->root_entries : -1;
    struct list_ud u = { cb, ud };
    return dir_iterate(&st, fs->bd, target.first_cluster, fixed, list_cb, &u);
}

static long fat_read_file(struct vfs_fs *fs, const char *path, char *buf,
                          size_t cap)
{
    struct fat_ctx *f = fs->ctx;
    struct dirent_fat target;
    int rc = fat_resolve(fs, path, &target);
    if (rc != 0 || target.is_dir)
        return -1;

    uint8_t *cbuf = malloc(f->cluster_bytes);
    if (!cbuf)
        return -1;

    long total = 0;
    uint32_t cl = target.first_cluster;
    /* dirent size is authoritative for every FAT flavour now that
     * fat_resolve preserves it; cap still bounds the copy */
    size_t want = cap;
    if (target.size < want)
        want = (size_t)target.size;

    for (;;) {
        if (cl < 2 || is_eoc(f, cl) || read_cluster(f, fs->bd, cl, cbuf)) {
            break;
        }
        size_t chunk = f->cluster_bytes;
        if (total + (long)chunk > (long)want)
            chunk = (size_t)((long)want - total);
        if (chunk == 0)
            break;
        memcpy(buf + total, cbuf, chunk);
        total += (long)chunk;
        if (total >= (long)want)
            break;
        uint32_t next = fat_entry(f, fs->bd, cl);
        if (next < 2)
            break;
        cl = next;
    }
    free(cbuf);
    return total;
}

static void fat_info(struct vfs_fs *fs, char *buf, size_t cap)
{
    struct fat_ctx *f = fs->ctx;
    const char *t = f->type == EXFAT ? "exFAT" : f->type == FAT12 ? "FAT12"
                  : f->type == FAT16 ? "FAT16" : "FAT32";
    snprintf(buf, cap, "%s%s", t, f->label[0] ? " \"" : "");
    if (f->label[0])
        strncat(buf, f->label, cap - strlen(buf) - 2);
    strncat(buf, f->label[0] ? "\"" : "", cap - strlen(buf) - 1);
}

const struct fs_ops fat_fs_ops = {
    .name = "fat",
    .probe = fat_probe,
    .mount = fat_mount,
    .list = fat_list,
    .read_file = fat_read_file,
    .info = fat_info,
};
