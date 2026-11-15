#ifndef KELP_IDT_H
#define KELP_IDT_H

#include <stdint.h>

/* register frame as built by boot.S (ascending addresses) */
struct isr_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

void idt_init(void);

#endif
