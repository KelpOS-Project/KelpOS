/* kernel/pic.c — remap the 8259 PIC (IRQ0->32 ... IRQ15->47) */
#include <stdint.h>
#include "io.h"
#include "pic.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

void pic_init(void)
{
    outb(PIC1_CMD, 0x11); io_wait();   /* ICW1: init, cascade */
    outb(PIC2_CMD, 0x11); io_wait();
    outb(PIC1_DATA, 0x20); io_wait();  /* ICW2: offset 32 */
    outb(PIC2_DATA, 0x28); io_wait();  /* ICW2: offset 40 */
    outb(PIC1_DATA, 0x04); io_wait();  /* ICW3: IRQ2 is the slave link */
    outb(PIC2_DATA, 0x02); io_wait();
    outb(PIC1_DATA, 0x01); io_wait();  /* ICW4: 8086 mode */
    outb(PIC2_DATA, 0x01); io_wait();
    outb(PIC1_DATA, 0xF8);             /* mask all but IRQ0/1 and IRQ2 (cascade for the slave) */
    outb(PIC2_DATA, 0xEF);             /* slave: only IRQ12 (mouse) unmasked */
}

void pic_eoi(int vector)
{
    if (vector >= 40)
        outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}
