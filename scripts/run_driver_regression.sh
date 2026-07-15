#!/usr/bin/env bash
# KL1 driver regression gate (S1–S6 correctness + bandwidth floors).
# Run from repo root; does not require Podman. Needs loaded kunlun.ko.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }
pass() { echo "PASS: $*"; }
warn() { echo "WARN: $*"; }

ge() { awk -v a="$1" -v b="$2" 'BEGIN { exit !(a+0 >= b+0) }'; }

# Parse first "64MB ... H2D: X ... D2H: Y" after a section header in a log file.
# Args: log_file section_substring (portable; no gawk-only match())
parse_64mb_bw() {
    local log="$1" section="$2" line h2d d2h
    line="$(awk -v sec="$section" '
        index($0, sec) { insec=1; next }
        insec && /===/ && !index($0, sec) { insec=0 }
        insec && /64MB/ { print; exit }
    ' "$log")"
    h2d="$(printf '%s\n' "$line" | sed -n 's/.*H2D:[[:space:]]*\([0-9.]*\).*/\1/p')"
    d2h="$(printf '%s\n' "$line" | sed -n 's/.*D2H:[[:space:]]*\([0-9.]*\).*/\1/p')"
    printf '%s %s\n' "$h2d" "$d2h"
}

echo "========================================"
echo " KL1 driver regression (S7 gate)"
echo " Date: $(date)"
echo " Repo: $REPO_ROOT"
echo "========================================"

if ! lsmod | grep -q '^kunlun '; then
    fail "kunlun module not loaded — run: sudo scripts/install_driver.sh"
fi

stub=0
direct=0
pipe=0
if [[ -f /sys/module/kunlun/parameters/kl1_p2p_stub ]]; then
    stub="$(cat /sys/module/kunlun/parameters/kl1_p2p_stub)"
    echo "kl1_p2p_stub=$stub (expect 0 for production)"
    [[ "$stub" == "0" ]] || warn "stub mode — P2P data verify may be meaningless"
else
    warn "kl1_p2p_stub sysfs missing — old driver?"
fi

if [[ -f /sys/module/kunlun/parameters/kl1_dma_direct ]]; then
    direct="$(cat /sys/module/kunlun/parameters/kl1_dma_direct)"
    echo "kl1_dma_direct=$direct (expect 1 for S4 production)"
    [[ "$direct" == "1" ]] || warn "S4 direct EDMA disabled — host_alloc uses bounce"
else
    warn "kl1_dma_direct sysfs missing — pre-S4 driver?"
fi

if [[ -f /sys/module/kunlun/parameters/kl1_bounce_pipe ]]; then
    pipe="$(cat /sys/module/kunlun/parameters/kl1_bounce_pipe)"
    echo "kl1_bounce_pipe=$pipe (expect 1 for S6 production)"
    [[ "$pipe" == "1" ]] || warn "S6 bounce pipeline disabled — pageable ~5.5 GB/s"
else
    warn "kl1_bounce_pipe sysfs missing — pre-S6 driver?"
fi

echo "srcversion=$(modinfo -F srcversion kunlun 2>/dev/null || echo '?')"

echo ""
echo "--- Build tests ---"
make tests/test_p2p_verify tests/test_host_alloc tests/test_pageable_verify \
     tests/test_p2p benchmarks/xpu_perf_test

echo ""
echo "--- S1: P2P verify (dev0 -> dev1) ---"
timeout 60 ./tests/test_p2p_verify 2>/tmp/regression_p2p_verify.err | tee /tmp/regression_p2p_verify.log
grep -q 'P2P verification: PASS' /tmp/regression_p2p_verify.log || fail "P2P verify"
grep -q 'D2D verification: PASS' /tmp/regression_p2p_verify.log || fail "D2D verify"
pass "P2P + D2D verify"

echo ""
echo "--- S2: host_alloc / host_free ---"
./tests/test_host_alloc 2>/tmp/regression_host_alloc.err | tee /tmp/regression_host_alloc.log
grep -q 'pattern check: PASS' /tmp/regression_host_alloc.log || fail "host_alloc pattern"
grep -q 'xpu_host_free ret=0' /tmp/regression_host_alloc.log || fail "host_free"
pass "host_alloc + host_free"

