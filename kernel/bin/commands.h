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
void cmd_fastfetch(const char *args);
void cmd_uname(const char *args);
void cmd_cpuinfo(const char *args);
void cmd_clear(const char *args);
void cmd_echo(const char *args);
void cmd_mem(const char *args);
void cmd_free(const char *args);
void cmd_syscrash(const char *args);
void cmd_version(const char *args);
void cmd_hostname(const char *args);
void cmd_arch(const char *args);
void cmd_sysinfo(const char *args);
void cmd_netinfo(const char *args);
void cmd_fsinfo(const char *args);
void cmd_whoami(const char *args);
void cmd_uptime(const char *args);
void cmd_halt(const char *args);
void cmd_reboot(const char *args);
void cmd_true(const char *args);
void cmd_false(const char *args);
void cmd_calc(const char *args);
void cmd_seq(const char *args);
void cmd_keymap(const char *args);

#define SHELL_COMMANDS \
    { "help", "Lists available commands", cmd_help }, \
    { "fastfetch", "Shows kernel and system information", cmd_fastfetch }, \
    { "uname", "Prints the kernel and architecture", cmd_uname }, \
    { "cpuinfo", "Shows CPU vendor and CPUID information", cmd_cpuinfo }, \
    { "about", "Displays OS version", cmd_about }, \
    { "clear", "Clears the screen", cmd_clear }, \
    { "echo", "Prints text", cmd_echo }, \
    { "mem", "Shows bootstrap heap usage", cmd_mem }, \
    { "free", "Shows available bootstrap heap memory", cmd_free }, \
    { "syscrash", "Triggers a kernel panic for testing", cmd_syscrash }, \
    { "version", "Print the KelpOS version", cmd_version }, \
    { "hostname", "Print the system hostname", cmd_hostname }, \
    { "arch", "Print the kernel architecture", cmd_arch }, \
    { "sysinfo", "Show current system information", cmd_sysinfo }, \
    { "netinfo", "Show network driver capabilities", cmd_netinfo }, \
    { "fsinfo", "Show filesystem support status", cmd_fsinfo }, \
    { "whoami", "Show the current execution context", cmd_whoami }, \
    { "uptime", "Show timer and uptime availability", cmd_uptime }, \
    { "halt", "Halt the processor", cmd_halt }, \
    { "reboot", "Request a hardware reboot", cmd_reboot }, \
    { "true", "Print true", cmd_true }, \
    { "false", "Print false", cmd_false }, \
    { "calc", "Calculate a bounded integer expression", cmd_calc }, \
    { "seq", "Print a bounded integer sequence", cmd_seq }, \
    { "keymap", "Show or change keyboard layout (en/de/fr)", cmd_keymap }

#endif
