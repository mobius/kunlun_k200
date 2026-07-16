# C3 Dual-PD P2P Pipeline — real case (N1 deep)

- Date: 2026-07-16T09:51:45+08:00
- Driver: 1BF517814DF547139CD5FCE
- Mode: quick
- Default path: xpu0 stageA → memcpy_peer → xpu1 stageB

## Results

```

### batch=32 feat=2048 runs=15  path xpu0->xpu1
XPURT /usr/local/xpu-4.33.0/lib64/libxpurt.so.1 loaded
========================================
 C3 Dual-PD P2P Pipeline (K200)
========================================
devices=4 | src=0 dst=1 batch=32 feat=2048 layers=4+4 runs=15

--- Dual-PD + P2P ---
dual+P2P: 1.437 ms/batch | 22261.4 samples/s

--- Single-PD serial (D2D mid) ---
single-PD: 2.000 ms/batch | 16001.1 samples/s

SUMMARY dual_ms=1.437 solo_ms=2.000 ratio_solo/dual=1.39
Note: dual may be slower if stages are small (P2P overhead); value is functional split.
```

## Notes

- Demonstrates S5 same-card P2P in a split-compute story
- Sweep shows when dual wins vs P2P tax
- Cross-card P2P unsupported on KL1
