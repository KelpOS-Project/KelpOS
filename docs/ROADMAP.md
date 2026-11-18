# Roadmap

## M1 — Base kernel + shell ✅
- [x] Limine boot, higher-half kernel (0xffffffff80000000)
- [x] Serial console + framebuffer splash, framebuffer text terminal (8x16 font)
- [x] GDT, IDT, exceptions, PIC remap
- [x] PIT timer (100 Hz) + uptime
- [x] PS/2 keyboard, shell with builtins, packages (pkg command)
- [x] Bump heap, klibc, host tests, QEMU smoke test

## M2 — Users, files & desktop ✅ (v0.3.0)
- [x] RAM-fs (ramfs): dirs/files, shell cwd, /etc/passwd & /etc/group
- [x] Users & groups: adduser, groupmod, whoami, id, doas/sudo (password gate)
- [x] Unix file commands: ls, pwd, cd, mkdir, tree, cat
- [x] fs/ stack: block devices + ramdisk, VFS mount layer,
      FAT12/16/32 + exFAT (read-only), ext4/btrfs superblock probe skeletons
- [x] Real-disk demo volume (boot module disk.img) mounted at /media/disk0
- [x] Graphical desktop: Terminal (into the shell), Files (ramfs browser),
      Browser (honest placeholder until the network stack), About, Reboot/Shutdown
- [x] Boot menu: wallpaper, interface branding, KelpOS + KelpOS (Silent Boot)

## M2.5 — Real desktop & installer ✅ (current, v0.4.0)
- [x] PS/2 mouse driver (IRQ12, stream mode, polled API)
- [x] Mouse-driven window manager: overlapping windows, title bars,
      drag & drop focus, close box, start menu, taskbar with RTC clock
- [x] Desktop wallpaper: the boot wallpaper.bmp shipped as a boot
      module and drawn on the desktop; dirty-region redraws (no
      flicker from cursor movement)
- [x] kpm package manager: install/uninstall, /etc/kpm/installed DB
- [x] More Unix commands: rm, touch, wc, head, tail, grep, uname, date (RTC), env
- [x] kelpinstall in C: real install by default, ANSI TUI,
      device + package selection, sfdisk/mkfs.vfat/limine bios-install
- [x] Remade boot menu wallpaper (v0.4.0 composition)
- [x] TinyGL-compatible OpenGL 1.x subset (matrices, immediate mode,
      z-buffered Gouraud/flat triangles, SSE-enabled renderer) and the
      GLGears desktop app (320x240 window, animated at ~25 fps)

## M3 — Memory management
- [ ] PMM (free-list/bitmap instead of bump)
- [ ] VMM (own page tables, vm_map/vm_free)
- [ ] kfree / real malloc
- [ ] Higher-half virtualization via own page tables

## M4 — Filesystem deepening
- [ ] ext4: block groups, extents, real file access
- [ ] btrfs: chunk trees + B-tree reader
- [ ] FAT writes (create/delete)
- [ ] Real block drivers (AHCI/NVMe or virtio-blk)

## M5 — Userspace
- [ ] Syscalls (syscall/sysret), scheduler (RR)
- [ ] Load ELFs, first userspace process
- [ ] TTY as an abstraction

## M6 — Graphics & more
- [ ] TinyGL extensions: textures, lighting, display lists, alpha blending
- [ ] Network stack (TCP/IP) — then the Browser app goes live
- [ ] Grow the Rust share (drivers in Rust)

## Later
- SMP (APIC, IPIs)
- Real package repository + userland collection
