# K200 优化一页纸（对外 / 演示）

**日期**: 2026-07-16  
**硬件**: 2× Kunlun K200（4× XPU），PCIe Gen4×8，8GB HBM/设备，900 MHz  
**软件**: 本仓库修改版 KL1 驱动 + xdnn 2.0 / XPURT 4.33  
**驱动指纹**: `srcversion` 1BF517814DF547139CD5FCE  

---

## 1. 优化前 → 后（驱动）

| 能力 | 优化前 | 优化后 |
|------|--------|--------|
| 同卡 P2P | hang / 不可用 | **~11.2 GB/s**，校验 PASS |
| host_alloc | API 失败 | **~12.5 / 12.9 GB/s** H2D/D2H |
| pageable memcpy | ~5.5 / 4.9 GB/s | **~10.1 / 7.8 GB/s** |
| INT8 / 跨卡直传 | 无 | 仍无（硬件/固件） |

生产旋钮：`kl1_dma_direct=1`，`kl1_bounce_pipe=1`，`kl1_pageable_pin=0`。

---

## 2. 真实案例头条

| 案例 | 结果 | 故事 |
|------|------|------|
| **C2** 降噪 256² FP16 | **~100 img/s** | 端到端图处理；大图更能体现 pinned |
| **C3** 双 PD+P2P | dual **1.44 ms** vs solo **2.00 ms**（~1.39×） | 同卡两芯协作 |
| **C1** ResNet-50 | b1 **167** / b8–32 **~250** img/s | **现栈基线**（≠历史 1069） |
| **C4** MLP 塔 | b32 **~22.8k** / b64 **~42.2k** / b128 **~74.3k** img/s | FP16+host_alloc ~1.8× FP32 pageable |
| **C5** 双卡弱耦合 | sum/single **~2.00×** | 无跨卡 P2P；两进程绑不同卡 |

详情：`results/cases/SUMMARY.md`

---

## 3. 应用默认三板斧

1. **I/O**：`xpu_host_alloc`（要高带宽时）  
2. **算力**：FP16 + `ncluster=4`，K∈[1024,4096]  
3. **同卡多 PD**：`xpu_memcpy_peer`；跨卡勿依赖硬件 P2P  

---

## 4. 五分钟演示

```bash
# 驱动已加载后
make regression              # 可选：DEMO_REGRESSION=1
CASE_MODE=quick scripts/run_demo.sh
# 完整含 C1/C5：
# DEMO_FULL=1 scripts/run_demo.sh

# 阅读
less results/cases/SUMMARY.md
less docs/impl/20260716-demo-one-pager.md
```

---

## 5. 边界（诚实）

- 8GB 装不下大 LLM / Flux；无训练友好通信  
- pageable D2H 再抠驱动 **无效**（S9）；高 D2H 用 host_alloc  
- C1 对齐/栈差异会拉低相对历史数字，以现报告为准  
- C2 synth 权重、C4 FC 代理不是产线模型  

---

## 6. 文档索引

| 文档 | 用途 |
|------|------|
| `docs/impl/20260715-project-closure.md` | 驱动结案 |
| `docs/impl/20260715-real-cases-c1-c2-c3.md` | 案例 C1–C5 交付 |
| `docs/plan/20260716-next-phase-plan.md` | A+B+C（N1–N3）完成记录 |
| `docs/plan/20260716-phase-after-abc.md` | **下阶段**（P/Q/R/S/X） |
| `results/cases/SUMMARY.md` | 最新数字 |
| `README.md` | 仓库入口 |
