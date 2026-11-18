# ─────────────────────────────────────────────────────────────
#  KelpOS — FOSS Unix-like x86_64 kernel (Limine)
# ─────────────────────────────────────────────────────────────

VERSION := 0.4.0
BUILD   := build
OBJDIR  := $(BUILD)/obj

# Tools
HOSTCC  ?= cc
NASM    ?= nasm
QEMU    ?= qemu-system-x86_64
XORRISO ?= xorriso

# Cross toolchain: x86_64-elf- > x86_64-linux-gnu- > host-gcc
CROSS := x86_64-elf-
ifeq ($(shell command -v $(CROSS)gcc 2>/dev/null),)
  CROSS := x86_64-linux-gnu-
endif
ifeq ($(shell command -v $(CROSS)gcc 2>/dev/null),)
  CROSS :=
endif
CC := $(CROSS)gcc

# Optional packages compiled into the image (see scripts/pkgconf.py):
#   PKGS=all                    everything (default)
#   PKGS="crc mem"              only these
#   PKGS="all -fortune"         everything except fortune
# The `pkg` command can enable/disable packages at run time.
PKGS ?= all

KCFLAGS_BASE := -std=c17 -ffreestanding -fno-builtin -fno-stack-protector \
           -fno-pic -fno-pie -mno-red-zone \
           -mcmodel=kernel -O2 -g -Wall -Wextra \
           -Iinclude -Iklibc/include -Ikernel -Imemory -Idrivers -Ifs -I$(BUILD)

# The kernel proper stays -mno-sse: interrupt handlers never touch XMM.
KCFLAGS := $(KCFLAGS_BASE) -mno-mmx -mno-sse -mno-sse2

GFXINCLUDES := -Ikernel/gfx -Ikernel/gfx/tinygl/include -Ikernel/gfx/tinygl/src \
               -Ikernel/gfx/compat -Ikernel/gfx/stb -Ikernel/gfx/apps

# The TinyGL renderer is the one SSE-enabled part (x86_64 can only
# return floats in XMM registers). The FPU/SSE is enabled in kmain().
# The vendored TinyGL/GLU code is not clean under -Wextra, so its
# harmless warnings are suppressed here instead of touching upstream.
GFXFLAGS := $(KCFLAGS_BASE) $(GFXINCLUDES) -msse2 -mfpmath=sse -mno-avx \
            -Wno-unused-parameter -Wno-sign-compare \
            -Wno-implicit-fallthrough

