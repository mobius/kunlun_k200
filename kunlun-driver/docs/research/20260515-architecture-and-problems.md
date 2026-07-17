# K200 XPU 驱动调优 — Research 阶段

**时间**: 2026-05-15

> ⚠️ 早期调研档案。后续完整补救与结案见仓库根目录 `docs/impl/20260715-project-closure.md`。

## 1. 背景

2× K200 推理卡（每卡 2 PD = 4 个 /dev/xpu），xdnn SDK 2.0.0.725，驱动 4.33。

## 2. 架构分析

### 2.1 K200 硬件拓扑

```
K200 Card (PCIe Gen4 ×8, DeviceID 0x3684)
  └── xpu_device (1 个/卡)
       ├── PD0 (/dev/xpu0) — rbase=0x0, BAR 0-16GB
       └── PD1 (/dev/xpu1) — rbase=0x40000000, BAR 16-32GB
```

两个 PD 共享同一个 PCIe BAR，在同一物理芯片上，但驱动将 HBM 按 8GB/PD 分区，无共享内存。

### 2.2 KL1 vs KL2 驱动架构

K200 走 KL1 路径（subsys_vendor=0, subsys_device=0 → `kl_info_table[KL1]`）。
KL2 路径服务于 R200/R300 等更新款卡。

```
KL1: xpu_char_ioctl → xpu_fops.c (KL1 ioctl dispatch)
KL2: kl2_ioctl       → kl2/ioctl.c (KL2 ioctl dispatch)
```

两个 dispatch 表内容不同：KL2 有 `IOCTL_HOST_REGISTER`/`P2P`，KL1 没有。

### 2.3 KL1 DMA 架构

```
H2D: copy_from_user → 1MB DMA bounce buffer → PCIe EDMA → HBM
D2H: HBM → PCIe EDMA → 1MB DMA bounce buffer → copy_to_user
D2D: SSE DMA Engine (64-bit address, within single PD only)
```

Bounce buffer 尺寸: `KL1_DMA_KBUF_SIZE = 1 * 1024 * 1024` (1MB)，EDMA 通道: 3/PD。

### 2.4 SSE DMA (内部 D2D)

- 每个 PD 有独立的 ssedma 引擎
- 通过 64-bit 绝对 BAR 地址编程
- 当前只支持 PD 内部传输

## 3. 问题诊断

### 3.1 xpu_host_register 返回 -807

**根因**: K200 走 KL1 路径，`xpu_char_ioctl` 没有 `IOCTL_HOST_REGISTER` case，落入 `default: return -XPUERR_NOIOC`。

**修复**: 在 `xpu_char_ioctl` 加 HOST_REGISTER/UNREGISTER case + stub handler。

### 3.2 Pinned memory 无带宽提升

**根因**: KL1 的 H2D/D2H 永远经过 CPU `copy_from_user/to_user`。pin 内存无法绕过 CPU 拷贝。

**验证**: KL1_DMA_KBUF_SIZE 从 1MB→4MB，PCIe 带宽不变（~5.6 GB/s H2D / ~3.6 GB/s D2H）。

### 3.3 xpu_host_alloc 返回 -706

**根因**: `xpu_host_alloc` 走 mmap 路径 (vm_pgoff=0)，KL1 的 `xpu_char_mmap` 不支持此路径。

### 3.4 P2P memcpy 返回 -807

**根因**: KL1 dispatch 缺少 `IOCTL_MEMCPY_P2P` 和 `IOCTL_MEMCPY_P2P_DIRECT`。
但修复后 SSE DMA hang — PD0 的 ssedma 访问 PD1 地址时硬件无响应。

### 3.5 性能基线

| 指标 | 实测 | 理论 | 效率 | 瓶颈 |
|------|:------:|:----:|:---:|------|
| FP16 GEMM | 22.7 TFLOPS | 57.6 | 39% | xdnn SDK/频率 |
| FP32 GEMM | 7.8 TFLOPS | 14.4 | 54% | xdnn SDK/频率 |
| PCIe H2D | 5.6 GB/s | 15.8 | 35% | DMA 引擎硬件 |
| PCIe D2H | 3.6 GB/s | 15.8 | 23% | 读引擎 buffer |
| Paddle ResNet | 1069 img/s | — | — | FP32, BS=1 |
