# Kunlun K200 XPU Performance Analysis & Driver Tuning

2× Kunlun K200 inference cards (4 XPU devices) benchmarked for compute throughput, PCIe bandwidth, and kernel launch latency. Includes xdnn SDK GEMM benchmarks, PaddlePaddle ResNet-50 inference, and KL1 driver fixes for P2P and host memory APIs.

## Hardware

| Component | Detail |
|-----------|--------|
| GPUs | 2× Kunlun K200 (DeviceID 0x3684, 14nm) |
| Architecture | KL1 (Kunlun 1st-gen), 900 MHz |
| HBM | 8 GB per XPU (16 GB per card) |
| PCIe | Gen4 ×8 per card |
| Host | Gigabyte G292-Z20, 2× AMD EPYC 7K62 |
| Kernel | 6.8.0-124-generic, GCC 12.3.0 |

## Key Benchmarks (post driver fix, 2026-06)

### Compute (xdnn GEMM)

| Precision | Peak | Efficiency | Notes |
|-----------|:----:|:----------:|-------|
| FP16 | ~20 TFLOPS | ~35% | `xdnn::fc`, 2048³, nc=4 |
| FP32 | ~7.8 TFLOPS | ~54% | `xdnn::fc`, 2048³ |
| INT8 | N/A | — | KL1 firmware has no INT8 CDNN kernel (see S3 probe) |

### PCIe Bandwidth (XPURT `xpu_memcpy`)

| Direction | BW | Notes |
|-----------|:----:|-------|
| H2D | ~5.5 GB/s | pageable and `xpu_host_alloc` — same path |
| D2H | ~4.9 GB/s | KL1 uses bounce-buffer DMA |

### P2P (same card, PD0 → PD1)

| Metric | Value | Notes |
|--------|:-----:|-------|
| Bandwidth | ~2.5 GB/s | kvmalloc staging (D2H → H2D) |
| Data verify | PASS | `tests/test_p2p_verify` |

### Latency

| Operation | Latency |
|-----------|:------:|
| Kernel Launch | ~2 µs |
| 1-thread dispatch | ~26 µs |
| 4-thread dispatch | ~25 µs |

### Inference

| Model | Framework | Precision | Batch | img/s |
|-------|-----------|:---------:|:-----:|:-----:|
| ResNet-50 | PaddlePaddle 2.6.1 | FP32 | 1 | 1069 |

## Driver fixes (KL1 / K200)

| Feature | Before | After |
|---------|--------|-------|
| `xpu_memcpy_peer` | ioctl hang | PASS, ~2.5 GB/s |
| `xpu_host_alloc` | -706 | PASS (64MB mmap) |
| `xpu_host_free` | -707 | PASS |
| H2D/D2H via host_alloc | N/A | No bandwidth gain vs pageable |

Install modified driver:

```bash
make driver
sudo KL1_P2P_STUB=0 scripts/install_driver.sh
```

Debug P2P ioctl hangs only: `sudo modprobe kunlun kl1_p2p_stub=1` or `echo 1 > /sys/module/kunlun/parameters/kl1_p2p_stub`.

Detailed write-ups: `docs/impl/`, `docs/plan/20260612-remediation-iteration-plan.md`.

## Project Structure

```
.
├── benchmarks/
│   ├── xpu_perf_test.cpp       # GEMM + bandwidth (incl. host_alloc)
│   ├── xpu_int8_probe.cpp      # S3 INT8 availability probe
│   └── xpu_denoise.cpp
├── tests/
│   ├── test_p2p.cpp
│   ├── test_p2p_verify.cpp     # P2P data checksum
│   └── test_host_alloc.cpp
├── scripts/
│   ├── install_driver.sh       # Build ko install + reload
│   ├── run_driver_regression.sh
│   └── run_perf_tests.sh       # Podman / vendor tool suite
├── docs/                       # Research, plan, impl notes (2026-06)
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

# Driver regression (P2P + host_alloc + bandwidth)
make regression

# INT8 probe
make benchmarks/xpu_int8_probe && ./benchmarks/xpu_int8_probe 0
```

### Paddle inference

```bash
python3 scripts/paddle_infer_benchmark.py --model paddle_models --batch_size 1
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
- Pinned DMA zero-copy (S4 deferred — no bandwidth gain observed in S2.4)