/* klibc/src/printf.c — kleines printf: %c %s %d %i %u %x %o %p %%, 'l'/'ll' lengths, width/zero-padding */
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include "stdio.h"

struct sbuf {
    char  *buf;
    size_t cap, pos;
};

static void sb_putc(struct sbuf *sb, char c)
{
    if (sb->buf && sb->pos < sb->cap)
        sb->buf[sb->pos] = c;
    sb->pos++;
}

static void sb_puts(struct sbuf *sb, const char *s)
{
    while (*s)
        sb_putc(sb, *s++);
}

static const char digits[] = "0123456789abcdef";

static void sb_pad(struct sbuf *sb, int n, char c)
{
    while (n-- > 0)
        sb_putc(sb, c);
}

/* number with optional padding: width=0 -> no padding; left=1 -> pad right */
static void sb_num(struct sbuf *sb, unsigned long long v, unsigned base,
                   int width, char padc, bool left)
{
    char tmp[24];
    int i = 0;
    if (v == 0)
        tmp[i++] = '0';
    while (v) {
        tmp[i++] = digits[v % base];
        v /= base;
    }
    if (left) {
        int n = i;
        while (i)
            sb_putc(sb, tmp[--i]);
        sb_pad(sb, width - n, ' ');
    } else {
        sb_pad(sb, width - i, padc);
        while (i)
            sb_putc(sb, tmp[--i]);
    }
}

static int vformat(char *buf, size_t cap, const char *fmt, va_list ap)
{
    struct sbuf sb = { buf, cap, 0 };

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            sb_putc(&sb, *fmt);
            continue;
        }
        fmt++;

        /* flags + width: '-' (left-align), '0' (zero-padding) and digits,
         * e.g. %-9s, %08x, %03d */
        int len = 0;    /* 0=int, 1=long, 2=long long */
        int width = 0;  /* minimum field width */
        char padc = ' ';
        bool left = false;
        for (;; fmt++) {
            if (*fmt == '-')
                left = true;
            else if (*fmt == '0') {
                if (!left)
                    padc = '0';
            } else
                break;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        while (*fmt == 'l') {
            len++;
            fmt++;
        }

        switch (*fmt) {
        case 'c': {
            if (left) {
                sb_putc(&sb, (char)va_arg(ap, int));
                sb_pad(&sb, width - 1, ' ');
            } else {
                sb_pad(&sb, width - 1, ' ');
                sb_putc(&sb, (char)va_arg(ap, int));
            }
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            const char *t = s ? s : "(null)";
            int slen = 0;
            while (t[slen])
                slen++;
            if (left) {
                sb_puts(&sb, t);
                sb_pad(&sb, width - slen, ' ');
            } else {
                sb_pad(&sb, width - slen, ' ');
                sb_puts(&sb, t);
            }
            break;
        }
        case 'd':
        case 'i': {
            long long v = len == 2 ? va_arg(ap, long long)
                        : len == 1 ? va_arg(ap, long)
                                   : va_arg(ap, int);
            unsigned long long u;
            if (v < 0) {
                sb_putc(&sb, '-');
                u = (unsigned long long)(-(v + 1)) + 1ULL;
                width--;
            } else {
                u = (unsigned long long)v;
            }
            sb_num(&sb, u, 10, width, padc, left);
            break;
        }
        case 'u':
        case 'x':
        case 'o': {
            unsigned long long v = len == 2 ? va_arg(ap, unsigned long long)
                                 : len == 1 ? va_arg(ap, unsigned long)
                                            : va_arg(ap, unsigned int);
            unsigned base = *fmt == 'u' ? 10 : *fmt == 'x' ? 16 : 8;
            sb_num(&sb, v, base, width, padc, left);
            break;
        }
        case 'p':
            sb_puts(&sb, "0x");
            sb_num(&sb, (uintptr_t)va_arg(ap, void *), 16, 0, ' ', false);
            break;
        case '%':
            sb_putc(&sb, '%');
            break;
        case 0:
            goto done;
        default:
            sb_putc(&sb, '%');
            sb_putc(&sb, *fmt);
            break;
        }
    }
done:
    if (buf && cap)
        buf[sb.pos < cap ? sb.pos : cap - 1] = 0;
    return (int)sb.pos;
}

void kprintf(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vformat(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    for (const char *p = buf; *p; p++)
        kputc(*p);
}

void kputs(const char *s)
{
    while (*s)
        kputc(*s++);
    kputc('\n');
}

int kvsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    return vformat(buf, size, fmt, ap);
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vformat(buf, size, fmt, ap);
    va_end(ap);
    return r;
}
