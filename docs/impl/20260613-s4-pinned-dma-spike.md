# S4 Pinned DMA Spike

**时间**: 2026-06-13  
**状态**: 代码就绪，待装驱动后测带宽

---

## 1. 目标

验证 KL1 能否对 `xpu_host_alloc` 页 **跳过 `copy_from/to_user` bounce**，用 `dma_map_page` + EDMA 直传。

模块参数：`kl1_dma_direct=1`（默认 0）。

---

## 2. 实现摘要

| 文件 | 变更 |
|------|------|
| `kl1/kl1_host_mem.c` | `kl1_user_range_is_host_alloc`, `kl1_host_alloc_get_page` |
| `kl1/xpu_dma.c` | `dma_*_direct`：`dma_map_page` → `xpuhw_edma_*` |
| `kl_main.c` | `module_param(kl1_dma_direct)` |
| `scripts/install_driver.sh` | `KL1_DMA_DIRECT=1` 加载 |

路径（仅 host_alloc 区间）：

```
H2D: dma_map_page(page) → edma_read(dev ← bus_addr) → dma_unmap
D2H: dma_map_page(page) → edma_write(bus_addr ← dev) → dma_unmap
```

非 host_alloc 仍走原 bounce buffer。

---

## 3. 限制（spike 已知）

- `host_alloc` 每页 `alloc_page`，**物理不连续** → 每段最多一页（~4KB）一次 EDMA
- 1MB 旧路径 = 1 次 EDMA + 1 次 memcpy；直传 = 最多 256 次 EDMA
- **可能更慢**，spike 用于验证「能否工作」和「量级」，不是最终优化

若直传可用但慢 → 正式 S4 需 hugepage 或合并连续 pfn。

---

## 4. 测试步骤

```bash
cd /mnt/storage/test_xpu
make driver
sudo KL1_DMA_DIRECT=1 KL1_P2P_STUB=0 scripts/install_driver.sh

# A/B 带宽对比
./scripts/run_s4_spike.sh

# 回归（P2P + host_alloc 不被破坏）
make regression

# 内核日志
sudo dmesg | grep 'S4 '
```

### 成功标准（spike）

| 项 | 期望 |
|----|------|
| `kl1_dma_direct=1` 时 host_alloc bw 测试 | 不 hang、不 -EFAULT |
| dmesg | 出现 `S4 H2D direct` / `S4 D2H direct` |
| vs `=0` | 记录 H2D/D2H GB/s，判断是否有提升 |

### 决策

| 结果 | 下一步 |
|------|--------|
| 失败（DMA error / hang） | 放弃 S4；EDMA 仅支持 coherent bounce |
| 成功但带宽 ≤ bounce | 归档；需 hugepage 才值得继续 |
| 成功且带宽明显提升 | 立项正式 S4 + hugepage host_alloc |

---

## 5. 结果（待填）

| kl1_dma_direct | 64MB H2D | 64MB D2H | 备注 |
|:--------------:|:--------:|:--------:|------|
| 0 | | | |
| 1 | | | |