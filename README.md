# Kunlun K200 XPU Performance Analysis & Driver Tuning

2× Kunlun K200 inference cards (4 XPU devices) benchmarked for compute throughput, PCIe bandwidth, and kernel launch latency. Includes xdnn SDK GEMM benchmarks, PaddlePaddle ResNet-50 inference, and low-level driver modifications.

## Hardware

| Component | Detail |
|-----------|--------|
| GPUs | 2× Kunlun K200 (DeviceID 0x3684, 14nm) |
| Architecture | KL1 (Kunlun 1st-gen), 900 MHz |
| HBM | 8 GB per XPU (16 GB per card) |
| PCIe | Gen4 ×8 per card |
| Host | Gigabyte G292-Z20, 2× AMD EPYC 7K62 |
| Kernel | 6.8.0-111-generic, GCC 12.3.0 |

## Key Benchmarks

### Compute (xdnn GEMM)

| Precision | Peak TFLOPS | Efficiency | Notes |
|-----------|:-----------:|:----------:|-------|
| FP16 | 22.7 | 39% | xdnn::fc, 2048³ peak |
| FP32 | 7.8 | 54% | xdnn::fc, 2048³ |
| INT8 | N/A | — | SDK API mismatch |

### PCIe Bandwidth

| Direction | BW | Efficiency | Notes |
|-----------|:----:|:----------:|-------|
| H2D (upload) | 5.6 GB/s | 35% | DMA engine hardware limit |
| D2H (download) | 3.6 GB/s | 23% | Read engine buffer limit |

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

## Comparison: K200 vs AMD MI50 (Radeon Pro VII)

| Metric | K200 | MI50 (gfx906) |
|--------|:----:|:------------:|
| FP16 GEMM | 22.7 TFLOPS | 11.5 TFLOPS |
| FP32 GEMM | 7.8 TFLOPS | 11.2 TFLOPS |
| PCIe H2D | 5.6 GB/s (4.0×8) | 26 GB/s (4.0×16) |
| Kernel Launch | ~2 µs | 8.16 µs |

K200 leads in FP16 throughput and launch latency (ideal for small-batch inference). MI50 leads in FP32 compute and PCIe bandwidth.

## Project Structure

```
.
├── xpu_perf_test.cpp          # Optimized benchmark (FP16/FP32/INT8 GEMM, D2D, launch)
├── test_p2p.cpp               # P2P DMA test (same-card PD→PD)
├── test_p2p_verify.cpp        # P2P data verification (reads back & checks)
├── xpu_denoise.cpp            # XPU denoising kernel
├── paddle_infer_benchmark.py  # PaddlePaddle ResNet-50 inference script
├── resnet50_benchmark.py      # Paddle model loading & warmup
├── run_perf_tests.sh          # Batch runner for all perf tests
├── run_paddle_benchmark.sh    # Paddle benchmark runner
├── results/                   # Test output logs & performance summaries
│   ├── PERF_SUMMARY.md
│   ├── K200_ANALYSIS_REPORT.md
│   └── *.txt                  # Raw device outputs
├── paddle_models/             # PaddlePaddle model archive
│   ├── inference.pdmodel
│   ├── inference.pdiparams
│   └── inference.pdiparams.info
├── xdnn-ubuntu_x86_64/        # xdnn SDK 2.0.0.725
│   ├── include/               # xdnn headers & kernel libs
│   ├── lib/                   # Static API library
│   ├── so/                    # Shared runtime (libxpurt.so.1)
│   └── plugin/                # xdnn plugin example
├── kunlun-driver/              # Kernel driver 4.33 + modifications
│   ├── docs/
│   │   ├── research/           # Architecture analysis
│   │   ├── plan/               # Iteration plans & risk assessment
│   │   └── impl/               # Code changes & test results
│   ├── kunlun_module/
│   │   └── kunlun/
│   │       ├── kl1/            # KL1 driver (K200) — modified
│   │       └── kl2/            # KL2 driver (R200/R300) — unchanged
│   └── kunlun.ko               # Compiled module (debug info retained)
├── .gitattributes              # LFS tracking rules
├── .gitignore
└── README.md
```

## Driver Modifications (kunlun-driver)

### What was changed

All changes are limited to KL1 code path (K200/K100 only — R200/R300 unaffected).

| File | Change |
|------|--------|
| `kl1/xpu_fops.c` | +35 lines: IOCTL_HOST_REGISTER, P2P dispatch, ioctl routing debug |
| `kl1/xpu_ioctl.c` | +100 lines: host_register stub, P2P handler, address logging |
| `kl1/xpu_dma.c` | +25 lines: dma_device_to_device_p2p function |
| `kl1/xpu_drv.h` | +7 lines: forward declarations, DMA buffer size experiment |

### What worked

- **xpu_host_register / unregister**: IOCTL routing fixed — returns 0 instead of -807
- **P2P IOCTL routing**: Command properly dispatched to handler after fixing `_IOC_TYPE` comparison bug
- **ioctl number debugging**: Logging at `kl_char_ioctl`, `xpu_char_ioctl`, `ctrldev_ioctl` levels

### What didn't work

- **Pinned memory bandwidth**: No improvement — KL1 DMA uses `copy_from_user` bounce buffer
- **DMA buffer size**: Increasing from 1MB→4MB had no effect — PCIe DMA engine is the bottleneck
- **xpu_host_alloc**: Returns -706 — requires mmap pgoff=0 support (not implemented for KL1)
- **P2P data transfer**: SSE DMA hangs when accessing cross-PD addresses — likely hardware-level PD boundary restriction

### What was NOT attempted

- SM clock frequency increase (firmware-locked at 900 MHz)
- PCIe link width change (K200 designed for ×8)
- INT8 GEMM (xdnn SDK API mismatch with available headers)

## Environment

| Component | Path |
|-----------|------|
| XPURT library | `/usr/local/xpu-4.33.0/lib64/libxpurt.so.1` |
| XPU driver (original) | `/lib/modules/6.8.0-111-generic/updates/dkms/kunlun.ko` |
| XPU driver (backup) | `/lib/modules/6.8.0-111-generic/updates/dkms/kunlun.ko.bak` |
| xdnn headers | `xdnn-ubuntu_x86_64/include/` |
| PaddlePaddle | `/usr/local/paddle/` |

## Build & Run

### GEMM benchmark
```bash
g++ -std=c++17 -O2 -o xpu_perf_test xpu_perf_test.cpp \
  -I xdnn-ubuntu_x86_64/include -I /usr/local/xpu-4.33.0/include \
  -L xdnn-ubuntu_x86_64/so -L /usr/local/xpu-4.33.0/lib64 \
  -lxpurt -lxpuapi -lpthread -lnuma \
  -Wl,-rpath,xdnn-ubuntu_x86_64/so -Wl,-rpath,/usr/local/xpu-4.33.0/lib64

./xpu_perf_test 2>&1 | tee results/compute_$(date +%Y%m%d).txt
```

### Paddle inference
```bash
python3 paddle_infer_benchmark.py --model paddle_models --batch_size 1
```

### Driver rebuild
```bash
cd kunlun-driver && make clean && make modules
# Then as root:
sudo modprobe -r kunlun
sudo cp kunlun.ko /lib/modules/$(uname -r)/updates/dkms/kunlun.ko
sudo depmod -a && sudo modprobe kunlun
```
