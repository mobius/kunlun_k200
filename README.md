# Kunlun K200 XPU Performance Analysis & Driver Tuning

2× Kunlun K200 inference cards (4 XPU devices) benchmarked for compute throughput, PCIe bandwidth, and kernel launch latency. Includes xdnn SDK GEMM benchmarks, PaddlePaddle ResNet-50 inference, and KL1 driver fixes for P2P and host memory APIs.

**Status (2026-07-15): milestone closed.** S0–S9 delivered; see `docs/impl/20260715-project-closure.md`.

## Hardware

| Component | Detail |
|-----------|--------|
| GPUs | 2× Kunlun K200 (DeviceID 0x3684, 14nm) |
| Architecture | KL1 (Kunlun 1st-gen), 900 MHz |
| HBM | 8 GB per XPU (16 GB per card) |
| PCIe | Gen4 ×8 per card |
| Host | Gigabyte G292-Z20, 2× AMD EPYC 7K62 |
| Kernel | 6.8.0-124-generic, GCC 12.3.0 |

## Key Benchmarks (post S5, 2026-07-14)

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
| Bandwidth (S5) | **~11.2 GB/s** | zero-copy + ping-pong (was ~2.5) |
| Data verify | PASS | `tests/test_p2p_verify` |

### Latency

| Operation | Latency |
|-----------|:------:|
| Kernel Launch | ~2 µs |
| 1-thread dispatch | ~26 µs |
| 4-thread dispatch | ~25 µs |

### Inference / App guidance (S8)

| Model / Workload | Framework | Precision | Batch | img/s | Notes |
|------------------|-----------|:---------:|:-----:|------:|-------|
| ResNet-50 | PaddlePaddle 2.6.1 | FP32 | 1 | 1069 | historical container run |
| FC pipeline e2e | native xdnn | FP32 pageable | 32 | ~12.5k | `xpu_app_pipeline` |
| FC pipeline e2e | native xdnn | **FP16 host_alloc** | 32 | **~22.8k** | **~1.8× vs FP32** |

App defaults: `xpu_host_alloc` for staging, **FP16** compute with `ncluster=4`, keep GEMM K in 1024–4096. See `docs/impl/20260715-s8-app-guidance.md`.

## Driver fixes (KL1 / K200)

| Feature | Before | After (S5) |
|---------|--------|------------|
| `xpu_memcpy_peer` (same card) | hang / ~2.5 GB/s fallback | **~11.2 GB/s**, verify PASS |
| `xpu_host_alloc` / `host_free` | -706 / -707 | PASS (2MB hugepage mmap) |
| H2D/D2H via host_alloc | bounce only | **~12.5 / 12.9 GB/s** (S4 direct) |

Install modified driver:

```bash
make driver
sudo KL1_P2P_STUB=0 KL1_DMA_DIRECT=1 scripts/install_driver.sh
# or: make driver-install
```

Debug: `echo 1 > /sys/module/kunlun/parameters/kl1_p2p_stub`  
Disable S4: `echo 0 > /sys/module/kunlun/parameters/kl1_dma_direct`

Detailed write-ups: `docs/impl/` (closure + S4–S9), `docs/plan/20260715-real-world-case-plan.md`, `results/cases/SUMMARY.md`.

## Project Structure

```
.
├── benchmarks/
│   ├── xpu_perf_test.cpp       # GEMM + bandwidth (incl. host_alloc)
│   ├── xpu_app_pipeline.cpp    # S8 e2e pageable/pinned × FP32/FP16
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
│   ├── run_real_cases.sh       # C1+C2+C3
│   ├── run_c1_resnet.sh / run_c2_denoise.sh / run_c3_p2p_pipeline.sh
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

# Real cases C1 ResNet / C2 denoise / C3 dual-PD P2P
make cases
# or: scripts/run_c2_denoise.sh | run_c3_p2p_pipeline.sh | run_c1_resnet.sh

# Driver regression gate (S7: correctness + S4/S5/S6 BW floors)
make regression

# INT8 probe
make benchmarks/xpu_int8_probe && ./benchmarks/xpu_int8_probe 0
```

### Paddle inference

```bash
# Requires paddlepaddle-xpu in the environment
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
- PCIe ×16
- Hardware cross-card P2P
- INT8 GEMM without firmware update
- Hardware cross-card P2P / BAR IOVA