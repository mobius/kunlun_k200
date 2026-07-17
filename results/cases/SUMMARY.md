# Real cases summary (A+B+C phase)

- Date: 2026-07-16T09:52:03
- Driver: 1BF517814DF547139CD5FCE
- Delivery: `docs/impl/20260715-real-cases-c1-c2-c3.md` (covers C1–C5)
- Plan (done): `docs/plan/20260716-next-phase-plan.md`
- Next: `docs/plan/20260716-phase-after-abc.md`
- One-pager: `docs/impl/20260716-demo-one-pager.md`

## Headline numbers

| Case | Metric | Result |
|------|--------|--------|
| **C2** Denoise 256² | host_alloc / pageable | **~100 img/s** both (compute-bound) |
| **C3** Dual-PD + P2P | b32 f2048 | **~1.44 ms/batch** (~22.2k samp/s) |
| **C3** Single-PD | same | **~2.00 ms/batch** (~16.0k); dual **~1.39×** |
| **C4** MLP host_alloc FP16 | b32 / b64 / b128 | **~22.8k / ~42.2k / ~74.3k** img/s |
| **C4** vs FP32 pageable | b32 | FP16 host_alloc **~1.8×** FP32 pageable |
| **C5** Dual-card | sum/single efficiency | **~2.00×** (xpu0+xpu2, no cross-card P2P) |
| **C1** ResNet-50 | current stack b1/b8/b32 | **167 / 249 / 246** img/s (≠ historical ~1069) |

## Reports

- [c1_resnet50.md](c1_resnet50.md)
- [c2_denoise.md](c2_denoise.md)
- [c3_p2p_pipeline.md](c3_p2p_pipeline.md)
- [c4_mlp.md](c4_mlp.md)
- [c5_dual_card.md](c5_dual_card.md)

## Commands

```bash
make demo          # quick C2/C3/C4
make cases         # fuller suite
DEMO_FULL=1 scripts/run_demo.sh
```
