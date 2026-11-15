#ifndef KELP_KEYBOARD_H
#define KELP_KEYBOARD_H

void keyboard_init(void);
void keyboard_isr(void);
int  kbd_getchar(void);

#endif
