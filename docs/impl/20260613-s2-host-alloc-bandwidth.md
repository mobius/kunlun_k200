# S2.4 host_alloc 带宽基线

**时间**: 2026-06-13  
**环境**: K200 KL1，kernel 6.8.0-124，驱动 `B8C3D291...`，XPURT 4.33

---

## 1. 结论

**`xpu_host_alloc` 相对 pageable 内存无带宽提升**（差异 < 2%，在测量噪声内）。

| 决策 | 结果 |
|------|------|
| S4 Pinned DMA 是否排期？ | **暂缓** — 需改 EDMA 直访页，投入 2–3 天，预期收益不确定 |
| S2 是否收口？ | **是** — API 可用即可，带宽非目标 |

---

## 2. 测试命令

```bash
# from repository root
make benchmarks/xpu_perf_test
./benchmarks/xpu_perf_test 0 bw
```

---

## 3. 结果对比

| 大小 | Pageable H2D | host_alloc H2D | Pageable D2H | host_alloc D2H |
|------|:------------:|:--------------:|:------------:|:--------------:|
| 1MB | 5.38 | 5.34 | 5.56 | 5.47 |
| 64MB | 5.57 | 5.59 | 4.88 | 4.91 |
| 256MB | 5.53 | 5.55 | 4.88 | 4.91 |
| 1GB | 5.54 | 5.60 | 4.90 | 4.98 |

单位：GB/s，100 次取平均（`xpu_perf_test` WARMUP=3, RUNS=100）。

---

## 4. 根因（与架构文档一致）

KL1 `dma_host_to_device` / `dma_device_to_host` 固定路径：

```
H2D: copy_from_user → edma->kbuf → EDMA → device
D2H: device → EDMA → edma->kbuf → copy_to_user
```

`ioctl_host_register_kl1` 为 stub，**不 pin 用户页到 EDMA**。  
`xpu_host_alloc` 仅改变内存来源（驱动 mmap 页），**不绕过 bounce buffer**。

因此 host_alloc 与 `aligned_alloc` + pageable 的 DMA 性能等价。

---

## 5. S4 若要做的前提

1. 验证 EDMA 是否支持 host 物理地址 / scatter-gather（当前 `edma->dmabuf` 为设备侧缓冲）
2. 在 `dma_*` 中识别 host_alloc VMA，对 pin 页 `dma_map_page` 后直传
3. 分片逻辑（`KL1_DMA_KBUF_SIZE`）需与直传路径共存

无明确业务需求前，**不建议启动 S4**。