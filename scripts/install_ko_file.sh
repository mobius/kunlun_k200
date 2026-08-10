#!/usr/bin/env bash
# Install an arbitrary kunlun.ko to the DKMS path (disk only). Root required.
# Usage: sudo scripts/install_ko_file.sh /path/to/kunlun.ko
set -euo pipefail
SRC="${1:?usage: $0 /path/to/kunlun.ko}"
KVER="$(uname -r)"
DST="/lib/modules/${KVER}/updates/dkms/kunlun.ko"
[[ "$(id -u)" -eq 0 ]] || { echo "run with sudo"; exit 1; }
[[ -f "$SRC" ]] || { echo "missing $SRC"; exit 1; }
cp -a "$SRC" "$DST"
depmod -a
echo "Installed $SRC -> $DST"
modinfo -F srcversion,vermagic "$DST"