echo ""
echo "--- S6/S7: pageable pattern ---"
timeout 120 ./tests/test_pageable_verify 2>/tmp/regression_pageable.err | tee /tmp/regression_pageable.log
grep -q 'pageable 64MB pattern: PASS' /tmp/regression_pageable.log || fail "pageable pattern"
pass "pageable 64MB pattern"

echo ""
echo "--- S2.4 / S4 / S6: bandwidth (pageable + host_alloc) ---"
timeout 300 ./benchmarks/xpu_perf_test 0 bw 2>/tmp/regression_bw.err | tee /tmp/regression_bw.log
grep -q 'PAGEABLE Memory' /tmp/regression_bw.log || fail "pageable bw section"
grep -q 'xpu_host_alloc' /tmp/regression_bw.log || fail "host_alloc bw section"

read -r page_h2d page_d2h <<<"$(parse_64mb_bw /tmp/regression_bw.log 'PAGEABLE Memory')"
read -r host_h2d host_d2h <<<"$(parse_64mb_bw /tmp/regression_bw.log 'xpu_host_alloc')"
echo "parsed pageable 64MB H2D=${page_h2d:-?} D2H=${page_d2h:-?}"
echo "parsed host_alloc 64MB H2D=${host_h2d:-?} D2H=${host_d2h:-?}"

[[ -n "${page_h2d:-}" && -n "${page_d2h:-}" ]] || fail "could not parse pageable 64MB bandwidth"
[[ -n "${host_h2d:-}" && -n "${host_d2h:-}" ]] || fail "could not parse host_alloc 64MB bandwidth"

# Floors depend on module knobs (avoid false fail when intentionally disabled).
if [[ "$pipe" == "1" ]]; then
    ge "$page_h2d" 7.0 || fail "pageable 64MB H2D ${page_h2d} < 7.0 (S6 pipe on)"
    ge "$page_d2h" 6.0 || fail "pageable 64MB D2H ${page_d2h} < 6.0 (S6 pipe on)"
    pass "S6 pageable 64MB H2D ${page_h2d} / D2H ${page_d2h} GB/s"
else
    ge "$page_h2d" 4.0 || fail "pageable 64MB H2D ${page_h2d} < 4.0 (serial)"
    ge "$page_d2h" 3.5 || fail "pageable 64MB D2H ${page_d2h} < 3.5 (serial)"
    pass "pageable serial 64MB H2D ${page_h2d} / D2H ${page_d2h} GB/s"
fi

if [[ "$direct" == "1" ]]; then
    ge "$host_h2d" 8.0 || fail "host_alloc 64MB H2D ${host_h2d} < 8.0 (S4 direct on)"
    ge "$host_d2h" 8.0 || fail "host_alloc 64MB D2H ${host_d2h} < 8.0 (S4 direct on)"
    pass "S4 host_alloc 64MB H2D ${host_h2d} / D2H ${host_d2h} GB/s"
else
    ge "$host_h2d" 4.0 || fail "host_alloc 64MB H2D ${host_h2d} < 4.0 (bounce)"
    pass "host_alloc bounce 64MB H2D ${host_h2d} / D2H ${host_d2h} GB/s"
fi

echo ""
echo "--- S5: P2P bandwidth floor (same card) ---"
timeout 180 ./tests/test_p2p 2>/tmp/regression_p2p_bw.err | tee /tmp/regression_p2p_bw.log
p2p_64="$(awk '/^64MB/ { print $2; exit }' /tmp/regression_p2p_bw.log)"
echo "parsed P2P 64MB=${p2p_64:-?} GB/s"
if [[ "$stub" == "1" ]]; then
    warn "skipping P2P BW floor (stub mode)"
elif [[ -n "${p2p_64:-}" ]]; then
    ge "$p2p_64" 8.0 || fail "P2P 64MB ${p2p_64} < 8.0 GB/s (S5)"
    pass "S5 P2P 64MB ${p2p_64} GB/s"
else
    fail "could not parse P2P 64MB bandwidth"
fi

echo ""
echo "========================================"
echo " All regression checks passed"
echo "========================================"
echo "Summary:"
echo "  pageable  64MB  H2D=${page_h2d} D2H=${page_d2h}  (pipe=${pipe})"
echo "  host_alloc 64MB H2D=${host_h2d} D2H=${host_d2h}  (direct=${direct})"
echo "  P2P        64MB ${p2p_64:-n/a} GB/s"
echo "========================================"
