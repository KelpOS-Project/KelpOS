#include "../klibc/include/ctype.h"
#include "../klibc/include/errno.h"
#include "../klibc/include/stdio.h"
#include "../klibc/include/stdlib.h"
#include "../klibc/include/string.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

int main(void)
{
    char memory[16] = "abcdef";
    assert(memmove(memory + 2, memory, 6) == memory + 2);
    assert(strcmp(memory, "ababcdef") == 0);
    assert(memcmp(memory + 2, "abcdef", 6) == 0);
    assert(memchr(memory, 'd', sizeof(memory)) == memory + 5);
    assert(memchr(memory, 'z', sizeof(memory)) == NULL);
    assert(strnlen(memory, 4) == 4);
    assert(strchr(memory, 'd') == memory + 5);
    const char repeated[] = "one two one";
    assert(strrchr(repeated, 'o') == repeated + 8);
    assert(strstr(memory, "cde") == memory + 4);
    assert(strspn("aaabxy", "ab") == 4);
    assert(strcspn("hello.world", ".!") == 5);
    assert(strpbrk("hello.world", ".!") == "hello.world" + 5);

    char copied[8];
    assert(strncpy(copied, "xy", sizeof(copied)) == copied);
    assert(copied[0] == 'x' && copied[1] == 'y' && copied[2] == '\0');
    assert(memset(copied, 'q', 2) == copied);
    assert(copied[0] == 'q' && copied[1] == 'q');

    char *end = NULL;
    errno = 0;
    assert(strtol("  -0x2a tail", &end, 0) == -42);
    assert(strcmp(end, " tail") == 0);
    assert(strtol(" -9223372036854775808", NULL, 10) == LONG_MIN);
    assert(strtoul("-1", NULL, 10) == ULONG_MAX);
    assert(atoi("17") == 17);
    assert(atol("-123456") == -123456L);
    assert(atoll("1234567890123") == 1234567890123LL);
    assert(strtoll("-42", NULL, 10) == -42LL);
    assert(strtoull("42", NULL, 10) == 42ULL);
    errno = 0;
    assert(strtol("999999999999999999999999999999", NULL, 10) == LONG_MAX);
    assert(errno == ERANGE);
    errno = 0;
    assert(strtol("-999999999999999999999999999999", NULL, 10) == LONG_MIN);
    assert(errno == ERANGE);
    errno = 0;
    assert(strtoul("999999999999999999999999999999", NULL, 10) == ULONG_MAX);
    assert(errno == ERANGE);
    errno = 0;
    assert(strtol("12", &end, 1) == 0);
    assert(end[0] == '1' && errno == EINVAL);

    assert(isalpha('K') && isdigit('7') && isspace('\n'));
    assert(isblank('\t') && iscntrl('\n') && isgraph('!'));
    assert(isprint(' ') && ispunct('!'));
    assert(!isalpha(0xE9) && tolower('A') == 'a' && toupper('z') == 'Z');

    char formatted[32];
    assert(snprintf(formatted, sizeof(formatted), "%s:%d:%X", "cpu", -12, 0x2A) == 10);
    assert(strcmp(formatted, "cpu:-12:2A") == 0);
    assert(snprintf(formatted, sizeof(formatted), "%lld", LLONG_MIN) == 20);
    assert(strcmp(formatted, "-9223372036854775808") == 0);
    assert(snprintf(formatted, 5, "%s", "abcdef") == 6);
    assert(strcmp(formatted, "abcd") == 0);
    assert(snprintf(NULL, 0, "%u", 123U) == 3);

    puts("klibc core tests passed");
    return 0;
}