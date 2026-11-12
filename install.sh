#!/usr/bin/env bash
set -euo pipefail

if ! command -v dnf >/dev/null 2>&1; then
	printf 'install.sh: Fedora dnf is required for this setup script.\n' >&2
	exit 1
fi

packages=(
	'@development-tools'
	grub2-tools
	grub2-tools-extra
	xorriso
	qemu-system-x86
	qemu-ui-gtk
	python3
)

if [[ $(id -u) -eq 0 ]]; then
	dnf install -y "${packages[@]}"
else
	sudo dnf install -y "${packages[@]}"
fi

project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$project_dir"
make create

printf 'KelpOS ISO is ready: %s/build/kelpos.iso\n' "$project_dir"