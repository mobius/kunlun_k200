# 真实案例交付：C1–C5

**日期**: 2026-07-15（C1–C3）/ 2026-07-16（C4–C5 与 N1 深耕）  
**计划**: `docs/plan/20260715-real-world-case-plan.md`、`docs/plan/20260716-next-phase-plan.md`  
**结果汇总**: `results/cases/SUMMARY.md`  
**驱动**: 结案生产旋钮（`kl1_dma_direct=1` 等）；`srcversion` 随重编变化，见当次 `results/cases/*`  
**文件名保留** `…c1-c2-c3…`（历史路径）；内容覆盖 **C1–C5**。

---

## 1. 怎么跑

```bash
# 需已加载本仓库 kunlun.ko
make cases                    # C1–C5（full）
make demo                     # 快速 C2/C3/C4
# DEMO_FULL=1 scripts/run_demo.sh   # + C1/C5
# 或单独：
scripts/run_c2_denoise.sh
scripts/run_c3_p2p_pipeline.sh
scripts/run_c4_mlp.sh
scripts/run_c5_dual_card.sh
scripts/run_c1_resnet.sh      # 优先本机/容器 paddle-xpu
```

报告输出到 `results/cases/`。

---

## 2. 案例与实测

### C1 — ResNet-50（Paddle Inference）

| 项 | 内容 |
|----|------|
| 脚本 | `scripts/run_c1_resnet.sh`、`scripts/paddle_infer_benchmark.py` |
| 模型 | `paddle_models/inference.pdmodel` |
| 环境 | podman + `paddlepaddle-xpu==2.6.1`（自动尝试） |

| Batch | img/s | ms/batch |
|------:|------:|---------:|
| 1 | 166.9 | 5.99 |
| 8 | 249.2 | 32.10 |
| 32 | 246.0 | 130.08 |

说明：日志有 64 字节对齐警告；历史 ~1069 img/s @b1 为另一套栈，**本表为当前栈基线**（非驱动回退）。  
报告：`results/cases/c1_resnet50.md`

---

### C2 — FP16 残差 CNN 降噪（native）

| 项 | 内容 |
|----|------|
| 二进制 | `benchmarks/xpu_denoise`（`--pinned` / `--pageable` / `--bench`） |
| 脚本 | `scripts/run_c2_denoise.sh`、`scripts/gen_test_ppm.py` |
| 权重 | `data/xpu_denoise_synth.bin` |
| 分辨率 | 256×256（可 `CASE_W/H` 覆盖；full 模式可扫更大图） |

| Staging | ms/img | img/s |
|---------|-------:|------:|
| host_alloc | 9.998 | **100.0** |
| pageable | 10.001 | **100.0** |

说明：该分辨率下 e2e 为 **算力主导**，I/O 差异被摊平；更大图时 host_alloc 更有意义。synth 权重 PSNR 仅作演示，非画质评测。  
报告：`results/cases/c2_denoise.md`

---

### C3 — 同卡双 PD + P2P 流水线

| 项 | 内容 |
|----|------|
| 二进制 | `benchmarks/xpu_pipeline_p2p` |
| 脚本 | `scripts/run_c3_p2p_pipeline.sh` |
| 拓扑 | xpu0 stageA → `xpu_memcpy_peer` → xpu1 stageB |
| 负载 | batch=32, feat=2048, layers=4+4, FP16, ncluster=4 |

| 路径 | ms/batch | samples/s |
|------|---------:|----------:|
| **Dual-PD + P2P** | **1.441** | **22206** |
| Single-PD serial (D2D mid) | 2.000 | 16000 |

dual/solo 延迟比 ≈ **1.39×**（本拆分下双 PD 更快）。  
报告：`results/cases/c3_p2p_pipeline.md`

---

### C4 — MLP / 排序小塔批推理

| 项 | 内容 |
|----|------|
| 二进制 | `benchmarks/xpu_app_pipeline` |
| 脚本 | `scripts/run_c4_mlp.sh` |
| 负载 | feat=2048, layers=8, batch 32/64/128 |

| Config @b32 | img/s |
|-------------|------:|
| pageable + FP32 | ~12.5k |
| **host_alloc + FP16** | **~22.8k** (~1.8×) |
| host_alloc + FP16 @b64 / b128 | ~42.2k / ~74.3k |

报告：`results/cases/c4_mlp.md`

---

### C5 — 双卡弱耦合并行

| 项 | 内容 |
|----|------|
| 脚本 | `scripts/run_c5_dual_card.sh` |
| 负载 | 两进程分别绑 xpu0 / xpu2，host_alloc+FP16 e2e |
| **禁止** | 跨卡硬件 P2P |

| Mode | img/s |
|------|------:|
| Single xpu0 | ~22833 |
| Parallel xpu0 + xpu2 sum / single | **~2.00×** |

报告：`results/cases/c5_dual_card.md`

---

## 3. 与驱动红利的对应

| 驱动能力 | 案例中的体现 |
|----------|----------------|
| S4 host_alloc | C2 `--pinned`；C4/C5 host_alloc 路径 |
| S5 同卡 P2P | C3 `xpu_memcpy_peer` 跨 PD |
| S6 pageable pipe | C2 `--pageable` 仍可用 |
| S7 regression | 案例不替代门禁；装驱动后仍应 `make regression` |
| S8 FP16 用法 | C2/C3/C4/C5 计算默认 FP16 |

---

## 4. 文件清单

| 路径 | 说明 |
|------|------|
| `benchmarks/xpu_denoise.cpp` | C2 |
| `benchmarks/xpu_pipeline_p2p.cpp` | C3 |
| `benchmarks/xpu_app_pipeline.cpp` | C4 / C5 / S8 |
| `scripts/run_c1_resnet.sh` … `run_c5_dual_card.sh` | 各案例 |
| `scripts/run_real_cases.sh` | C1–C5 串联（`make cases`） |
| `scripts/run_demo.sh` | 快速演示（`make demo`） |
| `scripts/gen_test_ppm.py` | 合成测试图 |
| `results/cases/*` | 报告与汇总 |
| `Makefile` `cases` / `demo` 目标 | 入口 |

---

## 5. 已知限制

- C1 依赖 paddle-xpu；装包失败时脚本会落 native proxy（非 ResNet）。现栈 ~167 img/s @b1 ≪ 历史 1069，属栈/对齐差异。  
- C2 合成权重仅演示吞吐，非画质评测。  
- C3 跨卡 P2P 仍不支持；stage 很小时 P2P 开销可能盖过收益。  
- C4/C5 是 FC micro-app 代理，不是线上训练好的业务模型。  
- 勿默认打开 `kl1_pageable_pin=1`（4K pin 极慢，见 S9）。
