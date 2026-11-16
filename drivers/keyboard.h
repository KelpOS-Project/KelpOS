#ifndef KELP_KEYBOARD_H
#define KELP_KEYBOARD_H

/* special keys returned by kbd_getchar() (outside printable ASCII) */
#define KBD_UP    0x11
#define KBD_DOWN  0x12
#define KBD_LEFT  0x13
#define KBD_RIGHT 0x14
#define KBD_HOME  0x15
#define KBD_END   0x16
#define KBD_ESC   0x1b

void keyboard_init(void);
void keyboard_isr(void);
int  kbd_getchar(void);
int  kbd_poll(void); /* -1 when the queue is empty (non-blocking) */

#endif
