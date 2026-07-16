#!/usr/bin/env bash
# C2: denoise e2e — multi-res, pinned vs pageable, optional PSNR
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT_DIR="${CASE_OUT:-$ROOT/results/cases}"
mkdir -p "$OUT_DIR"
DEV="${CASE_DEV:-0}"
MODE="${CASE_MODE:-full}"   # quick | full
WEIGHTS="${CASE_WEIGHTS:-$ROOT/data/xpu_denoise_synth.bin}"
BENCH="${CASE_BENCH:-}"

if [[ "$MODE" == "quick" ]]; then
  SIZES="${CASE_SIZES:-256}"
  BENCH="${BENCH:-5}"
else
  SIZES="${CASE_SIZES:-256,512}"
  BENCH="${BENCH:-10}"
fi

echo "========================================"
echo " C2 Denoise case (mode=$MODE sizes=$SIZES)"
echo "========================================"
make benchmarks/xpu_denoise

if [[ ! -f "$WEIGHTS" ]]; then
  echo "WARN: $WEIGHTS missing, using rand"
  WEIGHTS=rand
fi

TMP_LOG=/tmp/c2_matrix.log
: > "$TMP_LOG"

IFS=',' read -ra SZ_ARR <<<"$SIZES"
for SZ in "${SZ_ARR[@]}"; do
  SZ=$(echo "$SZ" | tr -d ' ')
  echo ""
  echo "=== ${SZ}x${SZ} ==="
  python3 scripts/gen_test_ppm.py -o "/tmp/case_c2_in_${SZ}.ppm" \
    --clean "/tmp/case_c2_clean_${SZ}.ppm" --w "$SZ" --h "$SZ" --noise 25

  for ST in pinned pageable; do
    FLAG="--${ST}"
    OUT="/tmp/case_c2_out_${ST}_${SZ}.ppm"
    echo "--- $ST ---"
    ./benchmarks/xpu_denoise "$DEV" "$WEIGHTS" "/tmp/case_c2_in_${SZ}.ppm" "$OUT" \
      $FLAG --bench "$BENCH" --warmup 3 | tee -a "$TMP_LOG"
    # quality vs clean (synth weights → PSNR may be low; still records metric)
    if [[ -f "/tmp/case_c2_clean_${SZ}.ppm" && -f "$OUT" ]]; then
      python3 scripts/ppm_psnr.py "/tmp/case_c2_clean_${SZ}.ppm" "$OUT" | tee -a "$TMP_LOG" || true
      python3 scripts/ppm_psnr.py "/tmp/case_c2_in_${SZ}.ppm" "$OUT" | \
        sed "s/PSNR/PSNR(noisy_vs_out)/" | tee -a "$TMP_LOG" || true
    fi
  done
done

{
  echo "# C2 Denoise — real case (N1 deep)"
  echo
  echo "- Date: $(date -Iseconds)"
  echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
  echo "- Device: $DEV  mode=$MODE  sizes=$SIZES  bench=$BENCH"
  echo "- Weights: $WEIGHTS"
  echo
  echo "## Results"
  echo
  echo '```'
  grep -E 'BENCH|PSNR|C2 denoise' "$TMP_LOG" || cat "$TMP_LOG"
  echo '```'
  echo
  echo "## Notes"
  echo
  echo "- FP16 residual CNN, ncluster=4"
  echo "- PSNR vs clean uses **synthetic weights** → absolute quality is not production-grade"
  echo "- PSNR(noisy_vs_out) shows how much the net moves the image"
  echo "- Larger sizes expose I/O share; small sizes are compute-bound"
} | tee "$OUT_DIR/c2_denoise.md"

echo "Wrote $OUT_DIR/c2_denoise.md"
