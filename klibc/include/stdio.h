#ifndef KLIBC_STDIO_H
#define KLIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

int snprintf(char *buffer, size_t size, const char *format, ...);
int vsnprintf(char *buffer, size_t size, const char *format, va_list arguments);

#endif