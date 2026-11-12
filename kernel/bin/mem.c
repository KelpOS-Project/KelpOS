#include "commands.h"
#include "../../drivers/vga_terminal.h"
#include "../../memory/heap.h"

static void write_size(size_t value)
{
    char digits[sizeof(size_t) * 3 + 1];
    size_t count = 0;
    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);

    while (count != 0) {
        terminal_write(&digits[--count], 1);
    }
}

void cmd_mem(const char *args)
{
    (void)args;
    const size_t available = kernel_heap_available();
    terminal_write_string("Bootstrap heap capacity: ");
    write_size(KERNEL_HEAP_CAPACITY);
    terminal_write_string(" bytes\nFree heap payload: ");
    write_size(available);
    terminal_write_string(" bytes\nAllocated or allocator metadata: ");
    write_size(KERNEL_HEAP_CAPACITY - available);
    terminal_write_string(" bytes\n");
}
