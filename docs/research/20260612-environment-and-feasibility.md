# 环境核查与未解决问题可行性评估

**时间**: 2026-06-12  
**目的**: 在实施任何修改前，确认硬件/软件环境是否支持后续迭代，并评估 5 项遗留问题的软件解决可能性。

---

## 1. 环境核查结果

### 1.1 主机 CPU

| 项目 | 实测值 | 评估 |
|------|--------|------|
| 型号 | AMD EPYC 7402 24-Core | ✅ 满足内核编译与并行测试 |
| 架构 | x86_64 | ✅ |
| 在线核心 | 48 (1 socket × 24C × 2T) | ✅ |
| 内核 | 6.8.0-111-generic | ✅ 与驱动 conftest 缓存匹配 |
| GCC | 12.3.0 | ✅ 可编译 kunlun.ko |
| Make | 4.3 | ✅ |

### 1.2 GPU（对比基准）

| 设备 | PCI | 用途 |
|------|-----|------|
| AMD Radeon Pro VII / MI50 ×2 | 05:00.0, 8a:00.0 | 性能对比基准，不参与 K200 驱动开发 |
| ASPEED BMC VGA | 84:00.0 | 管理口，无关 |

### 1.3 算力卡（Kunlun K200）

| 项目 | 实测值 | 评估 |
|------|--------|------|
| 卡数 | 2 (PCI 43:00.0, 44:00.0) | ✅ |
| XPU 设备 | /dev/xpu0–3, /dev/xpuctrl | ✅ 全部可访问 |
| 型号 | K200 (KL1) | ✅ |
| 固件 | 0001.0016.0021 / 0022 | ✅ |
| 频率 | 900 MHz | 固件锁定，软件不可调 |
| 温度/功耗 | 48°C / 38W (空闲) | ✅ 正常 |
| Runtime | 4.33.0 @ /usr/local/xpu-4.33.0 | ✅ |
| xpu_smi | 可用 | ✅ |

### 1.4 驱动状态（重要）

| 项目 | 路径/值 | 说明 |
|------|---------|------|
| 当前加载模块 | `/lib/modules/.../updates/dkms/kunlun.ko` | DKMS 原版 |
| 仓库编译模块 | `kunlun-driver/kunlun.ko` | 含 KL1 修改 |
| SHA256 | **不一致** | 当前系统加载的是原版驱动，非仓库修改版 |
| srcversion | C11AF93CC63E3B3D7AB8E54 | |

**结论**: 硬件环境完全就绪，但后续驱动迭代测试前必须先 `modprobe` 替换为仓库编译的 `kunlun.ko`。

### 1.5 环境管理工具

| 工具 | 状态 | 用途规划 |
|------|------|----------|
| uv 0.11.11 | ✅ `/home/joey/.local/bin/uv` | Python 脚本/基准测试本地 venv（首选） |
| podman 3.4.4 | ✅ | 可选隔离容器（复现 5/7 容器测试） |
| conda | ❌ 未安装 | 不使用 |
| docker | ❌ 未安装 | 不使用 |

### 1.6 构建与测试二进制

| 项目 | 状态 |
|------|------|
| `kunlun-driver/kunlun.ko` | ✅ 已编译 (21.5 MB) |
| `tests/test_p2p` | ❌ 未编译 |
| `tests/test_p2p_verify` | ❌ 未编译 |
| `benchmarks/xpu_perf_test` | ❌ 未编译 |

---

## 2. 未解决问题逐项评估

### 2.1 P2P DMA hang（同卡 PD0↔PD1）

**现状**:
- IOCTL 路由已修复（`_IOC_NR` 匹配 + `_IOC_TYPE` 修正）
- `dma_device_to_device_p2p` 使用源 PD 的 SSE DMA 引擎写目标 PD 的 BAR 地址
- 结果：SSE DMA 无完成中断，进程 hang，读回全零

**根因假设**:
1. SSE DMA 硬件有 PD 域边界限制（每个 PD 独立 ssedma @ `RDMA_BASE` / `PD1_OFFSET+RDMA_BASE`）
2. 跨 PD 地址虽在同一 PCIe BAR 空间，但 ssedma 不接受对端 PD 的绝对地址

**软件解决可能性**: ⭐⭐⭐⭐ **高（功能正确性）** / ⭐ **低（高带宽）**

| 方案 | 可行性 | 预期效果 | 参考 |
|------|:------:|----------|------|
| A. Host-staging P2P（D2H→H2D ping-pong） | **高** | 功能可用，带宽 ~2–4 GB/s | KL2 `kl2_dma_peer_to_peer` |
| B. 继续 SSE DMA 跨 PD + 寄存器调试 | 低 | 若成功可达 ~60 GB/s | 需硬件文档 |
| C. 用户态 workaround（D2H+H2D） | **高** | 无需驱动，但 API 不兼容 | 应用层手动 |
| D. 跨卡 P2P | 低 | K200 无 NVLink/PCIe P2P 映射 | KL2 有 `kl2_map_memcpy_p2p_direct` |

**关键发现**: KL2 的 P2P 实现注释明确写道 *"走 PCIE 的方法在诸多机器上出现了问题，暂时只能通过 host 进行 p2p"*。KL2 实际使用的是 `kl2_dma_peer_to_peer`（双 EDMA 引擎 + 内核 bounce buffer ping-pong），而非设备直连。KL1 完全可移植此模式。

**推荐**: 方案 A — 在 `ioctl_memcpy_p2p_kl1` 中替换 `dma_device_to_device_p2p` 为 `kl1_dma_peer_to_peer`（新增函数，仿 KL2）。

