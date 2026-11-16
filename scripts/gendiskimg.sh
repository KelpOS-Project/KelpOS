#!/usr/bin/env bash
# scripts/gendiskimg.sh — build boot/disk.img, the demo FAT disk image
# shipped as a Limine module. Contains sample files the diskfs package
# can read on a running KelpOS (mounts / diskls / diskcat).
#
# Requires: mkfs.vfat + mcopy (mtools). 16 MiB => FAT16 volume.
set -euo pipefail

cd "$(dirname "$0")/.."

OUT="boot/disk.img"
rm -f "$OUT"
dd if=/dev/zero of="$OUT" bs=1M count=16 status=none

mkfs.vfat -F 16 -n KELPDEMO "$OUT" >/dev/null

mmd  -i "$OUT" ::/docs
mmd  -i "$OUT" ::/photos
mcopy -i "$OUT" /dev/stdin ::/docs/hello.txt <<'EOF'
Hello from the FAT filesystem!

This file lives on boot/disk.img, a FAT16 image shipped as a Limine
module and mounted by KelpOS at /media/disk0.

Try on the KelpOS shell:
    mounts
    diskls /media/disk0
    diskcat /media/disk0/docs/hello.txt
EOF
mcopy -i "$OUT" /dev/stdin ::/docs/license.txt <<'EOF'
KelpOS is free software: GPL-3.0-or-later.
See the LICENSE file in the kernel sources.
EOF
mcopy -i "$OUT" /dev/stdin ::/readme.txt <<'EOF'
KelpOS demo disk (FAT16, read-only support for now).
EOF
mcopy -i "$OUT" /dev/stdin ::/notes.md <<'EOF'
# Notes

- fs/vfs.c: mount table + backend ops
- fs/fat.c: FAT12/16/32 + exFAT reader
- fs/ext4.c, fs/btrfs.c: probe skeletons (M5 milestone)
EOF

echo "boot/disk.img ready ($(stat -c %s "$OUT") bytes)"
