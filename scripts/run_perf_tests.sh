#!/bin/bash
# Kunlun K200 Performance Test Suite
# Runs inside Podman container with /usr/local/xpu mounted at /usr/local/xpu

XPU_BIN=/usr/local/xpu/bin
XPU_TOOLS=/usr/local/xpu/tools
XPU_LIB=/usr/local/xpu/lib64

export LD_LIBRARY_PATH=$XPU_LIB
export PATH=$XPU_BIN:$XPU_TOOLS:$XPU_TOOLS/kunlun2:$PATH

RESULTS_DIR=/workspace/results
mkdir -p $RESULTS_DIR

# Helper: convert human-friendly size to bytes
to_bytes() {
    local val=$1
    case "$val" in
        *K) echo $((${val%K} * 1024)) ;;
        *M) echo $((${val%M} * 1024 * 1024)) ;;
        *G) echo $((${val%G} * 1024 * 1024 * 1024)) ;;
        *)   echo "$val" ;;
    esac
}

echo "========================================"
echo " Kunlun K200 Performance Test Suite"
echo " Date: $(date)"
echo "========================================"
echo ""

# System baseline
echo "--- System Baseline ---"
xpu_smi > $RESULTS_DIR/xpu_smi_baseline.txt 2>&1
xpu_smi -m > $RESULTS_DIR/xpu_smi_baseline_raw.txt 2>&1
cat $RESULTS_DIR/xpu_smi_baseline.txt

# ============= DMA HBM Bandwidth Tests =============
echo ""
echo "========================================"
echo " DMA HBM Bandwidth Tests (H2D + D2H)"
echo "========================================"

for dev in 0 1 2 3; do
    echo ""
    echo "--- Device $dev (HBM) ---"
    for slabel in 1M 64M 256M 1G; do
        sbytes=$(to_bytes "$slabel")
        echo "  Size: $slabel ($sbytes bytes)"
        result_file=$RESULTS_DIR/dma_hbm_dev${dev}_${slabel}.txt
        test_dma --loop 3 "$dev" "$sbytes" > "$result_file" 2>&1
        h2d=$(grep "AverageUserSpeed" "$result_file" | head -1 | awk '{print $2}')
        d2h=$(grep "AverageUserSpeed" "$result_file" | tail -1 | awk '{print $2}')
        h2d_pcie=$(grep "AveragePCIeSpeed" "$result_file" | head -1 | awk '{print $2}')
        d2h_pcie=$(grep "AveragePCIeSpeed" "$result_file" | tail -1 | awk '{print $2}')
        echo "    H2D: ${h2d} GB/s (user) / ${h2d_pcie} GB/s (pcie)"
        echo "    D2H: ${d2h} GB/s (user) / ${d2h_pcie} GB/s (pcie)"
    done
done

# ============= DMA L3 Bandwidth Tests =============
echo ""
echo "========================================"
echo " DMA L3 Bandwidth Tests (H2D + D2H)"
echo "========================================"

for dev in 0 1 2 3; do
    echo ""
    echo "--- Device $dev (L3) ---"
    for slabel in 1M 4M 8M; do
        sbytes=$(to_bytes "$slabel")
        echo "  Size: $slabel ($sbytes bytes)"
        result_file=$RESULTS_DIR/dma_l3_dev${dev}_${slabel}.txt
        test_dma --l3 --loop 3 "$dev" "$sbytes" > "$result_file" 2>&1
        h2d=$(grep "AverageUserSpeed" "$result_file" | head -1 | awk '{print $2}')
        d2h=$(grep "AverageUserSpeed" "$result_file" | tail -1 | awk '{print $2}')
        h2d_pcie=$(grep "AveragePCIeSpeed" "$result_file" | head -1 | awk '{print $2}')
        d2h_pcie=$(grep "AveragePCIeSpeed" "$result_file" | tail -1 | awk '{print $2}')
        echo "    H2D: ${h2d} GB/s (user) / ${h2d_pcie} GB/s (pcie)"
        echo "    D2H: ${d2h} GB/s (user) / ${d2h_pcie} GB/s (pcie)"
    done
done

# ============= Peer Memcpy Tests =============
echo ""
echo "========================================"
echo " Peer Memcpy Performance"
echo "========================================"

echo ""
echo "--- NOTE: peer memcpy requires P2P IOCTL support in driver --"
echo "--- Testing anyway, may fail with 'Unknown IOCTL command' ---"

run_peer_test() {
    local label=$1 src=$2 dst=$3 slabel=$4
    sbytes=$(to_bytes "$slabel")
    echo "  [$label] src=$src dst=$dst Size: $slabel ($sbytes bytes)"
    result_file=$RESULTS_DIR/peer_${label}_${slabel}.txt
    test_memcpy_peer --perf --loop 3 "$src" "$dst" "$sbytes" > "$result_file" 2>&1
    if grep -q "error" "$result_file" 2>/dev/null; then
        echo "    FAILED: $(head -1 "$result_file")"
    else
        cat "$result_file"
    fi
}

run_peer_test "cross_0to2" 0 2 "1M"
run_peer_test "cross_2to0" 2 0 "1M"
run_peer_test "same_0to1" 0 1 "1M"
run_peer_test "same_2to3" 2 3 "1M"
echo ""
echo "--- Skipping larger peer sizes (driver limitation) ---"

# ============= Kernel Launch Tests =============
echo ""
echo "========================================"
echo " Kernel Launch Stress/Performance Test"
echo "========================================"

for dev in 0 1 2 3; do
    echo ""
    echo "--- Device $dev: single-thread, 100 kernels ---"
    result_file=$RESULTS_DIR/launch_dev${dev}_st.txt
    test_launch --loop 100 --kernel-num 10 --perf "$dev" > "$result_file" 2>&1
    cat "$result_file"
done

echo ""
echo "--- Device 0: multi-thread (4 threads, 100 kernels each) ---"
result_file=$RESULTS_DIR/launch_dev0_mt4.txt
test_launch --loop 100 --kernel-num 10 --thread-num 4 --use-stream --perf 0 > "$result_file" 2>&1
cat "$result_file"

echo ""
echo "--- Device 1: multi-thread (4 threads, 100 kernels each) ---"
result_file=$RESULTS_DIR/launch_dev1_mt4.txt
test_launch --loop 100 --kernel-num 10 --thread-num 4 --use-stream --perf 1 > "$result_file" 2>&1
cat "$result_file"

echo ""
echo "--- Device 2: multi-thread (4 threads, 100 kernels each) ---"
result_file=$RESULTS_DIR/launch_dev2_mt4.txt
test_launch --loop 100 --kernel-num 10 --thread-num 4 --use-stream --perf 2 > "$result_file" 2>&1
cat "$result_file"

echo ""
echo "--- Device 3: multi-thread (4 threads, 100 kernels each) ---"
result_file=$RESULTS_DIR/launch_dev3_mt4.txt
test_launch --loop 100 --kernel-num 10 --thread-num 4 --use-stream --perf 3 > "$result_file" 2>&1
cat "$result_file"

# System final state
echo ""
echo "========================================"
echo " System Final State"
echo "========================================"
xpu_smi > $RESULTS_DIR/xpu_smi_final.txt 2>&1
cat $RESULTS_DIR/xpu_smi_final.txt

echo ""
echo "========================================"
echo " All tests completed: $(date)"
echo " Results saved to: $RESULTS_DIR"
echo "========================================"
