#!/usr/bin/env bash
# Install and load the modified kunlun.ko from this repo.
# Requires root (sudo).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
KO_SRC="${REPO_ROOT}/kunlun-driver/kunlun.ko"
KVER="$(uname -r)"
KO_DST="/lib/modules/${KVER}/updates/dkms/kunlun.ko"

die() { echo "ERROR: $*" >&2; exit 1; }

if [[ ! -f "$KO_SRC" ]]; then
    die "${KO_SRC} not found. Run: make driver"
fi

if [[ "$(id -u)" -ne 0 ]]; then
    die "run with sudo"
fi

unload_kunlun() {
    local i refcnt

    echo "Stopping userspace holders..."
    pkill -9 -f 'test_p2p|test_ioctl_p2p|test_host_alloc' 2>/dev/null || true
    fuser -k /dev/xpu0 /dev/xpu1 /dev/xpu2 /dev/xpu3 /dev/xpuctrl 2>/dev/null || true
    sleep 1

    if ! lsmod | grep -q '^kunlun '; then
        echo "kunlun not loaded"
        return 0
    fi

    refcnt="$(cat /sys/module/kunlun/refcnt 2>/dev/null || echo '?')"
    echo "kunlun refcnt=${refcnt} (stuck P2P ioctls often keep this elevated)"

    for i in 1 2 3 4 5; do
        if modprobe -r kunlun 2>/dev/null; then
            echo "kunlun unloaded"
            return 0
        fi
        echo "modprobe -r failed (attempt ${i}/5), waiting for kernel paths to unwind..."
        sleep 5
        pkill -9 -f 'test_p2p|test_ioctl_p2p' 2>/dev/null || true
        fuser -k /dev/xpu0 /dev/xpu1 /dev/xpu2 /dev/xpu3 2>/dev/null || true
    done

    refcnt="$(cat /sys/module/kunlun/refcnt 2>/dev/null || echo '?')"
    die "kunlun still in use (refcnt=${refcnt}). Reboot, then rerun: sudo KL1_P2P_STUB=1 $0"
}

verify_ko_match() {
    local src_ver dst_ver

    src_ver="$(modinfo -F srcversion "$KO_SRC" 2>/dev/null || true)"
    if [[ -f "$KO_DST" ]]; then
        dst_ver="$(modinfo -F srcversion "$KO_DST" 2>/dev/null || true)"
        if [[ -n "$src_ver" && -n "$dst_ver" && "$src_ver" != "$dst_ver" ]]; then
            echo "Note: replacing installed ko (srcversion ${dst_ver}) with build (${src_ver})"
        fi
    fi
}

if [[ ! -f "${KO_DST}.bak.20260612" && -f "$KO_DST" ]]; then
    cp "$KO_DST" "${KO_DST}.bak.20260612"
    echo "Backed up original to ${KO_DST}.bak.20260612"
fi

unload_kunlun
verify_ko_match

cp "$KO_SRC" "$KO_DST"
echo "Installed ${KO_SRC} -> ${KO_DST}"

depmod -a
MODARGS=()
[[ "${KL1_P2P_STUB:-0}" == "1" ]] && MODARGS+=(kl1_p2p_stub=1)
[[ "${KL1_DMA_DIRECT:-0}" == "1" ]] && MODARGS+=(kl1_dma_direct=1)

if [[ ${#MODARGS[@]} -gt 0 ]]; then
    modprobe kunlun "${MODARGS[@]}"
    echo "Loaded with ${MODARGS[*]}"
else
    modprobe kunlun
fi

echo "Driver loaded:"
modinfo kunlun | grep -E 'filename|version|srcversion|kl1_p2p'
sha256sum "$KO_DST" "$KO_SRC"

for param in kl1_p2p_stub kl1_dma_direct; do
    if [[ ! -f /sys/module/kunlun/parameters/${param} ]]; then
        die "${param} sysfs missing — loaded module does not match ${KO_SRC}. Reboot and rerun."
    fi
    echo "${param}=$(cat /sys/module/kunlun/parameters/${param})"
done