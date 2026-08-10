# S1 修复 + S2 mmap 实施记录

**时间**: 2026-06-12

---

## 1. S1 P2P 问题诊断

加载首版 host-staging 驱动后，`test_p2p_verify` 仍在 Test 2 hang。

**根因**: `ioctl_memcpy_p2p_kl1` 向 EDMA 传入了含 `rbase` 的绝对 BAR 地址，而 KL1 的 H2D/D2H ioctl 路径使用 **PD 相对地址**。KL2 的 `kl2_dma_peer_to_peer` 同样使用相对地址。

**修复** (`kl1/xpu_ioctl.c`):

```c
// 前: kl1_dma_peer_to_peer(xpd, dst_xpd, dst_bar, src_bar, ...)
// 后: kl1_dma_peer_to_peer(xpd, dst_xpd, args.dest, args.src, ...)
```

---

## 2. S2 xpu_host_alloc mmap 实现

| 文件 | 变更 |
|------|------|
| `kl1/kl1_host_mem.c` | 新增：`alloc_page` + `vm_insert_page` + `vm_ops` 清理 |
| `kl1/xpu_fops.c` | 实现 `xpu_char_mmap`，注册 `.mmap` |
| `kl1/xpu_drv.h` | 声明 `kl1_mmap_host_alloc` |
| `kunlun-sources.Kbuild` | 加入 `kl1_host_mem.c` |
| `tests/test_host_alloc.cpp` | 冒烟测试 |

---

## 3. 需重新加载驱动

当前已加载模块与最新编译产物 hash 可能不一致。请执行：

```bash
sudo scripts/install_driver.sh
# from repo root
make tests/test_p2p_verify tests/test_p2p tests/test_host_alloc

./tests/test_p2p_verify     # 期望: D2D PASS, P2P PASS
./tests/test_p2p            # 期望: P2P 带宽 ~2-4 GB/s
./tests/test_host_alloc     # 期望: ret=0, pattern PASS
```

---

## 4. 第二轮修复（2026-06-12 晚）

加载 `4a27f6c` 后 P2P 仍 hang（90s+）。排查结论：

1. **libxpurt ioctl 布局确认**（反汇编）：栈上 40 字节，`src@0 dest@8 size@0x10 time_ns@0x20`，devid 编码在 bit[63:60]
2. **userspace staging PASS**：D2H+H2D 分步可用，问题在 kernel P2P 路径
3. **根因**：旧实现同时 `down()` 两个 PD 的 semaphore，可能导致长时间阻塞

**修复**：
- 重写 `kl1_dma_peer_to_peer`：顺序调用 `dma_device_to_kbuf` → `dma_kbuf_to_device`，每次只持有一个 PD 的 semaphore（与 userspace staging 一致）
- `ioctl_memcpy_p2p_kl1` 改用 `XPUMemcpyExIoctlArgs`，`time_ns` 回写 offset 0x20

新驱动 hash: 编译后 `make modules` 生成，需再次 `sudo scripts/install_driver.sh`

## 6. 第三轮修复（vm_mmap staging）

kbuf staging（`7b62285a...`）仍 hang（`test_p2p_verify` 45s+ 无输出）。userspace D2H+H2D 分步仍 PASS，说明应复用 `dma_device_to_host` / `dma_host_to_device`（`copy_to/from_user` 路径）。

**修复** (`kl1/xpu_dma.c`):
- 删除 `dma_device_to_kbuf` / `dma_kbuf_to_device`
- `kl1_dma_peer_to_peer`：`vm_mmap(NULL, … MAP_ANONYMOUS)` 在调用进程 mm 中映射临时缓冲 → D2H → H2D → `vm_munmap`
- 内核 6.8 `vm_mmap` 首参为 `struct file *`（匿名映射传 `NULL`），`vm_mmap`/`vm_munmap` 自行持锁

新驱动 hash: `53f50a6d6a147e04474e7e6b9cecec36eb15b034386121a82204066f6b5103d0`

```bash
sudo scripts/install_driver.sh
timeout 60 ./tests/test_p2p_verify
./tests/test_p2p
```

## 7. vm_mmap 驱动验证（`53f50a6d...`）

