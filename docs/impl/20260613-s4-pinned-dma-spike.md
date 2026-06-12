# S4 Pinned DMA Spike

**时间**: 2026-06-13  
**状态**: 已并入正式 S4 → `docs/impl/20260613-s4-pinned-dma.md`

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

### 3.1 Hugepage 实验（2026-06-12）

`kl1_host_mem.c` 改为按 **2MB**（`HPAGE_PMD_ORDER`）`alloc_pages` 分配，尾块用更小 order 补齐（同 KL2 分解算法）。`kl1_host_alloc_get_span` 返回块内连续字节；direct EDMA 每段最长 `min(1MB, chunk_remain)`。

| 64MB 传输 | 4KB 页 | 2MB hugepage |
|-----------|--------|--------------|
| EDMA+map 次数/次 memcpy | ~16384 | ~64 |

装驱动后测：`sudo scripts/install_driver.sh && ./scripts/run_s4_hugepage_test.sh`

**结果**（ko `D54DEEFC91174B3BF772916`，2026-06-12）：

| 尺寸 | bounce (=0) H2D/D2H | huge+direct (=1) H2D/D2H |
|------|---------------------|--------------------------|
| 1MB | 5.35 / 5.48 | **11.42 / 11.64** |
| 64MB | 5.53 / 4.87 | **11.91 / 12.25** |
| 256MB | 5.53 / 4.89 | **11.87 / 12.23** |
| 1GB | 5.61 / 4.95 | 5.56 / 4.94 * |

\* 1GB @ =1 仍失败/回退（512×2MB chunk，semaphore 超时）；≤256MB 有效。

**结论**：跳过 bounce `memcpy` 后带宽约 **2×**（~12 GB/s），hugepage 实验成功；正式 S4 值得做，需修 1GB 超时。

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

## 5. 结果

**测试**: `./scripts/run_s4_spike.sh`（ko `E5990ABFF83A1B977C52C12`，2026-06-12）

| kl1_dma_direct | 64MB H2D | 64MB D2H | 备注 |
|:--------------:|:--------:|:--------:|------|
| 0 | 5.54 GB/s | 4.89 GB/s | host_alloc ≈ pageable（bounce） |
| 1 | 0.44 GB/s | 0.69 GB/s | 直传可用，约 **12× 慢** |

host_alloc 全尺寸对比：

| 尺寸 | =0 H2D / D2H | =1 H2D / D2H |
|------|--------------|--------------|
| 1MB | 5.38 / 5.55 | 0.43 / 0.69 |
| 64MB | 5.54 / 4.89 | 0.44 / 0.69 |
| 256MB | 5.54 / 4.90 | 0.44 / 0.69 |
| 1GB | 5.63 / 4.97 | 5.61 / 4.96 * |

\* **1GB @ =1 数值不可信**：`xpu_perf_test` 不检查 `xpu_memcpy` 返回值；1GB 直传需 ~26 万次按页 EDMA，易触发超时/失败，计时接近 0 会算出虚高带宽。小尺寸结果有效。

### 结论

| 项 | 结果 |
|----|------|
| 功能 | ✅ 直传路径工作，无 hang（≤256MB） |
| 带宽 | ❌ 远低于 bounce（符合 spike 预期：非连续页 → 每 4KB 一次 EDMA） |
| 决策 | **归档 S4 spike**；若继续需 hugepage/连续 pfn，非本 spike 范围 |

**切换方式**（装新驱动后）：

- `echo 0\|1 > /sys/module/kunlun/parameters/kl1_dma_direct`（install 脚本 chmod 666）
- `IOCTL_TEST` arg=`5 \| (val<<8)` on `/dev/xpu0`