# K200 XPU 驱动调优 — Implementation 阶段

**时间**: 2026-05-15

## 1. 修改文件清单

| 文件 | 行变化 | 说明 |
|------|:------:|------|
| `kl1/xpu_drv.h` | +5 | 前向声明 (dma_p2p, ioctl_p2p, ioctl_host_*) |
| `kl1/xpu_fops.c` | +35 | dispatch + _IOC_NR 拦截 + 调试日志 |
| `kl1/xpu_ioctl.c` | +100 | host_register stub, memcpy_p2p handler, 调试日志 |
| `kl1/xpu_dma.c` | +25 | dma_device_to_device_p2p 函数 |
| `kl1/xpu_drv.h` | ±1 | KL1_DMA_KBUF_SIZE 1MB→4MB (reverted in concept) |

## 2. Git 提交历史

```
17d8726 fix: _IOC_TYPE(cmd) == _IOC_TYPE(magic), not == magic directly
1c7b2fb debug P2P: log raw src/dst/bar addresses
32c3a00 fix P2P: match by _IOC_NR instead of exact cmd (library uses different struct size)
90280ef debug: log at kl_char_ioctl and ctrldev levels to trace P2P ioctl routing
465bcbe debug: log P2P ioctl command numbers to diagnose -807
2d737bc kl1: also handle IOCTL_MEMCPY_P2P_DIRECT (cmd 146)
8c50878 kl1: add P2P DMA between PDs on same K200 card
5ec3c42 KL1 DMA buffer: 1MB -> 4MB
3d22cab kl1: add IOCTL_HOST_REGISTER/UNREGISTER stub handlers
0c0cd7e Build: 首次编译成功, kunlun.ko (21.5MB)
7dfe316 Initial: Kunlun K200 driver 4.33.0 source
```

## 3. 关键代码片段

### 3.1 _IOC_NR 拦截 (xpu_fops.c:144)

```c
if (_IOC_TYPE(cmd) == _IOC_TYPE(IOCTL_IOC_MAGIC)) {
    if (_IOC_NR(cmd) == _IOC_NR(IOCTL_MEMCPY_P2P_DIRECT) ||
        _IOC_NR(cmd) == _IOC_NR(IOCTL_MEMCPY_P2P)) {
        return ioctl_memcpy_p2p_kl1(xpd, argp);
    }
    if (_IOC_NR(cmd) == _IOC_NR(IOCTL_HOST_REGISTER)) {
        return ioctl_host_register_kl1(xpd, argp);
    }
    if (_IOC_NR(cmd) == _IOC_NR(IOCTL_HOST_UNREGISTER)) {
        return ioctl_host_unregister_kl1(xpd, argp);
    }
}
```

**原因**: XPURT 库编译时使用了与驱动不同版本的 `XPUMemcpyIoctlArgs`（size=3837 vs 48），导致 ioctl cmd 编码不同。按 `_IOC_NR` 匹配而非精确 cmd 匹配。

### 3.2 P2P handler (xpu_ioctl.c:1216)

```c
int ioctl_memcpy_p2p_kl1(struct xpu_pd *xpd, void __user *argp) {
    // 提取 device ID（编码在地址高 4 位，同 KL2 协议）
    src_devid = (args.src >> 60) & 0xf;
    dst_devid = (args.dest >> 60) & 0xf;
    
    // 限定同卡 P2P
    if (dst_devid / XPU_PD_NUM != xpd->devfile_id / XPU_PD_NUM)
        return -XPUERR_NOIOC;  // 跨卡不支持
    
    // 转换为绝对 BAR 地址
    u64 dst_bar = args.dest + dst_xpd->rbase;
    u64 src_bar = args.src + xpd->rbase;
    
    return dma_device_to_device_p2p(xpd, dst_bar, src_bar, args.size, &cycles);
}
```

### 3.3 调试: ioctl 命令号追踪

