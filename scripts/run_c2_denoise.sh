#!/usr/bin/env bash
# C2: denoise e2e — pageable vs host_alloc staging
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT_DIR="${CASE_OUT:-$ROOT/results/cases}"
mkdir -p "$OUT_DIR"
DEV="${CASE_DEV:-0}"
W="${CASE_W:-512}"
H="${CASE_H:-512}"
BENCH="${CASE_BENCH:-20}"
WEIGHTS="${CASE_WEIGHTS:-$ROOT/data/xpu_denoise_synth.bin}"

echo "========================================"
echo " C2 Denoise case"
echo "========================================"
make benchmarks/xpu_denoise
python3 scripts/gen_test_ppm.py -o /tmp/case_c2_in.ppm --w "$W" --h "$H" --noise 25

if [[ ! -f "$WEIGHTS" ]]; then
    echo "WARN: $WEIGHTS missing, using rand"
    WEIGHTS=rand
fi

echo ""
echo "--- pinned (host_alloc) ---"
./benchmarks/xpu_denoise "$DEV" "$WEIGHTS" /tmp/case_c2_in.ppm /tmp/case_c2_out_pin.ppm \
    --pinned --bench "$BENCH" --warmup 3 | tee /tmp/c2_pin.log

echo ""
echo "--- pageable ---"
./benchmarks/xpu_denoise "$DEV" "$WEIGHTS" /tmp/case_c2_in.ppm /tmp/case_c2_out_page.ppm \
    --pageable --bench "$BENCH" --warmup 3 | tee /tmp/c2_page.log

{
    echo "# C2 Denoise — real case"
    echo
    echo "- Date: $(date -Iseconds)"
    echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
    echo "- Device: $DEV  size: ${W}x${H}  bench: $BENCH"
    echo "- Weights: $WEIGHTS"
    echo
    echo "## Results"
    echo
    echo '```'
    grep BENCH /tmp/c2_pin.log || true
    grep BENCH /tmp/c2_page.log || true
    echo '```'
    echo
    echo "## Outputs"
    echo
    echo "- Input: \`/tmp/case_c2_in.ppm\`"
    echo "- Pinned out: \`/tmp/case_c2_out_pin.ppm\`"
    echo "- Pageable out: \`/tmp/case_c2_out_page.ppm\`"
    echo
    echo "## Notes"
    echo
    echo "- FP16 residual CNN, ncluster=4"
    echo "- Compare host_alloc vs pageable staging for H2D/D2H of activations"
} | tee "$OUT_DIR/c2_denoise.md"

echo "Wrote $OUT_DIR/c2_denoise.md"
