# 真实案例导向计划（K200 后驱动时代）

**日期**: 2026-07-15  
**状态**: C1/C2/C3 **已实现并实测** → `docs/impl/20260715-real-cases-c1-c2-c3.md`  
**前置**: 驱动里程碑已结案（S0–S9）→ `docs/impl/20260715-project-closure.md`  
**定位**: 不再以 microbench 为主，而以 **可演示、可度量的业务切片** 为主。

---

## 0. 原则

| 原则 | 含义 |
|------|------|
| 垂直切片 | 每个案例：输入数据 → 设备路径 → 输出指标（吞吐/延迟/质量） |
| 吃满已有红利 | host_alloc（S4）、同卡 P2P（S5）、pageable pipe（S6）、FP16 用法（S8） |
| 不扩硬件幻想 | 8GB HBM、无 INT8、无跨卡直传、900 MHz 锁定 |
| 可回归 | 每个案例有固定命令 + 基线数字，可并入或旁挂 `make` 目标 |
| 失败可降级 | 某框架装不上时，有 native xdnn 兜底路径 |

---

## 1. 能力 × 场景匹配

驱动结案后，真实案例应优先落在「计算密 + 模型能装进 8GB + FP16 友好」：

| 案例 ID | 业务形态 | 可行性 | 主要吃驱动哪一块 | 仓库现有资产 |
|:-------:|----------|:------:|------------------|--------------|
| **C1** | 图像分类推理（ResNet-50） | ✅ 首选 | host_alloc I/O、batch、（若有）FP16 | `paddle_models/`、`paddle_infer_benchmark.py` |
| **C2** | AI 降噪离线批处理 | ✅ 推荐 | host_alloc 图输入/输出、FP16 GEMM/FC | `xpu_denoise`、`train_denoise.py`、`test_pipeline.py` |
| **C3** | 同卡双 PD 流水线（producer/consumer） | ✅ 差异化 | **S5 P2P ~11 GB/s** | `test_p2p`、两 PD 拓扑 |
| **C4** | MLP / 小网络批推理（排序/召回小塔） | ✅ | FP16 + host_alloc 权重一次加载 | `xpu_app_pipeline` |
| **C5** | 多卡数据并行（弱耦合） | ⚠️ | 跨卡仅 host 中转；避免强 P2P | 2 卡 4 设备 |
| — | 大 LLM / SDXL / 训练 | ❌ 不做 | — | — |

**第一波只做 C1–C3**（演示价值最高、路径最短）。C4 作轻量补充，C5 可选。

---

## 2. 案例详细设计

### C1 — ResNet-50 真实推理吞吐（Paddle）

**业务问题**: 「这张卡跑标准分类模型，batch 扫完后 img/s 是多少？驱动优化有没有反映到推理？」

| 项 | 内容 |
|----|------|
| 输入 | `paddle_models/inference.pdmodel` + 合成或 ImageNet 尺寸 224² |
| 路径 | Paddle Inference XPU；输入缓冲尽量 pinned/可复用；batch = 1/8/32 |
| 指标 | img/s、ms/batch；对比「仅换驱动前后」若有历史 1069 @b1 |
| 成功标准 | b1 ≥ 历史量级；b32 吞吐随 batch 明显上升；全程无 hang |
| 交付 | `results/cases/c1_resnet50.md` + 固定脚本命令 |
| 工期 | 1–2 天（含 paddle-xpu 环境搞定） |
| 风险 | 本机/旧容器可能无 `paddlepaddle-xpu` → 优先装 wheel 或新镜像 |

**任务拆解**

1. 锁定可运行的 Paddle-XPU 环境（venv 或 podman，写入 `scripts/run_c1_resnet.sh`）  
2. 扩展 `paddle_infer_benchmark.py`：warmup/runs 可配、结果写 `results/cases/`  
3. 可选：预分配大 host 缓冲复用，减少每 iter alloc  
4. 记录：驱动参数快照 + img/s 表  

