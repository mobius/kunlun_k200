# C5 Dual-card weak parallel — case report

- Date: 2026-07-16T09:52:03+08:00
- Driver: 1BF517814DF547139CD5FCE
- Workload: host_alloc+FP16 e2e pipeline batch=32 feat=2048 layers=8
- **No cross-card P2P** — independent processes

## Results

| Mode | img/s (host_alloc+FP16 e2e) |
|------|----------------------------:|
| Single xpu0 | 22833.2 |
| Parallel xpu0 | 22832.1 |
| Parallel xpu2 | 22825.1 |
| Sum / single | **2.00×** |

## Raw

```
=== single ===
  host_alloc+FP16 e2e           22833.2 img/s     1.40 ms   1532.3 GFLOPS  host=host_alloc dtype=FP16 IO=H2D+D2H
=== dual ===
/tmp/c5_dev0.log: S8 App Pipeline Microbench (K200)
/tmp/c5_dev0.log:  host_alloc+FP16 e2e           22832.1 img/s     1.40 ms   1532.2 GFLOPS  host=host_alloc dtype=FP16 IO=H2D+D2H
/tmp/c5_dev2.log: S8 App Pipeline Microbench (K200)
/tmp/c5_dev2.log:  host_alloc+FP16 e2e           22825.1 img/s     1.40 ms   1531.8 GFLOPS  host=host_alloc dtype=FP16 IO=H2D+D2H
```

## Notes

- Efficiency target ~≥1.7× if PCIe/CPU not saturated
- Re-run: scripts/run_c5_dual_card.sh
