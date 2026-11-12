`klibc` currently provides a freestanding core archive for string/memory, ASCII
character handling, integer parsing, `errno`, and bounded formatting. It is not
a complete hosted libc. KelpOS does not yet have a block-device driver, VFS,
ext4 mount support, or an installer.

The boot path remains Multiboot v1.
# Read AI-Policy.md before this

# KelpOS

KelpOS is an x86_64 operating-system project written from scratch. Its goals are
to be lightweight and approachable, and to use its own kernel rather than the
Linux, Windows NT, or Darwin kernels.

> **Development disclosure:** KelpOS is built with AI assistance and identifies
> this workflow as **Vibecoded OSDev (V-OSDev)**. See [AI-Policy.md](AI-Policy.md)
> for the project's review, attribution, and responsibility policy.

## Current Status

The project currently contains a GRUB Multiboot v1 boot path and a minimal
x86_64 kernel. Its 32-bit bootstrap checks CPU support, creates identity-mapped
page tables for the first 4 GiB, and enters long mode before calling the kernel.
GRUB is configured to request an RGB framebuffer and start KelpOS in its TTY
shell. The kernel reports each startup stage to COM1 serial output. This remains an early
development system, not a feature-complete or crash-proof operating system.

The kernel initializes a fixed 64 KiB aligned heap and a bounded-polling COM1
serial diagnostic driver when the port is detected. The TTY shell accepts
polled PS/2 keyboard input and can render to VGA text memory or GRUB's RGB
framebuffer. Run `startkelpde` from the shell to open KelpDE; press `Esc` at the
desktop to return to the shell (`T` opens the desktop terminal and `Esc` closes
that terminal first).

