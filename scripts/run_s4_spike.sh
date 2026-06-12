#!/usr/bin/env bash
# S4 spike: compare host_alloc bandwidth with kl1_dma_direct=0 vs 1.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

PARAM=/sys/module/kunlun/parameters/kl1_dma_direct
IOCTL_TEST=0x8eedce65  # _IO(IOCTL_IOC_MAGIC, 101) from xpurt_priv/ioctl.h

if [[ ! -f "$PARAM" ]]; then
    echo "ERROR: kl1_dma_direct sysfs missing. Install spike driver:"
    echo "  sudo KL1_DMA_DIRECT=1 scripts/install_driver.sh"
    exit 1
fi

set_dma_direct_ioctl() {
    local val=$1
    python3 - "$val" <<'PY'
import fcntl, os, sys
val = int(sys.argv[1])
fd = os.open("/dev/xpu0", os.O_RDWR)
try:
    arg = 5 | (val << 8)
    fcntl.ioctl(fd, 0x8EEDCE65, arg)
finally:
    os.close(fd)
PY
}

set_dma_direct() {
    local val=$1
    local cur

    cur="$(cat "$PARAM")"
    if [[ "$cur" == "$val" ]]; then
        return 0
    fi

    if echo "$val" > "$PARAM" 2>/dev/null; then
        return 0
    fi
    if sudo -n sh -c "echo $val > $PARAM" 2>/dev/null; then
        return 0
    fi
    if set_dma_direct_ioctl "$val" 2>/dev/null; then
        cur="$(cat "$PARAM")"
        [[ "$cur" == "$val" ]] && return 0
    fi
    if sudo -n env KL1_DMA_DIRECT="$val" KL1_P2P_STUB=0 "$REPO_ROOT/scripts/install_driver.sh" \
        >/dev/null 2>&1; then
        cur="$(cat "$PARAM")"
        [[ "$cur" == "$val" ]] && return 0
    fi

    echo "ERROR: cannot set kl1_dma_direct=$val (current=$cur)" >&2
    echo "  Run once: sudo scripts/install_driver.sh" >&2
    echo "  Then retry: ./scripts/run_s4_spike.sh" >&2
    exit 1
}

make benchmarks/xpu_perf_test

run_bw() {
    local label=$1 val=$2
    set_dma_direct "$val"
    echo ""
    echo "=== kl1_dma_direct=$val ($label) ==="
    ./benchmarks/xpu_perf_test 0 bw 2>&1 | grep -E 'host_alloc|PAGEABLE|H2D|D2H|1MB|64MB|256MB|1GB'
}

echo "S4 spike: host_alloc direct EDMA vs bounce buffer"
run_bw "bounce (baseline)" 0
run_bw "direct dma_map_page" 1

echo ""
echo "Check dmesg for S4 H2D/D2H direct lines:"
echo "  sudo dmesg | grep 'S4 ' | tail -20"