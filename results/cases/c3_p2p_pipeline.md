# C3 Dual-PD P2P Pipeline — real case

- Date: 2026-07-15T14:41:28+08:00
- Driver: 1BF517814DF547139CD5FCE
- Path: xpu0 stageA → memcpy_peer → xpu1 stageB
- Batch=32 feat=2048 layers=4+4 FP16 ncluster=4 host_alloc input

## Results

```
========================================
 C3 Dual-PD P2P Pipeline (K200)
========================================
devices=4 | src=0 dst=1 batch=32 feat=2048 layers=4+4 runs=30

--- Dual-PD + P2P ---
dual+P2P: 1.441 ms/batch | 22205.8 samples/s

--- Single-PD serial (D2D mid) ---
single-PD: 2.000 ms/batch | 15999.5 samples/s

SUMMARY dual_ms=1.441 solo_ms=2.000 ratio_solo/dual=1.39
Note: dual may be slower if stages are small (P2P overhead); value is functional split.
```

## Notes

- Demonstrates S5 same-card P2P in a split-compute story
- Dual may not always beat single-PD if stages are small (P2P tax)
- Cross-card P2P is unsupported on KL1
