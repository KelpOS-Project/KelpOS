#ifndef KELP_KLIB_STDIO_H
#define KELP_KLIB_STDIO_H

#include <stddef.h>
#include <stdarg.h>

/* output goes through kputc (kernel/console.c; host tests provide a stub) */
void kputc(int c);
void kputs(const char *s);
void kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

int  kvsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int  snprintf(char *buf, size_t size, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#endif
