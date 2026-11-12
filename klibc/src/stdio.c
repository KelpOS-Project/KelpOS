#include "../include/stdio.h"

#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

enum format_length {
    FORMAT_DEFAULT,
    FORMAT_LONG,
    FORMAT_LONG_LONG,
    FORMAT_SIZE,
};

struct output_buffer {
    char *buffer;
    size_t capacity;
    size_t length;
};

static void write_character(struct output_buffer *output, char character)
{
    if (output->capacity != 0 && output->length < output->capacity - 1) {
        output->buffer[output->length] = character;
    }
    if (output->length != SIZE_MAX) {
        output->length++;
    }
}

static void write_string(struct output_buffer *output, const char *string)
{
    if (string == NULL) {
        string = "(null)";
    }
    while (*string != '\0') {
        write_character(output, *string++);
    }
}

static void write_unsigned(struct output_buffer *output, uintmax_t value,
    unsigned int base, bool uppercase)
{
    static const char lower_digits[] = "0123456789abcdef";
    static const char upper_digits[] = "0123456789ABCDEF";
    const char *digits = uppercase ? upper_digits : lower_digits;
    char reversed[sizeof(value) * 8];
    size_t count = 0;
    do {
        reversed[count++] = digits[value % base];
        value /= base;
    } while (value != 0);
    while (count != 0) {
        write_character(output, reversed[--count]);
    }
}

static uintmax_t read_unsigned(va_list *arguments, enum format_length length)
{
    switch (length) {
    case FORMAT_LONG:
        return va_arg(*arguments, unsigned long);
    case FORMAT_LONG_LONG:
        return va_arg(*arguments, unsigned long long);
    case FORMAT_SIZE:
        return va_arg(*arguments, size_t);
    default:
        return va_arg(*arguments, unsigned int);
    }
}

static intmax_t read_signed(va_list *arguments, enum format_length length)
{
    switch (length) {
    case FORMAT_LONG:
        return va_arg(*arguments, long);
    case FORMAT_LONG_LONG:
        return va_arg(*arguments, long long);
    case FORMAT_SIZE:
        return va_arg(*arguments, ptrdiff_t);
    default:
        return va_arg(*arguments, int);
    }
}

int vsnprintf(char *buffer, size_t size, const char *format, va_list arguments)
{
    struct output_buffer output = { buffer, size, 0 };
    va_list values;
    va_copy(values, arguments);

    while (*format != '\0') {
        if (*format != '%') {
            write_character(&output, *format++);
            continue;
        }
        format++;
        if (*format == '%') {
            write_character(&output, *format++);
            continue;
        }

        enum format_length length = FORMAT_DEFAULT;
        if (*format == 'l') {
            format++;
            length = FORMAT_LONG;
            if (*format == 'l') {
                format++;
                length = FORMAT_LONG_LONG;
            }
        } else if (*format == 'z') {
            format++;
            length = FORMAT_SIZE;
        }

        switch (*format) {
        case 'c':
            write_character(&output, (char)va_arg(values, int));
            break;
        case 's':
            write_string(&output, va_arg(values, const char *));
            break;
        case 'd':
        case 'i': {
            const intmax_t number = read_signed(&values, length);
            if (number < 0) {
                write_character(&output, '-');
                write_unsigned(&output, (uintmax_t)0 - (uintmax_t)number, 10, false);
            } else {
                write_unsigned(&output, (uintmax_t)number, 10, false);
            }
            break;
        }
        case 'u':
            write_unsigned(&output, read_unsigned(&values, length), 10, false);
            break;
        case 'x':
        case 'X':
            write_unsigned(&output, read_unsigned(&values, length), 16,
                *format == 'X');
            break;
        case 'p':
            write_string(&output, "0x");
            write_unsigned(&output, (uintptr_t)va_arg(values, void *), 16, false);
            break;
        case '\0':
            write_character(&output, '%');
            format--;
            break;
        default:
            write_character(&output, '%');
            write_character(&output, *format);
            break;
        }
        if (*format != '\0') {
            format++;
        }
    }

    va_end(values);
    if (size != 0) {
        const size_t terminator = output.length < size ? output.length : size - 1;
        buffer[terminator] = '\0';
    }
    if (output.length > INT_MAX) {
        return -1;
    }
    return (int)output.length;
}

int snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    const int result = vsnprintf(buffer, size, format, arguments);
    va_end(arguments);
    return result;
}