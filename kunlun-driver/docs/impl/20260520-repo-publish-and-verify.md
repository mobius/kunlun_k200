# 2026-05-20 工作总结

**主题**: 项目封装、GitHub 发布、构建验证

## 1. 仓库组织

### 1.1 清理冗余

- 移除 `paddle_models/ResNet50_infer.tar` 和 `.tar.1` (182 MB 重复数据)
- 模型已提取为 `inference.pdmodel` + `inference.pdiparams` + `inference.pdiparams.info`

### 1.2 Git LFS 配置

大文件 (>50 MB) 通过 Git LFS 跟踪:

| Pattern | 文件数 | 总大小 |
|---------|:------:|:------:|
| `xdnn-ubuntu_x86_64/lib/*.a` | 1 | 1357 MB |
| `xdnn-ubuntu_x86_64/lib/*.so` | 0 | — |
| `xdnn-ubuntu_x86_64/so/*.so` | 5 | ~625 MB |
| `paddle_models/*.pdiparams` | 1 | 98 MB |
| `paddle_models/*.tar` | 0 (已移除) | — |
| `*.bin` | 1 | 0.3 MB |

### 1.3 .gitignore

排除编译产物和工作文件:
- `*.ko, *.o, *.mod, ...` (内核模块构建)
- `xpu_perf_test, test_p2p, ...` (编译测试二进制)
- `.venv/`, `.deepseek/`
- `xdnn-ubuntu_x86_64.tar.gz` (548 MB SDK 归档, 已提取)

## 2. 文档结构

```
docs/
├── research/20260515-architecture-and-problems.md   (架构分析)
├── plan/20260515-iterations-and-plan.md             (迭代计划)
└── impl/20260515-modifications-and-test-results.md  (代码变更+测试+构建验证)
```

## 3. 构建可复现性

- 436 源文件全部追踪
- `make modules` 唯一外部依赖: `linux-headers-$(uname -r)`
- conftest 152 头文件已缓存 (针对 6.8.0-111)
- 跨内核自动重新生成 conftest

## 4. GitHub

```
https://github.com/mobius/kunlun_k200
SSH: git@github.com:mobius/kunlun_k200.git

Commits:
  a91e2ea  Add README with benchmarks, comparison, and driver mod summary
  d7a0bb7  Remove redundant tar archives
  76f1afd  Initial commit (LFS)
```

## 5. 遗留问题

| 问题 | 状态 | 备注 |
|------|:----:|------|
| P2P DMA hang | 未解决 | SSE DMA 跨 PD 无响应 |
| xpu_host_alloc | 未实现 | 需 KL1 mmap pgoff=0 支持 |
| DMA buffer size | 无效 | 已回退 1MB→4MB 实验 |
| INT8 GEMM | SDK 不支持 | xdnn API 不匹配 |
