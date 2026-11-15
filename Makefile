# ─────────────────────────────────────────────────────────────
#  KelpOS — FOSS Unix-like x86_64 kernel (Limine)
# ─────────────────────────────────────────────────────────────

VERSION := 0.2.0
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

KCFLAGS := -std=c17 -ffreestanding -fno-builtin -fno-stack-protector \
           -fno-pic -fno-pie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
           -mcmodel=kernel -O2 -g -Wall -Wextra \
           -Iinclude -Iklibc/include -Ikernel -Imemory -Idrivers -I$(BUILD)

KSRC := $(wildcard kernel/*.c) $(wildcard kernel/bin/*.c) \
        $(wildcard drivers/*.c) $(wildcard memory/*.c) \
        $(wildcard klibc/src/*.c)
KOBJ := $(OBJDIR)/kernel/boot.o $(patsubst %.c,$(OBJDIR)/%.o,$(KSRC))

# Optional Rust component (linked if built: make rust)
RUSTLIB  := rust/target/x86_64-unknown-none/release/libkrust.a
LINKLIBS :=
ifneq ($(wildcard $(RUSTLIB)),)
  LINKLIBS := $(RUSTLIB)
endif

.PHONY: all kernel iso limine font rust run run-debug test test-qemu clean help FORCE

all: kernel

kernel: $(BUILD)/kelpos.elf

$(OBJDIR)/%.o: %.c $(BUILD)/pkgconf.h
	@mkdir -p $(dir $@)
	$(CC) $(KCFLAGS) -c $< -o $@

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

$(BUILD)/kelpos.iso: $(BUILD)/kelpos.elf boot/limine.conf $(BUILD)/limine/limine
	@mkdir -p $(BUILD)/iso/boot
	cp $(BUILD)/kelpos.elf $(BUILD)/iso/boot/
	cp boot/limine.conf $(BUILD)/iso/boot/
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

$(BUILD)/test_all: $(wildcard tests/host/*.c) $(wildcard klibc/src/*.c) $(wildcard memory/*.c)
	$(HOSTCC) -fno-builtin -Iinclude -Iklibc/include -Imemory -o $@ $^

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
	@echo "make kernel      — build the kernel (build/kelpos.elf)"
	@echo "make iso         — build the boot ISO (build/kelpos.iso)"
	@echo "make run         — boot the ISO in QEMU (GUI window + serial)"
	@echo "make run-debug   — QEMU with GDB stub (port 1234)"
	@echo "make test        — host unit tests (klibc/heap)"
	@echo "make test-qemu   — boot smoke test in QEMU"
	@echo "make rust        — build the optional Rust component"
	@echo "make font        — regenerate the embedded font (kernel/font.c)"
	@echo "make limine      — re-download the Limine bootloader"
	@echo "make PKGS=\"crc cowsay\" iso — build with selected optional packages"
	@echo "make clean"