在 `kl_char_ioctl`、`xpu_char_ioctl`、`ctrldev_ioctl` 三个入口记录所有 P2P ioctl 的 cmd/type/nr/size，用于诊断 -807 根因。

## 4. 测试结果汇总

### 4.1 host_register

| 项目 | 修改前 | 修改后 |
|------|:-----:|:-----:|
| `xpu_host_register` | **-807** | **0** (OK) |
| `xpu_host_unregister` | -807 | 0 (OK) |
| H2D bandwidth | 5.58 GB/s | 5.66 GB/s |
| D2H bandwidth | 3.66 GB/s | 3.58 GB/s |

**结论**: 通路打通，带宽无变化（KL1 DMA 架构限制）。

### 4.2 DMA buffer

| Size | H2D 1MB | H2D 4MB | D2H 1MB | D2H 4MB |
|------|---------|---------|---------|---------|
| 256MB | 5.58 | 5.67 | 3.66 | 3.66 |
| 1GB | 5.35 | 5.36 | 3.63 | 3.64 |

**结论**: 无差异，PCIe DMA 引擎硬件上限。

### 4.3 P2P DMA

| 阶段 | 状态 |
|------|------|
| IOCTL 路由 | ✓ 成功路由到 handler |
| 数据验证 (dev0→dev1) | ✗ 全零 — SSE DMA hang |
| 跨卡 (dev0→dev2) | ✗ 不支持（预期） |

**dmesg 追踪**:
- 首次 `_IOC_TYPE` bug: ioctl 到达但 `_IOC_TYPE(cmd) == IOCTL_IOC_MAGIC` 永远 false
- 修复后: handler 被调用，SSE DMA 写入后无完成中断，进程 hang

## 5. 环境信息

- **内核**: 6.8.0-111-generic, GCC 12.3.0
- **模块**: build 于 `kunlun-driver/kunlun.ko`
- **备份**: 原模块 `kunlun.ko.bak` (BuildID: 798c818f), 源文件 `kunlun.ko.orig`
- **XPURT**: `/usr/local/xpu-4.33.0/lib64/libxpurt.so.1`
- **测试工具**: `tests/test_p2p_verify`, `tests/test_p2p`, `tests/test_bw`

## 6. 构建可复现性 (2026-05-20 验证)

### 6.1 源码完整性

```
kunlun-driver 追踪文件: 436
├── 源码 (.c/.h):  199 files
├── conftest 头:    152 files (编译时生成, 已缓存)
├── 构建文件:         3 files (Makefile, Kbuild, conftest.sh)
├── 文档:            3 files (docs/research|plan|impl)
└── 其他:           79 files (clang-format, udev, 版权等)
```

### 6.2 构建命令

```bash
cd kunlun-driver && make modules
# 输出: kunlun.ko (21.5 MB, with debug_info)
```

### 6.3 外部依赖

| 依赖 | 来源 | 必要性 |
|------|------|:-----:|
| kernel headers | `linux-headers-$(uname -r)` (系统包) | 必须 |
| GCC | 系统包 (12.3.0 已验证) | 必须 |
| ld | binutils | 必须 |

无其他外部工具链依赖。Makefile 自动检测 `/lib/modules/$(uname -r)/build`。

### 6.4 未追踪文件

| 文件 | 原因 |
|------|------|
| `*.ko, *.o, *.mod, *.mod.c, ...` | 编译产物, .gitignore |
| `kunlun.ko.new`, `kunlun.ko.orig` | 编译备份 (已清理) |
| `conftest/*` 重新生成 | 若内核版本不同, conftest 自动重跑 |

### 6.5 跨内核编译注意

conftest 缓存在 `kunlun_module/conftest/compile-tests/*.h`，针对内核 6.8.0-111-generic 生成。换内核版本时 `make modules` 会自动重新运行 `conftest.sh` 并覆盖这些文件 — git 会显示它们被修改。这是正常行为，无需提交新版本。
