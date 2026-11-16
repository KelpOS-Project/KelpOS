#ifndef KELP_MOUSE_H
#define KELP_MOUSE_H

/* PS/2 mouse (IRQ12, aux channel of the 8042 controller).
 * Stream mode, 3-byte packets, position in pixels. */

void mouse_init(int screen_w, int screen_h);
void mouse_isr(void);

int  mouse_x(void);        /* pixel position */
int  mouse_y(void);
int  mouse_buttons(void);  /* bit0 left, bit1 right, bit2 middle */
int  mouse_left_click(void); /* 1 once per left-button press (edge) */
int  mouse_moved(void);      /* 1 if position/buttons changed since last poll */

#endif
