#include "commands.h"
#include "../../drivers/vga_terminal.h"

// Externe Deklaration der Befehlstabelle für den Help-Befehl
static const struct shell_command command_list[] = {
    SHELL_COMMANDS,
    { NULL, NULL, NULL }
};

void cmd_help(const char *args) {
    (void)args;
    terminal_write_string("Available commands:\n");
    for (size_t i = 0; command_list[i].name != NULL; i++) {
        terminal_write_string("  ");
        terminal_write_string(command_list[i].name);
        terminal_write_string(" - ");
        terminal_write_string(command_list[i].description);
        terminal_write_string("\n");
    }
}
