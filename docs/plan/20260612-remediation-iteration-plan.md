# 未解决问题软件补救迭代计划

**时间**: 2026-06-12  
**前置文档**:  
- `docs/research/20260612-environment-and-feasibility.md`  
- `docs/architecture/20260612-kl1-kl2-capability-gap.md`

---

## 0. 总览

| 阶段 | 目标 | 预估工期 | 前置条件 |
|:----:|------|:--------:|----------|
| S0 | 环境就绪 + 基线复现 | 0.5 天 | 无 |
| S1 | P2P host-staging 驱动实现 | 1–2 天 | S0 | **已完成** → `docs/impl/20260612-s1-p2p-fix-and-s2-mmap.md` |
| S2 | xpu_host_alloc mmap 实现 | 1–2 天 | S0 | **已完成** → 同上；**S2.4 带宽** → `docs/impl/20260613-s2-host-alloc-bandwidth.md` |
| S3 | INT8 性能探测 | 0.5–1 天 | S0 | **已完成（不可用）** → `docs/impl/20260613-s3-int8-probe-results.md` |
| S4 | Pinned DMA 直传 | 2–3 天 | S2 完成 | **已完成** → `docs/impl/20260613-s4-pinned-dma.md` |
| S5 | P2P 零拷贝 + ping-pong + ioctl 修复 | 1 天 | S1/S4 | **已完成** P2P ~11.2 GB/s → `docs/impl/20260714-s5-p2p-pingpong.md` |
| S6 | pageable bounce 双缓冲（可回退） | 0.5–1 天 | S5 | **已完成** pageable H2D ~10 GB/s → `docs/impl/20260714-s6-bounce-pipeline.md` |
| S7 | 回归门禁（正确性 + 带宽地板） | 0.5 天 | S4–S6 | **已完成** → `docs/impl/20260715-s7-regression-gate.md` |

每阶段产出 `docs/impl/YYYYMMDD-<phase>-results.md`。

---

## S0: 环境就绪与基线复现

### 目标
确认修改版驱动可加载，测试二进制可编译运行，建立可对比的基线数据。

### 步骤

| # | 动作 | 验证标准 |
|---|------|----------|
| 0.1 | 备份当前 DKMS 模块 → `kunlun.ko.bak` | 文件存在 |
| 0.2 | 编译 `kunlun-driver/kunlun.ko` | `make modules` 成功 |
| 0.3 | 替换加载: `modprobe -r kunlun && cp kunlun.ko ... && modprobe kunlun` | `xpu_smi` 正常 |
| 0.4 | 编译测试二进制 (`test_p2p`, `test_p2p_verify`, `xpu_perf_test`) | 无链接错误 |
| 0.5 | `uv venv .venv` + 安装 Paddle 基准依赖 | 仅项目本地 |
| 0.6 | 运行基线: 带宽 / D2D / P2P(预期 hang 或失败) | 记录到 impl 文档 |

### 环境约束
- 驱动替换需 root 权限
- 不修改 `/usr/local/xpu-4.33.0` 全局安装
- Python 依赖限定在 `/mnt/storage/test_xpu/.venv/`

### 回滚
```bash
sudo modprobe -r kunlun
sudo cp /lib/modules/$(uname -r)/updates/dkms/kunlun.ko.bak \
        /lib/modules/$(uname -r)/updates/dkms/kunlun.ko
sudo depmod -a && sudo modprobe kunlun
```

---

## S1: P2P Host-Staging 实现

### 目标
同卡 PD0↔PD1 的 `xpu_memcpy_peer` 功能正确（数据校验 PASS），不再 hang。

### 设计

放弃 SSE DMA 跨 PD 路径，改为仿 KL2 的双 EDMA ping-pong：

```
ioctl_memcpy_p2p_kl1()
    └── kl1_dma_peer_to_peer(src_xpd, dst_xpd, dst, src, sz)   [新增]
            ├── src: dma_device_to_host_edma()  → bounce[k]
            └── dst: dma_host_to_device_edma()  ← bounce[k]
            (ping-pong 重叠，使用两 PD 各自的 EDMA 通道)
```

### 修改文件

| 文件 | 变更 |
|------|------|
| `kl1/xpu_dma.c` | 新增 `kl1_dma_peer_to_peer()` |
| `kl1/xpu_drv.h` | 前向声明 |
| `kl1/xpu_ioctl.c` | `ioctl_memcpy_p2p_kl1` 改调新函数 |
| `kl1/xpu_dma.c` | 保留 `dma_device_to_device_p2p` 但默认不走 |

### 实现要点

1. **锁顺序**: 按 `devfile_id` 排序获取两 PD 的 `rdch_sema`/`wrch_sema`，避免死锁（参考 KL2）
2. **Bounce buffer**: 复用各 PD 现有 `edma->kbuf`（1MB），无需额外分配
3. **同卡检查**: 保持现有 `dst_devid / XPU_PD_NUM == src_devid / XPU_PD_NUM` 逻辑
4. **跨卡**: 返回 `-XPUERR_NOIOC`（K200 不支持）
5. **SSE DMA 路径**: 保留代码但加 `#if 0` 或 compile-time 开关，供后续硬件调试

### 测试

```bash
./tests/test_p2p_verify          # 期望: D2D PASS, P2P PASS
./tests/test_p2p               # 期望: 带宽 ~2-4 GB/s (host-staging)
```

### 预期结果

| 指标 | SSE DMA (当前) | Host-staging (目标) |
|------|:--------------:|:------------------:|
| 功能 | hang | PASS |
| 带宽 | N/A | 2–4 GB/s |
| 延迟 | N/A | 高于 D2D，可接受 |

