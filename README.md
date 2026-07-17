# Kunlun K200 XPU Performance Analysis & Driver Tuning

2× Kunlun K200 inference cards (4 XPU devices) benchmarked for compute throughput, PCIe bandwidth, and kernel launch latency. Includes xdnn SDK GEMM benchmarks, PaddlePaddle ResNet-50 inference, real-case demos (C1–C5), and KL1 driver fixes for P2P and host memory APIs.

**Status (2026-07-16):**

| Layer | State |
|-------|--------|
| Driver S0–S9 | **Closed** → `docs/impl/20260715-project-closure.md` |
| Cases C1–C5 + demo | **Delivered** → `results/cases/SUMMARY.md` |
| Next work | Productize / real models / freeze → `docs/plan/20260716-phase-after-abc.md` |

Live driver check: `srcversion` **1BF517814DF547139CD5FCE**; knobs `kl1_dma_direct=1`, `kl1_bounce_pipe=1`, `kl1_pageable_pin=0`, `kl1_p2p_stub=0`.

## Hardware

| Component | Detail |
|-----------|--------|
| GPUs | 2× Kunlun K200 (DeviceID 0x3684, 14nm) |
| Architecture | KL1 (Kunlun 1st-gen), 900 MHz |
| HBM | 8 GB per XPU (16 GB per card) |
| PCIe | Gen4 ×8 per card |
| Host | Gigabyte G292-Z20, 2× AMD EPYC 7K62 |
| Kernel | 6.8.0-124-generic, GCC 12.3.0 |

## Key Benchmarks (post S9 + cases, 2026-07-16)

### Compute (xdnn GEMM)

| Precision | Peak | Efficiency | Notes |
|-----------|:----:|:----------:|-------|
| FP16 | ~20 TFLOPS | ~35% | `xdnn::fc`, 2048³, nc=4 |
| FP32 | ~7.8 TFLOPS | ~54% | `xdnn::fc`, 2048³ |
| INT8 | N/A | — | KL1 firmware has no INT8 CDNN kernel (see S3 probe) |

### PCIe Bandwidth (XPURT `xpu_memcpy`)

| Direction | BW | Notes |
|-----------|:----:|-------|
| H2D pageable (S6/S9) | **~10.1 GB/s** | bounce pipeline (`kl1_bounce_pipe=1`) |
| D2H pageable (S6/S9) | **~7.8 GB/s** | S9 experiments did not improve; use host_alloc for ~12.9 |
| H2D `xpu_host_alloc` (S4) | **~12.5 GB/s** | direct EDMA, `kl1_dma_direct=1` |
| D2H `xpu_host_alloc` (S4) | **~12.9 GB/s** | ≤256MB; 1GB may fall back if hugepages exhausted |

### P2P (same card, PD0 → PD1)

| Metric | Value | Notes |
|--------|:-----:|-------|
| Bandwidth (S5) | **~11.2 GB/s** | zero-copy + ping-pong (was hang / ~2.5) |
| Data verify | PASS | `tests/test_p2p_verify` |

### Latency

| Operation | Latency |
|-----------|:------:|
| Kernel Launch | ~2 µs |
| 1-thread dispatch | ~26 µs |
| 4-thread dispatch | ~25 µs |

### Inference / real cases

| Workload | Framework | Precision | Metric | Notes |
|----------|-----------|:---------:|--------|-------|
| **C1** ResNet-50 | Paddle 2.6.1 XPU | FP32 | b1 **~167** / b8–32 **~250** img/s | **current stack baseline** |
| ResNet-50 (historical) | older container/SDK | FP32 b1 | ~1069 img/s | not reproducible on current stack; not a driver regression |
| **C2** Denoise 256² | native xdnn | FP16 | **~100 img/s** | pinned ≈ pageable (compute-bound) |
| **C3** Dual-PD + P2P | native | FP16 | dual **1.44 ms** vs solo **2.00 ms** (~1.39×) | same-card only |
| **C4** MLP tower e2e | native | FP16 host_alloc | b32 **~22.8k** img/s | ~1.8× vs FP32 pageable |
| **C5** Dual-card | 2× process | FP16 host_alloc | sum/single **~2.00×** | no cross-card P2P |

App defaults: `xpu_host_alloc` for staging, **FP16** compute with `ncluster=4`, keep GEMM K in 1024–4096. See `docs/impl/20260715-s8-app-guidance.md`.

## Driver fixes (KL1 / K200)

| Feature | Before | After (S4–S9) |
|---------|--------|---------------|
| `xpu_memcpy_peer` (same card) | hang / ~2.5 GB/s fallback | **~11.2 GB/s**, verify PASS (S5) |
| `xpu_host_alloc` / `host_free` | -706 / -707 | PASS (2MB hugepage mmap, S2) |
| H2D/D2H via host_alloc | bounce only | **~12.5 / 12.9 GB/s** (S4 direct) |
| pageable H2D/D2H | ~5.5 / ~4.9 | **~10.1 / ~7.8** (S6; S9 no further D2H win) |

Install modified driver:

```bash
make driver
sudo KL1_P2P_STUB=0 KL1_DMA_DIRECT=1 KL1_BOUNCE_PIPE=1 scripts/install_driver.sh
# or: make driver-install
```

