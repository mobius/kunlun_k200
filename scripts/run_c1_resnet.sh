#!/usr/bin/env bash
# C1: ResNet-50 inference batch sweep (Paddle if available)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT_DIR="${CASE_OUT:-$ROOT/results/cases}"
mkdir -p "$OUT_DIR"
DEV="${CASE_DEV:-0}"
MODEL="${CASE_MODEL:-$ROOT/paddle_models}"

echo "========================================"
echo " C1 ResNet-50 inference"
echo "========================================"

run_native_proxy() {
    echo "Paddle unavailable — native FC proxy (not ResNet; for env without paddle-xpu)"
    make benchmarks/xpu_app_pipeline
    ./benchmarks/xpu_app_pipeline -d "$DEV" -b 1 -f 2048 -l 8 -r 40 | tee /tmp/c1_native_b1.log
    ./benchmarks/xpu_app_pipeline -d "$DEV" -b 32 -f 2048 -l 8 -r 30 | tee /tmp/c1_native_b32.log
    {
        echo "# C1 ResNet-50 — case report (native proxy fallback)"
        echo
        echo "- Date: $(date -Iseconds)"
        echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
        echo "- **Paddle not available** in this environment"
        echo "- Proxy: \`xpu_app_pipeline\` FP16 FC stack (illustrative compute density only)"
        echo
        echo "## Native proxy results"
        echo
        echo '```'
        grep -E 'host_alloc\+FP16 e2e|pageable\+FP16 e2e|pageable\+FP32 e2e' /tmp/c1_native_b1.log /tmp/c1_native_b32.log || true
        echo '```'
        echo
        echo "## How to run real ResNet"
        echo
        echo '```bash'
        echo '# Install paddlepaddle-xpu into a venv, then:'
        echo "python3 scripts/paddle_infer_benchmark.py --model $MODEL --batch 1,8,32 --device $DEV \\"
        echo "  --out $OUT_DIR/c1_resnet50_raw.txt"
        echo '```'
        echo
        echo "Historical reference (earlier container): ~1069 img/s FP32 batch=1"
    } | tee "$OUT_DIR/c1_resnet50.md"
}

if python3 -c "import paddle.inference" 2>/dev/null; then
    echo "Using host python paddle"
    python3 scripts/paddle_infer_benchmark.py --model "$MODEL" --batch 1,8,32 --device "$DEV" \
        --out "$OUT_DIR/c1_resnet50_raw.txt" | tee /tmp/c1_paddle.log
    {
        echo "# C1 ResNet-50 — case report"
        echo
        echo "- Date: $(date -Iseconds)"
        echo "- Driver: $(modinfo -F srcversion kunlun 2>/dev/null || echo n/a)"
        echo "- Model: $MODEL"
        echo
        echo "## Results"
        echo
        echo '```'
        cat /tmp/c1_paddle.log
        echo '```'
    } | tee "$OUT_DIR/c1_resnet50.md"
elif command -v podman >/dev/null 2>&1; then
    # Try installing paddle wheel inside paddle-xpu toolchain image (network)
    IMG="${PADDLE_IMG:-registry.baidubce.com/device/paddle-xpu:ubuntu20-x86_64-gcc84-py310}"
    if podman image exists "$IMG" 2>/dev/null; then
        echo "Trying paddle install in container $IMG ..."
        DEV_ARGS=()
        for n in 0 1 2 3; do [[ -e /dev/xpu$n ]] && DEV_ARGS+=(--device=/dev/xpu$n); done
        [[ -e /dev/xpuctrl ]] && DEV_ARGS+=(--device=/dev/xpuctrl)
        if timeout 300 podman run --rm "${DEV_ARGS[@]}" \
            -v "$ROOT:/work:ro" -v "$OUT_DIR:/work/results/cases:rw" \
            -e FLAGS_selected_xpus="$DEV" -w /work \
            "$IMG" bash -lc '
                pip3 install -q numpy 2>/dev/null || true
                # Best-effort official XPU wheel (may fail offline)
                pip3 install -q "paddlepaddle-xpu==2.6.1" -f https://www.paddlepaddle.org.cn/whl/linux/mkl/avx/stable.html 2>/dev/null \
                  || pip3 install -q https://paddle-whl.bj.bcebos.com/paddlex/xpu/paddlepaddle_xpu-2.6.1-cp310-cp310-linux_x86_64.whl 2>/dev/null \
                  || exit 42
                python3 scripts/paddle_infer_benchmark.py --model /work/paddle_models --batch 1,8,32 \
                  --device '"$DEV"' --out /work/results/cases/c1_resnet50_raw.txt
            ' 2>&1 | tee /tmp/c1_paddle.log; then
            {
                echo "# C1 ResNet-50 — case report"
                echo
                echo "- Date: $(date -Iseconds)"
                echo "- Via container + paddlepaddle-xpu"
                echo
                echo '```'
                cat /tmp/c1_paddle.log
                echo '```'
            } | tee "$OUT_DIR/c1_resnet50.md"
        else
            echo "Container paddle install/run failed — native proxy"
            run_native_proxy
        fi
    else
        run_native_proxy
    fi
else
    run_native_proxy
fi

echo "Wrote $OUT_DIR/c1_resnet50.md"