KSRC := $(wildcard kernel/*.c) $(wildcard kernel/bin/*.c) \
        $(wildcard drivers/*.c) $(wildcard memory/*.c) $(wildcard fs/*.c) \
        $(wildcard klibc/src/*.c)
GFXSRC := $(wildcard kernel/gfx/*.c) \
          $(wildcard kernel/gfx/compat/*.c) \
          $(wildcard kernel/gfx/tinygl/src/*.c) \
          $(wildcard kernel/gfx/apps/*.c) \
          $(wildcard kernel/gfx/stb/*.c)
KOBJ := $(OBJDIR)/kernel/boot.o $(patsubst %.c,$(OBJDIR)/%.o,$(KSRC)) \
        $(patsubst %.c,$(OBJDIR)/%.o,$(GFXSRC))

# Optional Rust component (linked if built: make rust)
RUSTLIB  := rust/target/x86_64-unknown-none/release/libkrust.a
LINKLIBS :=
ifneq ($(wildcard $(RUSTLIB)),)
  LINKLIBS := $(RUSTLIB)
endif

.PHONY: all kernel kelpinstall install uninstall iso limine font rust run run-debug test test-qemu clean help FORCE

all: iso

kernel: $(BUILD)/kelpos.elf

kelpinstall: $(BUILD)/kelpinstall

# kelpinstall is a real command: it is NOT meant to be run from build/
# by accident. Install it explicitly (PREFIX ?= /usr/local) and run it
# as `kelpinstall` afterwards.
PREFIX ?= /usr/local

install: kelpinstall
	install -D -m 0755 $(BUILD)/kelpinstall $(DESTDIR)$(PREFIX)/bin/kelpinstall

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/kelpinstall

$(BUILD)/kelpinstall: scripts/kelpinstall.c
	$(HOSTCC) -Wall -Wextra -o $@ $^

$(OBJDIR)/%.o: %.c $(BUILD)/pkgconf.h
	@mkdir -p $(dir $@)
	$(CC) $(KCFLAGS) -c $< -o $@

# SSE-enabled graphics objects (more specific pattern wins over the
# generic %.o rule above)
$(OBJDIR)/kernel/gfx/%.o: kernel/gfx/%.c $(BUILD)/pkgconf.h
	@mkdir -p $(dir $@)
	$(CC) $(GFXFLAGS) -c $< -o $@

$(OBJDIR)/kernel/boot.o: kernel/boot.S
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

# Regenerate build/pkgconf.h whenever PKGS changes (content compared, so
# object files are only rebuilt when the selection actually changes).
$(BUILD)/pkgconf.h: FORCE
	@mkdir -p $(BUILD)
	@python3 scripts/pkgconf.py '$(PKGS)' > $@.tmp
	@cmp -s $@.tmp $@ || mv $@.tmp $@
	@rm -f $@.tmp
FORCE:

# include/limine.h is fetched first if missing (scripts/setup-limine.sh)
$(BUILD)/kelpos.elf: include/limine.h $(KOBJ) kernel/linker.ld
	$(CC) -T kernel/linker.ld -nostdlib -Wl,--build-id=none \
		-o $@ $(KOBJ) $(LINKLIBS) -lgcc

limine include/limine.h:
	bash scripts/setup-limine.sh

$(BUILD)/limine/limine:
	bash scripts/setup-limine.sh

# Regenerate the embedded font (kernel/font.c is committed; this is only
# needed when porting to a different font)
font:
	python3 scripts/genfont.py > kernel/font.c

iso: $(BUILD)/kelpos.iso

$(BUILD)/kelpos.iso: $(BUILD)/kelpos.elf boot/limine.conf boot/wallpaper.bmp \
		boot/silent.flag boot/disk.img $(BUILD)/limine/limine
	mkdir -p $(BUILD)/iso/boot
	cp $(BUILD)/kelpos.elf $(BUILD)/iso/boot/
	cp boot/limine.conf $(BUILD)/iso/boot/
	cp boot/wallpaper.bmp $(BUILD)/iso/
	cp boot/silent.flag $(BUILD)/iso/boot/
	cp boot/disk.img $(BUILD)/iso/boot/
	cp $(BUILD)/limine/limine-bios.sys $(BUILD)/limine/limine-bios-cd.bin \
		$(BUILD)/limine/limine-uefi-cd.bin $(BUILD)/iso/
	# Omit --protective-msdos-label: xorriso 1.5.8 would write an MBR
	# partition with start LBA 1, which `limine bios-install` rejects (< 63).
	# Limine installs MBR + active partition itself during bios-install.
	$(XORRISO) -as mkisofs -b limine-bios-cd.bin -no-emul-boot \
		-boot-load-size 4 -boot-info-table \
		-eltorito-alt-boot -b limine-uefi-cd.bin -no-emul-boot \
		-o $@ $(BUILD)/iso
	$(BUILD)/limine/limine bios-install $@

rust:
	CARGO_TARGET_DIR=$(CURDIR)/rust/target \
		cargo build --release --target x86_64-unknown-none \
		--manifest-path rust/krust/Cargo.toml

run: iso
	$(QEMU) -M q35 -m 2G -cdrom $(BUILD)/kelpos.iso -serial stdio

run-debug: iso
	$(QEMU) -M q35 -m 2G -cdrom $(BUILD)/kelpos.iso -serial stdio -s -S

test: $(BUILD)/test_all
	./$(BUILD)/test_all

$(BUILD)/test_all: $(wildcard tests/host/*.c) $(wildcard klibc/src/*.c) \
                   $(wildcard memory/*.c) $(wildcard fs/*.c)
	$(HOSTCC) -fno-builtin -Iinclude -Iklibc/include -Imemory -Ifs -o $@ $^

test-qemu: iso
	@rm -f $(BUILD)/smoke.log
	@timeout 25 $(QEMU) -M q35 -m 2G -display none -no-reboot \
		-cdrom $(BUILD)/kelpos.iso -serial file:$(BUILD)/smoke.log || true
	@grep -q KELPOS_SMOKE_OK $(BUILD)/smoke.log \
		&& echo "OK: QEMU smoke test passed" \
		|| ( echo "ERROR: QEMU smoke test failed"; exit 1 )

clean:
	rm -rf $(BUILD)

help:
	@echo "make             — build the boot ISO, full verbose output"
	@echo "make kernel      — build only the kernel (build/kelpos.elf)"
	@echo "make iso         — build the boot ISO (build/kelpos.iso)"
	@echo "make run         — boot the ISO in QEMU (GUI window + serial)"
	@echo "make run-debug   — QEMU with GDB stub (port 1234)"
	@echo "make test        — host unit tests (klibc/heap)"
	@echo "make test-qemu   — boot smoke test in QEMU"
	@echo "make rust        — build the optional Rust component"
	@echo "make font        — regenerate the embedded font (kernel/font.c)"
	@echo "make kelpinstall — build the C installer (build/kelpinstall)"
	@echo "make install     — install kelpinstall as a command (PREFIX, default /usr/local/bin)"
	@echo "make uninstall   — remove the installed kelpinstall command"
	@echo "make limine      — re-download the Limine bootloader"
	@echo "make PKGS=\"crc cowsay\" iso — build with selected optional packages"
	@echo "make clean"
