#include "commands.h"
#include "../../drivers/vga_terminal.h"
#include "../../memory/heap.h"

void cmd_mem(const char *args)
{
    (void)args;
    terminal_write_string("Memory Heap Status:\n");
    // Hier können bei Bedarf Heap-Metadaten ausgegeben werden
}
