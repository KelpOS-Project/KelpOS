#include "../include/string.h"

#include <stdint.h>

void *memcpy(void *destination, const void *source, size_t count)
{
    unsigned char *output = destination;
    const unsigned char *input = source;
    for (size_t index = 0; index < count; index++) {
        output[index] = input[index];
    }
    return destination;
}

void *memmove(void *destination, const void *source, size_t count)
{
    unsigned char *output = destination;
    const unsigned char *input = source;
    const uintptr_t output_address = (uintptr_t)output;
    const uintptr_t input_address = (uintptr_t)input;
    if (output_address <= input_address || output_address - input_address >= count) {
        for (size_t index = 0; index < count; index++) {
            output[index] = input[index];
        }
    } else if (output > input) {
        while (count != 0) {
            count--;
            output[count] = input[count];
        }
    }
    return destination;
}

void *memset(void *destination, int value, size_t count)
{
    unsigned char *output = destination;
    for (size_t index = 0; index < count; index++) {
        output[index] = (unsigned char)value;
    }
    return destination;
}

int memcmp(const void *left, const void *right, size_t count)
{
    const unsigned char *first = left;
    const unsigned char *second = right;
    for (size_t index = 0; index < count; index++) {
        if (first[index] != second[index]) {
            return first[index] < second[index] ? -1 : 1;
        }
    }
    return 0;
}

void *memchr(const void *memory, int character, size_t count)
{
    const unsigned char *bytes = memory;
    const unsigned char target = (unsigned char)character;
    for (size_t index = 0; index < count; index++) {
        if (bytes[index] == target) {
            return (void *)(bytes + index);
        }
    }
    return NULL;
}

size_t strlen(const char *string)
{
    size_t length = 0;
    while (string[length] != '\0') {
        length++;
    }
    return length;
}

size_t strnlen(const char *string, size_t maximum)
{
    size_t length = 0;
    while (length < maximum && string[length] != '\0') {
        length++;
    }
    return length;
}

int strcmp(const char *left, const char *right)
{
    while (*left != '\0' && (unsigned char)*left == (unsigned char)*right) {
        left++;
        right++;
    }
    const unsigned char first = (unsigned char)*left;
    const unsigned char second = (unsigned char)*right;
    return first == second ? 0 : (first < second ? -1 : 1);
}

int strncmp(const char *left, const char *right, size_t count)
{
    for (size_t index = 0; index < count; index++) {
        const unsigned char first = (unsigned char)left[index];
        const unsigned char second = (unsigned char)right[index];
        if (first != second) {
            return first < second ? -1 : 1;
        }
        if (first == '\0') {
            return 0;
        }
    }
    return 0;
}

char *strcpy(char *destination, const char *source)
{
    char *result = destination;
    while ((*destination++ = *source++) != '\0') {
    }
    return result;
}

char *strncpy(char *destination, const char *source, size_t count)
{
    size_t index = 0;
    while (index < count && source[index] != '\0') {
        destination[index] = source[index];
        index++;
    }
    while (index < count) {
        destination[index++] = '\0';
    }
    return destination;
}

char *strcat(char *destination, const char *source)
{
    strcpy(destination + strlen(destination), source);
    return destination;
}

char *strchr(const char *string, int character)
{
    const unsigned char target = (unsigned char)character;
    for (;;) {
        if ((unsigned char)*string == target) {
            return (char *)string;
        }
        if (*string == '\0') {
            return NULL;
        }
        string++;
    }
}

char *strrchr(const char *string, int character)
{
    const unsigned char target = (unsigned char)character;
    const char *match = NULL;
    for (;;) {
        if ((unsigned char)*string == target) {
            match = string;
        }
        if (*string == '\0') {
            return (char *)match;
        }
        string++;
    }
}

char *strstr(const char *string, const char *substring)
{
    if (*substring == '\0') {
        return (char *)string;
    }
    for (; *string != '\0'; string++) {
        size_t index = 0;
        while (substring[index] != '\0' && string[index] == substring[index]) {
            index++;
        }
        if (substring[index] == '\0') {
            return (char *)string;
        }
    }
    return NULL;
}

static int contains_character(const char *set, unsigned char character)
{
    while (*set != '\0') {
        if ((unsigned char)*set++ == character) {
            return 1;
        }
    }
    return 0;
}

size_t strspn(const char *string, const char *accept)
{
    size_t length = 0;
    while (string[length] != '\0'
            && contains_character(accept, (unsigned char)string[length])) {
        length++;
    }
    return length;
}

size_t strcspn(const char *string, const char *reject)
{
    size_t length = 0;
    while (string[length] != '\0'
            && !contains_character(reject, (unsigned char)string[length])) {
        length++;
    }
    return length;
}

char *strpbrk(const char *string, const char *accept)
{
    while (*string != '\0') {
        if (contains_character(accept, (unsigned char)*string)) {
            return (char *)string;
        }
        string++;
    }
    return NULL;
}