#include "commands.h"

#include "../../drivers/vga_terminal.h"

void cmd_fsinfo(const char *args)
{
    (void)args;
    terminal_write_string("Block devices: none\nVFS: not implemented\nExt4: not mounted or supported yet\n");
}