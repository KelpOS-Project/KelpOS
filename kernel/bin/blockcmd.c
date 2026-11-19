/* kernel/bin/blockcmd.c — Unix-style block device tools:
 *
 *   lsblk   list block devices (name, size, read-only flag)
 *   sync    flush the write caches of all devices
 *   dd      copy raw blocks between block devices
 *
 * Device names are given with or without the /dev/ prefix. `zero` and
 * `null` behave like /dev/zero and /dev/null, so e.g.
 *   dd if=zero of=sda bs=512 count=2048
 * wipes the first MiB of sda.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "commands.h"
#include "block.h"
#include "console.h"

static struct blockdev *dev_lookup(const char *name)
{
    if (!name || !*name)
        return 0;
    if (strncmp(name, "/dev/", 5) == 0)
        name += 5;
    return block_find(name);
}

int cmd_lsblk(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("NAME      SECTORS      SIZE  MODE\n");
    for (size_t i = 0; i < block_count(); i++) {
        struct blockdev *bd = block_get(i);
        if (!bd)
            continue;
        kprintf("%-8s %10llu %7lluM  %s\n", bd->name,
                (unsigned long long)bd->blocks,
                (unsigned long long)(bd->blocks / 2048),
                bd->write ? "rw" : "ro");
    }
    return 0;
}

int cmd_sync(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    int n = 0, err = 0;
    for (size_t i = 0; i < block_count(); i++) {
        struct blockdev *bd = block_get(i);
        if (!bd || !bd->flush)
            continue;
        if (bd->flush(bd) != BLOCK_ERR_OK)
            err = 1;
        else
            n++;
    }
    kprintf("sync: %d device(s) flushed%s\n", n,
            err ? " (some failures)" : "");
    return err;
}

static int parse_num(const char *s, uint64_t *out)
{
    uint64_t v = 0;
    if (!s || !*s)
        return -1;
    for (; *s; s++) {
        if (*s < '0' || *s > '9')
            return -1;
        v = v * 10 + (uint64_t)(*s - '0');
        if (v > (1ULL << 40))
            return -1;
    }
    *out = v;
    return 0;
}

int cmd_dd(int argc, char **argv)
{
    const char *in_name = 0, *out_name = 0;
    uint64_t bs = 512, count = 0, skip = 0, seek = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *val = strchr(a, '=');
        if (!val || val == a) {
            kprintf("dd: expected key=value, got '%s'\n", a);
            return 1;
        }
        size_t klen = (size_t)(val - a);
        val++;
        if (klen == 2 && !strncmp(a, "if", 2))
            in_name = val;
        else if (klen == 2 && !strncmp(a, "of", 2))
            out_name = val;
        else if (klen == 2 && !strncmp(a, "bs", 2)) {
            if (parse_num(val, &bs) < 0 || bs == 0 || bs > (1u << 20)) {
                kprintf("dd: bs must be 1..1048576\n");
                return 1;
            }
        } else if (klen == 5 && !strncmp(a, "count", 5)) {
            if (parse_num(val, &count) < 0) {
                kprintf("dd: bad count\n");
                return 1;
            }
        } else if (klen == 4 && !strncmp(a, "skip", 4)) {
            if (parse_num(val, &skip) < 0) {
                kprintf("dd: bad skip\n");
                return 1;
            }
        } else if (klen == 4 && !strncmp(a, "seek", 4)) {
            if (parse_num(val, &seek) < 0) {
                kprintf("dd: bad seek\n");
                return 1;
            }
        } else {
            kprintf("dd: unknown operand '%s'\n", a);
            return 1;
        }
    }
    if (!in_name || !out_name) {
        kprintf("usage: dd if=<dev> of=<dev> [bs=N] [count=N] [skip=N] [seek=N]\n");
        return 1;
    }

    int in_zero = !strcmp(in_name, "zero") || !strcmp(in_name, "/dev/zero");
    int out_null = !strcmp(out_name, "null") || !strcmp(out_name, "/dev/null");
    struct blockdev *in = in_zero ? 0 : dev_lookup(in_name);
    struct blockdev *out = out_null ? 0 : dev_lookup(out_name);
    if (!in_zero && !in)
        return (kprintf("dd: no such device: %s\n", in_name), 1);
    if (!out_null && !out)
        return (kprintf("dd: no such device: %s\n", out_name), 1);
    if (out && !out->write)
        return (kprintf("dd: %s is read-only\n", out->name), 1);
    if (!in_zero || !out_null) {
        if ((bs & 511u) || ((skip * bs) & 511u) || ((seek * bs) & 511u)) {
            kprintf("dd: bs, skip and seek must be multiples of 512 "
                    "for block devices\n");
            return 1;
        }
    }

    uint64_t total = 0;
    if (in_zero) {
        total = count ? count * bs : (out ? out->blocks * 512u : 0);
    } else {
        total = in->blocks * 512u;
    }

    uint64_t off_in = skip * bs;
    uint64_t off_out = seek * bs;
    if (off_in >= total)
        total = 0;
    else
        total -= off_in;
    if (count && count * bs < total)
        total = count * bs;
    if (out && off_out + total > out->blocks * 512u)
        total = out->blocks * 512u - off_out;

    /* block devices move whole sectors; zero-fill partial tails */
    uint8_t *buf = malloc((size_t)bs);
    if (!buf) {
        kprintf("dd: out of memory\n");
        return 1;
    }

    uint64_t done = 0;
    while (done < total) {
        uint64_t left = total - done;
        size_t n = left > bs ? (size_t)bs : (size_t)left;
        if (in_zero)
            memset(buf, 0, n);
        else if (in->read(in, (off_in + done) / 512u,
                          (uint32_t)((n + 511) / 512u), buf) != BLOCK_ERR_OK) {
            kprintf("\ndd: read error at byte %llu\n",
                    (unsigned long long)(off_in + done));
            free(buf);
            return 1;
        }
        if (!out_null &&
            out->write(out, (off_out + done) / 512u,
                       (uint32_t)((n + 511) / 512u), buf) != BLOCK_ERR_OK) {
            kprintf("\ndd: write error at byte %llu\n",
                    (unsigned long long)(off_out + done));
            free(buf);
            return 1;
        }
        done += n;
    }

    if (out && out->flush)
        out->flush(out);
    free(buf);
    kprintf("%llu bytes copied\n", (unsigned long long)done);
    return 0;
}
