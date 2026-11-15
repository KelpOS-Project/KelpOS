# Roadmap

## M1 — Base kernel + shell ✅ (current)
- [x] Limine boot, higher-half kernel (0xffffffff80000000)
- [x] Serial console + framebuffer splash, framebuffer text terminal (8x16 font)
- [x] GDT, IDT, exceptions, PIC remap
- [x] PIT timer (100 Hz) + uptime
- [x] PS/2 keyboard, shell with builtins, packages (pkg command)
- [x] Bump heap, klibc, host tests, QEMU smoke test

## M2 — Memory management
- [ ] PMM (free-list/bitmap instead of bump)
- [ ] VMM (own page tables, vm_map/vm_free)
- [ ] kfree / real malloc
- [ ] Higher-half virtualization via own page tables

## M3 — Userspace
- [ ] Syscalls (syscall/sysret), scheduler (RR)
- [ ] Load ELFs, first userspace process
- [ ] TTY as an abstraction

## M4 — Filesystem & I/O
- [ ] VFS layer
- [ ] RAM-fs, then FAT (IDE/AHCI)
- [ ] Keyboard via /dev/kbd, pipes

## M5 — Graphics & more
- [ ] Framebuffer terminal with font renderer
- [ ] Window manager basics (like the earlier squarentk)
- [ ] Grow the Rust share (drivers in Rust)

## Later
- SMP (APIC, IPIs)
- Network stack
- Package manager + userland collection