已加载 `srcversion=E51B54E1...`，hash 与仓库一致。

| 测试 | 结果 | 备注 |
|------|:----:|------|
| `test_p2p_mini` 4KB | **hang** | 60s timeout |
| `test_p2p_verify` | **hang** | 90s timeout，无 D2D 输出（可能 stdout 缓冲） |
| userspace staging | **PASS** | D2H→H2D 分步 ~200ms |
| 直接 `IOCTL_MEMCPY_D2H` | **PASS** | `src=0x21a0c0000` ret=0 |
| 直接 `IOCTL_MEMCPY_P2P_DIRECT` | **hang** | 25s timeout，同地址 |

**结论**: vm_mmap 与 kbuf 均 hang；问题不在 libxpurt，而在 P2P ioctl 内核路径（`dma_device_to_host` 从 `ioctl_memcpy_d2h` 调用正常，从 `kl1_dma_peer_to_peer` 调用 hang）。

### dmesg 诊断（`A5C8EEE8...`）

```
[P2P] src_raw=0x21a0c0000 dst_raw=0x100000060b000000 sz=0x1000 src_dev=0 dst_dev=1
RIP: ioctl_memcpy_p2p_kl1+0xb5
note: test_ioctl_p2p_ exited with irqs disabled
```

- ioctl handler **已进入**，参数正确
- **无** `vm_mmap` / `D2H` / `done` 日志 → 卡在 `kl1_dma_peer_to_peer` 入口（`vm_mmap` 死锁/hang）
- `exited with irqs disabled` = timeout 强杀进程时内核仍持锁

### 第四轮修复（去掉 vm_mmap）

- 改回 `kvmalloc` + `dma_device_to_kbuf` / `dma_kbuf_to_device`
- `down_interruptible` 避免不可中断 sleep
- `get_xpd_by_devfile_id(dst_devid)` 查找目标 PD
- chunk 间 `cond_resched()`

```bash
sudo scripts/install_driver.sh
sudo dmesg -C
timeout 15 /tmp/test_ioctl_p2p_direct
sudo dmesg | rg 'P2P|p2p'
```

已加 `LOGI` 跟踪（`xpu_ioctl.c` / `xpu_dma.c`），需再装一次驱动后：

```bash
sudo scripts/install_driver.sh
sudo dmesg -C
timeout 15 /tmp/test_ioctl_p2p_direct
sudo dmesg | rg 'P2P|p2p'
```

## 5. 验证结果（2026-06-12，kernel 6.8.0-124）

环境：2× K200，`/dev/xpu0–3`，XPURT 4.33，`kl1_p2p_stub=0`。

| 测试 | 结果 | 备注 |
|------|:----:|------|
| D2D | **PASS** | `test_p2p_verify` Test 1 |
| P2P verify | **PASS** | dev0→dev1，16MB×3，数据校验正确，~280ms |
| P2P bandwidth | **PASS** | 1–512MB，~2.5 GB/s（同卡 PD0→PD1） |
| host_alloc | **PASS** | 64MB `ret=0`，pattern 读写正确 |
| host_free | **PASS** | `ret=0`（见 §8） |

驱动：`srcversion=B8C3D2912CACABFE566CAB5`，`scripts/install_driver.sh` 安装。

---

## 8. S2 host_free 修复

`xpu_host_free` 流程：`IOCTL_HOST_UNREGISTER` → 驱动回写 `size` → `munmap(ptr, size)`。

KL1 stub 未 `copy_to_user` size → `munmap(ptr, 0)` → `XPUERR_INVALID_PARAM` (-707)。

**修复** (`kl1/xpu_ioctl.c` `ioctl_host_unregister_kl1`)：`find_vma` 查映射，`args.size = vm_end - vm_start`，`copy_to_user` 回写。

```bash
./tests/test_host_alloc   # alloc ret=0, free ret=0, exit=0
```

---

## 9. 调试参数

`kl1_p2p_stub=1`（`kl_main.c` `module_param`）：P2P ioctl 立即成功，不搬数据，用于确认 ioctl 不 hang。生产使用保持 `0`。

---

## 10. 优化影响

详见 `docs/impl/20260612-s1-s2-optimization-impact.md`。