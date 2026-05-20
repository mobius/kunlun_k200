# K200 XPU 驱动调优 — Plan 阶段

**时间**: 2026-05-15

## 1. 修改策略

所有修改限制在 KL1 代码路径（不影响 KL2/R200）:
- `kunlun_module/kunlun/kl1/xpu_fops.c` — IOCTL dispatch 表
- `kunlun_module/kunlun/kl1/xpu_ioctl.c` — handler 实现
- `kunlun_module/kunlun/kl1/xpu_dma.c`  — DMA 函数
- `kunlun_module/kunlun/kl1/xpu_drv.h`  — 前向声明

## 2. 迭代计划

### Iteration 1: host_register 通路打通 ✓

| 步骤 | 描述 | 状态 |
|------|------|:----:|
| 1.1 | 在 `xpu_char_ioctl` 加 IOCTL_HOST_REGISTER/UNREGISTER case | ✓ |
| 1.2 | 实现 stub handler（返回 0，写 LOGI 日志） | ✓ |
| 1.3 | 编译 → 替换模块 → 测试 | ✓ |

**预期**: `xpu_host_register` 返回 0，带宽不变。
**结果**: ✓ 通过，0 返回，带宽 5.66=5.65 GB/s。

### Iteration 2: DMA buffer 调优 ✗

| 步骤 | 描述 | 状态 |
|------|------|:----:|
| 2.1 | KL1_DMA_KBUF_SIZE: 1MB → 4MB | ✓ |
| 2.2 | 编译 → 替换 → 多尺寸带宽测试 | ✓ |

**预期**: 可能减少 bounce buffer 往返次数。
**结果**: ✗ 无效。1MB–1GB 全范围带宽不变。瓶颈在 PCIe DMA 引擎硬件。

### Iteration 3: P2P DMA（同卡 PD 间）— 部分成功

| 步骤 | 描述 | 状态 |
|------|------|:----:|
| 3.1 | 添加 `dma_device_to_device_p2p` 函数 | ✓ |
| 3.2 | 添加 `ioctl_memcpy_p2p_kl1` handler | ✓ |
| 3.3 | 在 dispatch 表加 IOCTL_MEMCPY_P2P case | ✓ |
| 3.4 | 发现库使用 P2P_DIRECT (cmd 146)，补上 | ✓ |
| 3.5 | 发现库使用不同 struct size(3837)，改按 _IOC_NR 匹配 | ✓ |
| 3.6 | 修复 _IOC_TYPE 比较 bug | ✓ |
| 3.7 | SSE DMA hang — PD0→PD1 硬件无响应 | ✗ |

**预期**: 同卡 PD 间 ~60 GB/s P2P 带宽。
**结果**: IOCTL 通路打通（命令正确路由到 handler），但 SSE DMA 硬件不接受 PD0→PD1 的跨域地址。需要进一步排查 BAR 边界或硬件寄存器级限制。

### Iteration 4: _IOC_TYPE 匹配修复

发现 `_IOC_TYPE(cmd) == IOCTL_IOC_MAGIC` 直接把 8-bit 提取值和 24-bit 常量比较，永远 false。修正为 `_IOC_TYPE(cmd) == _IOC_TYPE(IOCTL_IOC_MAGIC)`。

## 3. 风险评估

| 风险 | 等级 | 缓解 |
|------|:----:|------|
| 模块崩溃 | 低 | MODVERSIONS 验证，git revert 可回退 |
| DMA hang | 中 | 只影响调用进程，smi 可检测，重载模块恢复 |
| 影响其他设备 | 极低 | 修改仅限 KL1 路径，KL2/R200 不受影响 |
| 数据损坏 | 低 | 修改不涉及写操作逻辑 |

## 4. 未解决问题（留档）

1. **P2P SSE DMA hang**: PD0 的 ssedma 不接受 PD1 的 BAR 地址。可能原因:
   - SSE DMA 硬件有 PD 边界限制
   - BAR window 配置不支持跨域访问
   - 需要额外硬件配置（参考 KL2 的 p2p_init 逻辑）

2. **xpu_host_alloc (-706)**: 需要实现 KL1 的 mmap pgoff=0 路径，参考 KL2 的 `kl2_mmap_host_alloc`
