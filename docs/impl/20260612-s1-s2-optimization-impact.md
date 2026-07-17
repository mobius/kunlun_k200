# S1/S2 修复带来的优化影响评估

**时间**: 2026-06-12  
**前置**: `docs/impl/20260612-s1-p2p-fix-and-s2-mmap.md`（实施与验证记录）

> ⚠️ **阶段性快照**：本文写于 S1/S2 刚完成后。其中「P2P ~2.5 GB/s」「host 零拷贝 DMA 未实现（S4 待做）」已被后续阶段取代：  
> S4 host_alloc ~12.5/12.9、S5 P2P ~11.2、S6 pageable ~10/7.8 — 见 `docs/impl/20260715-project-closure.md`。

---

## 1. 结论摘要

本次修复的**主要收益是能力解锁与稳定性**，而非单卡算子或 DMA 路径的带宽提升。

| 维度 | 影响等级 | 说明 |
|------|:--------:|------|
| 功能可用性 | ★★★★★ | P2P、host_alloc/free 从不可用变为可用 |
| 跨设备吞吐 | ★★★☆☆ | 同卡 P2P ~2.5 GB/s，够用但远低于本地 D2D |
| 单卡 H2D/D2H/D2D | ☆☆☆☆☆ | 未改动既有路径 |
| host 零拷贝 DMA | ☆☆☆☆☆ | 未实现（S4 待做） |
| 硬件直传 P2P | ☆☆☆☆☆ | 当前为 host staging，非设备间直传 |
| 稳定性 / 可运维 | ★★★★☆ | 消除 P2P hang、驱动可安全重装 |

---

## 2. S1 P2P：从 hang 到 ~2.5 GB/s

### 修复前

- `xpu_memcpy_peer` / `IOCTL_MEMCPY_P2P(_DIRECT)` **永久 hang**
- 超时杀进程后模块 `refcnt` 飙高，需重启才能重装驱动
- 多卡 pipeline **完全不可用**

### 修复后（实测，kernel 6.8.0-124）

| 路径 | 带宽（约） | 备注 |
|------|-----------|------|
| 本地 D2D（dev0 内） | ~62 GB/s | 大块基线，未变 |
| P2P（PD0→PD1，同卡） | ~2.5 GB/s | `test_p2p` 1–512MB |
| userspace D2H+H2D 分步 | 可用 | 与 kernel staging 同量级 |

### 实现本质

`kl1_dma_peer_to_peer` 使用 **kvmalloc 中转**：

```
源设备 --D2H--> 内核 bounce --H2D--> 目标设备
```

与 userspace 分步拷贝等价，**不是** PCIe/CCIX 设备间直传。相对 KL2 的 `kl2_dma_peer_to_peer`（可走硬件 P2P 引擎），KL1 当前是**软件兜底路径**。

### 对业务的影响

- **正面**：多卡间 tensor 搬运、权重广播、pipeline 分 stage 等场景从 0 变为可用；16MB 校验 ~280ms 量级可接受。
- **局限**：大块跨 PD 拷贝约为本地 D2D 的 **1/25**；若模型 I/O 绑在多卡同步上，仍可能成为瓶颈。
- **未优化**：单卡 EDMA、SSE launch 等路径性能不变。

---

## 3. S2 host_alloc：API 补齐，DMA 加速尚未兑现

### 修复前

| API | 结果 |
|-----|------|
| `xpu_host_alloc` | -706（mmap stub） |
| `xpu_host_free` | -707（unregister 未回写 size） |

### 修复后

| API | 结果 |
|-----|------|
| `xpu_host_alloc` 64MB | `ret=0`，用户态可读写 |
| `xpu_host_free` | `ret=0`，`munmap` 正常 |

### 实现本质

- `kl1_mmap_host_alloc`：`alloc_page` + `vm_insert_page`，`VM_LOCKED`
- 相对 KL2 hugepage 方案更轻量，适合 KL1 无 hugetlb 基础设施的环境

### DMA 路径现状

`ioctl_host_register_kl1` 仍为 **stub**：KL1 EDMA 继续走 `copy_from_user` / `copy_to_user` bounce buffer，**未**把 `host_alloc` 页 pin 进 EDMA 描述符。

因此：

- **已解锁**：Runtime / 上层可合法使用 `xpu_host_alloc` 语义（对齐、生命周期、驱动 mmap）
- **未加速**：`host_alloc` 内存的 H2D/D2H 带宽预期与 `malloc` + 普通 `xpu_memcpy` 相近
- **后续**：S4 pinned DMA 才有希望减少一次 host 拷贝

---

## 4. 稳定性与可运维性

| 项 | 修复前 | 修复后 |
|----|--------|--------|
| P2P ioctl | hang，需 SIGKILL | 正常返回 |
| 模块 refcount |  stuck 后 ~48 | 正常卸载（refcnt=0） |
| 驱动迭代 | 手动 cp ko + modprobe | `scripts/install_driver.sh` 带清理与校验 |
| 调试 | 无 | `kl1_p2p_stub=1` 可隔离 ioctl vs DMA（仅调试） |

---

## 5. 后续性能工作（未包含在本提交）

按 `docs/plan/20260612-remediation-iteration-plan.md`：

| 阶段 | 目标 | 预期收益 |
|:----:|------|----------|
| S3 | INT8 性能探测 | 算子精度/吞吐（与本次驱动修复无关） |
| S4 | Pinned DMA 直传 | **暂缓** — S2.4 证实 host_alloc 带宽无提升；见 `20260613-s2-host-alloc-bandwidth.md` |
| （远期） | KL1 硬件 P2P / SSE-EDMA | 跨 PD 带宽从 ~2.5 GB/s 向 D2D 量级靠拢 |

---

## 6. 一句话

> 本次提交让 K200（KL1）具备了与 XPURT 4.33 对齐的关键 API（P2P、host_alloc），多卡数据通路从「不能用」变为「能用、约 2.5 GB/s」；**没有**显著加速单卡算子，也**没有**实现 host 零拷贝或硬件直传 P2P。