# Kunlun K200 性能测试总结报告

> ⚠️ **历史基线档案（优化前）** — 测试时间 **2026-05-07**。  
> 文中 PCIe ~4.3 GB/s、P2P「未实现」等数字 **不是当前仓库状态**。  
> **当前数字** 请看 `README.md`、`docs/impl/20260715-project-closure.md`、`results/cases/SUMMARY.md`。

> 测试时间: 2026-05-07  
> 测试环境: Podman 容器 (localhost/xpu-dev:22.04), Driver 4.33  
> 硬件配置: 2 张 K200 卡, 4 个 XPU 设备 (/dev/xpu0-3)

---

## 1. 硬件概览

| 项目 | 值 |
|------|-----|
| 型号 | Kunlun K200 x 2 |
| 设备数 | 4 (每卡 2 个 XPU) |
| HBM 容量 | 8064 MB / 设备 |
| L3 缓存 | 16 MB / 设备 |
| 工作频率 | 900 MHz |
| PCIe | same-card pairs (BDF machine-local; redacted) |
| 基线温度 | 36-38°C |
| 基线功耗 | 37-38 W |

---

## 2. DMA HBM 带宽测试 (Host ↔ Device)

测试指令: `test_dma --loop 3 <dev> <size_bytes>`
数据为 4 设备的算术平均。

| 数据量 | H2D PCIe | D2H PCIe | H2D User | D2H User |
|--------|----------|----------|----------|----------|
| 1 MB   | 3.898 GB/s | 4.358 GB/s | 4.266 GB/s | 4.562 GB/s |
| 64 MB  | 4.265 GB/s | 4.379 GB/s | 4.720 GB/s | 4.263 GB/s |
| 256 MB | 4.273 GB/s | 4.398 GB/s | 4.906 GB/s | 4.282 GB/s |
| 1 GB   | 4.283 GB/s | 4.401 GB/s | 5.090 GB/s | 4.336 GB/s |

结论:
- PCIe 实测带宽约 **4.3 GB/s (H2D) / 4.4 GB/s (D2H)**
- 随数据量增大带宽稳定，未见降速
- 各设备间差异 < 3%, 一致性良好
- D2H 方向略高于 H2D

---

## 3. DMA L3 带宽测试

测试指令: `test_dma --l3 --loop 3 <dev> <size_bytes>`

| 数据量 | H2D PCIe | D2H PCIe | H2D User | D2H User |
|--------|----------|----------|----------|----------|
| 1 MB   | 3.868 GB/s | 4.399 GB/s | 4.218 GB/s | 4.542 GB/s |
| 4 MB   | 3.929 GB/s | 4.473 GB/s | 4.265 GB/s | 4.752 GB/s |
| 8 MB   | 3.938 GB/s | 4.472 GB/s | 4.204 GB/s | 4.739 GB/s |

结论:
- L3 带宽与 HBM 持平，不构成通路瓶颈
- D2H 方向表现略优

---

## 4. CDNN 计算单元诊断

测试指令: `cdnn_diag_kl1 <dev_id>`

| 设备 | CDNN 单元 | 状态 |
|------|-----------|------|
| Dev 0 | cdnn 0/1/2/3 | OK, 无异常 |
| Dev 1 | cdnn 0/1/2/3 | OK, 无异常 |
| Dev 2 | cdnn 0/1/2/3 | OK, 无异常 |
| Dev 3 | cdnn 0/1/2/3 | OK, 无异常 |

结论: 每个 XPU 设备包含 4 个 CDNN (Compute) 计算单元, 全部 16 个单元工作正常。

## 5. Compute 吞吐测试

测试指令: `test_launch --loop 50 --kernel-num 1 --data-len <N> --nclusters <C> --perf <dev>`
Kernel: elementwise_add (C[i] = A[i] + B[i], float32)

| Elements | nClusters | Wait Avg | HBM BW(est) | 说明 |
|----------|-----------|----------|-------------|------|
| 128      | 1         | 11.7 µs  | -           | 基准延迟 (launch+sync overhead) |
| 128      | 4         | 14.0 µs  | -           | 多 cluster, 小数据量无收益 |
| 5,120    | 1         | 29.6 µs  | 2.1 GB/s   | 受 overhead 影响 |
| 5,120    | 4         | 20.6 µs  | 3.0 GB/s   | 4×cluster 加速 ~1.4x |
| 1,048,576| 1         | 3165 µs  | 4.0 GB/s   | 稳定在 PCIe BW 水平 |
| 16,777,216| 1        | 50267 µs | 4.0 GB/s   | 线性扩展 |
| 67,108,864| 1        | 200443 µs| 4.0 GB/s   | 线性扩展, PASS |