---

### 2.2 xpu_host_alloc 返回 -706

**现状**:
- `xpu_host_alloc` 通过 `mmap(/dev/xpuN, pgoff=0)` 分配 pinned host memory
- KL1 `xpu_char_mmap` 当前为 **空 stub**（`return 0`，未实际映射页面）
- `xpu_fops` 结构体甚至未注册 `.mmap` 回调

**软件解决可能性**: ⭐⭐⭐ **中高**

| 方案 | 可行性 | 预期效果 |
|------|:------:|----------|
| A. 实现 KL1 `xpu_char_mmap`（pgoff=0 路径） | 中高 | API 不再返回 -706 |
| B. 完整移植 KL2 `kl2_mmap_host_alloc` | 中 | 功能完整但依赖 hugepage 基础设施 |
| C. 简化版：vmalloc + remap_pfn_range | 中 | 快速验证，可能不满足 XPURT 对 pinned 的语义 |

**限制**: 即使 mmap 成功，KL1 的 `dma_host_to_device` / `dma_device_to_host` 仍走 `copy_from_user`/`copy_to_user` + 1MB bounce buffer。**pinned memory 带宽提升需要额外改造 DMA 路径**（使用 `dma_map_page` 直传），工作量大。

**推荐**: 分两阶段 — 先实现 mmap 让 API 可用（阶段 2），再评估 DMA 直传改造是否值得（阶段 4，低优先级）。

---

### 2.3 Pinned memory 带宽无提升

**现状**:
- `xpu_host_register` stub 已返回 0，但带宽不变（H2D 5.66 GB/s）
- KL1 EDMA 路径：`copy_from_user → 1MB kbuf → EDMA → HBM`，无法绕过 CPU 拷贝

**软件解决可能性**: ⭐⭐ **低中**

| 方案 | 可行性 | 预期效果 |
|------|:------:|----------|
| A. EDMA 改用 dma_map_single 直传已注册页面 | 中 | 理论可接近 PCIe 上限 ~15 GB/s 的 50%+ |
| B. 增大 KL1_DMA_KBUF_SIZE | **已验证无效** | 无变化 |
| C. 接受现状，文档标注架构限制 | — | 务实选择 |

**推荐**: 在 host_alloc + host_register 真正实现后，再尝试方案 A。若 EDMA 硬件只接受 dmabuf 地址（当前 `edma->dmabuf`），则可能需要更深层的硬件层改动，风险较高。

---

### 2.4 INT8 GEMM 不可测

**现状**:
- `xdnn::fc<int8_t,int8_t,int8_t,int8_t>` 模板无实例化 → 编译/运行失败
- `libxpuapi_hidden.a` 中存在 `fc_int8`、`fc_int8_internal`、`fc_int8_quant_qkv` 等符号
- 公开头文件 `xdnn.h` / `nn.h` 中 **无** `fc_int8` 声明

**软件解决可能性**: ⭐⭐⭐ **中高**

| 方案 | 可行性 | 预期效果 |
|------|:------:|----------|
| A. 使用 `fc_int8` C API（需逆向签名或找内部头） | 中高 | 可能测到真实 INT8 吞吐 |
| B. 混合精度 `fc<int8_t,int8_t,float16,int32_t>` 等组合探测 | 中 | 需逐个模板尝试 |
| C. 使用 Paddle INT8 量化推理间接验证 | 高 | 端到端 img/s，非裸 GEMM |
| D. 换新版 xdnn SDK | 未知 | 需获取安装包 |

**推荐**: 阶段 3 先用 `nm`/`c++filt` 还原 `fc_int8` 签名，编写探测程序；并行用 Paddle 动态量化跑 ResNet-50 对比。

---

### 2.5 SM 超频 / PCIe 扩宽

| 项目 | 可行性 | 说明 |
|------|:------:|------|
| SM 900→更高 | ❌ 不可行 | 固件锁定，无 sysfs 接口 |
| PCIe ×8→×16 | ❌ 不可行 | K200 硬件设计 ×8，LnkCap 已封顶 |
| ncluster 调优 | ✅ 已完成 | FP16 最优 nc=4 |

**结论**: 非软件可解，移出迭代范围。

---

## 3. 优先级排序

| 优先级 | 问题 | 可行性 | 收益 | 风险 |
|:------:|------|:------:|------|:----:|
| P0 | 加载修改版驱动 + 编译测试二进制 | — | 解锁后续所有驱动测试 | 低 |
| P1 | P2P host-staging 实现 | 高 | 同卡双 PD 可协同推理 | 中 |
| P2 | xpu_host_alloc mmap 实现 | 中高 | API 完整性 | 中 |
| P3 | INT8 探测与基准 | 中高 | 补齐性能画像 | 低 |
| P4 | Pinned DMA 直传改造 | 低中 | 带宽提升（不确定） | 高 |
| — | SSE DMA 跨 PD 硬件调试 | 低 | 极高带宽（若成功） | 高 |
| — | 超频/PCIe | 不可行 | — | — |

---

## 4. 环境结论

**可以开始实施**。硬件（2×K200 + 4 XPU）、工具链（GCC/Make/headers）、运行时（XPURT 4.33）均就绪。

**前置动作**:
1. 替换加载仓库版 `kunlun.ko`（当前为 DKMS 原版）
2. 编译 `tests/test_p2p_verify` 等测试二进制
3. Python 依赖用 `uv venv` 隔离在项目 `.venv/`