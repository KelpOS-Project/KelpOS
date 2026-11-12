#include "panic.h"

#include <stdbool.h>

#include "../drivers/vga_terminal.h"

static void write_hex(uint64_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    for (int shift = 60; shift >= 0; shift -= 4) {
        const char digit = digits[(value >> shift) & 0x0F];
        terminal_write(&digit, 1);
    }
}

static _Noreturn void panic_display(const char *reason, bool has_exception,
    uint64_t vector, uint64_t error_code)
{
    terminal_initialize();
    terminal_set_color(0x4F);
    terminal_write_string("KERNEL PANIC\n");
    terminal_set_color(0x0F);
    terminal_write_string(reason != NULL ? reason : "No panic reason supplied.");
    if (has_exception) {
        terminal_write_string("\nException vector: 0x");
        write_hex(vector);
        terminal_write_string("  error code: 0x");
        write_hex(error_code);
    }
    terminal_write_string("\nThe system has halted. CPU Stopped Processing, Reboot the Device now\n");

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

_Noreturn void kernel_panic(const char *reason)
{
    panic_display(reason, false, 0, 0);
}

_Noreturn void kernel_panic_exception(const char *reason, uint64_t vector,
    uint64_t error_code)
{
    panic_display(reason, true, vector, error_code);
}
