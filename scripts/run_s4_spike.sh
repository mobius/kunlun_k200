#!/usr/bin/env bash
# S4 spike: compare host_alloc bandwidth with kl1_dma_direct=0 vs 1.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

PARAM=/sys/module/kunlun/parameters/kl1_dma_direct

if [[ ! -f "$PARAM" ]]; then
    echo "ERROR: kl1_dma_direct sysfs missing. Install spike driver:"
    echo "  sudo KL1_DMA_DIRECT=1 scripts/install_driver.sh"
    exit 1
fi

make benchmarks/xpu_perf_test

run_bw() {
    local label=$1 val=$2
    echo "$val" | sudo tee "$PARAM" >/dev/null
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