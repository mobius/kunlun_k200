#!/usr/bin/env bash
# C4: MLP / small-tower batch inference narrative (native FP16 pipeline)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT_DIR="${CASE_OUT:-$ROOT/results/cases}"
mkdir -p "$OUT_DIR"
DEV="${CASE_DEV:-0}"

echo "========================================"
echo " C4 MLP batch inference (xpu_app_pipeline)"
echo "========================================"
make benchmarks/xpu_app_pipeline

LOG=/tmp/c4_mlp.log
: > "$LOG"

for B in 32 64 128; do
  echo "" | tee -a "$LOG"
  echo "### batch=$B feat=2048 layers=8" | tee -a "$LOG"
  ./benchmarks/xpu_app_pipeline -d "$DEV" -b "$B" -f 2048 -l 8 -r 25 2>&1 | \
    grep -E 'host_alloc\+FP16 e2e|pageable\+FP16 e2e|pageable\+FP32 e2e|host_alloc\+FP32 e2e|config' | \
    tee -a "$LOG"
done

{
  echo "# C4 MLP / small-tower batch — case report"
  echo
  echo "- Date: $(date -Iseconds)"
  echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
  echo "- Device: $DEV"
  echo "- Workload: multi-layer FP16 FC (feat=2048, layers=8) — ranking/recall tower proxy"
  echo
  echo "## Results"
  echo
  echo '```'
  cat "$LOG"
  echo '```'
  echo
  echo "## Notes"
  echo
  echo "- Prefer host_alloc + FP16 for production towers"
  echo "- Not a trained ranker; measures path + precision impact"
} | tee "$OUT_DIR/c4_mlp.md"

echo "Wrote $OUT_DIR/c4_mlp.md"
