#!/usr/bin/env bash
# Run real-world cases (default: C1–C5 when FULL=1, else C2–C4 quick-friendly)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
export CASE_OUT="${CASE_OUT:-$ROOT/results/cases}"
export CASE_MODE="${CASE_MODE:-full}"
mkdir -p "$CASE_OUT"

echo "########################################"
echo "# Real cases  CASE_MODE=$CASE_MODE"
echo "########################################"

bash scripts/run_c2_denoise.sh
bash scripts/run_c3_p2p_pipeline.sh
bash scripts/run_c4_mlp.sh

if [[ "${CASE_SKIP_C1:-0}" != "1" ]]; then
  bash scripts/run_c1_resnet.sh || echo "WARN: C1 failed (paddle env?)"
fi
if [[ "${CASE_SKIP_C5:-0}" != "1" ]]; then
  bash scripts/run_c5_dual_card.sh || echo "WARN: C5 failed"
fi

{
  echo "# Real cases summary"
  echo
  echo "- Date: $(date -Iseconds)"
  echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
  echo "- CASE_MODE=$CASE_MODE"
  echo
  echo "| Case | Report |"
  echo "|------|--------|"
  for f in c1_resnet50 c2_denoise c3_p2p_pipeline c4_mlp c5_dual_card; do
    [[ -f "$CASE_OUT/${f}.md" ]] && echo "| $f | [${f}.md](${f}.md) |"
  done
  echo
  echo "One-pager: \`docs/impl/20260716-demo-one-pager.md\`"
  echo "Next plan: \`docs/plan/20260716-next-phase-plan.md\`"
} | tee "$CASE_OUT/SUMMARY.md"

echo "All case reports under $CASE_OUT"
