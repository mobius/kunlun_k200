#!/usr/bin/env bash
# C3: same-card dual-PD P2P pipeline
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT_DIR="${CASE_OUT:-$ROOT/results/cases}"
mkdir -p "$OUT_DIR"
SRC="${CASE_SRC:-0}"
DST="${CASE_DST:-1}"

echo "========================================"
echo " C3 Dual-PD P2P pipeline"
echo "========================================"
make benchmarks/xpu_pipeline_p2p

./benchmarks/xpu_pipeline_p2p -s "$SRC" -t "$DST" -b 32 -f 2048 -r 30 | tee /tmp/c3_p2p.log

{
    echo "# C3 Dual-PD P2P Pipeline — real case"
    echo
    echo "- Date: $(date -Iseconds)"
    echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
    echo "- Path: xpu${SRC} stageA → memcpy_peer → xpu${DST} stageB"
    echo "- Batch=32 feat=2048 layers=4+4 FP16 ncluster=4 host_alloc input"
    echo
    echo "## Results"
    echo
    echo '```'
    cat /tmp/c3_p2p.log
    echo '```'
    echo
    echo "## Notes"
    echo
    echo "- Demonstrates S5 same-card P2P in a split-compute story"
    echo "- Dual may not always beat single-PD if stages are small (P2P tax)"
    echo "- Cross-card P2P is unsupported on KL1"
} | tee "$OUT_DIR/c3_p2p_pipeline.md"

echo "Wrote $OUT_DIR/c3_p2p_pipeline.md"
