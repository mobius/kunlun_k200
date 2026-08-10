# 昆仑 K200 XPU 性能分析与驱动调优

[English / 英文版](README.md)

本仓库面向 **昆仑 K200** 推理卡：PCIe 带宽、GEMM、Paddle ResNet-50、真实案例演示（C1–C5），以及同卡 P2P / `xpu_host_alloc` 的 KL1 驱动修复。  
同一主机还跟踪 **Intel Xeon Phi 7220P（KNL）** 上电与同机 AMD MI50（**不属于** K200 驱动结案范围）。

**状态（2026-08）：**

| 线 | 状态 |
|----|------|
| K200 驱动 S0–S9 | **已结案** → [`docs/impl/20260715-project-closure.md`](docs/impl/20260715-project-closure.md) |
| K200 案例 C1–C5 + demo | **已交付** → [`results/cases/SUMMARY.md`](results/cases/SUMMARY.md) |
| K200 后续 | 产品化 / 真模型 / 冻结 → [`docs/plan/20260716-phase-after-abc.md`](docs/plan/20260716-phase-after-abc.md) |
| Phi 7220P（KNL） | Phase 0 完成 · Phase 1 阻塞于 **Rocky 8 + MPSS 4.4.1** → [`docs/plan/20260805-xeon-phi-7220p-bringup.md`](docs/plan/20260805-xeon-phi-7220p-bringup.md) |

