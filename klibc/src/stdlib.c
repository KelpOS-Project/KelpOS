#include "../include/ctype.h"
#include "../include/errno.h"
#include "../include/stdlib.h"

#include <limits.h>

static int digit_value(char character)
{
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'z') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'Z') {
        return character - 'A' + 10;
    }
    return -1;
}

static unsigned long parse_unsigned(const char *string, char **end, int base,
    unsigned long positive_limit, unsigned long negative_limit,
    int *negative, int *range_error)
{
    const char *cursor = string;
    while (isspace((unsigned char)*cursor)) {
        cursor++;
    }

    *negative = 0;
    if (*cursor == '+' || *cursor == '-') {
        *negative = *cursor == '-';
        cursor++;
    }
    const unsigned long limit = *negative ? negative_limit : positive_limit;
    *range_error = 0;

    if ((base == 0 || base == 16)
            && cursor[0] == '0'
            && (cursor[1] == 'x' || cursor[1] == 'X')
            && digit_value(cursor[2]) >= 0 && digit_value(cursor[2]) < 16) {
        base = 16;
        cursor += 2;
    } else if (base == 0) {
        base = cursor[0] == '0' ? 8 : 10;
    }

    if (base < 2 || base > 36) {
        errno = EINVAL;
        if (end != NULL) {
            *end = (char *)string;
        }
        return 0;
    }

    const char *digits = cursor;
    unsigned long value = 0;
    int overflow = 0;
    for (;;) {
        const int digit = digit_value(*cursor);
        if (digit < 0 || digit >= base) {
            break;
        }
        if (value > (limit - (unsigned long)digit) / (unsigned long)base) {
            overflow = 1;
            value = limit;
        } else if (!overflow) {
            value = value * (unsigned long)base + (unsigned long)digit;
        }
        cursor++;
    }

    if (cursor == digits) {
        if (end != NULL) {
            *end = (char *)string;
        }
        return 0;
    }
    if (end != NULL) {
        *end = (char *)cursor;
    }
    if (overflow) {
        errno = ERANGE;
        *range_error = 1;
    }
    return value;
}

int abs(int value)
{
    return value < 0 ? -value : value;
}

long labs(long value)
{
    return value < 0 ? -value : value;
}

long long llabs(long long value)
{
    return value < 0 ? -value : value;
}

long strtol(const char *string, char **end, int base)
{
    int negative;
    int range_error;
    const unsigned long negative_limit = (unsigned long)LONG_MAX + 1UL;
    const unsigned long value = parse_unsigned(string, end, base,
        (unsigned long)LONG_MAX, negative_limit, &negative, &range_error);
    if (range_error) {
        return negative ? LONG_MIN : LONG_MAX;
    }
    if (negative) {
        if (value == negative_limit) {
            return LONG_MIN;
        }
        return -(long)value;
    }
    return (long)value;
}

unsigned long strtoul(const char *string, char **end, int base)
{
    int negative;
    int range_error;
    const unsigned long value = parse_unsigned(string, end, base, ULONG_MAX,
        ULONG_MAX, &negative, &range_error);
    if (range_error) {
        return ULONG_MAX;
    }
    return negative ? 0UL - value : value;
}

int atoi(const char *string)
{
    return (int)strtol(string, NULL, 10);
}

long atol(const char *string)
{
    return strtol(string, NULL, 10);
}

long long atoll(const char *string)
{
    return strtoll(string, NULL, 10);
}

long long strtoll(const char *string, char **end, int base)
{
    return (long long)strtol(string, end, base);
}

unsigned long long strtoull(const char *string, char **end, int base)
{
    return (unsigned long long)strtoul(string, end, base);
}