# KL1 (K200) vs KL2 (R200/R300) 能力差距架构分析

**时间**: 2026-06-12  
**目的**: 为软件补救方案提供架构依据，明确哪些 KL2 能力可移植到 KL1。

> ⚠️ **架构调研快照（补救前）**。文中「未实现」描述的是 **2026-06 改造前** 的 KL1 驱动能力。  
> 补救后：`xpu_host_alloc` mmap / 同卡 P2P host-staging / 直传 EDMA 等已落地 — 见 `docs/impl/20260715-project-closure.md`。  
> 硬件边界（无跨卡 P2P、无 INT8 CDNN、900 MHz）仍然成立。

---

## 1. 硬件拓扑

```
Host (x86_64 server CPU)
  │
  ├── PCIe Gen4 ×8 ── K200 Card #0 (PCI ID 1d22:3684)
  │     └── xpu_device
  │           ├── PD0 (/dev/xpu0)  rbase=0x00000000  HBM 0–8 GB
  │           └── PD1 (/dev/xpu1)  rbase=0x40000000  HBM 8–16 GB
  │
  └── PCIe Gen4 ×8 ── K200 Card #1 (same PCI ID; BDF machine-local)
        └── xpu_device
              ├── PD0 (/dev/xpu2)
              └── PD1 (/dev/xpu3)
```

两 PD 共享同一 PCIe 设备的 BAR 空间，但驱动按 8 GB 分区，无共享内存池。

---

## 2. IOCTL 分发架构

```
用户态 XPURT 库
    │
    ▼
/dev/xpuN 或 /dev/xpuctrl
    │
    ├── KL1 路径 (K200): xpu_char_ioctl → xpu_fops.c dispatch
    │     原始: 无 HOST_REGISTER / P2P
    │     修改后: _IOC_NR 拦截 → ioctl_host_register_kl1 / ioctl_memcpy_p2p_kl1
    │
    └── KL2 路径 (R200+): kl2_ioctl → kl2/ioctl.c dispatch
          完整: HOST_REGISTER, HOST_ALLOC(mmap), MEMCPY_P2P, MEMCPY_P2P_DIRECT
```

---

## 3. DMA 子系统对比

| 能力 | KL1 (K200) | KL2 (R200+) |
|------|------------|-------------|
| H2D 路径 | `copy_from_user` → 1MB kbuf → EDMA → HBM | SG-DMA / dma_map → EDMA → GDDR |
| D2H 路径 | HBM → EDMA → 1MB kbuf → `copy_to_user` | 反向 SG-DMA |
| D2D (同 PD) | SSE DMA (ssedma @ RDMA_BASE) | 内部 DMA 引擎 |
| D2D (跨 PD) | **SSE DMA hang**（当前实现） | `kl2_dma_peer_to_peer` (host-staging) |
| 跨卡 P2P | 不支持 | `kl2_map_memcpy_p2p_direct` (PCIe BAR IOVA 映射) |
| Bounce buffer | 固定 1MB (`KL1_DMA_KBUF_SIZE`) | 可配置 (`KL2_DMA_KBUF_SIZE`) |
| EDMA 通道 | 3 read + 3 write / PD | 多通道 + ping-pong |

### 3.1 KL1 EDMA 数据流（当前瓶颈）

```
H2D:  userspace buf ──copy_from_user──► edma->kbuf (1MB) ──EDMA──► HBM
D2H:  HBM ──EDMA──► edma->kbuf (1MB) ──copy_to_user──► userspace buf
```

无论 host_register 是否成功，CPU 拷贝不可避免（除非改造为 dma_map 直传）。

### 3.2 KL2 P2P 数据流（可移植方案）

```
PD_src HBM ──EDMA(D2H)──► src bounce buf (1MB)
                              │
                              ▼  (ping-pong)
                         dst bounce buf (1MB) ──EDMA(H2D)──► PD_dst HBM
```

关键函数：`kl2_dma_peer_to_peer()` @ `kl2/dma.c:574`

- 使用两个设备的 DMA 引擎
- ping-pong 双缓冲重叠传输
- 带宽受 PCIe 双向限制，但功能正确

---

## 4. mmap / host_alloc 对比

| 项目 | KL1 | KL2 |
|------|-----|-----|
| `xpu_char_mmap` / `kl2_mmap` | **stub (`return 0`)** | 完整实现 |
| pgoff=0 语义 | 未实现 | `xpu_host_alloc` |
| 实现方式 | — | hugepage 分配 + `vm_insert_page` |
| fops 注册 | **无 `.mmap`** | 有 `.mmap = kl2_mmap` |

### KL1 当前代码

```c
// kl1/xpu_fops.c
int xpu_char_mmap(struct file *file, struct vm_area_struct *vma) {
    return 0;  // 不映射任何页面
}

struct file_operations xpu_fops = {
    .owner          = THIS_MODULE,
    .open           = xpu_char_open,
    .release        = xpu_char_release,
    .unlocked_ioctl = xpu_char_ioctl,
    // 缺少 .mmap
};
```

---

## 5. SSE DMA 跨 PD 问题

### 寄存器布局

```
PD0: ssedma.base = RDMA_BASE
PD1: ssedma.base = PD1_OFFSET + RDMA_BASE
```

每个 PD 有独立 SSE DMA 引擎和 channel 0。

### 当前 P2P 实现

```c
// kl1/xpu_dma.c — dma_device_to_device_p2p
mutex_lock(&src_xpd->ssedma.lock);
ret = xpuhw_ssedma_locked(&src_xpd->ssedma, dst_bar, src_bar, sz, cycles);
```

`xpuhw_ssedma_locked` 编程 DMA_KT_GM2GM，写入 64-bit 绝对 BAR 地址。当 `dst_bar` 落在 PD1 区域时，PD0 的 ssedma 无完成中断。

### 可能原因

1. SSE DMA 地址解码限定在本 PD 的 HBM 窗口
2. 需要切换 ssedma 引擎到目标 PD（当前只用源 PD 引擎）
3. 需要额外 BAR window 配置（KL2 有 `p2p_init` / IOVA 映射逻辑）

---

## 6. INT8 计算路径

```
xdnn::fc<TX,TW,TY,TGEMM>  ──模板实例化──► libxpuapi.so
                              │
                              ├── FP16/FP32: 已实例化 ✅
                              └── INT8/INT8/INT8/INT8: 未实例化 ❌

fc_int8() ──C API──► libxpuapi (符号存在，头文件未公开)
  └── fc_int8_internal<float16, ...> 等混合精度变体
```

INT8 计算能力在固件/SDK 内部存在，但公开 API 层未暴露完整模板。

---

## 7. 可移植性矩阵

| KL2 特性 | 移植到 KL1 难度 | 建议 |
|----------|:---------------:|------|
| P2P host-staging | **低** | 优先实施 |
| host_alloc mmap | **中** | 简化版先行 |
| P2P PCIe direct | **高** | K200 无对应硬件接口 |
| SG-DMA 直传 | **高** | 需重构 EDMA 路径 |
| INT8 fc_int8 API | **低** | 仅需签名探测 |

---

## 8. 推荐架构演进

```
Phase 1 (当前)
  KL1: IOCTL 路由修复 → P2P/SSE hang

Phase 2 (计划)
  KL1: P2P → host-staging (移植 kl2_dma_peer_to_peer 模式)
  KL1: mmap pgoff=0 → xpu_host_alloc 可用

Phase 3 (可选)
  INT8: fc_int8 API 探测
  KL1: EDMA dma_map 直传（若 Phase 2 验证有收益）
```