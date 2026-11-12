#ifndef KELPOS_DRIVERS_KEYBOARD_H
#define KELPOS_DRIVERS_KEYBOARD_H

#include <stdbool.h>

enum keyboard_layout {
	KEYBOARD_LAYOUT_ENGLISH,
	KEYBOARD_LAYOUT_GERMAN,
	KEYBOARD_LAYOUT_FRENCH,
};

bool keyboard_read_char(char *character);
bool keyboard_set_layout(enum keyboard_layout layout);
enum keyboard_layout keyboard_get_layout(void);

#endif