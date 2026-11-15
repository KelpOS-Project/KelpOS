/* drivers/serial.c — 16550 UART, COM1 */
#include <stdbool.h>
#include <stdint.h>
#include "io.h"
#include "serial.h"

#define COM1 0x3F8

void serial_init(void)
{
    outb(COM1 + 1, 0x00);   /* interrupts off */
    outb(COM1 + 3, 0x80);   /* DLAB on */
    outb(COM1 + 0, 0x03);   /* 38400 baud (divisor 3) */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   /* 8N1, DLAB off */
    outb(COM1 + 2, 0xC7);   /* enable + clear FIFOs */
    outb(COM1 + 4, 0x0B);   /* RTS/DSR */
}

void serial_putc(char c)
{
    while (!(inb(COM1 + 5) & 0x20))
        ;
    outb(COM1, (uint8_t)c);
}

char serial_recv(void)
{
    while (!(inb(COM1 + 5) & 0x01))
        ;
    return (char)inb(COM1);
}

int serial_has(void)
{
    return (inb(COM1 + 5) & 0x01) != 0;
}
