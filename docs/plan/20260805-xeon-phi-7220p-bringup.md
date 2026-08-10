# Intel Xeon Phi 7220P Bring-up Plan

**日期**: 2026-08-05  
**状态**: Phase 0 完成 · Phase 1 调研完成 → **待 Rocky 8 + MPSS 4.4.1 安装**  
**主机**: x86_64 服务器级 CPU（EPYC 类），Ubuntu 22.04 / kernel 6.8（具体机型/主机名不在公开文档中记录）  
**卡**: Intel Xeon Phi **7220P**（Knights Landing, PCIe 被动散热）  
**PCI ID**: `8086:2260`（主功能）+ `8086:2264`（DMA 功能）；本机 BDF 用 `lspci` / `PHI_BDF_*` 自查，**勿把完整拓扑 dump 提交进 git**  

**参考**: [mobius/uni-framework](https://github.com/mobius/uni-framework) 的 Phi 方法论（**不可照搬 7120P/KNC 工具链**）

---

## 0. 与 uni-framework 7120P 的差异（冻结）

| 项 | 本机 **7220P** | uni **7120P** |
|----|----------------|---------------|
| 代际 | **Knights Landing (KNL)** | Knights Corner (KNC) |
| 典型规格 | 64 核 / 256 线程，~1.3 GHz，16 GB HBM，~300 W 被动 | 61 核 / 244 线程，16 GB GDDR5，300 W 被动 |
| 指令集 | **AVX-512 (KNL)** | IMCI（`icc -mmic`） |
| Host 栈 | **非** MPSS 3.x + `micnativeloadex` 主路径 | MPSS 3.8 + `micnativeloadex` + `.mic` |
| 编译 | `icc/icx -xMIC-AVX512` 等 → 普通 x86_64 ELF | `icc -mmic` → `.mic` |
| 发现 | 固定 BDF / `8086:2260` | `lspci` 字符串 “Xeon Phi Co-processor” |

**可复用**: 分阶段 bring-up、独立编译链、Host Python 调度、PCIe 最小化、功耗/风道约束。  
**不可复用**: `phi.py` 里的 `-mmic` / `micnativeloadex` / 7120P 性能数字。

---

## 1. 本机锚点（Phase 0 实测摘要，已脱敏）

| 项 | 值 |
|----|-----|
| ID | `8086:2260` / `8086:2264`，subsys `7498`，rev `ca` |
| Class | Bridge `0x068000` + System peripheral |
| BAR2 | **32 GiB** prefetch（具体物理地址因主板而异，勿写入公开文档） |
| 链路 | **8.0 GT/s ×16**（实测本机） |
| Driver | **无**；`enable=0`；无 `/dev/mic*` |
| 主机 | AMD EPYC 类（MPSS/KNL PCIe 官方支持弱） |

检查命令（日志保留在 `/tmp` 或本地，**不要**把含主机名/序列号的 raw log 提交到仓库）:

```bash
bash scripts/check_phi_7220p.sh 2>&1 | tee /tmp/phi_7220p_hw.log
```

---

## 2. 阶段划分

### Phase 0 — 硬件基线（当前）

- [x] 型号确认为 **7220P (KNL)**
- [x] PCI/链路/无驱动状态脚本化（`scripts/check_phi_7220p.sh`）
- [x] 首次基线日志：PASS 7 / WARN 2（enable=0、无驱动）/ FAIL 0
- [ ] 供电 6/8-pin、槽位风道（人工）
- [ ] 整机功耗预算记录（与 K200×4 + MI50×3 互斥策略）

**通过**: 链路稳定 ×16；型号与 BDF 文档冻结。

### Phase 1 — 执行通道（阻塞）— **MPSS 4.4.x (KNL x200)**

> 详细调研: `docs/research/20260805-phi-7220p-phase1-mpss4.md`  
> **不要**装 MPSS 3.8（那是 7120P/KNC）。本卡 PCI ID 在 MPSS4 源码中为 `0x2260`/`0x2264`。

| 序 | 动作 | 状态 / 成功信号 |
|----|------|-----------------|
| 1 | 确认栈 = **MPSS 4** + `mic_x200` | **已确认**（源码 ID 对齐） |
| 2 | 本机 6.8 试编 modules | **已试 → 失败（预期）** |
| 3 | 放置 `mpss-4.4.1` tarball 到 `third_party/mpss4/` | 待人工 |
| 4 | 双系统 / 第二盘 **Rocky 8 + kernel 4.18** | 待人工 |
| 5 | 编装 `jjkeijser/mpss` 的 `mpss4/mpss-modules` | `modprobe mic_x200` 绑定本机 `8086:2260` 设备 |
| 6 | `micctrl` 起卡 → `micinfo` / ssh | hello 可执行 |
| 7 | BIOS Above4G / IOMMU 试验（若 bind 失败） | 无 AER 风暴 |

就绪检查:

```bash
bash scripts/phase1_mpss4_readiness.sh
bash scripts/try_build_mpss4_modules.sh   # 仅在 ≤4.18 目标系统
```

**失败则停**: 仅登记为 PCI 资产，不投入调度层开发。

### Phase 2 — 编译与最小算力

- 容器固定 ICC/ICX 或 GCC，目标 **KNL AVX-512**（`-xMIC-AVX512` 等）
- 产物: 普通 **ELF**，不是 `.mic`
- 验证: OpenMP hello → STREAM → FP64 peak（指标按 7220P 自算）

### Phase 3 — Host 调度骨架

- `scripts/phi_discover.py` 已提供发现逻辑
- 后续: deploy/run runner（ssh/scp 或 Phase 1 loader）
- 功耗: 与昆仑/AMD GPU **互斥满载**

### Phase 4 — 基准与可选异构

- TC: PCIe 搬运、STREAM、peak、30 min 烤机、共存分时
- 应用: 不规则 OpenMP 核；稠密算力仍走 K200/MI50

---

## 3. 风险

| 风险 | 等级 | 缓解 |
|------|------|------|
| KNL PCIe 生态稀缺 | 高 | Phase 1 设硬门禁 |
| Ubuntu 6.8 + EPYC | 高 | 必要时 Rocky 旧内核双系统 |
| 300 W 被动 + 同 switch MI50 | 中 | 槽位/分时/监控温度 |
| 整机功耗 | 中 | 与 XPU/GPU 互斥 |

---

## 4. 目录约定

```
docs/plan/20260805-xeon-phi-7220p-bringup.md   # 本文
docs/research/*_phi_7220p_hw.log                 # 检查日志
scripts/check_phi_7220p.sh                       # Phase 0/1 硬件检查
scripts/phi_discover.py                          # 设备发现
```

---

## 5. 下一步（立即）

1. ~~跑 Phase 0 检查~~ · ~~调研 MPSS4~~  
2. 人工：供电/风道；**获取 `mpss-4.4.1` 完整包** 放入 `third_party/mpss4/`  
3. 人工：安装 **Rocky Linux 8.x**（第二系统或第二磁盘，kernel 4.18）  
4. 在 Rocky 上：`git clone` jjkeijser/mpss → `try_build_mpss4_modules.sh` → 安装用户态 → `mpss` 起卡  
5. 验收 P1-1…P1-6 后，再写 OpenMP hello + Host runner（Phase 2）

```bash
bash scripts/check_phi_7220p.sh
bash scripts/phase1_mpss4_readiness.sh
python3 scripts/phi_discover.py
```
