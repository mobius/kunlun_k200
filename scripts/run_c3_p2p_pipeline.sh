#!/usr/bin/env bash
# C3: same-card dual-PD P2P pipeline + optional load sweep
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT_DIR="${CASE_OUT:-$ROOT/results/cases}"
mkdir -p "$OUT_DIR"
SRC="${CASE_SRC:-0}"
DST="${CASE_DST:-1}"
MODE="${CASE_MODE:-full}"  # quick | full

echo "========================================"
echo " C3 Dual-PD P2P pipeline (mode=$MODE)"
echo "========================================"
make benchmarks/xpu_pipeline_p2p

LOG=/tmp/c3_p2p.log
: > "$LOG"

run_one() {
  local b="$1" f="$2" r="$3"
  echo "" | tee -a "$LOG"
  echo "### batch=$b feat=$f runs=$r  path xpu${SRC}->xpu${DST}" | tee -a "$LOG"
  ./benchmarks/xpu_pipeline_p2p -s "$SRC" -t "$DST" -b "$b" -f "$f" -r "$r" 2>&1 | tee -a "$LOG"
}

if [[ "$MODE" == "quick" ]]; then
  run_one 32 2048 15
else
  # baseline + light sweep
  run_one 32 2048 30
  run_one 32 1024 20
  run_one 64 2048 20
  # second card pair if present
  if [[ -e /dev/xpu2 && -e /dev/xpu3 ]]; then
    echo "" | tee -a "$LOG"
    echo "### card1 pair xpu2->xpu3" | tee -a "$LOG"
    ./benchmarks/xpu_pipeline_p2p -s 2 -t 3 -b 32 -f 2048 -r 20 2>&1 | tee -a "$LOG"
  fi
fi

{
  echo "# C3 Dual-PD P2P Pipeline — real case (N1 deep)"
  echo
  echo "- Date: $(date -Iseconds)"
  echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
  echo "- Mode: $MODE"
  echo "- Default path: xpu${SRC} stageA → memcpy_peer → xpu${DST} stageB"
  echo
  echo "## Results"
  echo
  echo '```'
  cat "$LOG"
  echo '```'
  echo
  echo "## Notes"
  echo
  echo "- Demonstrates S5 same-card P2P in a split-compute story"
  echo "- Sweep shows when dual wins vs P2P tax"
  echo "- Cross-card P2P unsupported on KL1"
} | tee "$OUT_DIR/c3_p2p_pipeline.md"

echo "Wrote $OUT_DIR/c3_p2p_pipeline.md"
