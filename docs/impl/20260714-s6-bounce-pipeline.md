# S6 Pageable Bounce 双缓冲流水线

**时间**: 2026-07-14 / 验证 2026-07-15  
**状态**: 已装载验证通过（ko `B8FE90CAFA106B0B5E95A2A`）  
**安全策略**: 默认可回退；任何时刻 `echo 0 > .../kl1_bounce_pipe` 即回串行旧路径

---

## 1. 目标与实测

| 路径 | S5 后 | S6 目标 | **实测 (pipe=1)** | A/B (pipe=0) |
|------|-------|---------|-------------------|--------------|
| pageable H2D @64MB | ~5.5 | 7–10+ | **9.95 GB/s** | 5.53 |
| pageable D2H @64MB | ~4.9 | 7–10+ | **7.70 GB/s** | 4.92 |
| pageable H2D @1GB | ~5.5 | — | **10.16 GB/s** | 5.58 |
| host_alloc H2D/D2H @64MB | ~12.5/12.9 | 不回归 | **12.5 / 12.9** | 12.5 / 12.9 |
| P2P @64MB | ~11.2 | 不回归 | **11.2** | — |

---

## 2. 安全设计（防崩溃 / 防卡死）

| 措施 | 说明 |
|------|------|
| 保留完整串行路径 | `dma_*_serial()` = 改前逻辑（仅 semaphore 改为 timeout） |
| `kl1_bounce_pipe=0` | 强制串行，**无需重装驱动** |
| 单 chunk（≤1MB） | 永不走 pipe，直接 serial |
| 第二 channel 不可用 | `down_trylock` 失败 → serial |
| kbuf / enable 检查失败 | 释放资源后 → serial |
| 失败 drain | 任何 error 前 `edma_*_wait` 排空 in-flight，避免 channel 半状态 |
| mutex 序 | 按 **channel 下标升序** 加锁，避免 AB-BA 死锁 |
| 不改 kbuf 大小 | 仍 1MB coherent，无额外大分配 |
| 不改 poll/udelay | 硬件时序与 S5 一致 |

回退：

```bash
echo 0 > /sys/module/kunlun/parameters/kl1_bounce_pipe
# 或装载时
sudo KL1_BOUNCE_PIPE=0 scripts/install_driver.sh
```

---

## 3. 算法

### H2D

```
copy_from_user(buf0); start_EDMA(buf0)
while remain:
  copy_from_user(buf1)     # CPU 与 buf0 的 EDMA 重叠
  wait_EDMA(buf0)
  start_EDMA(buf1)
  swap
wait last
```

### D2H

```
start_EDMA(buf0)
while remain:
  start_EDMA(buf1)         # 双 write channel 并发
  wait_EDMA(buf0)
  copy_to_user(buf0)       # CPU 与 buf1 的 EDMA 重叠
  swap
wait last; copy last
```

---

## 4. 修改文件

| 文件 | 变更 |
|------|------|
| `kl1/xpu_dma.c` | serial / pipe / 分发 |
| `kl1/xpu_drv.h` | `extern int kl1_bounce_pipe` |
| `kl_main.c` | `module_param(kl1_bounce_pipe)` 默认 1 |
| `scripts/install_driver.sh` | `KL1_BOUNCE_PIPE` |

---

## 5. 装载与验证

```bash
cd /mnt/storage/test_xpu
make driver
sudo KL1_P2P_STUB=0 KL1_DMA_DIRECT=1 KL1_BOUNCE_PIPE=1 scripts/install_driver.sh

# 参数
cat /sys/module/kunlun/parameters/kl1_bounce_pipe   # 1

# 正确性
./tests/test_p2p_verify
./tests/test_host_alloc
make regression

# 带宽（pageable 应高于 ~5.5；host_alloc 应仍 ≥8）
./benchmarks/xpu_perf_test 0 bw

# A/B：关 pipe 对比
echo 0 > /sys/module/kunlun/parameters/kl1_bounce_pipe
./benchmarks/xpu_perf_test 0 bw
echo 1 > /sys/module/kunlun/parameters/kl1_bounce_pipe
```

### 成功标准（已满足）

| 检查 | 期望 | 结果 |
|------|------|------|
| pageable 64MB pattern | PASS | **PASS** |
| p2p_verify / host_alloc | PASS | **PASS** |
| pageable 64MB H2D | ≥ 6.5 | **9.95** |
| host_alloc 64MB | ≥ 8 | **12.5** |
| P2P 64MB | ≥ 8 | **11.2** |
| pipe=0 回退 | ~5.5 无错误 | **5.53 / 4.92 PASS** |

---

## 6. 实测明细（2026-07-15）

### pageable（pipe=1 vs 0）

| Size | pipe=1 H2D/D2H | pipe=0 H2D/D2H |
|------|----------------|----------------|
| 1MB | 5.03 / 5.59 | 5.03 / 5.41（单 chunk → serial） |
| 64MB | **9.95 / 7.70** | 5.53 / 4.92 |
| 256MB | **10.11 / 7.85** | 5.53 / 4.91 |
| 1GB | **10.16 / 7.83** | 5.58 / 4.92 |

相对 serial：H2D **~1.8×**，D2H **~1.6×**。

### 不回归

- host_alloc 64–256MB：~12.5 / 12.9 GB/s  
- 同卡 P2P：~11.2–11.3 GB/s  

---

## 7. 风险

| 风险 | 等级 | 缓解 |
|------|:----:|------|
| 双 channel 状态机错误 | 中 | drain + serial fallback + param；实测 PASS |
| 并发占满 channel | 低 | put 后释放；第二 ch trylock |
| 1MB 无提升 | 预期 | ≤1MB 强制 serial |

ko：`B8FE90CAFA106B0B5E95A2A`