Runtime knobs (`/sys/module/kunlun/parameters/`):

| Param | Prod default | Notes |
|-------|:------------:|-------|
| `kl1_dma_direct` | 1 | S4 host_alloc direct EDMA |
| `kl1_bounce_pipe` | 1 | S6 pageable double-buffer |
| `kl1_pageable_pin` | **0** | keep off (4K pin hangs/slow on large xfers) |
| `kl1_p2p_stub` | 0 | debug path off |

Docs: `docs/impl/` (closure + S4–S9 + cases), `docs/plan/20260716-phase-after-abc.md`, `results/cases/SUMMARY.md`.

## Project Structure

```
.
├── benchmarks/
│   ├── xpu_perf_test.cpp       # GEMM + bandwidth (incl. host_alloc)
│   ├── xpu_app_pipeline.cpp    # S8/C4 e2e pageable/pinned × FP32/FP16
│   ├── xpu_pipeline_p2p.cpp    # C3 same-card dual-PD + P2P
│   ├── xpu_int8_probe.cpp      # S3 INT8 availability probe
│   └── xpu_denoise.cpp         # C2 FP16 denoise (+ pinned/pageable)
├── tests/
│   ├── test_p2p.cpp
│   ├── test_p2p_verify.cpp     # P2P data checksum
│   ├── test_pageable_verify.cpp
│   └── test_host_alloc.cpp
├── scripts/
│   ├── install_driver.sh
│   ├── run_driver_regression.sh
│   ├── run_real_cases.sh       # C1–C5 (full)
│   ├── run_demo.sh             # quick C2/C3/C4; DEMO_FULL=1 adds C1/C5
│   ├── run_c1_resnet.sh … run_c5_dual_card.sh
│   └── run_s8_app_bench.sh
├── results/cases/              # Real-case reports (SUMMARY.md)
├── docs/                       # Research, plan, impl, closure
├── kunlun-driver/              # Kernel 4.33 + KL1 modifications
└── xdnn-ubuntu_x86_64/         # xdnn SDK 2.0.0.725
```

## Build & Run

```bash
# Build all benchmarks and tests
make all

# Full perf (bandwidth + GEMM)
./benchmarks/xpu_perf_test 0

# Bandwidth only (pageable vs host_alloc)
./benchmarks/xpu_perf_test 0 bw

# S8 app pipeline (pageable/host_alloc × FP32/FP16 e2e)
make benchmarks/xpu_app_pipeline && ./benchmarks/xpu_app_pipeline -d 0
# or: scripts/run_s8_app_bench.sh

# Real cases C1–C5
make cases

# 5-minute demo (quick C2/C3/C4)
make demo
# DEMO_FULL=1 scripts/run_demo.sh
# DEMO_REGRESSION=1 scripts/run_demo.sh

# Driver regression gate (S7)
make regression

# INT8 probe
make benchmarks/xpu_int8_probe && ./benchmarks/xpu_int8_probe 0
```

### Real cases

| Case | Command | Report |
|------|---------|--------|
| C1 ResNet-50 | `scripts/run_c1_resnet.sh` | `results/cases/c1_resnet50.md` |
| C2 Denoise | `scripts/run_c2_denoise.sh` | `results/cases/c2_denoise.md` |
| C3 Dual-PD P2P | `scripts/run_c3_p2p_pipeline.sh` | `results/cases/c3_p2p_pipeline.md` |
| C4 MLP tower | `scripts/run_c4_mlp.sh` | `results/cases/c4_mlp.md` |
| C5 Dual-card | `scripts/run_c5_dual_card.sh` | `results/cases/c5_dual_card.md` |

One-pager: `docs/impl/20260716-demo-one-pager.md` · Cases delivery: `docs/impl/20260715-real-cases-c1-c2-c3.md` · After A/B/C plan: `docs/plan/20260716-phase-after-abc.md`

### Paddle inference

```bash
# Requires paddlepaddle-xpu in the environment (or podman path in run_c1)
python3 scripts/paddle_infer_benchmark.py --model paddle_models --batch 1,8,32
```

## Environment

| Component | Path |
|-----------|------|
| XPURT | `/usr/local/xpu-4.33.0/lib64/libxpurt.so.1` |
| Driver (installed) | `/lib/modules/$(uname -r)/updates/dkms/kunlun.ko` |
| xdnn | `xdnn-ubuntu_x86_64/` |

## Out of scope (hardware / firmware)

- SM overclock (900 MHz locked)
- PCIe ×16 renegotiation beyond Gen4×8
- Hardware cross-card P2P / BAR IOVA
- INT8 GEMM without firmware update
- Pageable D2H further kernel iteration (S9 closed: no win)
- Default `kl1_pageable_pin=1` (unsafe / too slow)

## Historical baselines (pre-optimization)

May 2026 pre-fix numbers (P2P broken, ~4–5 GB/s DMA) live only as archives:

- `results/PERF_SUMMARY.md` (2026-05-07)
- `results/K200_ANALYSIS_REPORT.md` (2026-05-08)

Do **not** treat those as current performance.
