# S8 应用层用法：host_alloc + FP16

**时间**: 2026-07-15  
**状态**: 已实现原生 pipeline 基准并实测  
**目标**: 把 S4–S6 驱动红利落到应用可选路径上

---

## 1. 应用可控制的开关

| 选择 | 驱动路径 | 实测量级 |
|------|----------|----------|
| `malloc` + `xpu_memcpy` | S6 bounce pipe | ~10 GB/s H2D |
| `xpu_host_alloc` + `xpu_memcpy` | S4 direct | ~12.5 GB/s H2D |
| FP32 GEMM `ncluster=4` | xdnn | ~7–8 TFLOPS 峰值级 |
| FP16 GEMM `ncluster=4` | xdnn | ~20+ TFLOPS 峰值级 |
| 权重常驻 device | 减 H2D | e2e 接近 compute-only |

---

## 2. 原生 microbench

```bash
make benchmarks/xpu_app_pipeline
./benchmarks/xpu_app_pipeline -d 0 -b 32 -f 2048 -l 8 -r 30
# or
scripts/run_s8_app_bench.sh
```

负载：H2D 输入 → N× FC(feat×feat) → D2H 输出；权重只上传一次。

### 实测（ko B8FE90…，S6 开启）

**Compute-heavy**（batch=32, feat=2048, layers=8）：

| 配置 | img/s | vs pageable+FP32 |
|------|------:|------------------|
| pageable+FP32 e2e | ~12500 | 1.00× |
| pageable+FP16 e2e | ~22500 | **~1.8×** |
| host_alloc+FP32 e2e | ~12800 | ~1.02× |
| **host_alloc+FP16 e2e** | **~22800** | **~1.8×** |
| FP16 compute-only | ~23400 | 上限 |

**I/O 占比更高**（batch=64, feat=4096, layers=1）：

| 配置 | img/s | 说明 |
|------|------:|------|
| pageable+FP16 e2e | ~55200 | |
| **host_alloc+FP16 e2e** | **~59600** | pinned 约 **+8%** |
| FP16 compute-only | ~65100 | H2D/D2H 仍吃掉约 8–15% |

结论：

1. **FP16 是第一刀**（算力密集时 ~1.8×）  
2. **host_alloc 在 I/O 可观时可见**（本机 +8% on larger mats）；S6 后 pageable 已不差  
3. 权重常驻 device 后 e2e 逼近 compute-only  

完整日志：`results/s8_app_pipeline.txt`

---

## 3. 推荐应用写法

```c
// 1) pinned staging for activations / one-shot weight upload
void *h = nullptr;
xpu_host_alloc(&h, nbytes, 0);
// fill h ...
xpu_memcpy(d_act, h, nbytes, XPU_HOST_TO_DEVICE);

// 2) FP16 compute, all CDNN clusters
ctx->set_ncluster(4);
xdnn::fc<float16,float16,float16,short>(ctx, A, B, C, m, n, k, ...);

// 3) keep K in [1024, 4096] for FP16; avoid FP32 K>2048 cliff
```

Python / Paddle 侧：

- 推理优先 **FP16**（若框架/插件支持）  
- 大 buffer 尽量 **pinned / host_alloc** 语义（Paddle XPU 版本能力因包而异）  
- batch 拉大以摊 launch 与 PCIe  
- 本仓库脚本：`scripts/paddle_infer_benchmark.py`（需本机或容器内有 `paddlepaddle-xpu`）

> 当前 `registry.baidubce.com/device/paddle-xpu:ubuntu20-...` 镜像 **未预装 paddle**，仅含工具链；历史 FP32 ResNet-50 ~1069 img/s (batch=1) 见 README。

---

## 4. 交付物

| 路径 | 说明 |
|------|------|
| `benchmarks/xpu_app_pipeline.cpp` | e2e 对比基准 |
| `scripts/run_s8_app_bench.sh` | 一键 native（+可选 paddle 容器） |
| `scripts/paddle_infer_benchmark.py` | batch 扫描 ResNet-50 |
| `results/s8_app_pipeline.txt` | 本次机器结果 |

---

## 5. 与驱动阶段关系

```
S4 host_alloc DMA  ──►  应用用 xpu_host_alloc
S6 pageable pipe   ──►  未 pinned 时也有 ~10 GB/s
S3 无 INT8         ──►  应用以 FP16 为主精度
S7 regression      ──►  make regression 锁 DMA 地板
S8                 ──►  教应用怎么选路径 + 量化收益
```
