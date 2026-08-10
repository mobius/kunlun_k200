#!/usr/bin/env bash
# Install and load the modified kunlun.ko from this repo.
# Requires root (sudo).
#
# Design (robust path):
#   1) Always install .ko to disk + depmod + modprobe.d options FIRST
#   2) Then try unload/reload to pick up the new image
#   3) If unload fails / module stuck "Unloading", exit with reboot instructions
#      (next boot loads the already-installed patched .ko)
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# Override: KO_SRC=/path/to/kunlun.ko (e.g. restore stock)
# Or create ${REPO_ROOT}/.force_ko_src with a single path line (for sudo make without env).
FORCE_KO_FILE="${REPO_ROOT}/.force_ko_src"
if [[ -z "${KO_SRC:-}" && -f "$FORCE_KO_FILE" ]]; then
    KO_SRC="$(head -1 "$FORCE_KO_FILE" | tr -d '\r' | xargs)"
    # Auto stock-friendly flags unless explicitly set
    DISK_ONLY="${DISK_ONLY:-1}"
    SKIP_MODPROBE_CONF="${SKIP_MODPROBE_CONF:-1}"
    info_force=1
fi
KO_SRC="${KO_SRC:-${REPO_ROOT}/kunlun-driver/kunlun.ko}"
KVER="$(uname -r)"
KO_DST="/lib/modules/${KVER}/updates/dkms/kunlun.ko"
MODPROBE_CONF="/etc/modprobe.d/kunlun-kl1.conf"
# If 1, only write disk artifacts; never unload (use before planned reboot).
DISK_ONLY="${DISK_ONLY:-0}"
# If 1, skip writing kl1_* options (for stock modules without those params).
SKIP_MODPROBE_CONF="${SKIP_MODPROBE_CONF:-0}"

die() { echo "ERROR: $*" >&2; exit 1; }
warn() { echo "WARN: $*" >&2; }
info() { echo "$*"; }

if [[ ! -f "$KO_SRC" ]]; then
    die "${KO_SRC} not found. Run: make driver"
fi

if [[ "$(id -u)" -ne 0 ]]; then
    die "run with sudo"
fi

module_state_line() {
    # e.g. "kunlun 123 - Live" or "kunlun 123 -1 - Unloading"
    grep '^kunlun ' /proc/modules 2>/dev/null || true
}

module_is_unloading() {
    module_state_line | grep -q 'Unloading'
}

module_is_loaded() {
    lsmod | grep -q '^kunlun '
}

build_modargs() {
    MODARGS=()
    [[ "${KL1_P2P_STUB:-0}" == "1" ]] && MODARGS+=(kl1_p2p_stub=1)
    if [[ "${KL1_DMA_DIRECT:-1}" == "1" ]]; then
        MODARGS+=(kl1_dma_direct=1)
    else
        MODARGS+=(kl1_dma_direct=0)
    fi
    if [[ "${KL1_BOUNCE_PIPE:-1}" == "1" ]]; then
        MODARGS+=(kl1_bounce_pipe=1)
    else
        MODARGS+=(kl1_bounce_pipe=0)
    fi
}

