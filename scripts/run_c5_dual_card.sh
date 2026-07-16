#!/usr/bin/env bash
# C5: weak multi-card parallel — two independent processes on xpu0 and xpu2
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT_DIR="${CASE_OUT:-$ROOT/results/cases}"
mkdir -p "$OUT_DIR"

if [[ ! -e /dev/xpu0 || ! -e /dev/xpu2 ]]; then
  echo "Need /dev/xpu0 and /dev/xpu2 for dual-card case" >&2
  exit 1
fi

echo "========================================"
echo " C5 Dual-card weak parallel"
echo "========================================"
make benchmarks/xpu_app_pipeline

extract_imgs() {
  # line like: host_alloc+FP16 e2e  22853.3 img/s ...
  sed -n 's/.*host_alloc+FP16 e2e[[:space:]]*\([0-9.]*\)[[:space:]]*img\/s.*/\1/p' "$1" | head -1
}

# Single-card baseline on dev0
echo "--- single card (xpu0) ---"
./benchmarks/xpu_app_pipeline -d 0 -b 32 -f 2048 -l 8 -r 30 2>&1 | tee /tmp/c5_single.log | \
  grep -E 'host_alloc\+FP16 e2e'

SINGLE=$(extract_imgs /tmp/c5_single.log)

# Parallel: two processes
echo "--- dual card (xpu0 + xpu2 concurrent) ---"
./benchmarks/xpu_app_pipeline -d 0 -b 32 -f 2048 -l 8 -r 30 > /tmp/c5_dev0.log 2>&1 &
P0=$!
./benchmarks/xpu_app_pipeline -d 2 -b 32 -f 2048 -l 8 -r 30 > /tmp/c5_dev2.log 2>&1 &
P2=$!
wait $P0
wait $P2

grep -E 'host_alloc\+FP16 e2e|S8 App' /tmp/c5_dev0.log /tmp/c5_dev2.log | tee /tmp/c5_dual.log
D0=$(extract_imgs /tmp/c5_dev0.log)
D2=$(extract_imgs /tmp/c5_dev2.log)

EFF=$(python3 -c "s=float('${SINGLE}' or 0); a=float('${D0}' or 0); b=float('${D2}' or 0); print(f'{(a+b)/s:.2f}' if s>0 else 'n/a')")

{
  echo "# C5 Dual-card weak parallel — case report"
  echo
  echo "- Date: $(date -Iseconds)"
  echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
  echo "- Workload: host_alloc+FP16 e2e pipeline batch=32 feat=2048 layers=8"
  echo "- **No cross-card P2P** — independent processes"
  echo
  echo "## Results"
  echo
  echo "| Mode | img/s (host_alloc+FP16 e2e) |"
  echo "|------|----------------------------:|"
  echo "| Single xpu0 | ${SINGLE:-?} |"
  echo "| Parallel xpu0 | ${D0:-?} |"
  echo "| Parallel xpu2 | ${D2:-?} |"
  echo "| Sum / single | **${EFF}×** |"
  echo
  echo "## Raw"
  echo
  echo '```'
  echo "=== single ==="
  grep 'host_alloc+FP16 e2e' /tmp/c5_single.log || true
  echo "=== dual ==="
  cat /tmp/c5_dual.log
  echo '```'
  echo
  echo "## Notes"
  echo
  echo "- Efficiency target ~≥1.7× if PCIe/CPU not saturated"
  echo "- Re-run: scripts/run_c5_dual_card.sh"
} | tee "$OUT_DIR/c5_dual_card.md"

echo "Wrote $OUT_DIR/c5_dual_card.md"
