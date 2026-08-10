# K200 XPU 驱动与性能优化 — 结案

**结案日**: 2026-07-15  
**仓库**: `github.com:mobius/kunlun_k200`（master）  
**结案 commit 锚点**: `307aa68`（S9）及之前 S4–S8  

**结案后延伸（非驱动内核）**: 2026-07-15～16 交付真实案例 C1–C5 + `make demo`（`0640a33`），后续计划见 `docs/plan/20260716-phase-after-abc.md`。

---

## 1. 一句话

在 **不改硬件/固件** 的前提下，KL1（K200）驱动补齐 P2P / host_alloc，PCIe 主路径拉到有效上限附近；算力侧以 **FP16 + host_alloc** 为应用默认；pageable D2H 再抠无收益，正式结案。

---

## 2. 阶段交付

| 阶段 | 内容 | 结果 |
|:----:|------|------|
| S0 | 环境与基线 | 可复现 |
| S1 | P2P host-staging | hang → 可用 |
| S2 | `xpu_host_alloc` mmap | API 可用 |
| S3 | INT8 探测 | **不可用**（固件无 kernel） |
| S4 | host_alloc 直传 EDMA | H2D/D2H **~12.5 / 12.9 GB/s** |
| S5 | P2P 零拷贝 + ping-pong + ioctl 修复 | 同卡 P2P **~11.2 GB/s** |
| S6 | pageable bounce 双缓冲 | pageable H2D **~10.1**，D2H **~7.8** |
| S7 | 回归门禁 | `make regression` 硬地板 |
| S8 | 应用 FP16 + host_alloc 基准 | e2e **~1.8×** vs FP32 |
| S9 | pageable D2H 再优化 | **无增益**；归档负结果 |

---

## 3. 最终性能表（结案机测）

| 路径 | 带宽 / 吞吐 | 备注 |
|------|-------------|------|
| pageable H2D / D2H | ~10.1 / ~7.8 GB/s | S6；S9 未再抬 D2H |
| host_alloc H2D / D2H | ~12.5 / ~12.9 GB/s | S4；生产高带宽首选 |
| 同卡 P2P | ~11.2 GB/s | S5；跨卡不支持 |
| 同 PD D2D | ~62 GB/s | 未改 |
| FP16 / FP32 GEMM 峰值 | ~20+ / ~7.8 TFLOPS | xdnn；INT8 N/A |
| 原生 FC pipeline e2e | FP16 ≈ 1.8× FP32 | S8 `xpu_app_pipeline` |
| ResNet-50 Paddle（**现栈**） | b1 **~167** / b8–32 **~250** img/s | `results/cases/c1_resnet50.md` |
| ResNet-50 Paddle（历史） | ~1069 img/s FP32 b1 | 旧容器/SDK 时代；**不可当作当前基线** |

硬件上限（不在范围内）：SM 900 MHz 锁定、PCIe Gen4×8、无跨卡硬件 P2P、无 INT8 CDNN。

生产驱动指纹：以 `modinfo -F srcversion kunlun` 与仓库 `kunlun-driver/kunlun.ko` 对齐为准（随内核重编变化）。

---

## 4. 生产运维

```bash
# 装载（默认参数）— 脚本先落盘再 reload；卸失败则 reboot 一次
make driver    # 内核小版本升级后必须重编（vermagic 必须 == uname -r）
sudo KL1_P2P_STUB=0 KL1_DMA_DIRECT=1 KL1_BOUNCE_PIPE=1 scripts/install_driver.sh

# 门禁前确认是补丁驱动，不是 boot 后 DKMS 官方包
modinfo -F srcversion,vermagic kunlun
# 期望 srcversion 与仓库 build 的 kunlun.ko 一致（随重编变化）
# 期望 vermagic 与 uname -r 一致；并存在 kl1_* sysfs

make regression

# 关键参数
kl1_dma_direct=1      # S4 host_alloc 直传
kl1_bounce_pipe=1     # S6 pageable 流水线
kl1_bounce_d2h=1      # S9 默认；对 pageable D2H 无额外增益
kl1_pageable_pin=0    # 保持关（4K pin 极慢）
kl1_p2p_stub=0
```

