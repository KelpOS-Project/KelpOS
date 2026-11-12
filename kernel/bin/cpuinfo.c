#include "commands.h"

#include <stdint.h>

#include "../../drivers/vga_terminal.h"

static void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx,
    uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile ("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(0));
}

static void write_u32(uint32_t value)
{
    char digits[10];
    size_t count = 0;
    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (count != 0) {
        terminal_write(&digits[--count], 1);
    }
}

void cmd_cpuinfo(const char *args)
{
    (void)args;
    uint32_t max_leaf;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    cpuid(0, &max_leaf, &ebx, &ecx, &edx);

    const char vendor[] = {
        (char)ebx, (char)(ebx >> 8), (char)(ebx >> 16), (char)(ebx >> 24),
        (char)edx, (char)(edx >> 8), (char)(edx >> 16), (char)(edx >> 24),
        (char)ecx, (char)(ecx >> 8), (char)(ecx >> 16), (char)(ecx >> 24),
    };
    terminal_write_string("CPU vendor: ");
    terminal_write(vendor, sizeof(vendor));
    terminal_write_string("\nMaximum basic CPUID leaf: ");
    write_u32(max_leaf);
    terminal_write_string("\n");
}