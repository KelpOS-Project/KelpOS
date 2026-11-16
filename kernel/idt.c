/* kernel/idt.c — 256 IDT gates + dispatcher */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "panic.h"

extern void *isr_stub_table[];

struct idt_entry {
    uint16_t off_lo;
    uint16_t sel;
    uint8_t  ist;
    uint8_t  attr;
    uint16_t off_mid;
    uint32_t off_hi;
    uint32_t rsvd;
} __attribute__((packed));

static struct idt_entry idt[256];

static void idt_set_gate(int n, void *handler)
{
    uint64_t addr = (uint64_t)handler;
    idt[n].off_lo  = addr & 0xFFFF;
    idt[n].sel     = 0x08;
    idt[n].ist     = 0;
    idt[n].attr    = 0x8E; /* present, ring0, 64-bit interrupt gate */
    idt[n].off_mid = (addr >> 16) & 0xFFFF;
    idt[n].off_hi  = (addr >> 32) & 0xFFFFFFFF;
    idt[n].rsvd    = 0;
}

void idt_init(void)
{
    memset(idt, 0, sizeof(idt));
    for (int i = 0; i < 256; i++)
        idt_set_gate(i, isr_stub_table[i]);

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) idtr = { sizeof(idt) - 1, (uint64_t)idt };

    __asm__ volatile("lidt %0" :: "m"(idtr));
}

void isr_handler(struct isr_frame *f)
{
    if (f->int_no == 32) {          /* PIT */
        timer_tick();
        pic_eoi(32);
    } else if (f->int_no == 33) {   /* PS/2 keyboard */
        keyboard_isr();
        pic_eoi(33);
    } else if (f->int_no == 44) {   /* PS/2 mouse */
        mouse_isr();
        pic_eoi(44);
    } else if (f->int_no >= 32 && f->int_no <= 47) {
        pic_eoi((int)f->int_no);    /* unused IRQ */
    } else if (f->int_no < 32) {
        panicf("Unhandled exception: vector %llu, err=%llu, rip=%p",
               (unsigned long long)f->int_no,
               (unsigned long long)f->err_code, (void *)f->rip);
    }
    /* vector >= 48: spurious, ignore */
}
