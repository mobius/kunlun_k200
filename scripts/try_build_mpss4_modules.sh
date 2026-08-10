#!/usr/bin/env bash
# try_build_mpss4_modules.sh — Attempt out-of-tree build of MPSS4 mic_x200 modules.
# Expected to FAIL on Ubuntu 6.8; useful on Rocky 8.x / kernel 4.18.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
JJ_MPSS="${JJ_MPSS_DIR:-$REPO_ROOT/third_party/jjkeijser-mpss}"
MOD_DIR="$JJ_MPSS/mpss4/mpss-modules"
LOG_DIR="${LOG_DIR:-$REPO_ROOT/docs/research}"
STAMP="$(date +%Y%m%d_%H%M%S)"
LOG="$LOG_DIR/${STAMP}_mpss4_modules_build.log"

mkdir -p "$LOG_DIR"

if [[ ! -d "$MOD_DIR" ]]; then
  echo "ERROR: $MOD_DIR not found."
  echo "  git clone https://github.com/jjkeijser/mpss.git $JJ_MPSS"
  exit 1
fi

if [[ ! -d "/lib/modules/$(uname -r)/build" ]]; then
  echo "ERROR: kernel build tree missing for $(uname -r)"
  exit 1
fi

echo "Building in $MOD_DIR against $(uname -r)"
echo "Log: $LOG"
{
  echo "=== host $(hostname) $(date -Is) kernel $(uname -r) ==="
  uname -a
  cd "$MOD_DIR"
  # clean best-effort
  make BUILD_CARD=false clean 2>/dev/null || true
  set +e
  make BUILD_CARD=false -j"$(nproc)"
  rc=$?
  set -e
  echo "=== exit $rc ==="
  exit $rc
} 2>&1 | tee "$LOG"

# tee loses exit; re-check last line
if grep -q '^=== exit 0 ===' "$LOG"; then
  echo "BUILD OK — modules under $MOD_DIR (see Makefile for install path)"
  echo "Next (as root on target OS):"
  echo "  make BUILD_CARD=false modules_install"
  echo "  depmod -a"
  echo "  modprobe mic_x200_dma scif_bus vop_bus cosm_bus scif vop mic_cosm mic_x200"
  exit 0
fi
echo "BUILD FAILED (expected on kernels > ~4.18). See $LOG"
echo "Use Rocky Linux 8 + docs/research/20260805-phi-7220p-phase1-mpss4.md"
exit 1
