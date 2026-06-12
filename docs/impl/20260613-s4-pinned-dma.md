# S4 Pinned DMA（正式）

**时间**: 2026-06-13  
**状态**: 已实现，默认开启  
**前置**: S2 `xpu_host_alloc` mmap、spike → `docs/impl/20260613-s4-pinned-dma-spike.md`

---

## 1. 目标

`xpu_host_alloc` 缓冲区 **跳过 bounce `copy_from/to_user`**，用 **2MB hugepage** + `dma_map_page` + EDMA 直传。

---

## 2. 实现

| 组件 | 说明 |
|------|------|
| `kl1/kl1_host_mem.c` | 2MB `alloc_pages` 分配；失败时递归拆成更小 order；`kl1_host_alloc_get_span` |
| `kl1/xpu_dma.c` | `kl1_dma_direct=1` 时对 host_alloc 区间走 `dma_*_direct`（每段最长 1MB EDMA） |
| `kl_main.c` | `kl1_dma_direct` **默认 1**；`0` 回退 bounce |
| `scripts/install_driver.sh` | 默认 `KL1_DMA_DIRECT=1` |

路径：

```
xpu_host_alloc → mmap (2MB chunks)
xpu_memcpy(host_alloc) → dma_map_page(compound head) → edma_* → dma_unmap
```

非 host_alloc（pageable）仍走 bounce。

---

## 3. 带宽（ko D54DEEFC+，Device 0）

| 尺寸 | bounce (=0) | S4 direct (=1) |
|------|-------------|----------------|
| 64MB H2D / D2H | ~5.5 / ~4.9 GB/s | **~12 / ~12 GB/s** |
| 256MB | ~5.5 / ~4.9 | **~12 / ~12** |

约 **2×** 于 bounce（消除 memcpy 瓶颈）。

1GB @ direct：曾测 ~5.5 GB/s（疑似 huge 耗尽后走 bounce 或传输失败）；已加 alloc 回退与 u64 区间检查，装新 ko 后复测。

---

## 4. 运维

```bash
# 默认 S4 开启
sudo scripts/install_driver.sh

# 临时关闭直传（调试）
echo 0 > /sys/module/kunlun/parameters/kl1_dma_direct

# 或装驱动时
sudo KL1_DMA_DIRECT=0 scripts/install_driver.sh

make regression
```

---

## 5. 回滚

```bash
echo 0 > /sys/module/kunlun/parameters/kl1_dma_direct
# 或
sudo KL1_DMA_DIRECT=0 scripts/install_driver.sh
```

---

## 6. 已知限制

- 需足够 **2MB hugepage** 连续内存；不足时自动拆小页（带宽下降）
- `KL1_DMA_KBUF_SIZE=1MB` 限制单次 EDMA 段长
- 1GB 单块分配压力最大，建议生产优先 ≤256MB pinned 缓冲