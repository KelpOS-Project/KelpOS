OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    /* Limine laedt Higher-Half-Kernel: virtuell = physikalisch + 0xFFFFFFFF80000000 */
    . = 0xffffffff80000000;

    .limine_requests : ALIGN(16) {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    }

    .text : ALIGN(16) {
        *(.text .text.*)
    }

    .rodata : ALIGN(16) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(16) {
        *(.data .data.*)
    }

    .bss : ALIGN(16) {
        *(COMMON)
        *(.bss .bss.*)
    }

    __kernel_end = ALIGN(4096);

    /DISCARD/ : {
        *(.eh_frame*) *(.note*) *(.comment)
    }
}
