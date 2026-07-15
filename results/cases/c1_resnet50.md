# C1 ResNet-50 — case report

- Date: 2026-07-15
- Driver: `1BF517814DF547139CD5FCE`
- Runtime: podman `registry.baidubce.com/device/paddle-xpu:ubuntu20-x86_64-gcc84-py310` + `paddlepaddle-xpu==2.6.1`
- Model: `paddle_models/inference.pdmodel` (ResNet-50)
- Device: 0, FP32 Paddle Inference

## Results

| Batch | img/s | ms/batch |
|------:|------:|---------:|
| 1 | **166.9** | 5.99 |
| 8 | **249.2** | 32.10 |
| 32 | **246.0** | 130.08 |

Raw: `c1_resnet50_raw.txt`

## Notes

- Many `BufferMgr::set ... align to 64` warnings in log — room for stack tuning vs historical ~1069 img/s @b1 (different container/SDK era).
- Throughput plateaus ~250 img/s for batch 8–32 on this stack.
- Re-run: `scripts/run_c1_resnet.sh`
