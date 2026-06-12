#!/usr/bin/env bash
# Native host regression after KL1 driver changes (S1/S2).
# Run from repo root; does not require Podman.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }
pass() { echo "PASS: $*"; }

echo "========================================"
echo " KL1 driver regression"
echo " Date: $(date)"
echo " Repo: $REPO_ROOT"
echo "========================================"

if ! lsmod | grep -q '^kunlun '; then
    fail "kunlun module not loaded — run: sudo scripts/install_driver.sh"
fi

if [[ -f /sys/module/kunlun/parameters/kl1_p2p_stub ]]; then
    stub="$(cat /sys/module/kunlun/parameters/kl1_p2p_stub)"
    echo "kl1_p2p_stub=$stub (expect 0 for production)"
    [[ "$stub" == "0" ]] || echo "WARN: stub mode enabled — P2P data verify may fail"
else
    echo "WARN: kl1_p2p_stub sysfs missing — old driver?"
fi

echo ""
echo "--- Build tests ---"
make tests/test_p2p_verify tests/test_host_alloc benchmarks/xpu_perf_test

echo ""
echo "--- S1: P2P verify (dev0 -> dev1) ---"
timeout 60 ./tests/test_p2p_verify | tee /tmp/regression_p2p_verify.log
grep -q 'P2P verification: PASS' /tmp/regression_p2p_verify.log || fail "P2P verify"
pass "P2P verify"

echo ""
echo "--- S2: host_alloc / host_free ---"
./tests/test_host_alloc | tee /tmp/regression_host_alloc.log
grep -q 'pattern check: PASS' /tmp/regression_host_alloc.log || fail "host_alloc pattern"
grep -q 'xpu_host_free ret=0' /tmp/regression_host_alloc.log || fail "host_free"
pass "host_alloc + host_free"

echo ""
echo "--- S2.4: bandwidth (pageable + host_alloc) ---"
timeout 180 ./benchmarks/xpu_perf_test 0 bw | tee /tmp/regression_bw.log
grep -q 'PAGEABLE Memory' /tmp/regression_bw.log || fail "pageable bw section"
grep -q 'xpu_host_alloc' /tmp/regression_bw.log || fail "host_alloc bw section"
pass "bandwidth baseline"

echo ""
echo "========================================"
echo " All regression checks passed"
echo "========================================"