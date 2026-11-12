HOSTCC ?= cc
CROSS_COMPILE ?= $(if $(wildcard $(CURDIR)/toolchain/local/bin/x86_64-elf-gcc),$(CURDIR)/toolchain/local/bin/x86_64-elf-,)
CC := $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,gcc)
AS := $(if $(CROSS_COMPILE),$(CROSS_COMPILE)as,as)
LD := $(if $(CROSS_COMPILE),$(CROSS_COMPILE)ld,ld)
AR := $(if $(CROSS_COMPILE),$(CROSS_COMPILE)ar,ar)
GRUB_MKRESCUE ?= grub2-mkrescue
GRUB_FILE ?= grub2-file
PYTHON ?= python3
KPM_ARGS ?= --help
QEMU_DISPLAY ?= gtk
QEMU_MEMORY ?= 256M
QEMU_NETWORK ?= user,model=e1000
QEMU_LOG ?= $(BUILD_DIR)/qemu-debug.log
QEMU_EXTRA_ARGS ?=

BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/grub-root
ROOTFS_DIR := rootfs

# Automatisches Finden aller Kernel-Befehle in kernel/bin/
BIN_SOURCES := $(wildcard kernel/bin/*.c)
BIN_OBJECTS := $(patsubst kernel/bin/%.c,$(BUILD_DIR)/bin_%.o,$(BIN_SOURCES))

OBJECTS := $(BUILD_DIR)/boot.o \
	$(BUILD_DIR)/interrupt_stubs.o \
	$(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/interrupts.o \
	$(BUILD_DIR)/shell.o \
	$(BIN_OBJECTS) \
	$(BUILD_DIR)/memory_heap.o \
	$(BUILD_DIR)/graphics_framebuffer.o \
	$(BUILD_DIR)/graphics_font.o \
	$(BUILD_DIR)/graphics_desktop.o \
	$(BUILD_DIR)/panic.o \
	$(BUILD_DIR)/vga_terminal.o \
	$(BUILD_DIR)/serial.o \
	$(BUILD_DIR)/keyboard.o \
	$(BUILD_DIR)/mouse.o \
	$(BUILD_DIR)/e1000.o \
	$(BUILD_DIR)/acpi.o \
	$(BUILD_DIR)/x86_io.o \
	$(BUILD_DIR)/string.o

KERNEL := $(BUILD_DIR)/kelpos.elf
ISO := $(BUILD_DIR)/kelpos.iso
CISWN := $(BUILD_DIR)/ciswn
KLIBC := $(BUILD_DIR)/libklibc.a
KLIBC_SOURCES := $(wildcard klibc/src/*.c)
KLIBC_OBJECTS := $(patsubst klibc/src/%.c,$(BUILD_DIR)/klibc_%.o,$(KLIBC_SOURCES))

CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-stack-protector -fno-pic -fno-pie -m64 -mcmodel=small -mno-red-zone
LDFLAGS := -m elf_x86_64 -nostdlib -z max-page-size=0x1000 -T kernel/linker.ld

.PHONY: all create iso verify toolchain kpm kpm-test ciswn-test kernel-test memory-test graphics-test boot-test drivers-test acpi-test commands-test keyboard-test klibc-test run clean check-tools rootfs-populate

all:
	$(MAKE) clean
	$(MAKE) create
	$(MAKE) run


create:
	$(MAKE) $(KERNEL) $(KLIBC) $(CISWN)
	$(MAKE) kpm-test ciswn-test memory-test drivers-test acpi-test commands-test keyboard-test klibc-test
	$(MAKE) check-tools
	$(MAKE) $(ISO)
	$(GRUB_FILE) --is-x86-multiboot $(KERNEL)
	test -s $(ISO)
	@printf 'Build complete: %s\n' $(ISO)

iso: $(ISO)

toolchain:
	bash toolchain/bootstrap.sh

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/boot.o: boot/boot.s | $(BUILD_DIR)
	$(AS) --64 $< -o $@

$(BUILD_DIR)/interrupt_stubs.o: kernel/interrupt_stubs.s | $(BUILD_DIR)
	$(AS) --64 $< -o $@

$(BUILD_DIR)/kernel.o: kernel/kernel.c boot/multiboot.h kernel/shell.h kernel/interrupts.h memory/heap.h drivers/vga_terminal.h drivers/serial.h drivers/e1000.h acpi/acpi.h graphics/framebuffer.h graphics/desktop.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupts.o: kernel/interrupts.c kernel/interrupts.h kernel/panic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/shell.o: kernel/shell.c kernel/shell.h kernel/bin/commands.h drivers/keyboard.h drivers/vga_terminal.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bin_%.o: kernel/bin/%.c kernel/bin/commands.h $(wildcard kernel/bin/*.h) drivers/e1000.h drivers/x86_io.h graphics/desktop.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/memory_heap.o: memory/heap.c memory/heap.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/graphics_framebuffer.o: graphics/framebuffer.c graphics/framebuffer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/graphics_font.o: graphics/font.c graphics/font.h graphics/framebuffer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/graphics_desktop.o: graphics/desktop.c graphics/desktop.h graphics/font.h drivers/keyboard.h drivers/mouse.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/panic.o: kernel/panic.c kernel/panic.h drivers/vga_terminal.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vga_terminal.o: drivers/vga_terminal.c drivers/vga_terminal.h base/string.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/serial.o: drivers/serial.c drivers/serial.h drivers/x86_io.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: drivers/keyboard.c drivers/keyboard.h drivers/x86_io.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mouse.o: drivers/mouse.c drivers/mouse.h drivers/x86_io.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/e1000.o: drivers/e1000.c drivers/e1000.h drivers/x86_io.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/acpi.o: acpi/acpi.c acpi/acpi.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/x86_io.o: drivers/x86_io.c drivers/x86_io.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/string.o: base/string.c base/string.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/klibc_%.o: klibc/src/%.c $(wildcard klibc/include/*.h) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fno-builtin -Iklibc/include -c $< -o $@

$(KLIBC): $(KLIBC_OBJECTS)
	$(AR) rcs $@ $(KLIBC_OBJECTS)

$(KERNEL): $(OBJECTS) kernel/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# CISWN wird nach build/ kompiliert
$(CISWN): kernel/ciswn/ciswn.c | $(BUILD_DIR)
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -pedantic $< -o $@

# Kopiert und platziert Binaries/Configs in die offizielle rootfs-Struktur
rootfs-populate: $(CISWN)
	mkdir -p $(ROOTFS_DIR)/sbin $(ROOTFS_DIR)/bin $(ROOTFS_DIR)/etc $(ROOTFS_DIR)/dev
	cp $(CISWN) $(ROOTFS_DIR)/sbin/ciswn
	# Falls ksh existiert, dorthin kopieren (Beispiel)
	# cp $(BUILD_DIR)/ksh $(ROOTFS_DIR)/bin/ksh

$(ISO): $(KERNEL) $(CISWN) rootfs-populate grub/grub.cfg
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(KERNEL) $(ISO_ROOT)/boot/kelpos.elf
	cp grub/grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	# Gesamtes Rootfs in den ISO-Root spiegeln
	cp -r $(ROOTFS_DIR)/* $(ISO_ROOT)/
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

check-tools:
	@for tool in "$(CC)" "$(AS)" "$(LD)" "$(AR)" "$(HOSTCC)" "$(GRUB_MKRESCUE)" "$(GRUB_FILE)" "$(PYTHON)"; do \
		if ! command -v "$$tool" >/dev/null 2>&1; then \
			printf 'Missing required build tool: %s\n' "$$tool" >&2; \
			exit 1; \
		fi; \
	done

verify: create

kpm:
	PYTHONPATH=. $(PYTHON) -m kpm $(KPM_ARGS)

kpm-test:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) -m unittest discover -s kpm/tests -v

ciswn-test: $(CISWN)
	CISWN_BIN=$(abspath $(CISWN)) PYTHONDONTWRITEBYTECODE=1 $(PYTHON) -m unittest discover -s kernel/ciswn/tests -v

memory-test: memory/heap.c memory/heap.h tests/kernel_heap_test.c | $(BUILD_DIR)
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -pedantic memory/heap.c tests/kernel_heap_test.c -o $(BUILD_DIR)/kernel-heap-test
	$(BUILD_DIR)/kernel-heap-test

kernel-test: memory-test

commands-test: $(BIN_SOURCES) kernel/bin/commands.h memory/heap.c memory/heap.h drivers/vga_terminal.h tests/kernel_commands_test.c | $(BUILD_DIR)
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -pedantic $(BIN_SOURCES) memory/heap.c tests/kernel_commands_test.c -o $(BUILD_DIR)/kernel-commands-test
	$(BUILD_DIR)/kernel-commands-test

keyboard-test: drivers/keyboard.c drivers/keyboard.h drivers/x86_io.h tests/keyboard_test.c | $(BUILD_DIR)
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -pedantic drivers/keyboard.c tests/keyboard_test.c -o $(BUILD_DIR)/keyboard-test
	$(BUILD_DIR)/keyboard-test

klibc-test: $(KLIBC_SOURCES) $(wildcard klibc/include/*.h) tests/klibc_test.c | $(BUILD_DIR)
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin $(KLIBC_SOURCES) tests/klibc_test.c -o $(BUILD_DIR)/klibc-test
	$(BUILD_DIR)/klibc-test

#graphics-test: graphics/framebuffer.c graphics/framebuffer.h graphics/font.c graphics/font.h graphics/desktop.c graphics/desktop.h drivers/mouse.h drivers/vga_terminal.c drivers/vga_terminal.h base/string.c base/string.h tests/graphics_framebuffer_test.c | $(BUILD_DIR)
#	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -pedantic graphics/framebuffer.c graphics/font.c graphics/desktop.c drivers/vga_terminal.c base/string.c tests/graphics_framebuffer_test.c -o $(BUILD_DIR)/graphics-framebuffer-test
#	$(BUILD_DIR)/graphics-framebuffer-test

boot-test: boot/multiboot.h tests/multiboot_info_test.c | $(BUILD_DIR)
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -pedantic tests/multiboot_info_test.c -o $(BUILD_DIR)/multiboot-info-test
	$(BUILD_DIR)/multiboot-info-test

drivers-test: $(BUILD_DIR)/serial.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/mouse.o $(BUILD_DIR)/e1000.o $(BUILD_DIR)/acpi.o $(BUILD_DIR)/x86_io.o
	@printf 'Driver objects compiled (privileged port I/O was not executed).\n'

acpi-test: acpi/acpi.c acpi/acpi.h tests/acpi_test.c | $(BUILD_DIR)
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -pedantic acpi/acpi.c tests/acpi_test.c -o $(BUILD_DIR)/acpi-test
	$(BUILD_DIR)/acpi-test

run: $(ISO)
	qemu-system-x86_64 -no-reboot -cdrom $(ISO) -boot order=d,menu=on -m $(QEMU_MEMORY) -display $(QEMU_DISPLAY) -nic $(QEMU_NETWORK) -serial stdio -monitor none -D $(QEMU_LOG) -d guest_errors,cpu_reset,int $(QEMU_EXTRA_ARGS)

clean:
	rm -rf $(BUILD_DIR)

compile: all