**远程仓库：** [github.com/mobius/kunlun_k200](https://github.com/mobius/kunlun_k200)（分支 `master`）

**隐私：** 公开文档不要写主机名、实验室绝对路径、完整 `lspci`/dmesg dump、BMC、设备序列号。只用 PCI **设备 ID** 与仓库相对路径。

### 现场驱动自检（K200）

```bash
modinfo -F srcversion,vermagic kunlun
uname -r    # vermagic 必须与 uname -r 一致
cat /sys/module/kunlun/parameters/kl1_dma_direct \
    /sys/module/kunlun/parameters/kl1_bounce_pipe \
    /sys/module/kunlun/parameters/kl1_pageable_pin \
    /sys/module/kunlun/parameters/kl1_p2p_stub
```

| 项 | 生产期望 |
|----|----------|
| 旋钮 | `kl1_dma_direct=1`，`kl1_bounce_pipe=1`，`kl1_pageable_pin=0`，`kl1_p2p_stub=0` |
| `vermagic` | 等于 `uname -r`（内核升级后必须重编） |
| `srcversion` | **每次重编都会变** — 与 `modinfo -F srcversion kunlun-driver/kunlun.ko` 对比，不要死记文档里的旧指纹 |

重编后自行记录当前 `srcversion` 与上述旋钮即可。旧案例报告里的指纹字符串不是固定 ID。

---

## 硬件

| 组件 | 说明 |
|------|------|
| K200 | 昆仑加速卡 `1d22:3684`（KL1），**每卡 2 PD** → `/dev/xpu*` |
| 清单（实验室） | 多卡 K200（常见 2–4 张）；可选同机 AMD MI50、Intel Phi — 并非全部为本仓优化目标 |
| 架构 / 频率 | KL1，900 MHz 锁定 |
| HBM | 每 XPU 8 GB（每卡 16 GB） |
| PCIe（K200） | 每卡 Gen4 ×8 |
| Phi 7220P（支线） | Knights Landing，PCI ID `8086:2260`（DMA 功能 `8086:2264`）；**尚无驱动** |
| 主机（泛化） | x86_64 服务器级 CPU，Ubuntu 22.04，内核 **6.8.x**（内核升级后需重编 `kunlun.ko`） |

> 文档中 **不要** 写主机名、绝对路径、完整 `lspci`  dump、设备序列号。本机 BDF 用 `lspci -nn | grep -E '3684|2260'` 自查。  
> 案例数字主要在单 XPU / 同卡 PD 上测得；卡数不改变 S4–S6 带宽地板。

### Phi 7220P（支线）

**7220P = Knights Landing → 走 MPSS 4**，不是 MPSS 3 / 7120P（KNC）。不要照搬 `-mmic` / `micnativeloadex`。  
槽位不同时用环境变量覆盖（`PHI_BDF_MAIN` / `PHI_BDF_AUX`）。

```bash
bash scripts/check_phi_7220p.sh 2>&1 | tee /tmp/phi_7220p_hw.log   # 日志留本地，勿提交原始 dump
bash scripts/phase1_mpss4_readiness.sh
python3 scripts/phi_discover.py
```

计划 / 调研：[`docs/plan/20260805-xeon-phi-7220p-bringup.md`](docs/plan/20260805-xeon-phi-7220p-bringup.md)、[`docs/research/20260805-phi-7220p-phase1-mpss4.md`](docs/research/20260805-phi-7220p-phase1-mpss4.md)。  
`third_party/` 下大型厂商树 **已加入 .gitignore**。

---

## 关键基准（K200，S9 + 案例后）

在生产旋钮下测得。详细见 `docs/impl/` 与 `results/cases/`。

### 算力（xdnn GEMM）

| 精度 | 峰值 | 效率 | 说明 |
|------|:----:|:----:|------|
| FP16 | ~20 TFLOPS | ~35% | `xdnn::fc`，2048³，nc=4 |
| FP32 | ~7.8 TFLOPS | ~54% | `xdnn::fc`，2048³ |
| INT8 | 不可用 | — | 固件无 INT8 CDNN kernel（S3） |

### PCIe 带宽（XPURT `xpu_memcpy`）

| 方向 | 带宽 | 说明 |
|------|:----:|------|
| H2D pageable（S6/S9） | **~10.1 GB/s** | bounce 流水线（`kl1_bounce_pipe=1`） |
| D2H pageable（S6/S9） | **~7.8 GB/s** | S9 未再提升；高带宽请用 host_alloc ~12.9 |
| H2D `xpu_host_alloc`（S4） | **~12.5 GB/s** | 直传 EDMA（`kl1_dma_direct=1`） |
| D2H `xpu_host_alloc`（S4） | **~12.9 GB/s** | ≤256MB；1GB 可能因大页不足回退 |

### P2P（同卡 PD0 → PD1）

| 指标 | 数值 | 说明 |
|------|:----:|------|
| 带宽（S5） | **~11.2 GB/s** | 原先 hang / ~2.5 GB/s |
| 数据校验 | PASS | `tests/test_p2p_verify` |
| 跨卡 P2P | 不支持 | 无硬件跨卡直传 |

### 延迟

| 操作 | 延迟 |
|------|:----:|
| Kernel launch | ~2 µs |
| 1 线程 dispatch | ~26 µs |
| 4 线程 dispatch | ~25 µs |

### 推理 / 真实案例

| 负载 | 框架 | 精度 | 指标 | 说明 |
|------|------|:----:|------|------|
| **C1** ResNet-50 | Paddle 2.6.1 XPU | FP32 | b1 **~167** / b8–32 **~250** img/s | **现栈基线** |
| ResNet-50（历史） | 旧容器/SDK | FP32 b1 | ~1069 img/s | 非当前数字；非驱动回退 |
| **C2** 降噪 256² | 原生 xdnn | FP16 | **~100 img/s** | pinned ≈ pageable（算力主导） |
| **C3** 双 PD + P2P | 原生 | FP16 | dual **1.44 ms** vs solo **2.00 ms**（~1.39×） | 仅同卡 |
| **C4** MLP 塔 e2e | 原生 | FP16 host_alloc | b32 **~22.8k** img/s | 相对 FP32 pageable ~1.8× |
| **C5** 双卡弱耦合 | 双进程 | FP16 host_alloc | 总/单 **~2.00×** | 无跨卡 P2P |

应用默认：大块 I/O 用 `xpu_host_alloc`，计算用 **FP16** + `ncluster=4`，GEMM K ∈ [1024, 4096]。见 [`docs/impl/20260715-s8-app-guidance.md`](docs/impl/20260715-s8-app-guidance.md)。

---

## 驱动修复（KL1 / K200）

| 能力 | 优化前 | 优化后（S4–S9） |
|------|--------|-----------------|
| 同卡 `xpu_memcpy_peer` | hang / ~2.5 GB/s | **~11.2 GB/s**，校验 PASS（S5） |
| `xpu_host_alloc` / `host_free` | -706 / -707 | PASS（2MB 大页 mmap，S2） |
| host_alloc 路径 H2D/D2H | 仅 bounce | **~12.5 / 12.9 GB/s**（S4） |
| pageable H2D/D2H | ~5.5 / ~4.9 | **~10.1 / ~7.8**（S6；S9 D2H 无增益） |

### 安装

脚本 **先把 `.ko` 写到磁盘**，再尝试卸载/重载。若卡在 *Unloading*，**重启一次即可**（补丁模块已在盘上）。

```bash
make driver    # 内核包升级后必须重编（vermagic 必须 == uname -r）
sudo KL1_P2P_STUB=0 KL1_DMA_DIRECT=1 KL1_BOUNCE_PIPE=1 scripts/install_driver.sh
# 或: make driver-install
# 仅写盘后重启: sudo DISK_ONLY=1 scripts/install_driver.sh && sudo reboot
# 不重编、用已有 ko: make driver-install-norebuild
# 指定路径: make install-ko KO=/path/to/kunlun.ko
modinfo -F srcversion,vermagic kunlun
make regression
```

开机默认参数：`/etc/modprobe.d/kunlun-kl1.conf`（由 `install_driver.sh` 维护）。  
更多：[`kunlun-driver/BUILD.md`](kunlun-driver/BUILD.md)、结案文档第 4 节。

### 运行时旋钮（`/sys/module/kunlun/parameters/`）

| 参数 | 生产默认 | 说明 |
|------|:--------:|------|
| `kl1_dma_direct` | 1 | S4 host_alloc 直传 EDMA |
| `kl1_bounce_pipe` | 1 | S6 pageable 双缓冲 |
| `kl1_pageable_pin` | **0** | 保持关闭（大块 4K pin 极慢/易挂） |
| `kl1_p2p_stub` | 0 | 关闭 stub 路径 |
| `kl1_bounce_d2h` | 1 | S9 默认；对 pageable D2H 无额外增益 |

---

## 目录结构

```
.
├── benchmarks/                 # 带宽/GEMM、app pipeline、降噪、P2P 流水线、INT8 探测
├── tests/                      # p2p / host_alloc / pageable 校验
├── scripts/
│   ├── install_driver.sh       # 稳健安装（先落盘、DISK_ONLY、modprobe.d）
│   ├── install_ko_file.sh
│   ├── run_driver_regression.sh
│   ├── run_real_cases.sh / run_demo.sh / run_c1…c5_*.sh
│   ├── check_phi_7220p.sh / phase1_mpss4_readiness.sh / phi_discover.py
│   └── try_build_mpss4_modules.sh
├── results/cases/              # C1–C5 报告与 SUMMARY.md
├── docs/                       # 调研、计划、实现（结案、S4–S9、Phi 计划）
├── kunlun-driver/              # 内核模块 4.33 + KL1 补丁
├── xdnn-ubuntu_x86_64/         # xdnn SDK 2.0.0.725
├── third_party/                # 本地 MPSS 树（gitignore）
├── README.md
└── README_cn.md
```

---

## 构建与运行

```bash
make all

./benchmarks/xpu_perf_test 0          # GEMM + 带宽
./benchmarks/xpu_perf_test 0 bw       # 仅带宽

make benchmarks/xpu_app_pipeline && ./benchmarks/xpu_app_pipeline -d 0
# 或: scripts/run_s8_app_bench.sh

make cases                            # C1–C5
make demo                             # 快速 C2/C3/C4
# DEMO_FULL=1 scripts/run_demo.sh
# DEMO_REGRESSION=1 scripts/run_demo.sh

make regression                       # S7 门禁
make benchmarks/xpu_int8_probe && ./benchmarks/xpu_int8_probe 0
```

### 真实案例

| 案例 | 命令 | 报告 |
|------|------|------|
| C1 ResNet-50 | `scripts/run_c1_resnet.sh` | `results/cases/c1_resnet50.md` |
| C2 降噪 | `scripts/run_c2_denoise.sh` | `results/cases/c2_denoise.md` |
| C3 双 PD P2P | `scripts/run_c3_p2p_pipeline.sh` | `results/cases/c3_p2p_pipeline.md` |
| C4 MLP 塔 | `scripts/run_c4_mlp.sh` | `results/cases/c4_mlp.md` |
| C5 双卡 | `scripts/run_c5_dual_card.sh` | `results/cases/c5_dual_card.md` |

一页纸：[`docs/impl/20260716-demo-one-pager.md`](docs/impl/20260716-demo-one-pager.md) · 案例交付说明：[`docs/impl/20260715-real-cases-c1-c2-c3.md`](docs/impl/20260715-real-cases-c1-c2-c3.md)

### Paddle 推理

```bash
# 需要 paddlepaddle-xpu（或 run_c1 中的 podman 路径）
python3 scripts/paddle_infer_benchmark.py --model paddle_models --batch 1,8,32
```

---

## 环境

| 组件 | 路径 / 说明 |
|------|-------------|
| XPURT | `/usr/local/xpu-4.33.0/lib64/libxpurt.so.1` |
| 已安装驱动 | `/lib/modules/$(uname -r)/updates/dkms/kunlun.ko` |
| xdnn | `xdnn-ubuntu_x86_64/` |

---

## 不做范围

**K200 硬件 / 固件**

- SM 超频（900 MHz 锁定）
- K200 协商到 PCIe ×16（本卡为 Gen4×8）
- 跨卡硬件 P2P / BAR IOVA
- 无固件更新下的 INT8 GEMM
- 再抠 pageable D2H 内核路径（S9 已关闭：无收益）
- 默认打开 `kl1_pageable_pin=1`

**Phi**

- 把 7220P 当成 7120P / MPSS 3
- 以主机 6.8 内核作为 MPSS4 modules 的正式支持路径（预期失败；目标 Rocky 8 / 4.18）

---

## 历史基线（优化前）

2026-05 档案（P2P 不可用、DMA ~4–5 GB/s）— **不是** 当前性能：

- [`results/PERF_SUMMARY.md`](results/PERF_SUMMARY.md)（2026-05-07）
- [`results/K200_ANALYSIS_REPORT.md`](results/K200_ANALYSIS_REPORT.md)（2026-05-08）
