# C4 MLP / small-tower batch — case report

- Date: 2026-07-16T09:51:48+08:00
- Driver: 1BF517814DF547139CD5FCE
- Device: 0
- Workload: multi-layer FP16 FC (feat=2048, layers=8) — ranking/recall tower proxy

## Results

```

### batch=32 feat=2048 layers=8
config                             img/s        ms    GFLOPS  notes
  pageable+FP32 e2e             12486.1 img/s     2.56 ms    837.9 GFLOPS  host=pageable dtype=FP32 IO=H2D+D2H
  pageable+FP16 e2e             22477.0 img/s     1.42 ms   1508.4 GFLOPS  host=pageable dtype=FP16 IO=H2D+D2H
  host_alloc+FP32 e2e           12758.6 img/s     2.51 ms    856.2 GFLOPS  host=host_alloc dtype=FP32 IO=H2D+D2H
  host_alloc+FP16 e2e           22845.4 img/s     1.40 ms   1533.1 GFLOPS  host=host_alloc dtype=FP16 IO=H2D+D2H

### batch=64 feat=2048 layers=8
config                             img/s        ms    GFLOPS  notes
  pageable+FP32 e2e             22095.8 img/s     2.90 ms   1482.8 GFLOPS  host=pageable dtype=FP32 IO=H2D+D2H
  pageable+FP16 e2e             40908.2 img/s     1.56 ms   2745.3 GFLOPS  host=pageable dtype=FP16 IO=H2D+D2H
  host_alloc+FP32 e2e           22946.6 img/s     2.79 ms   1539.9 GFLOPS  host=host_alloc dtype=FP32 IO=H2D+D2H
  host_alloc+FP16 e2e           42163.0 img/s     1.52 ms   2829.5 GFLOPS  host=host_alloc dtype=FP16 IO=H2D+D2H

### batch=128 feat=2048 layers=8
config                             img/s        ms    GFLOPS  notes
  pageable+FP32 e2e             36313.7 img/s     3.52 ms   2437.0 GFLOPS  host=pageable dtype=FP32 IO=H2D+D2H
  pageable+FP16 e2e             70260.2 img/s     1.82 ms   4715.1 GFLOPS  host=pageable dtype=FP16 IO=H2D+D2H
  host_alloc+FP32 e2e           38415.4 img/s     3.33 ms   2578.0 GFLOPS  host=host_alloc dtype=FP32 IO=H2D+D2H
  host_alloc+FP16 e2e           74266.6 img/s     1.72 ms   4983.9 GFLOPS  host=host_alloc dtype=FP16 IO=H2D+D2H
```

## Notes

- Prefer host_alloc + FP16 for production towers
- Not a trained ranker; measures path + precision impact
