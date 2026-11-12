.section .text
.code64
.extern kernel_exception_dispatch

.macro ISR_NO_ERROR number
.global exception_stub_\number
exception_stub_\number:
    pushq $0
    pushq $\number
    jmp exception_common
.endm

.macro ISR_ERROR number
.global exception_stub_\number
exception_stub_\number:
    pushq $\number
    jmp exception_common
.endm

ISR_NO_ERROR 0
ISR_NO_ERROR 1
ISR_NO_ERROR 2
ISR_NO_ERROR 3
ISR_NO_ERROR 4
ISR_NO_ERROR 5
ISR_NO_ERROR 6
ISR_NO_ERROR 7
ISR_ERROR 8
ISR_NO_ERROR 9
ISR_ERROR 10
ISR_ERROR 11
ISR_ERROR 12
ISR_ERROR 13
ISR_ERROR 14
ISR_NO_ERROR 15
ISR_NO_ERROR 16
ISR_ERROR 17
ISR_NO_ERROR 18
ISR_NO_ERROR 19
ISR_NO_ERROR 20
ISR_ERROR 21
ISR_NO_ERROR 22
ISR_NO_ERROR 23
ISR_NO_ERROR 24
ISR_NO_ERROR 25
ISR_NO_ERROR 26
ISR_NO_ERROR 27
ISR_NO_ERROR 28
ISR_ERROR 29
ISR_ERROR 30
ISR_NO_ERROR 31

exception_common:
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq 120(%rsp), %rdi
    movq 128(%rsp), %rsi
    andq $-16, %rsp
    call kernel_exception_dispatch
    ud2

.section .rodata
.align 8
.global exception_stub_table
exception_stub_table:
.quad exception_stub_0, exception_stub_1, exception_stub_2, exception_stub_3
.quad exception_stub_4, exception_stub_5, exception_stub_6, exception_stub_7
.quad exception_stub_8, exception_stub_9, exception_stub_10, exception_stub_11
.quad exception_stub_12, exception_stub_13, exception_stub_14, exception_stub_15
.quad exception_stub_16, exception_stub_17, exception_stub_18, exception_stub_19
.quad exception_stub_20, exception_stub_21, exception_stub_22, exception_stub_23
.quad exception_stub_24, exception_stub_25, exception_stub_26, exception_stub_27
.quad exception_stub_28, exception_stub_29, exception_stub_30, exception_stub_31

.section .note.GNU-stack,"",@progbits
