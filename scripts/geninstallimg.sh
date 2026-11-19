#!/usr/bin/env bash
# scripts/geninstallimg.sh — build build/install.img, a complete
# bootable hard-disk image of KelpOS.
#
# The ISO ships this image as the Limine module /boot/install.img. The
# in-kernel `kelpinstall` command streams it to the first SATA disk, so
# the installer needs no partitioning/formatting code in the kernel and
# the installed system always carries a working Limine bootloader.
#
# Layout: MBR + one active FAT16 partition at LBA 2048.
# Requires: dd, sfdisk, mkfs.vfat (dosfstools), mcopy/mmd (mtools) and
# the limine binary in build/limine (downloaded by scripts/setup-limine.sh).
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD=build
OUT="$BUILD/install.img"
PART="$BUILD/install-part.img"
CONF="$BUILD/install-limine.conf"
SIZE_MB=${INSTALL_SIZE_MB:-48}

for tool in dd sfdisk mkfs.vfat mcopy mmd; do
    command -v "$tool" >/dev/null || {
        echo "geninstallimg: missing tool '$tool'" >&2
        echo "  Fedora: sudo dnf install dosfstools mtools util-linux" >&2
        exit 1
    }
done
[ -x "$BUILD/limine/limine" ] || { echo "geninstallimg: run 'make limine' first" >&2; exit 1; }
[ -f "$BUILD/kelpos.elf" ] || { echo "geninstallimg: build the kernel first" >&2; exit 1; }

part_mb=$((SIZE_MB - 1))

rm -f "$OUT" "$PART" "$CONF"
truncate -s "${part_mb}M" "$PART"
mkfs.vfat -F 16 -n KELPOS "$PART" >/dev/null

# The installed limine.conf must not reference the install image itself.
grep -v 'install\.img' boot/limine.conf > "$CONF"

mmd -i "$PART" ::/boot
mcopy -i "$PART" "$BUILD/kelpos.elf"     ::/boot/
mcopy -i "$PART" "$CONF"                 ::/boot/limine.conf
mcopy -i "$PART" boot/silent.flag        ::/boot/
mcopy -i "$PART" boot/disk.img           ::/boot/
mcopy -i "$PART" "$BUILD/limine/limine-bios.sys" ::/boot/
mcopy -i "$PART" boot/wallpaper.bmp      ::/wallpaper.bmp
mcopy -i "$PART" boot/wallpaper.bmp      ::/boot/wallpaper.bmp

sectors=$(( $(stat -c %s "$PART") / 512 ))
dd if=/dev/zero of="$OUT" bs=1M count="$SIZE_MB" status=none
printf 'label: dos\nunit: sectors\n\nstart=2048, size=%d, type=0e, bootable\n' \
    "$sectors" | sfdisk "$OUT" >/dev/null
dd if="$PART" of="$OUT" bs=512 seek=2048 conv=notrunc status=none
"$BUILD/limine/limine" bios-install "$OUT" >/dev/null

rm -f "$PART" "$CONF"
echo "geninstallimg: $OUT ready ($(stat -c %s "$OUT") bytes)"
