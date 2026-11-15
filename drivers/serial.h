#ifndef KELP_SERIAL_H
#define KELP_SERIAL_H

void serial_init(void);
void serial_putc(char c);
char serial_recv(void);
int  serial_has(void);

#endif