### 装驱动注意（2026-08 补充）

1. **装前检查**：`modinfo -F srcversion,vermagic kunlun` 与 `uname -r`、期望指纹。  
2. **内核升级后**：先 `make driver` 再 `install_driver.sh`。  
3. **`modprobe -r` 失败 / `/proc/modules` 显示 Unloading**：补丁 ko 应由脚本**已写入磁盘**；**只 reboot**，不要反复 `-r`。  
4. 仅写盘、稍后重启：`sudo DISK_ONLY=1 scripts/install_driver.sh && sudo reboot`。  
5. Boot 参数：`/etc/modprobe.d/kunlun-kl1.conf`（由 install 脚本维护）。

回退示例：

```bash
echo 0 > /sys/module/kunlun/parameters/kl1_dma_direct    # host_alloc 走 bounce
echo 0 > /sys/module/kunlun/parameters/kl1_bounce_pipe   # pageable 串行
# 或恢复备份 ko：kunlun.ko.bak.20260612 / kunlun.ko.bak.<timestamp>
```

---

## 5. 应用默认写法

1. 大块 I/O 用 **`xpu_host_alloc`**（不要指望 pageable D2H 再涨）  
2. 计算用 **FP16** + `ncluster=4`，K 维约 1024–4096  
3. 权重常驻 device；activation 再 H2D/D2H  
4. 同卡跨 PD 可用 `xpu_memcpy_peer`（~11 GB/s），跨卡勿用  

详见：`docs/impl/20260715-s8-app-guidance.md`

---

## 6. 明确不做 / 已否决

| 项 | 原因 |
|----|------|
| INT8 GEMM | 固件无路径（S3） |
| 跨卡硬件 P2P | 无 BAR/IOVA 基建 |
| pageable D2H 再迭代 | S9 三种方案均无效或不可用 |
| SSE 跨 PD 真 DMA（曾规划 S10） | **未做**；高风险 hang；需要时另开 spike |
| SM 超频 / PCIe×16 | 硬件锁定 |

---

## 7. 关键文档索引

| 文档 | 主题 |
|------|------|
| `docs/plan/20260612-remediation-iteration-plan.md` | 驱动总计划与 S0–S9 状态 |
| `docs/impl/20260613-s4-pinned-dma.md` | S4 |
| `docs/impl/20260714-s5-p2p-pingpong.md` | S5 |
| `docs/impl/20260714-s6-bounce-pipeline.md` | S6 |
| `docs/impl/20260715-s7-regression-gate.md` | S7 |
| `docs/impl/20260715-s8-app-guidance.md` | S8 |
| `docs/impl/20260715-s9-pageable-d2h.md` | S9 负结果 |
| `docs/impl/20260715-real-cases-c1-c2-c3.md` | 案例 C1–C5 交付说明 |
| `docs/impl/20260716-demo-one-pager.md` | 对外一页纸 |
| `docs/plan/20260716-next-phase-plan.md` | A+B+C（N1–N3）**已完成** |
| `docs/plan/20260716-phase-after-abc.md` | A/B/C 之后的下阶段（P/Q/R/S/X） |
| `results/cases/SUMMARY.md` | 案例最新数字 |
| `README.md` | 对外摘要与用法 |

---

## 8. 结案判定

| 项 | 状态 |
|----|:----:|
| 功能：P2P / host_alloc | ✅ |
| 带宽：S4/S5/S6 达标 | ✅ |
| 回归门禁 | ✅ |
| 应用指导 + e2e 基准 | ✅ |
| S9 结论清晰 | ✅ |
| 可维护回退参数 | ✅ |

**驱动里程碑关闭。**  

真实案例垂直切片（C1–C5）与演示路径已另交付：

- 说明：`docs/impl/20260715-real-cases-c1-c2-c3.md`
- 数字：`results/cases/SUMMARY.md`
- 命令：`make cases` / `make demo`

后续仅接受：产品化收口、真业务模型接入、栈对齐（航道 P/Q/R/S），或单独立项的高风险 spike（航道 X，如 SSE 跨 PD）。
