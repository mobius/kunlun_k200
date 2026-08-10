# S5 P2P 零拷贝 + Ping-Pong + IOCTL 修复

**时间**: 2026-07-14  
**状态**: 已装载验证通过（ko `9A0E11148CC5A8E441F57AA`）  
**前置**: S1 P2P host-staging、S4 pinned DMA

---

## 1. 目标与实测

| 项 | 改前 | S5 目标 | **实测** |
|----|------|---------|----------|
| 同卡 P2P @64MB | ~2.5 GB/s | ≥4 GB/s | **11.2 GB/s** |
| 同卡 P2P @256–512MB | ~2.5 GB/s | — | **11.3 GB/s** |
| 内核 P2P ioctl | 多数未命中 | 命中 | **DIRECT 命中**（nr=146） |
| host_alloc H2D/D2H @64MB | ~5.5 / 4.9 | ≥8 | **12.5 / 12.9 GB/s** |
| pageable H2D/D2H | ~5.5 / 4.9 | 不变 | 5.5 / 4.9 |

---

## 2. 根因：P2P ioctl 从未进入内核路径

`xpu_fops.c` 中错误比较：

```c
// 错误：IOCTL_IOC_MAGIC = 0x8EEDCE
// _IOC_TYPE(MAGIC) == 0xED
// 而 _IOWR(MAGIC, ...) 编码后 type 字段为 0xCE
if (_IOC_TYPE(cmd) == _IOC_TYPE(IOCTL_IOC_MAGIC)) { ... }
```

结果：NR 拦截永不命中；若 userspace/kernel 的 `_IOWR` size 位也不一致，`switch` 同样失败 → **807 Unknown IOCTL**。  
XPURT 回退用户态 D2H+H2D，测得 ~2.5 GB/s，数据校验仍 PASS。

### 修复

按 `_IOC_NR(cmd)` 拦截 `MEMCPY_P2P` / `MEMCPY_P2P_DIRECT` / `HOST_REGISTER` / `HOST_UNREGISTER`，不再用 bare magic 做 type 比较。

---

## 3. P2P 路径重写（V6）

### 旧路径（V5）

```
for each 1MB:
  EDMA D2H → edma.kbuf → memcpy → kvmalloc staging
  memcpy → edma.kbuf → EDMA H2D
```

- 全量 `kvmalloc(sz)`（大块压力大）
- 每块 **2 次 CPU memcpy**
- 串行 D2H 然后 H2D
- 热路径 `LOGI` 刷屏

### 新路径（V6）

```
buf0/buf1 = 已有 dma_alloc_coherent kbuf（同卡共享 PCIe DMA 域）
D2H(chunk0 → buf0)
for i in 1..n-1:
  wait D2H
  start H2D(buf_prev)          // dst PD rdch
  start D2H(chunk_i → buf_next) // src PD wrch  （与 H2D 重叠）
  wait H2D
wait last D2H; H2D last
```

| 优化 | 说明 |
|------|------|
| 零 CPU memcpy | EDMA 直写/读 coherent staging |
| 无全量 kvmalloc | 复用 per-channel 1MB kbuf |
| Ping-pong | 双 wrch 缓冲，D2H∥H2D |
| 日志降级 | 热路径 `LOGI` → `LOGL2` |

### EDMA API

`xpu_hw.c` 拆分：

- `xpuhw_edma_read_start` / `xpuhw_edma_read_wait`
- `xpuhw_edma_write_start` / `xpuhw_edma_write_wait`
- 原 `*_locked` = start + wait

---

## 4. S4 微调

Direct path 单次 EDMA 上限由 `KL1_DMA_KBUF_SIZE`(1MB) 提到 **2MB**（与 hugepage span 对齐），减少 map/doorbell 次数。

---

## 5. 修改文件

| 文件 | 变更 |
|------|------|
| `kl1/xpu_fops.c` | 修复 ioctl NR 拦截 |
| `kl1/xpu_dma.c` | P2P V6 ping-pong；S4 段长 2MB |
| `kl1/xpu_hw.c` / `xpu_hw.h` | EDMA start/wait |
| `kl1/xpu_ioctl.c` | P2P 日志降级 |

---

## 6. 装载与验证

```bash
# from repository root
make driver
sudo KL1_P2P_STUB=0 KL1_DMA_DIRECT=1 scripts/install_driver.sh

./tests/test_p2p_verify
./tests/test_p2p
./benchmarks/xpu_perf_test 0 bw
make regression
```

### 成功标准（已满足）

| 检查 | 期望 | 结果 |
|------|------|------|
| `test_p2p_verify` | PASS | **PASS** |
| 同卡 P2P @64MB | ≥ 4.0 GB/s | **11.2 GB/s** |
| host_alloc 64MB H2D | ≥ 8 GB/s | **12.5 GB/s** |
| `kl1_dma_direct` | 1 | **1** |

---

## 7. 实测明细（2026-07-14，ko 9A0E11148CC5A8E441F57AA）

### 同卡 P2P（`tests/test_p2p`）

| Size | P2P GB/s | D2D GB/s |
|------|----------|----------|
| 1MB | 5.95 | 0.98 |
| 4MB | 9.24 | 3.91 |
| 16MB | 10.73 | 15.62 |
| **64MB** | **11.20** | 62.50 |
| 256MB | 11.31 | 62.50 |
| 512MB | 11.32 | 62.50 |

独立 kernel ioctl / XPURT 对照（64MB×20）：**11.21 / 11.20 GB/s**，数据校验 PASS。

相对改前 ~2.5 GB/s：**约 4.5×**。

### host_alloc vs pageable（`xpu_perf_test 0 bw`）

| Size | pageable H2D/D2H | host_alloc H2D/D2H |
|------|------------------|---------------------|
| 1MB | 5.0 / 5.5 | **11.8 / 12.0** |
| 64MB | 5.6 / 4.9 | **12.5 / 12.9** |
| 256MB | 5.5 / 4.9 | **12.5 / 12.9** |
| 1GB | 5.5 / 4.9 | 5.6 / 5.0（huge 不足回退 bounce） |

### strace 观察

- 同卡热路径：`MEMCPY_P2P_DIRECT`（nr=0x92）大量 `= 0`
- 跨卡：`MEMCPY_P2P`（nr=0xe）返回 807（预期，`XPUERR_NOIOC`）；XPURT 仍可能打印警告并给出无效带宽数
- 改前 type 比较 bug 已修，内核路径可稳定命中

---

## 8. 风险与回滚

| 风险 | 缓解 |
|------|------|
| 双通道 ping-pong 状态机错误 | 失败路径 drain wait；`test_p2p_verify` PASS |
| 第二 wrch 抢不到 | 自动单缓冲零拷贝回退 |
| 1GB host_alloc | hugepage 压力；生产建议 ≤256MB pinned |

```bash
echo 0 > /sys/module/kunlun/parameters/kl1_dma_direct   # 关 S4
# 或重装旧 ko
```

---

## 9. 基线（改前，同日）

| 指标 | 值 |
|------|-----|
| 驱动 srcversion | D54DEEFC91174B3BF772916 |
| `kl1_dma_direct` | 0 |
| P2P 同卡 | ~2.5 GB/s |
| pageable H2D/D2H | ~5.5 / 4.9 GB/s |
| host_alloc H2D/D2H | ~5.6 / 4.9 GB/s |
