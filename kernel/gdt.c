/* kernel/gdt.c — minimal 64-bit GDT (null, kernel code, kernel data) */
#include <stdint.h>
#include "gdt.h"

extern void gdt_flush(void *gdtr);

static uint64_t gdt[3];

void gdt_init(void)
{
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFF; /* kernel code: 64-bit, ring 0 */
    gdt[2] = 0x00CF92000000FFFF; /* kernel data */

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) gdtr = { sizeof(gdt) - 1, (uint64_t)gdt };

    gdt_flush(&gdtr);
}
