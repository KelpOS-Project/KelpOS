#!/usr/bin/env bash
# scripts/setup-limine.sh — downloads the Limine bootloader (binaries + limine.h)
set -euo pipefail

cd "$(dirname "$0")/.."

DEST="build/limine"
if [ ! -x "$DEST/limine" ]; then
    rm -rf "$DEST.tmp"
    BRANCH="${LIMINE_BRANCH:-v9.x-binary}"
    if ! git clone --depth=1 --branch="$BRANCH" --filter=blob:none \
            https://github.com/limine-bootloader/limine.git "$DEST.tmp"; then
        echo "Branch $BRANCH not found, trying v8.x-binary ..."
        rm -rf "$DEST.tmp"
        git clone --depth=1 --branch=v8.x-binary --filter=blob:none \
            https://github.com/limine-bootloader/limine.git "$DEST.tmp"
    fi
    make -C "$DEST.tmp"
    rm -rf "$DEST"
    mv "$DEST.tmp" "$DEST"
fi

mkdir -p include
cp -f "$DEST/limine.h" include/limine.h
echo "Limine ready: $DEST (header: include/limine.h)"
