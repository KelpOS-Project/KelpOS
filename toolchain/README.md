# toolchain/ — Cross-Compiler

Recommended: `x86_64-elf-gcc` (a real cross-compiler, no host-target stuff).

## Distributions

```sh
# Debian/Ubuntu
sudo apt install gcc-x86_64-linux-gnu nasm xorriso qemu-system-x86 git
# Fedora
sudo dnf install gcc nasm xorriso qemu-system-x86 git
# Arch
sudo pacman -S gcc nasm libisoburn qemu-desktop git
```

The Makefile detects, in this order:
1. `x86_64-elf-gcc`
2. `x86_64-linux-gnu-gcc`
3. host `gcc`

## Building it yourself (maximum "purity")

```sh
export PREFIX="$HOME/opt/cross"
export TARGET=x86_64-elf
./contrib/download_prerequisites  # in the gcc source tree
mkdir build-gcc && cd build-gcc
../configure --target=$TARGET --prefix="$PREFIX" \
    --disable-nls --enable-languages=c --without-headers
make all-gcc -j$(nproc) && make install-gcc
```

Then set `PATH="$PREFIX/bin:$PATH"` — the Makefile will find
`x86_64-elf-gcc` automatically.
