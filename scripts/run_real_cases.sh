#!/usr/bin/env bash
# Run C1+C2+C3 real-world cases
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
export CASE_OUT="${CASE_OUT:-$ROOT/results/cases}"
mkdir -p "$CASE_OUT"

echo "########################################"
echo "# Real cases C1 + C2 + C3"
echo "########################################"

bash scripts/run_c2_denoise.sh
bash scripts/run_c3_p2p_pipeline.sh
bash scripts/run_c1_resnet.sh

{
    echo "# Real cases summary"
    echo
    echo "- Date: $(date -Iseconds)"
    echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
    echo
    echo "| Case | Report |"
    echo "|------|--------|"
    echo "| C2 Denoise | [c2_denoise.md](c2_denoise.md) |"
    echo "| C3 Dual-PD P2P | [c3_p2p_pipeline.md](c3_p2p_pipeline.md) |"
    echo "| C1 ResNet / proxy | [c1_resnet50.md](c1_resnet50.md) |"
    echo
    echo "Plan: \`docs/plan/20260715-real-world-case-plan.md\`"
} | tee "$CASE_OUT/SUMMARY.md"

echo "All case reports under $CASE_OUT"
