#include "../kernel/bin/commands.h"
#include "../kernel/panic.h"
#include "../memory/heap.h"
#include "../drivers/e1000.h"
#include "../drivers/keyboard.h"
#include "../drivers/x86_io.h"
#include "../graphics/desktop.h"

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static char output[2048];
static size_t output_length;
static jmp_buf panic_jump;
static char panic_message[64];
static enum keyboard_layout mock_keyboard_layout = KEYBOARD_LAYOUT_ENGLISH;

static void append_char(char character)
{
    assert(output_length + 1 < sizeof(output));
    output[output_length++] = character;
    output[output_length] = '\0';
}

void terminal_initialize(void) {}
void terminal_set_color(uint8_t color) { (void)color; }
void terminal_clear(void) { output_length = 0; output[0] = '\0'; }

void terminal_write(const char *data, size_t size)
{
    for (size_t index = 0; index < size; index++) {
        append_char(data[index]);
    }
}

void terminal_write_string(const char *string)
{
    while (*string != '\0') {
        append_char(*string++);
    }
}

bool desktop_available(void) { return false; }
bool desktop_start(void) { return false; }
bool e1000_is_initialized(void) { return true; }
uint8_t x86_port_read8(uint16_t port) { (void)port; return 0; }
void x86_port_write8(uint16_t port, uint8_t value) { (void)port; (void)value; }
bool keyboard_set_layout(enum keyboard_layout layout)
{
    mock_keyboard_layout = layout;
    return true;
}
enum keyboard_layout keyboard_get_layout(void) { return mock_keyboard_layout; }

_Noreturn void kernel_panic(const char *reason)
{
    size_t index = 0;
    while (reason[index] != '\0' && index + 1 < sizeof(panic_message)) {
        panic_message[index] = reason[index];
        index++;
    }
    panic_message[index] = '\0';
    longjmp(panic_jump, 1);
}

_Noreturn void kernel_panic_exception(const char *reason, uint64_t vector,
    uint64_t error_code)
{
    (void)reason;
    (void)vector;
    (void)error_code;
    longjmp(panic_jump, 1);
}

int main(void)
{
    kernel_heap_initialize();

    cmd_mem(NULL);
    assert(strstr(output, "Bootstrap heap capacity: 65536 bytes") != NULL);
    assert(strstr(output, "Free heap payload: ") != NULL);
    assert(strstr(output, "Allocated or allocator metadata: ") != NULL);

    terminal_clear();
    cmd_help(NULL);
    assert(strstr(output, "cpuinfo - Shows CPU vendor and CPUID information") != NULL);
    assert(strstr(output, "free - Shows available bootstrap heap memory") != NULL);
    assert(strstr(output, "syscrash - Triggers a kernel panic for testing") != NULL);
    assert(strstr(output, "keymap - Show or change keyboard layout") != NULL);
    static const char *const added_commands[] = {
        "version", "hostname", "arch", "sysinfo", "netinfo",
        "fsinfo", "whoami", "uptime", "halt", "reboot", "true", "false",
        "calc", "seq",
    };
    for (size_t index = 0; index < sizeof(added_commands) / sizeof(added_commands[0]); index++) {
        assert(strstr(output, added_commands[index]) != NULL);
    }

    if (setjmp(panic_jump) == 0) {
        cmd_syscrash(NULL);
        assert(0 && "syscrash returned without panicking");
    }
    assert(strcmp(panic_message, "Triggered System Crash") == 0);

    terminal_clear();
    cmd_keymap("de");
    assert(mock_keyboard_layout == KEYBOARD_LAYOUT_GERMAN);
    assert(strstr(output, "German (QWERTZ)") != NULL);
    terminal_clear();
    cmd_keymap("fr");
    assert(mock_keyboard_layout == KEYBOARD_LAYOUT_FRENCH);
    terminal_clear();
    cmd_keymap("xx");
    assert(strstr(output, "Usage: keymap en|de|fr") != NULL);

    terminal_clear();
    cmd_calc("7 * -8");
    assert(strcmp(output, "-56\n") == 0);

    terminal_clear();
    cmd_calc("2 / 0");
    assert(strstr(output, "division by zero") != NULL);

    terminal_clear();
    cmd_calc("-2147483648 / -1");
    assert(strstr(output, "result out of range") != NULL);

    terminal_clear();
    cmd_seq("3 5");
    assert(strcmp(output, "3\n4\n5\n") == 0);

    terminal_clear();
    cmd_seq("1 100");
    assert(strstr(output, "range exceeds 64 values") != NULL);

    terminal_clear();
    cmd_fsinfo(NULL);
    assert(strstr(output, "VFS: not implemented") != NULL);

    terminal_clear();
    cmd_netinfo(NULL);
    assert(strstr(output, "Intel E1000 initialized") != NULL);

    terminal_clear();
    cmd_free(NULL);
    assert(strstr(output, "Free heap payload: ") != NULL);

    terminal_clear();
    cmd_cpuinfo(NULL);
    assert(strstr(output, "CPU vendor: ") != NULL);
    assert(strstr(output, "Maximum basic CPUID leaf: ") != NULL);

    terminal_clear();
    cmd_fastfetch(NULL);
    assert(strstr(output, "KelpOS development") != NULL);
    assert(strstr(output, "Architecture: x86_64") != NULL);
    assert(strstr(output, "E1000 raw-frame driver") != NULL);
    assert(strstr(output, "Free heap payload: ") != NULL);

    terminal_clear();
    cmd_uname(NULL);
    assert(strcmp(output, "KelpOS kelpos development x86_64 freestanding Multiboot-v1\n") == 0);

    terminal_clear();
    cmd_about(NULL);
    assert(strstr(output, "Filesystem: not implemented") != NULL);
    assert(strstr(output, "KPM: host-side Python prototype") != NULL);
    assert(strstr(output, "CISWN: host-side C prototype") != NULL);

    puts("Kernel shell command tests passed");
    return 0;
}