结论:
- elementwise_add 是**内存带宽敏感型**操作, 吞吐约 4 GB/s, 受 PCIe 通信限制
- 多 cluster (nc=4) 相比单 cluster 有小幅加速 (~1.4x on 5K elements)
- 无 compute-bound benchmark (矩阵乘/卷积), 无法直接测量 TFLOPS
- 如需精确计算性能, 建议: (a) 编译 CDNN 库中的 benchmark (b) 使用 klprof 对实际推理 workload profile (c) 联系技术支持获取专用测试工具

---

## 6. Kernel Launch 测试

测试指令: `test_launch --loop 100 --kernel-num 10 --perf <dev>`

每次迭代 launch 10 个 elementwise_add kernel 后执行一次 wait。

### 单线程 (1T)

| 设备 | Launch 平均 | Wait 平均 | Total 平均 | 状态 |
|------|-------------|-----------|-----------|------|
| Dev 0 | 2.05 µs | 17.17 µs | 19.22 µs | PASS |
| Dev 1 | 2.43 µs | 16.84 µs | 19.28 µs | PASS |
| Dev 2 | 2.46 µs | 16.88 µs | 19.33 µs | PASS |
| Dev 3 | 2.37 µs | 16.87 µs | 19.24 µs | PASS |

### 多线程 (4T, 每线程独立 stream)

| 设备 | Launch 平均 | Wait 平均 | Total 平均 | 状态 |
|------|-------------|-----------|-----------|------|
| Dev 0 | 2.54 µs | 16.94 µs | 19.48 µs | PASS |
| Dev 1 | 2.45 µs | 16.96 µs | 19.41 µs | PASS |
| Dev 2 | 2.61 µs | 16.88 µs | 19.49 µs | PASS |
| Dev 3 | 2.19 µs | 17.14 µs | 19.33 µs | PASS |

结论:
- Kernel launch 延迟约 **2.0-2.6 µs/kernel**
- Wait 延迟约 **16.8-17.2 µs** (1000 次迭代稳定)
- 多线程(4T)与单线程性能几乎持平，硬件调度效率高
- 全部 8 组测试 **PASS**, 无数据完整性错误

---

## 7. Peer-to-Peer Memcpy 测试

测试指令: `test_memcpy_peer --perf --loop 3 <src> <dst> <size>`

| 路径 | 结果 |
|------|------|
| 同卡 dev0→dev1 | 失败 - ioctl 不支持 (807) |
| 同卡 dev2→dev3 | 失败 - ioctl 不支持 (807) |
| 跨卡 dev0→dev2 | 失败 - ioctl 不支持 (807) |
| 跨卡 dev2→dev0 | 失败 - ioctl 不支持 (807) |

结论:
- 当前驱动版本 (4.33) **未实现 P2P memcpy ioctl**
- 同卡和跨卡均不可用
- 需升级驱动或联系 Kunlun 技术支持

---

## 8. 能耗与稳定性

| 指标 | 测试前 | 测试后 | 变化 |
|------|--------|--------|------|
| 温度 | 36-38°C | 36-38°C | ±0°C |
| 功耗 | 37-38 W | 37-38 W | ±0 W |
| 设备使用率 | 0% | 3-4% | 轻微 |

- 短期负载未引起温度/功耗显著上升
- 测试结束后有残余进程 (test_dma, test_launch)，数分钟内自动释放

---

## 9. 总结

1. **PCIe 带宽符合预期**: H2D ~4.3 GB/s, D2H ~4.4 GB/s (PCIe 3.0 x16 理论 ~7.9 GB/s, 实测约 55%)
2. **Kernel launch 效率高**: ~2µs/kernel, 多线程无调度开销
3. **L3 缓存不构成瓶颈**: 带宽与 HBM 一致
4. **P2P memcpy 不可用**: 驱动缺陷，需升级
5. **容器化方案验证通过**: Podman 直通 XPU 设备后功能完整
