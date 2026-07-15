# Real cases summary (C1+C2+C3)

- Date: 2026-07-15T14:43:12+08:00
- Driver: 1BF517814DF547139CD5FCE
- Plan: `docs/plan/20260715-real-world-case-plan.md`

## Headline numbers

| Case | Metric | Result |
|------|--------|--------|
| **C2** Denoise 256² FP16 | host_alloc | **~100 img/s** (~10.0 ms) |
| **C2** Denoise 256² FP16 | pageable | **~100 img/s** (~10.0 ms) |
| **C3** Dual-PD + P2P | batch32 FC 4+4 | **1.44 ms/batch**, 22.2k samples/s |
| **C3** Single-PD serial | same compute | **2.00 ms/batch**, 16.0k samples/s |
| **C1** ResNet-50 FP32 | batch=1 | **166.9 img/s** |
| **C1** ResNet-50 FP32 | batch=8 | **249.2 img/s** |
| **C1** ResNet-50 FP32 | batch=32 | **246.0 img/s** |

C3 dual/solo latency ratio ≈ **1.39×** (dual faster on this split).

C1 note: historical ~1069 img/s @b1 was a different container/stack; this run is paddle 2.6.1 in toolchain image with alignment warnings — use as current-stack baseline.

## Reports

- [c1_resnet50.md](c1_resnet50.md)
- [c2_denoise.md](c2_denoise.md)
- [c3_p2p_pipeline.md](c3_p2p_pipeline.md)
