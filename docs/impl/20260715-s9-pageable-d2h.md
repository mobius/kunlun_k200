# S9 Pageable D2H 优化实验

**时间**: 2026-07-15  
**状态**: 已装载验证；**带宽目标未达成**  
**ko**: `1BF517814DF547139CD5FCE`  
**前置**: S6 bounce pipeline

---

## 1. 目标 vs 结果

| 指标 | 目标 | 实测 (final) |
|------|------|----------------|
| pageable D2H @64MB | ≥ 9–10 GB/s | **7.77 GB/s**（与 S6 持平） |
| pageable H2D @64MB | 不回退 | **10.16 GB/s** ✓ |
| host_alloc / P2P | 不回归 | 12.5/12.9 · 11.2 ✓ |
| 正确性 | PASS | **全 PASS** · `make regression` 绿 |

**结论：S9 未抬高 pageable D2H。** 生产上要高 D2H 仍应用 **`xpu_host_alloc`（S4 ~12.9 GB/s）**。

---

## 2. 实验矩阵

| 尝试 | 假设 | 结果 |
|------|------|------|
| **A. D2H single-issue**（`kl1_bounce_d2h=1`） | 双 write EDMA 争用 | D2H 7.75 vs S6 双并发 7.68 → **无效** |
| **B. pin + 直传 EDMA**（`kl1_pageable_pin=1`） | 去掉 bounce CPU 拷贝 | `malloc` 4K 不连续 → **每页 EDMA，极慢/卡死**；默认 **关** |
| **C. cached bounce** | coherent 缓冲拖慢 `copy_to_user` | `__get_free_pages`+map+sync → D2H 仍 **7.7** → **本机无效** |

本机 EPYC 上 `dma_alloc_coherent` 与 cached map 对 pageable 吞吐等价；D2H 剩余瓶颈更可能是：

1. **EDMA write→host bounce + `copy_to_user` 的不可完全重叠**（稳态接近两者较差者）  
2. **1MB 分片 + poll 固定开销**  
3. 与 H2D 非对称的硬件/路径特性  

继续抠 pageable D2H 的性价比低；应用侧 **pinned（host_alloc）** 已解决。

---

## 3. 仍合入代码（工程价值，非吞吐跃迁）

| 项 | 说明 |
|----|------|
| `kl1_bounce_d2h` | 1=single-issue / 2=S6 dual-conc，便于 A/B |
| `kl1_pageable_pin` | **默认 0**；可选 pin 路径（大页/连续内存才可能有用） |
| cached bounce 分配 | free_pages + dma_map_single + sync（行为正确，吞吐≈原 coherent） |
| 失败 drain / 回退 | pin 失败 → bounce；pipe=0 → serial |

---

## 4. 参数

```bash
# 默认生产（本机实测）
kl1_bounce_pipe=1
kl1_bounce_d2h=1
kl1_pageable_pin=0   # 切勿对普通 malloc 开 1
kl1_dma_direct=1
```

---

## 5. 应用建议（不变）

1. **需要 ≥10 GB/s D2H → `xpu_host_alloc`**  
2. pageable 用 S6 即可（H2D ~10 / D2H ~7.8）  
3. 算力侧优先 FP16（S8）

---

## 6. 回归摘要（2026-07-15）

```
pageable  64MB  H2D=10.16 D2H=7.77  (pipe=1, pin=0)
host_alloc 64MB H2D=12.49 D2H=12.88
P2P        64MB 11.22 GB/s
All regression checks passed
```