**与驱动关系**: 间接（框架是否用 host_alloc 不完全可控）；价值是 **真实栈基线** 与对外可讲数字。

---

### C2 — 图像降噪端到端（native，最贴仓库）

**业务问题**: 「脏图进、净图出，整条链路在 K200 上多快、质量是否可用？」

| 项 | 内容 |
|----|------|
| 输入 | 真实 PNG/JPG → 加噪 → PPM；或已有 `data/xpu_denoise_synth.bin` 权重 |
| 路径 | `xpu_denoise`；**图缓冲 `xpu_host_alloc`**；权重 device 常驻；能 FP16 则 FP16 |
| 指标 | 张/s 或 ms/张（固定分辨率如 512²/1024²）；可选 PSNR vs 干净图 |
| 成功标准 | 固定分辨率下可重复吞吐；host_alloc 路径与 pageable 有对比表；无内存泄漏 |
| 交付 | `results/cases/c2_denoise.md`；示例图 in/out；一键 `scripts/run_c2_denoise.sh` |
| 工期 | 2–3 天 |
| 风险 | 当前 denoise 实现是否已用 host_alloc/FP16 需审计改造 |

**任务拆解**

1. 审计 `benchmarks/xpu_denoise.cpp`：H2D/D2H 是否 pageable、精度是否 FP32 only  
2. 改造：输入/输出 pinned；计算路径对齐 S8 建议  
3. `test_pipeline.py` 串：读图 → 加噪 → denoise → 存图 → 打吞吐  
4. 对比矩阵：pageable vs host_alloc（同一模型）  

**与驱动关系**: **直接吃 S4/S6**；最适合证明「驱动优化进真实 I/O 链路」。

---

### C3 — 同卡双 PD 流水线（S5 杀手锏）

**业务问题**: 「一张 K200 的两个 XPU（PD0/PD1）能否流水：设备 A 算 stage1，P2P 给 B 做 stage2？」

| 项 | 内容 |
|----|------|
| 拓扑 | 同卡 `/dev/xpu0`→`/dev/xpu1` 或 2→3 |
| 路径 | 权重分片或双 stage 小网；中间激活 **`xpu_memcpy_peer`** |
| 指标 | e2e 样本/s；P2P 占比；对比「单 PD 串行两 stage」与「双 PD + P2P」 |
| 成功标准 | P2P 路径稳定 ≥8 GB/s 量级（回归地板）；双 PD e2e ≥ 单 PD 串行（或延迟更优） |
| 交付 | `benchmarks/xpu_pipeline_p2p.cpp` + `results/cases/c3_p2p_pipeline.md` |
| 工期 | 2–3 天 |
| 风险 | 调度/同步；勿与跨卡混淆 |

**任务拆解**

1. 最小两 stage：PD0 上 FC/elementwise → peer → PD1 上 FC → D2H  
2. 双缓冲激活，重叠 P2P 与计算（用户态双缓冲即可）  
3. 与「全在 PD0 做两 stage」对比表  
4. 写入回归：可选 `make case-c3`（或只文档 + 手工）  

**与驱动关系**: **直接证明 S5**；对外讲述「K200 同卡双芯可协作」。

---

### C4 — 小塔 / MLP 批推理（轻量）

**业务问题**: 「搜索/推荐小模型（MLP）批量打分吞吐。」

| 项 | 内容 |
|----|------|
| 路径 | 扩展或包装 `xpu_app_pipeline`：host_alloc 权重一次上传 + FP16 + batch 扫 |
| 指标 | 样本/s（batch=32/64/128） |
| 成功标准 | 相对 FP32 pageable e2e ≥1.5×（对齐 S8） |
| 工期 | 0.5–1 天 |
| 交付 | `results/cases/c4_mlp_batch.md` |

---

### C5 — 双卡弱耦合（可选）

**业务问题**: 「两张卡各跑独立 batch，总吞吐是否近似 2×。」

