/* tests/host/stub_console.c — host stub for klibc tests */
#include <unistd.h>

void kputc(int c)
{
    char ch = (char)c;
    (void)write(1, &ch, 1);
}
