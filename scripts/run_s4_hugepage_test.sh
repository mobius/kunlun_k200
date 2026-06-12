#!/usr/bin/env bash
# Quick test: host_alloc with 2MB hugepages + kl1_dma_direct=1
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

PARAM=/sys/module/kunlun/parameters/kl1_dma_direct

if [[ ! -f "$PARAM" ]]; then
    echo "ERROR: install driver first: sudo scripts/install_driver.sh"
    exit 1
fi

echo 1 > "$PARAM"
echo "kl1_dma_direct=$(cat "$PARAM")"

make -s benchmarks/xpu_perf_test

echo ""
echo "=== host_alloc bandwidth (hugepage alloc + direct EDMA) ==="
./benchmarks/xpu_perf_test 0 bw 2>&1 | grep -E 'host_alloc|1MB|64MB|256MB|1GB|H2D|D2H'

echo 0 > "$PARAM"
echo ""
echo "Reset kl1_dma_direct=$(cat "$PARAM")"
echo "Check dmesg: sudo dmesg | grep -E 'host_alloc mmap|S4 ' | tail -30"