| 项 | 内容 |
|----|------|
| 路径 | 进程/线程绑定 xpu0 与 xpu2；**禁止依赖跨卡 P2P** |
| 指标 | 双实例 img/s 之和 / 单实例 |
| 成功标准 | 并行效率 ≥ 1.7×（视 PCIe/CPU 争用） |
| 工期 | 1 天 |

---

## 3. 统一案例规范

每个案例目录约定：

```
results/cases/
  c1_resnet50.md
  c2_denoise.md
  c3_p2p_pipeline.md
  ...
```

每份 md 固定章节：

1. 业务问题  
2. 环境快照（驱动 srcversion、`kl1_*` 参数、设备列表）  
3. 命令（可复制）  
4. 结果表（吞吐/延迟/质量）  
5. 与「优化前预期」对比（若适用）  
6. 已知限制  

脚本约定：

```
scripts/run_c1_resnet.sh
scripts/run_c2_denoise.sh
scripts/run_c3_p2p_pipeline.sh
```

环境变量统一：`CASE_DEV=0`、`CASE_OUT=results/cases`。

---

## 4. 实施时间线（建议 1–2 周）

```
Week 1
  D1–D2  C1 环境 + ResNet batch 扫描 + 案例报告
  D3–D5  C2 denoise 审计改造 + 真实图 e2e + host_alloc 对比

Week 2
  D1–D3  C3 双 PD P2P 流水线 micro-app + 对比
  D4     C4 轻量 MLP 批扫（可与 C1 并行压缩）
  D5     案例汇总页 + 可选 C5 双卡；演示脚本串烧
```

**里程碑**

| 节点 | 完成定义 |
|------|----------|
| M1 | C1 可重复跑出 img/s 表 |
| M2 | C2 真实图进、降噪出 + 吞吐对比 |
| M3 | C3 双 PD e2e 跑通且稳 |
| M4 | `docs/impl/YYYYMMDD-real-cases-summary.md` 对外可讲 |

---

## 5. 优先级与取舍

| 优先级 | 案例 | 原因 |
|:------:|------|------|
| P0 | **C2 降噪** | 仓库已有二进制/脚本；直接打 I/O + 计算 |
| P0 | **C3 双 PD** | 唯一能秀 S5 P2P 的业务故事 |
| P1 | **C1 ResNet** | 业界可对比；依赖 Paddle 环境 |
| P2 | C4 MLP | 成本低，补「小模型批量」叙事 |
| P3 | C5 双卡 | 锦上添花 |

**不做**: 新训大模型、SDXL/Flux、依赖跨卡 P2P 的方案、再开一轮 pageable D2H 驱动实验。

---

## 6. 成功总判据（案例里程碑）

1. 至少 **2 个** 垂直案例有 `results/cases/c*.md` 与一键脚本  
2. 每个案例在结案驱动参数下 **连续 3 次** 结果波动 < 10%  
3. C2 或 C3 中至少一处 **显式对比** pageable vs host_alloc 或 单 PD vs 双 PD+P2P  
4. 不破坏 `make regression`  

---

## 7. 与驱动结案的关系

```
驱动结案 (S0–S9)          真实案例计划 (本文)
─────────────────        ────────────────────
S4 host_alloc      ──►   C1/C2/C4 输入输出与权重
S5 P2P             ──►   C3 同卡流水线
S6 pageable pipe   ──►   未 pinned 时的保底路径
S7 regression      ──►   案例不得破坏门禁
S8 FP16 用法       ──►   C2/C4 精度与 batch 策略
S9 负结果          ──►   案例文档写明：高 D2H 必须 host_alloc
```

---

## 8. 建议的下一步动作（执行入口）

若同意本计划，推荐启动顺序：

1. **C2**（改 denoise + 真实图 pipeline）  
2. **C3**（新写双 PD 小流水线）  
3. **C1**（并行解决 Paddle 环境）  

回复指定 **「从 C2 开始」** 或 **「C1/C2/C3 都要」** 即可进入实现。
