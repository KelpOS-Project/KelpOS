#include "interrupts.h"

#include <stddef.h>
#include <stdint.h>

#include "panic.h"

struct idt_gate {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_register {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_gate idt[32] __attribute__((aligned(16)));
extern void *exception_stub_table[32];

void interrupts_initialize(void)
{
    for (size_t vector = 0; vector < 32; vector++) {
        const uint64_t address = (uint64_t)(uintptr_t)exception_stub_table[vector];
        idt[vector].offset_low = (uint16_t)address;
        idt[vector].selector = 0x08;
        idt[vector].ist = 0;
        idt[vector].attributes = 0x8E;
        idt[vector].offset_middle = (uint16_t)(address >> 16);
        idt[vector].offset_high = (uint32_t)(address >> 32);
        idt[vector].reserved = 0;
    }

    const struct idt_register register_value = {
        .limit = sizeof(idt) - 1,
        .base = (uint64_t)(uintptr_t)idt,
    };
    __asm__ volatile ("lidt %0" : : "m"(register_value));
}

_Noreturn void kernel_exception_dispatch(uint64_t vector, uint64_t error_code)
{
    static const char *const names[32] = {
        "Divide error", "Debug", "Non-maskable interrupt", "Breakpoint",
        "Overflow", "Bound range exceeded", "Invalid opcode", "Device not available",
        "Double fault", "Coprocessor segment overrun", "Invalid TSS", "Segment not present",
        "Stack-segment fault", "General protection fault", "Page fault", "Reserved exception",
        "x87 floating-point exception", "Alignment check", "Machine check", "SIMD floating-point exception",
        "Virtualization exception", "Control protection exception", "Reserved exception", "Reserved exception",
        "Reserved exception", "Reserved exception", "Reserved exception", "Reserved exception",
        "Hypervisor injection exception", "VMM communication exception", "Security exception", "Reserved exception",
    };
    kernel_panic_exception(vector < 32 ? names[vector] : "Unknown CPU exception",
        vector, error_code);
}