### 风险

| 风险 | 等级 | 缓解 |
|------|:----:|------|
| 双 PD EDMA 通道竞争 | 中 | 信号量顺序加锁 |
| 性能不达预期 | 低 | 功能优先，后续优化 ping-pong |
| 模块崩溃 | 低 | 备份 + 快速回滚 |

---

## S2: xpu_host_alloc mmap 实现

### 目标
`xpu_host_alloc()` 返回 0，分配的内存在用户态可读写。

### 设计（简化版，非完整 KL2 移植）

```
xpu_char_mmap(file, vma)
    if (vma->vm_pgoff == 0):
        kl1_mmap_host_alloc(xpd, vma)    [新增]
            ├── alloc_pages(GFP_KERNEL | __GFP_COMP, order)
            ├── vm_insert_page() 逐页映射
            └── 记录到 xpd->host_alloc_list (用于 munmap 清理)
    else:
        return -EINVAL
```

### 修改文件

| 文件 | 变更 |
|------|------|
| `kl1/xpu_fops.c` | 实现 `xpu_char_mmap`，注册 `.mmap` |
| `kl1/xpu_mem.c` 或新文件 | `kl1_mmap_host_alloc()` + 清理逻辑 |
| `kl1/xpu_drv.h` | host_alloc 结构体 |

### 分步验证

| 步骤 | 测试 | 期望 |
|------|------|------|
| 2.1 | `xpu_host_alloc(&ptr, 64MB, 0)` | 返回 0 |
| 2.2 | 写入 pattern + 读回 | 数据一致 |
| 2.3 | `xpu_host_free(ptr)` | 无泄漏 |
| 2.4 | `xpu_perf_test` pinned 段 | 带宽可能仍不变（已知限制） |

### 带宽说明
此阶段 **不承诺** 带宽提升。若 API 可用但带宽不变，在 impl 文档记录并决定是否进入 S4。

---

## S3: INT8 性能探测

### 目标
确定 K200 是否有可用的 INT8 GEMM 路径，并测量峰值 TOPS。

### 步骤

| # | 动作 | 环境 |
|---|------|------|
| 3.1 | `c++filt` 还原 `fc_int8` 符号签名 | 本地 shell |
| 3.2 | 编写 `benchmarks/xpu_int8_probe.cpp` 探测多种 API | C++ 编译 |
| 3.3 | 模板枚举: `fc<int8_t,int8_t,float16,int32_t>` 等 | 同 xpu_perf_test |
| 3.4 | Paddle 动态量化 ResNet-50 对比 (FP32 vs INT8) | `uv venv` |

### Python 环境

```bash
cd /mnt/storage/test_xpu
uv venv .venv
source .venv/bin/activate
uv pip install numpy  # 按需添加 paddle 相关
```

若 Paddle 安装包过大或依赖冲突，改用 podman 复现既有容器环境：
```bash
podman run --device=/dev/xpu0 --device=/dev/xpu1 ... localhost/xpu-dev:22.04
```

### 预期产出
- INT8 GEMM 是否可用（是/否 + 用的 API）
- 若可用：峰值 TOPS @ 2048³
- Paddle INT8 vs FP32 img/s 对比

---

## S4: Pinned DMA 直传（可选，低优先级）

### 触发条件
S2 完成 + 有明确需求提升 H2D/D2H 带宽。

### 设计思路

```
dma_host_to_device():
    if (src_is_host_alloc_or_registered):
        dma_addr = dma_map_page(src_page)
        xpuhw_edma_write_locked(edma, dma_addr, dst, sz)  // 跳过 copy_from_user
    else:
        现有 bounce buffer 路径
```

### 风险
- EDMA 硬件可能只支持设备侧 dmabuf（需实验验证）
- 与现有 1MB 分片逻辑冲突
- 改动面大，建议仅在 S1–S3 完成后再评估

---

## 不做事项

| 项目 | 原因 |
|------|------|
| SM 超频 | 固件锁定 900 MHz |
| PCIe ×8→×16 | 硬件限制 |
| 跨卡 P2P | K200 无 BAR IOVA 映射基础设施 |
| 全局 pip install | 污染系统环境 |
| 升级 xdnn SDK | 无安装包来源，暂不纳入 |

---

## 时间线

```
2026-06-12  评估文档 (research + architecture + plan)     ← 当前
2026-06-13  S0 环境就绪 + 基线
2026-06-14  S1 P2P host-staging 实现 + 测试
2026-06-16  S2 host_alloc mmap 实现 + 测试
2026-06-17  S3 INT8 探测
2026-06-19  S4 评估（按需）
```

---

## 文档规范

每次迭代在 `docs/impl/` 记录：

```
docs/impl/YYYYMMDD-<phase>-<summary>.md
```

内容模板：
1. 环境快照（驱动版本、模块 hash）
2. 代码变更清单（文件 + 行数）
3. 测试命令 + 原始输出
4. 结果对比表（修改前 vs 修改后）
5. 遗留问题与下一步

---

## 决策点

| 检查点 | 决策 |
|--------|------|
| S1 后 P2P PASS? | 是 → 合并代码；否 → 排查 EDMA 锁/地址 |
| S2 后 host_alloc 可用? | 是 → 文档化；否 → 检查 mmap 注册 |
| S2 后带宽提升? | **否（2026-06-13）** — host_alloc ≈ pageable ~5.5 GB/s；**S4 暂缓** |
| S3 后 INT8 可用? | **否（2026-06-13）** → KL1 固件无 INT8 CDNN kernel；见 s3 impl 文档 |