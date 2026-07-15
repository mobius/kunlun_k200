# S7 回归门禁

**时间**: 2026-07-15  
**状态**: 已实现并在当前驱动上跑通  
**目的**: 锁住 S4–S6 性能与正确性，防止后续改动静默回退

---

## 1. 内容

| 组件 | 说明 |
|------|------|
| `tests/test_pageable_verify.cpp` | pageable 64MB 双向 pattern |
| `scripts/run_driver_regression.sh` | 统一门禁（正确性 + 带宽下限） |
| `Makefile` | `make regression` 依赖扩展；`driver-install` 带 `KL1_BOUNCE_PIPE` |

### 硬门槛（生产参数）

| 项 | 条件 | 下限 |
|----|------|------|
| D2D / P2P pattern | 始终 | PASS |
| host_alloc pattern | 始终 | PASS |
| pageable pattern | 始终 | PASS |
| pageable 64MB H2D/D2H | `kl1_bounce_pipe=1` | ≥ 7.0 / 6.0 GB/s |
| pageable 64MB | `kl1_bounce_pipe=0` | ≥ 4.0 / 3.5 GB/s（serial） |
| host_alloc 64MB H2D/D2H | `kl1_dma_direct=1` | ≥ 8.0 / 8.0 GB/s |
| P2P 64MB | `kl1_p2p_stub=0` | ≥ 8.0 GB/s |

参数关闭时自动放宽对应下限，避免误杀调试配置。

---

## 2. 用法

```bash
# 装驱动后
make regression
```

失败即非零退出；摘要打印在末尾。

---

## 3. 自测结果（2026-07-15，ko B8FE90CAFA106B0B5E95A2A）

```
pageable  64MB  H2D=10.09 D2H=7.71  (pipe=1)
host_alloc 64MB H2D=12.50 D2H=12.88  (direct=1)
P2P        64MB 11.21 GB/s
All regression checks passed
```

---

## 4. 下一步

- **S8**: 应用层 FP16 + host_alloc 端到端  
- 可选：把 `make regression` 挂到 CI（需有 K200 的 runner）
