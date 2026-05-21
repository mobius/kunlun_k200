# Kunlun K200 Driver Build Guide

## Prerequisites

| Dependency | Ubuntu/Debian | RHEL/CentOS |
|------------|---------------|-------------|
| Kernel headers | `apt install linux-headers-$(uname -r)` | `dnf install kernel-devel-$(uname -r)` |
| GCC | `apt install gcc` | `dnf install gcc` |
| GNU Make | (system) | (system) |

## Build

```bash
cd kunlun-driver
make modules
```

Output: `kunlun.ko` (21 MB, includes debug info).

To strip debug info for smaller size:

```bash
strip --strip-debug kunlun.ko   # reduces to ~1.2 MB (same as DKMS install)
```

### Build Notes

- **conftest autoconfiguration**: First compilation runs `conftest.sh` to detect kernel API compatibility (145 checks). Cached headers at `kunlun_module/conftest/compile-tests/` are auto-generated and safe to delete — `make clean` removes them, `make modules` regenerates.
- **Kernel version**: On kernel versions other than 6.8.0-111, conftest regenerates automatically — `git status` will show modified conftest headers (expected, do not commit).
- **Compiler warning**: `warning: the compiler differs from the one used to build the kernel` is harmless — both use GCC 12.x family.

## Install

```bash
sudo modprobe -r kunlun                                    # unload current
sudo cp kunlun.ko /lib/modules/$(uname -r)/updates/dkms/  # copy
sudo depmod -a                                             # rebuild module map
sudo modprobe kunlun                                       # load new module
```

## Verify

```bash
xpu_smi                          # should show all devices
dmesg | grep -i kunlun | tail -5 # probe log
strings kunlun.ko | grep ioctl_host_register  # confirm symbols present
```

## Clean

```bash
make clean         # removes .o .ko .mod.c conftest/
```

## Modify

Source files for K200 (KL1 architecture):

```
kunlun_module/kunlun/kl1/
├── xpu_fops.c     # ioctl dispatch table
├── xpu_ioctl.c    # ioctl handler implementations
├── xpu_dma.c      # DMA functions (H2D, D2H, D2D, P2P)
├── xpu_drv.h      # forward declarations
└── xpu_hw.c       # hardware register access
```

After modifying any source, rebuild with `make modules`.