write_modprobe_conf() {
    build_modargs
    # Boot-time defaults so reboot picks production knobs without re-running script.
    {
        echo "# Managed by test_xpu scripts/install_driver.sh — do not hand-edit lightly"
        echo "# $(date -Is 2>/dev/null || date)"
        if [[ ${#MODARGS[@]} -gt 0 ]]; then
            echo "options kunlun ${MODARGS[*]}"
        else
            echo "# options kunlun (no extra params)"
        fi
    } >"${MODPROBE_CONF}"
    info "Wrote ${MODPROBE_CONF}: options kunlun ${MODARGS[*]:-(none)}"
}

install_ko_to_disk() {
    local src_ver dst_ver

    src_ver="$(modinfo -F srcversion "$KO_SRC" 2>/dev/null || true)"
    if [[ -f "$KO_DST" ]]; then
        dst_ver="$(modinfo -F srcversion "$KO_DST" 2>/dev/null || true)"
        if [[ -n "$src_ver" && -n "$dst_ver" && "$src_ver" != "$dst_ver" ]]; then
            info "Replacing on-disk ko srcversion ${dst_ver} -> ${src_ver}"
        fi
    fi

    if [[ ! -f "${KO_DST}.bak.20260612" && -f "$KO_DST" ]]; then
        cp -a "$KO_DST" "${KO_DST}.bak.20260612"
        info "Backed up original to ${KO_DST}.bak.20260612"
    fi
    # Always keep a timestamped backup of whatever we overwrite (best-effort).
    if [[ -f "$KO_DST" ]]; then
        cp -a "$KO_DST" "${KO_DST}.bak.$(date +%Y%m%d%H%M%S)" 2>/dev/null || true
    fi

    cp -a "$KO_SRC" "$KO_DST"
    info "Installed ${KO_SRC} -> ${KO_DST}"
    depmod -a
    info "depmod -a done for ${KVER}"
}

print_reboot_help() {
    cat >&2 <<EOF

=== ACTION REQUIRED: reboot to finish driver switch ===
On-disk module is already the patched build. Do NOT loop modprobe -r
(that can leave kunlun stuck in "Unloading").

  sudo reboot

After boot, verify:

  modinfo -F srcversion,vermagic kunlun
  # expect srcversion matching repo build (e.g. 1BF517814DF547139CD5FCE)
  # expect vermagic $(uname -r)
  cat /sys/module/kunlun/parameters/kl1_p2p_stub \\
      /sys/module/kunlun/parameters/kl1_dma_direct \\
      /sys/module/kunlun/parameters/kl1_bounce_pipe
  ls /dev/xpu* | wc -l    # expect 8 + xpuctrl
  cd ${REPO_ROOT} && make regression

If knobs missing after reboot, run again:
  sudo ${REPO_ROOT}/scripts/install_driver.sh

EOF
}

stop_userspace_holders() {
    # Narrow patterns only — avoid pkill -f that could race with modprobe.
    pkill -x test_p2p 2>/dev/null || true
    pkill -x test_p2p_verify 2>/dev/null || true
    pkill -x test_host_alloc 2>/dev/null || true
    pkill -x test_pageable_verify 2>/dev/null || true
    pkill -x xpu_perf_test 2>/dev/null || true
    # Do not fuser -k /dev/xpuctrl (can disrupt control path mid-unload).
    for n in /dev/xpu0 /dev/xpu1 /dev/xpu2 /dev/xpu3 /dev/xpu4 /dev/xpu5 /dev/xpu6 /dev/xpu7; do
        fuser -k "$n" 2>/dev/null || true
    done
    sleep 1
}

try_unload() {
    local i refcnt line

    if module_is_unloading; then
        warn "kunlun already in Unloading state — cannot recover without reboot"
        return 1
    fi

    if ! module_is_loaded; then
        info "kunlun not loaded"
        return 0
    fi

    stop_userspace_holders
    refcnt="$(cat /sys/module/kunlun/refcnt 2>/dev/null || echo '?')"
    info "kunlun refcnt=${refcnt}"

    for i in 1 2 3; do
        if module_is_unloading; then
            warn "entered Unloading during attempt ${i}"
            return 1
        fi
        # Prefer rmmod; short timeout via background+kill if hang
        if timeout 8 rmmod kunlun 2>/dev/null || timeout 8 modprobe -r kunlun 2>/dev/null; then
            info "kunlun unloaded"
            return 0
        fi
        info "unload attempt ${i}/3 failed, wait..."
        stop_userspace_holders
        sleep 3
    done

    line="$(module_state_line)"
    refcnt="$(cat /sys/module/kunlun/refcnt 2>/dev/null || echo '?')"
    warn "could not unload kunlun (refcnt=${refcnt} state='${line}')"
    return 1
}

try_load() {
    build_modargs
    if [[ ${#MODARGS[@]} -gt 0 ]]; then
        modprobe kunlun "${MODARGS[@]}"
        info "Loaded with ${MODARGS[*]}"
    else
        modprobe kunlun
        info "Loaded kunlun (default params)"
    fi
}

verify_loaded() {
    local src_ver live_ver p

    src_ver="$(modinfo -F srcversion "$KO_SRC" 2>/dev/null || true)"
    live_ver="$(modinfo -F srcversion kunlun 2>/dev/null || true)"

    info "Driver loaded:"
    modinfo kunlun | grep -E 'filename|version|srcversion|vermagic|kl1_p2p|kl1_dma|kl1_bounce' || true
    sha256sum "$KO_DST" "$KO_SRC"

    if [[ -n "$src_ver" && -n "$live_ver" && "$src_ver" != "$live_ver" ]]; then
        warn "live srcversion ${live_ver} != build ${src_ver} — old image still running; reboot"
        print_reboot_help
        exit 2
    fi

    for p in kl1_p2p_stub kl1_dma_direct kl1_bounce_pipe; do
        if [[ ! -f /sys/module/kunlun/parameters/${p} ]]; then
            die "${p} sysfs missing — loaded module is not the patched build. Reboot after disk install, or rebuild: make driver"
        fi
        chmod 666 "/sys/module/kunlun/parameters/${p}" 2>/dev/null || true
        info "${p}=$(cat /sys/module/kunlun/parameters/${p})"
    done
    for p in kl1_bounce_d2h kl1_pageable_pin; do
        if [[ -f /sys/module/kunlun/parameters/${p} ]]; then
            chmod 666 "/sys/module/kunlun/parameters/${p}" 2>/dev/null || true
            info "${p}=$(cat /sys/module/kunlun/parameters/${p})"
        else
            warn "${p} missing (older module?)"
        fi
    done
}

# ─── main ───
info "=== install_driver: kernel=${KVER} DISK_ONLY=${DISK_ONLY} SKIP_MODPROBE_CONF=${SKIP_MODPROBE_CONF} ==="
info "KO_SRC=${KO_SRC}"
if [[ -f "$FORCE_KO_FILE" ]]; then
    info "Consumed ${FORCE_KO_FILE}"
    rm -f "$FORCE_KO_FILE"
fi

if module_is_unloading; then
    warn "Module stuck Unloading — install .ko to disk only, then you must reboot"
    install_ko_to_disk
    if [[ "$SKIP_MODPROBE_CONF" == "1" ]]; then
        rm -f "$MODPROBE_CONF"
        info "Removed ${MODPROBE_CONF} (stock-friendly)"
    else
        write_modprobe_conf
    fi
    print_reboot_help
    exit 2
fi

install_ko_to_disk
if [[ "$SKIP_MODPROBE_CONF" == "1" ]]; then
    if [[ -f "$MODPROBE_CONF" ]]; then
        rm -f "$MODPROBE_CONF"
        info "Removed ${MODPROBE_CONF} (SKIP_MODPROBE_CONF=1)"
    fi
else
    write_modprobe_conf
fi

if [[ "$DISK_ONLY" == "1" ]]; then
    info "DISK_ONLY=1 — skipping unload/load. Reboot when ready."
    print_reboot_help
    exit 0
fi

if module_is_loaded; then
    if try_unload; then
        try_load
        if [[ "$SKIP_MODPROBE_CONF" == "1" ]]; then
            info "=== install_driver: OK (hot reload, no kl1 verify) ==="
            modinfo kunlun | grep -E 'filename|srcversion|vermagic' || true
            exit 0
        fi
        verify_loaded
        info "=== install_driver: OK (hot reload) ==="
        exit 0
    fi
    # Disk already has new ko; reboot will load it.
    print_reboot_help
    exit 2
fi

# Not loaded
try_load
if [[ "$SKIP_MODPROBE_CONF" == "1" ]]; then
    info "=== install_driver: OK (cold load, no kl1 verify) ==="
    modinfo kunlun | grep -E 'filename|srcversion|vermagic' || true
    exit 0
fi
verify_loaded
info "=== install_driver: OK (cold load) ==="
exit 0
