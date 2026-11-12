# klibc

`klibc` is KelpOS's freestanding C library, built separately from the kernel as
`build/libklibc.a`. It has no host libc dependency. Compile KelpOS-targeted code
with the cross compiler, `-ffreestanding -fno-builtin`, and `-Iklibc/include`.
The root `create` target builds the archive; `make klibc-test` runs host-side
behavior tests for its current core.

## Implemented Core

- `string.h`: byte/string operations, including overlap-safe `memmove`,
  `memchr`, `strspn`, `strcspn`, and `strpbrk`
- `ctype.h`: ASCII character classification and case conversion
- `stdlib.h`: `abs`, `labs`, `llabs`, `atoi`, `atol`, `atoll`, `strtol`,
  `strtoul`, `strtoll`, and `strtoull`
- `stdlib.h`: `abs`, `labs`, `llabs`, `atoi`, `atol`, `atoll`, `strtol`,
  `strtoul`, `strtoll`, and `strtoull`
- `errno.h`: basic `EDOM`, `EINVAL`, and `ERANGE` support
- `stdio.h`: bounded `snprintf` and `vsnprintf` for `%c`, `%s`, signed and
  unsigned decimal, hexadecimal, pointers, and `%%`

Character handling is ASCII-only, and `errno` is for the current single
execution context. Formatting does not yet support field width, precision,
locale, or stream output.

This is an initial C runtime core, not a complete glibc replacement. Allocation,
files, console streams, processes, threads, and syscalls need KelpOS memory,
filesystem, and userspace interfaces. In particular, ext4 and an OS installer
must wait for a block-device driver and VFS; the library itself cannot provide
those kernel facilities.
