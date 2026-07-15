#!/usr/bin/env bash
# S8: application guidance benchmarks (native pipeline + optional Paddle container).
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

echo "========================================"
echo " S8 application benchmarks"
echo " Date: $(date)"
echo "========================================"

if ! lsmod | grep -q '^kunlun '; then
    echo "ERROR: kunlun not loaded" >&2
    exit 1
fi

echo ""
echo "--- Module knobs (expect production defaults) ---"
for p in kl1_dma_direct kl1_bounce_pipe kl1_p2p_stub; do
    if [[ -f /sys/module/kunlun/parameters/$p ]]; then
        echo "$p=$(cat /sys/module/kunlun/parameters/$p)"
    fi
done

echo ""
echo "--- Build native pipeline ---"
make benchmarks/xpu_app_pipeline

echo ""
echo "--- Native pipeline (batch=32 feat=2048 layers=8) ---"
./benchmarks/xpu_app_pipeline -d 0 -b 32 -f 2048 -l 8 -r 30 | tee /tmp/s8_native_b32.log

echo ""
echo "--- Native pipeline (batch=1 feat=2048 layers=8) ---"
./benchmarks/xpu_app_pipeline -d 0 -b 1 -f 2048 -l 8 -r 50 | tee /tmp/s8_native_b1.log

# Optional: Paddle ResNet via existing device image if present
PADDLE_IMG="${PADDLE_IMG:-registry.baidubce.com/device/paddle-xpu:ubuntu20-x86_64-gcc84-py310}"
if command -v podman >/dev/null 2>&1 && podman image exists "$PADDLE_IMG" 2>/dev/null; then
    echo ""
    echo "--- Paddle Inference ResNet-50 (container) ---"
    # Mount models + scripts; pass XPU devices
    DEV_ARGS=()
    for n in 0 1 2 3; do
        [[ -e /dev/xpu$n ]] && DEV_ARGS+=(--device=/dev/xpu$n)
    done
    [[ -e /dev/xpuctrl ]] && DEV_ARGS+=(--device=/dev/xpuctrl)

    podman run --rm "${DEV_ARGS[@]}" \
        -v "$REPO_ROOT:/work:ro" \
        -v "$REPO_ROOT/results:/work/results:rw" \
        -e FLAGS_selected_xpus=0 \
        -w /work \
        "$PADDLE_IMG" \
        bash -lc '
            set -e
            python3 - <<'"'"'PY'"'"'
import os, time, numpy as np
import paddle.inference as pi

model_dir = "/work/paddle_models"
pdmodel = os.path.join(model_dir, "inference.pdmodel")
pdiparams = os.path.join(model_dir, "inference.pdiparams")
if not os.path.isfile(pdmodel):
    raise SystemExit("missing model")

def run(batch, precision="fp32", runs=50, warmup=10):
    cfg = pi.Config(pdmodel, pdiparams)
    cfg.enable_xpu(100)
    cfg.switch_ir_optim(True)
    # Best-effort: some builds expose mkldnn/fp16 switches; ignore if absent
    if precision == "fp16":
        for name in ("enable_xpu_multi_stream",):
            pass
        try:
            # Paddle-XPU may use env or experimental API
            os.environ["FLAGS_xpu_precision"] = "fp16"
        except Exception:
            pass
    pred = pi.create_predictor(cfg)
    name = pred.get_input_names()[0]
    handle = pred.get_input_handle(name)
    data = np.random.randn(batch, 3, 224, 224).astype("float32")
    handle.reshape([batch, 3, 224, 224])
    handle.copy_from_cpu(data)
    for _ in range(warmup):
        pred.run()
    t0 = time.perf_counter()
    for _ in range(runs):
        handle.copy_from_cpu(data)
        pred.run()
    t1 = time.perf_counter()
    thr = batch * runs / (t1 - t0)
    lat = (t1 - t0) / runs * 1000
    print(f"batch={batch} precision={precision}: {thr:.1f} img/s  {lat:.2f} ms/batch")
    return thr, lat

print("=== Paddle Inference ResNet-50 (XPU container) ===")
results = []
for b in (1, 8, 32):
    thr, lat = run(b, "fp32")
    results.append((b, thr, lat))
out = "/work/results/s8_paddle_resnet50.txt"
with open(out, "w") as f:
    f.write("S8 Paddle ResNet-50\n")
    for b, thr, lat in results:
        f.write(f"batch={b} fp32: {thr:.1f} img/s {lat:.2f} ms\n")
print("wrote", out)
PY
        ' | tee /tmp/s8_paddle.log || echo "WARN: Paddle container run failed (non-fatal)"
else
    echo ""
    echo "SKIP Paddle container (image not present or no podman)"
    echo "  Image: $PADDLE_IMG"
fi

echo ""
echo "S8 native logs: /tmp/s8_native_b32.log /tmp/s8_native_b1.log"
echo "Done."
