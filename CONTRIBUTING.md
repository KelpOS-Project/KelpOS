# Contributing to KelpOS

Thank you for considering contributing to KelpOS. Whether you are fixing memory allocators, adding x86_64 CPU drivers, optimizing the graphics/desktop system, or improving the shell, your contributions are welcome.

KelpOS is a modular, freestanding C11 x86_64 kernel designed with minimalist, clean architecture principles in mind.

---

## Project Architecture Overview

Before making changes, it helps to understand how the codebase is structured:

```text
KelpOS/
├── boot/           # Multiboot header and early x86_64 bootstrap (boot.s)
├── kernel/         # Kernel entry point (kernel.c), linker script, shell, panic handling
├── memory/         # Kernel heap & physical/virtual memory allocators (heap.c)
├── drivers/        # Bare-metal I/O drivers (VGA terminal, Serial COM1, Keyboard, x86 I/O)
├── graphics/       # Framebuffer graphics pipeline, bitmap fonts, & optional KelpDE interface
├── base/           # Minimal freestanding standard library functions (string.c, stddef, etc.)
└── tests/          # Host-compiled standalone unit & module tests
```

---

## Getting Started & Building

### Prerequisites

To build and run KelpOS, you need standard x86_64 development tools:

- gcc or x86_64-elf-gcc (C11 support)
- as & ld (GNU Assembler and Linker)
- grub-mkrescue & grub-file (for ISO creation)
- qemu-system-x86_64 (for running the bootable image)
- python3 (for host-side validation tests)

### Build Commands

```bash
# Clean previous build artifacts
make clean

# Compile kernel & build ISO image
make iso

# Run full host-side test suite & build ISO
make verify

# Launch KelpOS inside QEMU
make run

# Full clean build + test execution + QEMU launch
make compile
```

---

## Key Areas Open for Contributions

Here are some of the active areas where you can patch, extend, or rewrite modules:

### 1. Memory Management (memory/)
- Paging & Virtual Memory: Implementing dynamic PML4/PDPT paging tables and MMU mapping beyond the identity bootstrap tables.
- Kernel Heap Allocation: Enhancing the current bootstrap heap (heap.c) into a full Buddy Allocator or Slab/Sbrk allocator.

### 2. Graphics & UI (graphics/)
- KelpDE Features: Expanding graphics/desktop.c with window management, dynamic cursor drawing, and double-buffering / V-Sync to prevent tearing.
- Font System: Adding variable-width fonts or anti-aliasing to graphics/font.c.

### 3. Drivers & Hardware Interfacing (drivers/)
- Input Drivers: Extending PS/2 Keyboard handling or adding PS/2 Mouse packet decoding.
- Storage & Disk: Implementing AHCI/IDE disk controllers or RAM-disk support.
- Timer Interrupts: Setting up APIC / PIT timer interrupts for kernel scheduling.

### 4. User Space & System Calls
- Setting up GDT/IDT interrupt service routines (ISRs) for Syscalls.
- Ring 3 context switching and multitasking support.

---

## Testing Guidelines

KelpOS enforces host-side C unit testing to prevent boot regressions. Any new feature should include test coverage where applicable.

- Core tests are located in tests/ (e.g., kernel_heap_test.c, graphics_framebuffer_test.c).
- When writing tests for the graphics engine or kernel modules, ensure pointer validation (e.g., checking if framebuffer != NULL and framebuffer->address != NULL) is included so tests can run headless without segfaults.

Run tests locally using:
```bash
make kernel-test
make graphics-test
```

---

## Coding Standards & Rules

1. Freestanding C11: Avoid host <stdlib.h> or system-dependent headers in kernel code. Use freestanding standard headers (<stdint.h>, <stdbool.h>, <stddef.h>).
2. Explicit Compilation: Do not #include .c files. Keep headers (.h) strictly focused on interface declarations and implement logic in .c files.
3. No Red Zones / No Stack Protectors: Kernel code must adhere to -ffreestanding -mno-red-zone -fno-stack-protector.
4. Clean Commits: Make sure make clean && make verify succeeds without warnings or double-definition linker errors before submitting a pull request.

---

## Submitting a Patch / PR

1. Fork the repository and create a feature branch (git checkout -b feature/paging-impl).
2. Commit your changes with clear, descriptive commit messages.
3. Verify that your branch compiles cleanly and passes all test suites (make verify).
4. Open a Pull Request against the main branch with a summary of what your patch fixes or adds.

Happy hacking on KelpOS!
