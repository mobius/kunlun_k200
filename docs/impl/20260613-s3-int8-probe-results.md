# S3 INT8 性能探测结果

**时间**: 2026-06-13  
**环境**: 2× K200 (KL1)，kernel 6.8.0-124，XPURT 4.33，xdnn 2.0.0.725，`/dev/xpu0–3`

---

## 1. 结论

**K200 上 INT8 GEMM 不可用**（公开 API 可链接，运行时一律 `INVALID_PARAM`）。

| 能力 | 结果 |
|------|:----:|
| `fc<int8,int8,int8,int8>` | **不可用** ret=1 |
| Quant / dequant API | **不可用** ret=1 |
| `findmax<float>` | **不可用** ret=1 |
| FP16 GEMM 基线 (2048³) | **可用** ~19.8 TFLOPS |

根因与既有分析一致：**固件/CDNN 未部署 INT8 kernel**，非驱动或本次 S1/S2 修复引入的问题。

---

## 2. 测试命令

```bash
cd /mnt/storage/test_xpu
make benchmarks/xpu_int8_probe
./benchmarks/xpu_int8_probe 0

# 交叉验证（xpu_perf_test INT8 段）
make benchmarks/xpu_perf_test
./benchmarks/xpu_perf_test 0 | grep -A8 'GEMM INT8'
```

---

## 3. `xpu_int8_probe` 原始输出

```
=== fc<int8,int8,int8,int8> probe ===
  512^3  maxptr=null   ret=1 (INVALID_PARAM)
  512^3  maxptr=set    ret=1 (INVALID_PARAM)

=== fc<int8,int8,int8,int8> size sweep ===
  small     512^3  ret=1 (INVALID_PARAM)
  medium   1024^3  ret=1 (INVALID_PARAM)
  large    2048^3  ret=1 (INVALID_PARAM)

=== Quant API probe ===
  quantization<float,int8>     ret=1 (INVALID_PARAM)
  gpt_fp16_quant_2int8         ret=1 (INVALID_PARAM)
  dequantization<int8,float>   ret=1 (INVALID_PARAM)

=== INT8 GEMM benchmark skipped ===
  fp16 baseline  2048^3  0.87 ms  19.8 TFLOPS  (eff: 8.58% vs 57.6 TFLOPS @900MHz)
```

`xpu_perf_test` INT8 段：全部 `not supported (err=1)`。

---

## 4. libxpuapi 符号分析

`nm -D libxpuapi.so | grep fc_int8` → **45** 个符号，含：

```
fc_int8_v1<signed char, signed char, signed char>   # 纯 INT8
fc_int8_v1<float16, signed char, signed char>      # 混合精度
fc_int8_v2/v3 变体
quant_int8<float>
```

公开头文件 `xdnn::fc<TX,TW,TY,TGEMM>` 仅 **`<int8,int8,int8,int8>`** 可链接；混合精度组合（如 `fc<float16,int8,int8,short>`）**无模板实例化**，无法从用户态直接调用。

即使底层 `fc_int8_v1<aaa>` 符号存在，`fc<int8,int8,int8,int8>` 在 KL1 上 dispatch 仍返回 `INVALID_PARAM` → **运行时无对应 CDNN kernel / 设备能力表未注册**。

---

## 5. 与理论峰值对比

| 精度 | 理论 (@900MHz) | 实测 (2048³) | 效率 |
|------|---------------|-------------|------|
| INT8 | 230.4 TOPS | N/A | — |
| FP16 | 57.6 TFLOPS | 19.8 TFLOPS | 8.6%* |
| FP32 | 14.4 TFLOPS | (见 xpu_perf_test) | — |

\* probe 使用 nc=4；效率分母取 57.6 TFLOPS。

---

## 6. 决策（对照 plan 决策点）

| 检查点 | 决策 |
|--------|------|
| S3 后 INT8 可用? | **否** → README/性能表标注「KL1 固件无 INT8 kernel」 |
| 是否继续深挖 fc_int8 内部符号? | **否**（需非公开 API / 固件更新，投入产出比低） |
| 下一步 | **S2.4** host_alloc 带宽基线 → 视结果决定 **S4** |

---

## 7. 新增文件

| 文件 | 说明 |
|------|------|
| `benchmarks/xpu_int8_probe.cpp` | S3 探测：fc/quant 组合 + FP16 对照 |