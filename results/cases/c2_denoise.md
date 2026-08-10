# C2 Denoise — real case (N1 deep)

- Date: 2026-07-16T09:51:45+08:00
- Driver: 1BF517814DF547139CD5FCE
- Device: 0  mode=quick  sizes=256  bench=5
- Weights: data/xpu_denoise_synth.bin

## Results

```
C2 denoise | Input: 256x256 | staging=host_alloc
BENCH runs=5 warmup=3 | 9.998 ms/img | 100.02 img/s | staging=host_alloc
PSNR: 5.259 dB  (/tmp/case_c2_out_pinned_256.ppm vs /tmp/case_c2_clean_256.ppm, 256x256)
PSNR(noisy_vs_out): 5.118 dB  (/tmp/case_c2_out_pinned_256.ppm vs /tmp/case_c2_in_256.ppm, 256x256)
C2 denoise | Input: 256x256 | staging=pageable
BENCH runs=5 warmup=3 | 9.994 ms/img | 100.06 img/s | staging=pageable
PSNR: 5.259 dB  (/tmp/case_c2_out_pageable_256.ppm vs /tmp/case_c2_clean_256.ppm, 256x256)
PSNR(noisy_vs_out): 5.118 dB  (/tmp/case_c2_out_pageable_256.ppm vs /tmp/case_c2_in_256.ppm, 256x256)
```

## Notes

- FP16 residual CNN, ncluster=4
- PSNR vs clean uses **synthetic weights** → absolute quality is not production-grade
- PSNR(noisy_vs_out) shows how much the net moves the image
- Larger sizes expose I/O share; small sizes are compute-bound
