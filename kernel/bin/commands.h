#ifndef KELPOS_KERNEL_BIN_COMMANDS_H
#define KELPOS_KERNEL_BIN_COMMANDS_H

#include <stddef.h>

struct shell_command {
    const char *name;
    const char *description;
    void (*execute)(const char *args);
};

// Deklarationen der Befehle aus kernel/bin/
void cmd_help(const char *args);
void cmd_about(const char *args);
void cmd_clear(const char *args);
void cmd_echo(const char *args);
void cmd_mem(const char *args);

#define SHELL_COMMANDS \
    { "help", "Lists available commands", cmd_help }, \
    { "about", "Displays OS version", cmd_about }, \
    { "clear", "Clears the screen", cmd_clear }, \
    { "echo", "Prints text", cmd_echo }, \
    { "mem", "Shows heap usage", cmd_mem }

#endif
