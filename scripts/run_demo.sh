#!/usr/bin/env bash
# 5-minute demo path: regression (optional) + quick cases + open summary
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
export CASE_OUT="${CASE_OUT:-$ROOT/results/cases}"
export CASE_MODE="${CASE_MODE:-quick}"
mkdir -p "$CASE_OUT"

echo "########################################"
echo "# K200 demo (CASE_MODE=$CASE_MODE)"
echo "########################################"
echo "Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo 'NOT LOADED')"
date

if [[ "${DEMO_REGRESSION:-0}" == "1" ]]; then
  echo ""
  echo "=== make regression ==="
  make regression
fi

echo ""
echo "=== C2 denoise (quick) ==="
CASE_MODE=quick bash scripts/run_c2_denoise.sh

echo ""
echo "=== C3 dual-PD (quick) ==="
CASE_MODE=quick bash scripts/run_c3_p2p_pipeline.sh

echo ""
echo "=== C4 MLP ==="
bash scripts/run_c4_mlp.sh

if [[ "${DEMO_FULL:-0}" == "1" ]]; then
  echo ""
  echo "=== C1 ResNet (may need network/paddle) ==="
  bash scripts/run_c1_resnet.sh || true
  echo ""
  echo "=== C5 dual-card ==="
  bash scripts/run_c5_dual_card.sh || true
fi

# Refresh summary
{
  echo "# Demo / cases summary"
  echo
  echo "- Date: $(date -Iseconds)"
  echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
  echo "- Mode: CASE_MODE=$CASE_MODE DEMO_FULL=${DEMO_FULL:-0}"
  echo
  echo "| Report | Path |"
  echo "|--------|------|"
  for f in c1_resnet50 c2_denoise c3_p2p_pipeline c4_mlp c5_dual_card; do
    if [[ -f "$CASE_OUT/${f}.md" ]]; then
      echo "| $f | [${f}.md](${f}.md) |"
    fi
  done
  echo
  echo "One-pager: \`docs/impl/20260716-demo-one-pager.md\`"
  echo "Plan: \`docs/plan/20260716-next-phase-plan.md\`"
} | tee "$CASE_OUT/SUMMARY.md"

echo ""
echo "########################################"
echo "# Demo complete — see $CASE_OUT/SUMMARY.md"
echo "########################################"
