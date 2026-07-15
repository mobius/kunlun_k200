# C2 Denoise — real case

- Date: 2026-07-15T14:41:25+08:00
- Driver: 1BF517814DF547139CD5FCE
- Device: 0  size: 256x256  bench: 10
- Weights: /mnt/storage/test_xpu/data/xpu_denoise_synth.bin

## Results

```
BENCH runs=10 warmup=3 | 9.998 ms/img | 100.02 img/s | staging=host_alloc
BENCH runs=10 warmup=3 | 10.001 ms/img | 99.99 img/s | staging=pageable
```

## Outputs

- Input: `/tmp/case_c2_in.ppm`
- Pinned out: `/tmp/case_c2_out_pin.ppm`
- Pageable out: `/tmp/case_c2_out_page.ppm`

## Notes

- FP16 residual CNN, ncluster=4
- Compare host_alloc vs pageable staging for H2D/D2H of activations
