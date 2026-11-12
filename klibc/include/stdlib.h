#ifndef KLIBC_STDLIB_H
#define KLIBC_STDLIB_H

#include <stddef.h>

int abs(int value);
long labs(long value);
long long llabs(long long value);
int atoi(const char *string);
long atol(const char *string);
long long atoll(const char *string);
long strtol(const char *string, char **end, int base);
unsigned long strtoul(const char *string, char **end, int base);
long long strtoll(const char *string, char **end, int base);
unsigned long long strtoull(const char *string, char **end, int base);

#endif