Shell commands include `help`, `fastfetch`, `uname`, `cpuinfo`, `about`,
`clear`, `echo`, `mem`, `free`, `syscrash`, `startkelpde`, `version`,
`hostname`, `arch`, `sysinfo`, `netinfo`, `fsinfo`, `whoami`, `uptime`, `halt`,
`reboot`, `true`, `false`, `calc`, `seq`, and `keymap`. Run `keymap` to see or
change the layout with `keymap en`, `keymap de`, or `keymap fr` (English
QWERTY, German QWERTZ, and French AZERTY mappings; accented keys are represented
by ASCII approximations. `calc` supports bounded signed
integer `+`, `-`, `*`, `/`, and `%`; `seq` prints at most 64 values. `syscrash`
intentionally triggers a kernel panic. Status commands report missing OS
facilities honestly; for example, there is no filesystem or timer yet.

This is an early single-core prototype: keyboard and desktop mouse input are
polled, and the desktop is a single framebuffer surface with no window manager.
The kernel installs handlers for the first 32 CPU exceptions and reports their
names through the panic screen; IRQ-driven input and a double-fault task stack
remain future hardening work. ACPI RSDP discovery validates the firmware
checksum and reports the RSDT/XSDT root, but does not yet parse MADT/APIC or
power-management tables. The Intel E1000 driver supports raw Ethernet frames
only; there is no IP or socket stack. Runtime handoff still needs QEMU
verification. The heap remains a fixed bootstrap allocator rather than a
physical memory manager.

The boot path remains Multiboot v1. A Multiboot v2 migration was attempted but
did not pass the boot checks, so the project returned to the working v1 path.

## Build And Run

The root `Makefile` is the only Makefile in this project and is the single entry
point for compiling the current OS sources and creating the bootable ISO. The
`create` target also builds the host-side CISWN prototype, verifies the boot
image, builds the `klibc` archive, and runs KPM, CISWN, boot-layout, memory,
graphics, ACPI, shell-command, and klibc tests. Driver checks compile
privileged I/O code but do not execute it on the host. On Fedora, install the
build and boot tools with:

```sh
sudo dnf install @development-tools grub2-tools grub2-tools-extra xorriso qemu-system-x86 qemu-ui-gtk python3
```

Alternatively, run `./install.sh` on Fedora to install these host tools and
build/test the ISO. This prepares `build/kelpos.iso`; it does not install
KelpOS to a physical disk. The kernel still lacks a block-device driver, VFS,
and ext4 support.

Build, verify, and run the current OS:

```sh
make
```

Plain `make` clears the previous build directory, runs `create`, then starts
QEMU with the resulting ISO. `create` compiles the kernel and host tools, runs
the project checks, and writes `build/kelpos.iso`; use `make create` when you
want to build and verify without starting the guest. Build commands are printed
in full. QEMU opens in a GTK window with guest output in the terminal. QEMU diagnostics are
saved to `build/qemu-debug.log`; QEMU uses user-mode networking with the E1000
model by default. Override it with `QEMU_NETWORK=none`, or change the window or
memory with `QEMU_DISPLAY=sdl` or `QEMU_MEMORY=512M`. Remove generated build files with
`make clean`. The
`./bootstrap` and `./clearcache` scripts are convenience wrappers around those
root Makefile targets. Build output shows each source file being compiled; as
new subsystems are introduced, their sources and checks must be added to this
same build before a phase is considered complete.

The opt-in KelpOS cross-toolchain builds pinned `x86_64-elf` Binutils and GCC
for Linux or macOS. Install host prerequisites and run `make toolchain`; the
root build automatically prefers it after installation. See
[toolchain/README.md](toolchain/README.md) for platform packages, verification,
and the freestanding toolchain scope. The first GCC build can take a while and
requires several gigabytes of disk space.

The initial freestanding C library is built as `build/libklibc.a` by `make
create`. Run its focused tests with `make klibc-test`; see
[klibc/README.md](klibc/README.md) for its API and current limits.

KPM is a working host-side Python package tool, not a command in the freestanding
kernel. Run `make kpm` to show its CLI, or pass arguments with
`make kpm KPM_ARGS="pack examples/hello-kpm build/hello.kpm"`. Install packages
into a sandbox root using its `install` subcommand. `make kpm-test` runs its
focused tests; see [kpm/README.md](kpm/README.md) for all operations. KPM cannot
install into KelpOS until it has a booted userspace and filesystem.

CISWN is a host-buildable C service supervisor prototype, also not yet part of
the kernel or boot ISO. Run its focused checks with `make ciswn-test`; see
[ciswn/README.md](ciswn/README.md) for its config format and current limits.

Run the bootstrap heap tests with `make memory-test`, software framebuffer/font
tests with `make graphics-test`, Multiboot layout checks with `make boot-test`,
ACPI checksum tests with `make acpi-test`, shell command tests with
`make commands-test`, keyboard layout tests with `make keyboard-test`, klibc
tests with `make klibc-test`, and driver object compile checks with
`make drivers-test`.

## KelpOS Toolchain

The cross-toolchain is built once for the current host and then produces
`x86_64-elf` KelpOS binaries. Install the precompiled host build dependencies
first.

Linux, Debian/Ubuntu:

```sh
sudo apt install build-essential bison flex texinfo gawk curl xz-utils \
	gpg libgmp-dev libmpfr-dev libmpc-dev libisl-dev zlib1g-dev
```

Linux, Fedora:

```sh
sudo dnf install @development-tools bison flex texinfo gawk curl xz gpg \
	gmp-devel mpfr-devel libmpc-devel isl-devel zlib-devel
```

Linux, Arch/Artix:

```sh
sudo pacman -S base-devel bison flex texinfo gawk curl xz gmp mpfr libmpc isl zlib gpg
```

macOS with Homebrew:

```sh
xcode-select --install
brew install gcc gmp mpfr libmpc isl zlib gawk bison texinfo xz gpg
```

Build it with `make toolchain`. This compiles pinned GNU source releases and
installs host-native compiler tools into `toolchain/local/`; the generated
executables are not shared between Linux and macOS. `make create` automatically
uses the cross compiler after it is installed. See
[toolchain/README.md](toolchain/README.md) for signature verification, build
controls, and toolchain limits. The first build can take a while and needs
several gigabytes of free disk space.

## Layout

- `boot/`: x86 boot entry and transition to long mode
- `acpi/`: ACPI RSDP checksum validation and RSDT/XSDT discovery
- `base/`: small freestanding utility functions
- `drivers/`: VGA text terminal, PS/2 keyboard/mouse, COM1 serial, polling Intel E1000, and x86 port I/O
- `graphics/`: framebuffer drawing, 5x7 font, and KelpDE desktop/terminal
- `grub/`: GRUB Multiboot boot menu configuration
- `kernel/`: kernel entry point, TTY command shell, and linker script
- `klibc/`: initial freestanding C library headers and implementations
- `toolchain/`: Linux/macOS bootstrap for pinned `x86_64-elf` GCC and Binutils
- `ciswn/`: C init/service supervisor prototype and tests
- `kpm/`: Python package manager prototype and tests
- `memory/`: fixed bootstrap heap; physical/virtual memory management remains future work

## Project Direction

The intended audience is older hardware, with a lightweight desktop and a
custom C library named `klibc`. Work proceeds one component at a time. A
milestone needs a defined interface, a build path, and a repeatable check before
the next layer is built on top of it.

1. **KPM host prototype (first slice complete):** local package creation and
	sandbox operations have a tested Python implementation. KelpOS integration
	remains future work.
2. **CISWN (current component):** implement and test the service model and
	supervisor in C. The current program is host-buildable and is not yet a
	bootable KelpOS init process.
3. **Kernel foundation:** stabilize boot, kernel interfaces, panic reporting,
   and memory management with `make create` and QEMU checks. The current heap is
	only an early allocator; CPU exceptions now reach the panic screen, while
	IRQs and complete ACPI/APIC support remain future work.
4. **Core runtime:** extend the initial `klibc` core and develop the minimum
	process, block-device, filesystem/VFS, and device interfaces needed by
	user-space programs. This enables future ext4, init, and package-manager
	integration.
5. **CISWN integration:** port the tested service model to KelpOS after its
	process, filesystem, and signal/service interfaces exist.
6. **KPM integration:** adapt packages and installation to the KelpOS filesystem
	and runtime after those interfaces exist.
7. **SessionDM display manager:** add login/session startup after the kernel
	supports the required input, graphics, and process interfaces.
8. **KelpDE desktop:** build the desktop on top of the display manager and
	stable graphics/input APIs.
9. **Bootloader hardening:** retain Multiboot validation and QEMU boot checks
	while improving the GRUB handoff and future firmware support.

## Next ToDos

- [ ] **Build the KelpOS toolchain:** define and document the compiler, linker,
	assembler, sysroot, and reproducible build process for KelpOS programs.
- [ ] **Build a real shell and desktop environment:** start with an interactive
	shell, then build the desktop on top of the toolchain, userspace, graphics,
	and input foundations.

The roadmap is being worked through in dependency order. CISWN's host-side C
prototype and the initial kernel heap are implemented, but are not yet integrated
as a bootable userspace system. CISWN integration,
SessionDM and KelpDE are plans, not implemented features.
IRQ setup and complete ACPI/APIC support remain deferred to later hardening work.
Keep the scope of each change small, review generated or AI-assisted code, and
do not claim a milestone as working until its checks pass. See
[AI-Policy.md](AI-Policy.md).

The project was previously named SquareOS and TridentOS.