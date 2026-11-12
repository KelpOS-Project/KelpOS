#include "commands.h"

#include "../../drivers/keyboard.h"
#include "../../drivers/vga_terminal.h"

static bool text_equals(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

static const char *layout_name(enum keyboard_layout layout)
{
    switch (layout) {
    case KEYBOARD_LAYOUT_GERMAN: return "German (QWERTZ)";
    case KEYBOARD_LAYOUT_FRENCH: return "French (AZERTY)";
    default: return "English (QWERTY)";
    }
}

void cmd_keymap(const char *args)
{
    if (args == NULL || *args == '\0') {
        terminal_write_string("Keyboard layout: ");
        terminal_write_string(layout_name(keyboard_get_layout()));
        terminal_write_string("\nUsage: keymap en|de|fr\n");
        return;
    }

    enum keyboard_layout layout;
    if (text_equals(args, "en") || text_equals(args, "english")) {
        layout = KEYBOARD_LAYOUT_ENGLISH;
    } else if (text_equals(args, "de") || text_equals(args, "german")) {
        layout = KEYBOARD_LAYOUT_GERMAN;
    } else if (text_equals(args, "fr") || text_equals(args, "french")) {
        layout = KEYBOARD_LAYOUT_FRENCH;
    } else {
        terminal_write_string("Usage: keymap en|de|fr\n");
        return;
    }

    if (keyboard_set_layout(layout)) {
        terminal_write_string("Keyboard layout set to ");
        terminal_write_string(layout_name(layout));
        terminal_write_string("\n");
    }
}