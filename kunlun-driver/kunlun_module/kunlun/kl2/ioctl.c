/*
 * SPDX-FileCopyrightText: Copyright (c) 2021-2022 KUNLUNXIN CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/pci.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "kl2/kl2.h"

static int __maybe_unused wait_session_timeout_interruptible(struct kl2_session *sess,
                                                             u64                 timeout_us)
{
    unsigned long sleep_us = 400;
    ktime_t       __timeout;
    int           last_unfinished = 0;

    unsigned long hybrid_busy_time_us = 500;
    ktime_t       __hybrid_wait_timeout;
    int           hybrid_ite = 0;

    might_sleep();

    __timeout             = ktime_add_us(ktime_get(), timeout_us);
    __hybrid_wait_timeout = ktime_add_us(ktime_get(), hybrid_busy_time_us);

    for (;;) {
        ktime_t __current  = ktime_get();
        int     unfinished = atomic_read(&sess->unfinished_cnt);
        if (unfinished == 0)
            return -sess->errno;

        if (unfinished != last_unfinished) {
            last_unfinished = unfinished;
            __timeout       = ktime_add_us(__current, timeout_us);
        }

        if (timeout_us && ktime_compare(__current, __timeout) > 0)
            return -XPUERR_TIMEOUT;

        if (signal_pending_state(TASK_INTERRUPTIBLE, current))
            return -ERESTARTSYS;

        if (sess->errno)
            return -sess->errno;

        {
            // hybrid wait
            if (ktime_compare(__current, __hybrid_wait_timeout) > 0) {
                if (hybrid_ite < 20) {
                    __hybrid_wait_timeout = ktime_add_us(__current, hybrid_busy_time_us);
                    usleep_range(4, 16);
                } else {
                    usleep_range((sleep_us >> 2) + 1, sleep_us);
                }
                ++hybrid_ite;
            }
            cpu_relax();
        }
    }

    return 0;
}

static int __maybe_unused wait_condition(struct kl2_session *sess)
{
    return (atomic_read(&sess->unfinished_cnt) == 0) || (sess->errno != 0);
}

static int __maybe_unused wait_session_event_timeout_interruptible(struct kl2_session *sess,
                                                                   u64                 timeout_us)
{
    int ret = 0;

    switch (wait_event_interruptible_timeout(sess->wait_queue, wait_condition(sess),
                                             usecs_to_jiffies(timeout_us))) {
    case -ERESTARTSYS:
        // 直接返回-ERESTARTSYS，令glibc重新发起ioctl syscall或及时处理SIGINT/SIGTERM等终止信号
        // 20220406 快手测试时，不明原因线程收到信号，到达该分支，原返回-XPUERR_INTERRUPTED不正确
        // 20220420 kl2 xpu_wait()改为永远等待，直到发生kernel异常、超时或收到终止信号，
        //          返回-ERESTARTSYS仍可令glibc及时处理终止信号且不发生死循环
        ret = -ERESTARTSYS;
        break;
    case 0:
        ret = -XPUERR_TIMEOUT;
        break;
    default:
        if (sess->errno != 0) {
            ret = -sess->errno;
        }
        break;
    }
    return ret;
}

static int ioctl_malloc(struct kl2_session *sess, void __user *argp)
{
    struct XPUMemoryAllocIoctlArgs args;
    struct kl2_device             *kl2_dev = sess->kl2_dev;
    void                          *owner   = (void *)sess;
    struct kl_memory              *mem     = NULL;
    u64                            page_needed;
    u64                            addr;
    int                            idx;
    int err, err_cg __maybe_unused;
    kl_cxpu_instance_t *cxpu_instance = sess->uproc->cxpu_instance;
    int                 kind;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    kind = (args.kind == XPU_MEM_MAIN_KL2_HUGE) ? XPU_MEM_MAIN : args.kind;

    // XXX(miaotianxiang):
    // 不一定是最终提供内存分配的mem，但下面需要page_size信息，有优化空间
    for (idx = 0; idx < kl2_dev->mm.mem_count; ++idx) {
        if (kl2_dev->mm.mem[idx].kind == kind) {
            mem = &kl2_dev->mm.mem[idx];
            break;
        }
    }
    if (mem) {
        page_needed = (args.size + mem->page_size - 1) >> mem->page_bits;
    } else {
        KL2_LOGD("invalid ioctl_malloc, args.kind= %d\n", args.kind);
        return -XPUERR_INVALID_PARAM;
    }

    // TODO(zhengxiaowei): 后续cXPU memory相关的操作考虑挪到kl_mm模块中去处理
    spin_lock(&sess->uproc->lock);
    err_cg = kl_cxpu_instance_check_mem_limit(cxpu_instance, mem->kind, page_needed);
    if (err_cg) {
        KL2_LOGD(
                "alloc failed, cxpu mem page used %llu, mem page needed %llu, mem page limit %llu\n",
                kl_cxpu_instance_get_mem_used(cxpu_instance, mem->kind), page_needed,
                kl_cxpu_instance_get_mem_limit(cxpu_instance, mem->kind));
        spin_unlock(&sess->uproc->lock);
        return -XPUERR_NOMEM;
    } else {
        kl_cxpu_instance_add_mem_used(cxpu_instance, mem->kind, page_needed);
    }
    spin_unlock(&sess->uproc->lock);

    // XXX(miaotianxiang): 最终提供内存分配的mem在这里更新
    err = kl_mm_malloc(&kl2_dev->mm, args.size, args.kind, owner, &addr, &mem,
                       kl2_mm_malloc_update_stat_cb);
    if (err) {
        kl_cxpu_instance_sub_mem_used(cxpu_instance, mem->kind, page_needed);
        return err;
    }

    args.addr = addr;
    if (copy_to_user(argp, &args, sizeof(args))) {
        kl_mm_free(&kl2_dev->mm, args.addr, owner, kl2_mm_free_update_stat_cb);
        return -EFAULT;
    }

    return 0;
}

static int ioctl_free(struct kl2_session *sess, void __user *argp)
{
    struct kl2_device *kl2_dev = sess->kl2_dev;
    void              *owner   = (void *)sess;
    struct kl_memory  *mem;
    u64                page_id;
    u64                page_freed;
    u64                args;
    int                err;

    if (copy_from_user((void *)&args, argp, sizeof(args))) {
        return -EFAULT;
    }

    mem = find_memory_by_addr(&kl2_dev->mm, args);
    if (mem) {
        page_id    = addr_to_pageid(mem, args);
        page_freed = mem->free_table[page_id];
    } else {
        page_freed = 0;
    }

    err = kl_mm_free(&kl2_dev->mm, args, owner, kl2_mm_free_update_stat_cb);

    return err;
}

static int kl2_br_bw_valid_check(struct kl2_device *kl2_dev, struct XPURegisterIoctlArgs *args)
{
    struct kl_device *kdev     = kl2_dev->kdev;
    u64               reg_size = 4;

    if (args->bar < 0 || args->bar >= PCIE_BAR_NUM || !kdev->bar[args->bar]) {
        KL2_LOGW("invalid bar %d\n", args->bar);
        return 1;
    }
    // XXX(miaotianxiang): 首个判断条件不可省略，以防u64上溢出
    if (args->addr >= kdev->bar_info.bar_size[args->bar] ||
        args->addr + reg_size > kdev->bar_info.bar_size[args->bar]) {
        KL2_LOGW("invalid bar offset %llx, bar= %d\n", args->addr, args->bar);
        return 1;
    }

    return 0;
}

static int ioctl_br(struct kl2_device *kl2_dev, void __user *argp)
{
    struct XPURegisterIoctlArgs args;
    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    if (kl2_br_bw_valid_check(kl2_dev, &args)) {
        return -EINVAL;
    }

    args.value = kl2_readl(kl2_dev, kl2_dev->kdev->bar[args.bar] + args.addr);
    KL2_LOGD("br bar_%d + 0x%llx = 0x%x\n", args.bar, args.addr, args.value);

    if (copy_to_user(argp, &args, sizeof(args)))
        return -EFAULT;

    return 0;
}

static int ioctl_bw(struct kl2_device *kl2_dev, void __user *argp)
{
    struct XPURegisterIoctlArgs args;
    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    if (kl2_br_bw_valid_check(kl2_dev, &args)) {
        return -EINVAL;
    }

    kl2_writel(kl2_dev, args.value, kl2_dev->kdev->bar[args.bar] + args.addr);
    KL2_LOGD("bw bar_%d + 0x%llx = 0x%x\n", args.bar, args.addr, args.value);

    return 0;
}

int kl2_ioctl_memcpy_h2d(struct kl2_session *sess, void __user *argp)
{
    struct kl2_device          *kl2_dev = sess->kl2_dev;
    struct kl2_userprocess     *uproc   = sess->uproc;
    struct XPUMemcpyExIoctlArgs args;
    struct kl2_sg_minfo        *minfo = NULL;
    int                         ret   = 0;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    if (kl2_dma_valid_check(kl2_dev, &args, 1))
        return -EINVAL;

    args.time_ns = 0;

    KL2_LOGD("h2d %016llx\n", args.dest);

    read_lock(&uproc->sg_minfo_lock);
    ret = kl2_host_memory_is_pinned(uproc, args.src, args.size, &minfo);
    if (minfo) {
        xref_get(&minfo->xref);
    }
    read_unlock(&uproc->sg_minfo_lock);
    /*
     * XXX(weihaoji): 经过测试，连续的物理页大小为 2MB 左右时 dma 速率达到峰值 25GB/s
     *                在这个区间内 block dma 的性能优于 sgdma，因为 sgdma 可能拿不到连续的 2MB 物理页
     */
    if ((args.size > 2 * 1024 * 1024) && (ret == KL2_HOSTMEM_PINNED)) {
        ret = kl2_dma_sgdma(&kl2_dev->dma_engine, args.dest, args.src, args.size, minfo, 1,
                            &args.time_ns);
    } else {
        ret = kl2_dma_ddma_from_host(&kl2_dev->dma_engine, args.dest, args.src, args.size,
                                     &args.time_ns);
    }

    if (minfo) {
        xref_put(&minfo->xref, kl2_destroy_minfo_ref);
    }

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    trace_xpu_memcpy(&args, kl2_dev->kdev->idx, kl2_dev->kdev->idx, XPU_HOST_TO_DEVICE);

    return ret;
}

int kl2_ioctl_memcpy_d2h(struct kl2_session *sess, void __user *argp)
{
    struct kl2_device          *kl2_dev = sess->kl2_dev;
    struct kl2_userprocess     *uproc   = sess->uproc;
    struct XPUMemcpyExIoctlArgs args;
    struct kl2_sg_minfo        *minfo = NULL;
    int                         ret   = 0;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    if (kl2_dma_valid_check(kl2_dev, &args, 0))
        return -EINVAL;

    args.time_ns = 0;

    KL2_LOGD("d2h %016llx\n", args.src);

    read_lock(&uproc->sg_minfo_lock);
    ret = kl2_host_memory_is_pinned(uproc, args.dest, args.size, &minfo);
    if (minfo) {
        xref_get(&minfo->xref);
    }
    read_unlock(&uproc->sg_minfo_lock);

    if ((args.size > 2 * 1024 * 1024) && (ret == KL2_HOSTMEM_PINNED)) {
        ret = kl2_dma_sgdma(&kl2_dev->dma_engine, args.dest, args.src, args.size, minfo, 0,
                            &args.time_ns);
    } else {
        ret = kl2_dma_ddma_to_host(&kl2_dev->dma_engine, args.dest, args.src, args.size,
                                   &args.time_ns);
    }

    if (minfo) {
        xref_put(&minfo->xref, kl2_destroy_minfo_ref);
    }

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    trace_xpu_memcpy(&args, kl2_dev->kdev->idx, kl2_dev->kdev->idx, XPU_DEVICE_TO_HOST);

    return ret;
}

int kl2_ioctl_memcpy_d2d(struct kl2_session *sess, void __user *argp)
{
    struct kl2_device          *kl2_dev = sess->kl2_dev;
    struct XPUMemcpyExIoctlArgs args;
    int                         ret = 0;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    // 分别检查 src 和 dst 的合法性
    if (kl2_dma_valid_check(kl2_dev, &args, 1))
        return -EINVAL;
    if (kl2_dma_valid_check(kl2_dev, &args, 0))
        return -EINVAL;

    args.time_ns = 0;

    KL2_LOGD("d2d %016llx %016llx\n", args.src, args.dest);

    ret = kl2_dma_device_to_device(&kl2_dev->dma_engine, args.dest, args.src, args.size,
                                   &args.time_ns);

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    trace_xpu_memcpy(&args, kl2_dev->kdev->idx, kl2_dev->kdev->idx, XPU_DEVICE_TO_DEVICE);

    return ret;
}

int kl2_ioctl_memcpy_p2p(struct kl2_device *kl2_dev, void __user *argp)
{
    struct kl_device           *dst_kdev   = NULL;
    struct kl2_device          *dst_kl2dev = NULL;
    struct XPUMemcpyExIoctlArgs args;
    int                         src_devid, dst_devid;
    int                         ret = 0;

    if (copy_from_user(&args, argp, sizeof(args))) {
        return -EFAULT;
    }

    args.time_ns = 0;
    // 高 4bit 编码了 devid
    src_devid = (args.src >> 60) & 0xf;
    dst_devid = (args.dest >> 60) & 0xf;
    args.src  = args.src & (~(0xfULL << 60));
    args.dest = args.dest & (~(0xfULL << 60));

    dst_kdev = get_kdev_by_devfile_id(dst_devid);
    if (!dst_kdev)
        return -EINVAL;
    dst_kl2dev = (struct kl2_device *)dst_kdev->data;

    // 分别检查 src 和 dst 的合法性
    if (kl2_dma_valid_check(kl2_dev, &args, 1))
        return -EINVAL;
    if (kl2_dma_valid_check(kl2_dev, &args, 0))
        return -EINVAL;

    if (kl2_dev->kinode->devfile_id == dst_kl2dev->kinode->devfile_id) {
        return -EINVAL;
    }
    if (kl2_dev->kinode->devfile_id != src_devid) {
        return -EINVAL;
    }
    if (dst_kl2dev->kinode->devfile_id != dst_devid) {
        return -EINVAL;
    }

    // TODO(weihaoji): 走 PCIE 的方法在诸多机器上出现了问题，暂时只能通过 host 进行 p2p
    ret = kl2_dma_peer_to_peer(&dst_kl2dev->dma_engine, /* dst_dma  */
                               &kl2_dev->dma_engine,    /* src_dma  */
                               args.dest,               /* dst      */
                               args.src,                /* src      */
                               args.size,               /* cpsz     */
                               &args.time_ns);          /* time_ns  */

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    trace_xpu_memcpy(&args, kl2_dev->kdev->idx, dst_kdev->idx, 3);

    return ret;
}

int kl2_map_memcpy_p2p_direct(struct kl2_device *kl2_dev)
{
    int                ret = 0;
    int                i;
    struct kl2_device *dst_kl2_dev;
    nv_dma_device_t    src_dma_dev;
    nv_dma_device_t    dst_dma_dev;
    struct resource   *res;
    u64                iova;

    if (kl2_dev->p2p.valid) {
        return 0;
    }

    mutex_lock(&kl2_dev->big_global_lock);
    if (kl2_dev->p2p.valid) {
        goto out;
    }

    src_dma_dev.dev                     = &kl2_dev->kdev->pdev->dev;
    src_dma_dev.addressable_range.limit = kl2_dev->kdev->pdev->dma_mask;

    for (i = 0; i < MAX_DEVICE_NUM; ++i) {
        struct kl_device *kdev = &g_devs[i];
        if (!kdev->info)
            continue;

        if (kdev->info->kl_code == KL2) {
            dst_kl2_dev = kdev->data;
            if (kl2_dev == dst_kl2_dev) {
                continue;
            }

            dst_dma_dev.dev                     = &dst_kl2_dev->kdev->pdev->dev;
            dst_dma_dev.addressable_range.limit = dst_kl2_dev->kdev->pdev->dma_mask;
            res                                 = &dst_kl2_dev->kdev->pdev->resource[0x4];
            iova                                = res->start;

            nv_dma_map_peer(&src_dma_dev, &dst_dma_dev, 0x4,
                            (res->end + 1 - res->start) / PAGE_SIZE, &iova);
            kl2_dev->p2p.peer_bar4_iova[i] = iova;
            KL2_LOGI("kl2_dev%d map kl2_dev%d BAR4[%llx~%llx] to iova %llx\n", kl2_dev->kdev->idx,
                     dst_kl2_dev->kdev->idx, res->start, res->end, iova);

            if ((res->end + 1 - res->start) >= 0x4000000ull) {
                int dst_devfile_id = dst_kl2_dev->kinode->devfile_id;
                kl2_dev->p2p.peer_iova[dst_devfile_id][0].devfile_id = dst_devfile_id;
                kl2_dev->p2p.peer_iova[dst_devfile_id][0].start      = 0xc0000000ull;
                kl2_dev->p2p.peer_iova[dst_devfile_id][0].end        = 0xc4000000ull;
                kl2_dev->p2p.peer_iova[dst_devfile_id][0].iova       = iova;
                kl2_dev->p2p.peer_iova[dst_devfile_id][0].valid      = true;
            }
            if ((res->end + 1 - res->start) >= 0x1000000000ull) {
                int dst_devfile_id = dst_kl2_dev->kinode->devfile_id;
                kl2_dev->p2p.peer_iova[dst_devfile_id][1].devfile_id = dst_devfile_id;
                kl2_dev->p2p.peer_iova[dst_devfile_id][1].start      = 0x800000000ull;
                kl2_dev->p2p.peer_iova[dst_devfile_id][1].end        = 0x1000000000ull;
                kl2_dev->p2p.peer_iova[dst_devfile_id][1].iova       = iova + 0x800000000ull;
                kl2_dev->p2p.peer_iova[dst_devfile_id][1].valid      = true;
            }
        }
    }
    kl2_dev->p2p.valid = true;

out:
    mutex_unlock(&kl2_dev->big_global_lock);
    return ret;
}

int kl2_unmap_memcpy_p2p_direct(struct kl2_device *kl2_dev)
{
    int                ret = 0;
    int                i;
    struct kl2_device *dst_kl2_dev;
    nv_dma_device_t    src_dma_dev;
    struct resource   *res;
    u64                iova;

    if (!kl2_dev->p2p.valid) {
        return 0;
    }

    src_dma_dev.dev                     = &kl2_dev->kdev->pdev->dev;
    src_dma_dev.addressable_range.limit = kl2_dev->kdev->pdev->dma_mask;

    for (i = 0; i < MAX_DEVICE_NUM; ++i) {
        struct kl_device *kdev = &g_devs[i];
        if (!kdev->info)
            continue;

        if (kdev->info->kl_code == KL2) {
            dst_kl2_dev = kdev->data;
            if (kl2_dev == dst_kl2_dev) {
                continue;
            }

            res  = &dst_kl2_dev->kdev->pdev->resource[0x4];
            iova = res->start;

            nv_dma_unmap_peer(&src_dma_dev, (res->end + 1 - res->start) / PAGE_SIZE, iova);
            KL2_LOGI("kl2_dev%d unmap kl2_dev%d BAR4[%llx~%llx] from iova %llx\n",
                     kl2_dev->kdev->idx, dst_kl2_dev->kdev->idx, res->start, res->end,
                     kl2_dev->p2p.peer_bar4_iova[i]);
            kl2_dev->p2p.peer_bar4_iova[i] = 0ull;

            memset(&kl2_dev->p2p.peer_iova, 0, sizeof(kl2_dev->p2p.peer_iova));
        }
    }
    kl2_dev->p2p.valid = false;

    return ret;
}

static int ioctl_memcpy_p2p_direct(struct kl2_device *kl2_dev, void __user *argp)
{
    struct kl_device           *dst_kdev   = NULL;
    struct kl2_device          *dst_kl2dev = NULL;
    struct XPUMemcpyExIoctlArgs args;
    int                         src_devid, dst_devid;
    int                         ret       = 0;
    u64                         peer_dest = 0;
    int                         i;

    kl2_map_memcpy_p2p_direct(kl2_dev);

    if (copy_from_user(&args, argp, sizeof(args))) {
        return -EFAULT;
    }

    args.time_ns = 0;
    // 高 4bit 编码了 devid
    src_devid = (args.src >> 60) & 0xf;
    dst_devid = (args.dest >> 60) & 0xf;
    args.src  = args.src & (~(0xfULL << 60));
    args.dest = args.dest & (~(0xfULL << 60));

    dst_kdev = get_kdev_by_devfile_id(dst_devid);
    if (!dst_kdev)
        return -EINVAL;
    dst_kl2dev = (struct kl2_device *)dst_kdev->data;

    // 分别检查 src 和 dst 的合法性
    if (kl2_dma_valid_check(kl2_dev, &args, 1))
        return -EINVAL;
    if (kl2_dma_valid_check(kl2_dev, &args, 0))
        return -EINVAL;

    if (kl2_dev->kinode->devfile_id == dst_kl2dev->kinode->devfile_id) {
        return -EINVAL;
    }
    if (kl2_dev->kinode->devfile_id != src_devid) {
        return -EINVAL;
    }
    if (dst_kl2dev->kinode->devfile_id != dst_devid) {
        return -EINVAL;
    }

    if (kl2_dev->p2p.valid) {
        for (i = 0; i < 2; ++i) {
            if (kl2_dev->p2p.peer_iova[dst_devid][i].valid &&
                args.dest >= kl2_dev->p2p.peer_iova[dst_devid][i].start &&
                args.dest < kl2_dev->p2p.peer_iova[dst_devid][i].end) {
                peer_dest = kl2_dev->p2p.peer_iova[dst_devid][i].iova +
                            (args.dest - kl2_dev->p2p.peer_iova[dst_devid][i].start);
                break;
            }
        }
    }
    if (peer_dest) {
        // 对端BAR size满足要求，成功获取到了p2p direct应访问地址
        ret = kl2_dma_ddma_to_peer(&kl2_dev->dma_engine, peer_dest, args.src, args.size,
                                   &args.time_ns);
    } else {
        // 不满足条件，仍尝试slow path
        ret = kl2_dma_peer_to_peer(&dst_kl2dev->dma_engine, /* dst_dma  */
                                   &kl2_dev->dma_engine,    /* src_dma  */
                                   args.dest,               /* dst      */
                                   args.src,                /* src      */
                                   args.size,               /* cpsz     */
                                   &args.time_ns);          /* time_ns  */
    }

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    return ret;
}

static int ioctl_host_register(struct kl2_session *sess, void __user *argp)
{
    struct XPUHostRegisterIoctlArgs args;
    struct kl2_userprocess         *uproc = sess->uproc;
    struct kl2_sg_minfo            *minfo = NULL, *new_minfo = NULL;
    int                             ret = 0;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    read_lock(&uproc->sg_minfo_lock);
    ret = kl2_host_memory_is_pinned(uproc, args.ptr, args.size, &minfo);
    read_unlock(&uproc->sg_minfo_lock);
    if (ret == KL2_HOSTMEM_UNPINNED) {
        new_minfo = kzalloc(sizeof(*new_minfo), GFP_KERNEL);
        if (!new_minfo) {
            ret = -ENOMEM;
            return ret;
        }

        // XXX(miaotianxiang):
        // kl2_pin_host_memory前先unlock，返回后再重新lock，避免同kl2_host_alloc_hugepages构成死锁
        //
        // ======================================================
        // WARNING: possible circular locking dependency detected
        // 4.18.0-348.2.1.el8_5.x86_64+debug #1 Tainted: G           OE    --------- -  -
        // ------------------------------------------------------
        // test_memcpy_dea/7027 is trying to acquire lock:
        // ffff88810a655670 (&mm->mmap_lock#2){++++}-{3:3}, at: kl2_pin_host_memory+0x201/0xaa0 [kunlun]
        //
        // but task is already holding lock:
        // ffff88816762a918 (&uproc->sg_minfo_lock){+.+.}-{3:3}, at: kl2_ioctl+0x342/0x2040 [kunlun]
        //
        // which lock already depends on the new lock.
        //
        //
        // the existing dependency chain (in reverse order) is:
        //
        // -> #1 (&uproc->sg_minfo_lock){+.+.}-{3:3}:
        //        lock_acquire+0x1b1/0x890
        //        __mutex_lock+0x163/0x13d0
        //        kl2_mmap+0x535/0xaf0 [kunlun]
        //        mmap_region+0x914/0x1330
        //        do_mmap+0x6a0/0xd30
        //        vm_mmap_pgoff+0x171/0x1d0
        //        ksys_mmap_pgoff+0x391/0x620
        //        do_syscall_64+0xa5/0x430
        //        entry_SYSCALL_64_after_hwframe+0x6a/0xdf
        //
        // -> #0 (&mm->mmap_lock#2){++++}-{3:3}:
        //        check_prevs_add+0x3cd/0x21d0
        //        __lock_acquire+0x21d1/0x2c60
        //        lock_acquire+0x1b1/0x890
        //        down_read+0xaa/0x770
        //        kl2_pin_host_memory+0x201/0xaa0 [kunlun]
        //        kl2_ioctl+0x1748/0x2040 [kunlun]
        //        do_vfs_ioctl+0x193/0x1050
        //        ksys_ioctl+0x60/0x90
        //        __x64_sys_ioctl+0x6f/0xb0
        //        do_syscall_64+0xa5/0x430
        //        entry_SYSCALL_64_after_hwframe+0x6a/0xdf
        //
        // other info that might help us debug this:
        //
        //  Possible unsafe locking scenario:
        //
        //        CPU0                    CPU1
        //        ----                    ----
        //   lock(&uproc->sg_minfo_lock);
        //                                lock(&mm->mmap_lock#2);
        //                                lock(&uproc->sg_minfo_lock);
        //   lock(&mm->mmap_lock#2);
        //
        //  *** DEADLOCK ***
        //
        ret = kl2_pin_host_memory(uproc, args.ptr, args.size, new_minfo);
        if (ret) {
            kfree(new_minfo);
            return ret;
        }

        write_lock(&uproc->sg_minfo_lock);
        ret = kl2_minfo_rb_insert(&uproc->sg_minfo_rb, new_minfo);
        write_unlock(&uproc->sg_minfo_lock);
        if (ret) {
            // 极小概率，同进程的另一线程并行register了该区域
            kl2_unpin_host_memory(new_minfo);
            kfree(new_minfo);
        }
    } else if (ret == KL2_HOSTMEM_PARTLY_PINNED || ret == KL2_HOSTMEM_PINNED) {
        ret = -XPUERR_HOSTMEM_ALREADY_REGISTERED;
    } /* else ret == -EINVAL */

    return ret;
}

static int ioctl_host_unregister(struct kl2_session *sess, void __user *argp)
{
    struct XPUHostRegisterIoctlArgs args;
    struct kl2_userprocess         *uproc = sess->uproc;
    struct kl2_sg_minfo            *minfo = NULL;
    int                             ret   = 0;

    if (copy_from_user((void *)&args, argp, sizeof(args))) {
        return -EFAULT;
    }

    write_lock(&uproc->sg_minfo_lock);
    ret = kl2_host_memory_is_pinned(uproc, args.ptr, 1, &minfo);
    if (ret == KL2_HOSTMEM_PINNED) {
        args.size = minfo->size;
        if (minfo->addr != args.ptr) {
            ret = -EINVAL;
        } else {
            if (!minfo->mmaped) {
                kl2_minfo_rb_erase(&uproc->sg_minfo_rb, minfo);
            }
            ret = 0;
        }
    } else { /* KL2_HOSTMEM_UNPINNED or KL2_HOSTMEM_PARTLY_PINNED or -EINVAL */
        ret = -EINVAL;
    }
    write_unlock(&uproc->sg_minfo_lock);

    if (ret == 0) {
        if (!minfo->mmaped) {
            // 存在对应的 minfo 且对应的 pin-memory 是 register 出来的
            xref_put(&minfo->xref, kl2_destroy_minfo_ref);
        }
        if (copy_to_user(argp, &args, sizeof(args))) {
            ret = -EFAULT;
        }
    }

    return ret;
}

static void set_task_params(struct kl2_device *kl2_dev, struct kl2_task *task,
                            struct XPULaunchIoctlArgs *args)
{
    if (args->params[0] == 1) {
        // api launch kernel passes a cpu user-space pointer
        if (copy_from_user((void *)task->params, (const void __user *)args->param_addr,
                           args->kernel.param_dword_size * sizeof(u32))) {
            return;
        }
    } else {
        //kl2_dma_ddma_to_host_kernel(&kl2_dev->dma_engine, (u64)task->params, (u64)args->param_addr,
        //                            sz);
    }
}

int kl2_ioctl_launch(struct kl2_session *sess, void __user *argp)
{
    DECLARE_PROFILER(PROF_launch);
    DECLARE_PROFILER(PROF_launch_usercpy);
    DECLARE_PROFILER(PROF_launch_param_alloc);
    DECLARE_PROFILER(PROF_launch_param_dma);
    DECLARE_PROFILER(PROF_launch_preparextask);
    DECLARE_PROFILER(PROF_launch_sesslaunch);

    struct XPULaunchIoctlArgs args;
    struct kl2_device        *kl2_dev = sess->kl2_dev;
    struct kl2_task          *task;
    u64                       param_addr;
    u32                       token;
    int                       err;

    START_PROFILING(PROF_launch);
    START_PROFILING(PROF_launch_usercpy);

    if (copy_from_user(&args, argp, sizeof(args))) {
        END_PROFILING(PROF_launch_usercpy, kl2_dev->kdev);
        return -EFAULT;
    }

    END_PROFILING(PROF_launch_usercpy, kl2_dev->kdev);

    task = kzalloc(sizeof(*task), GFP_KERNEL);
    if (!task)
        return -ENOMEM;

    if (args.params[0] == 1) { // api launch kernel passes a cpu user-space pointer
        u64 time_ns;

        START_PROFILING(PROF_launch_param_alloc);
        // TODO(miaotianxiang): 后续更新XPU_MEM_PARAM占用内存统计
        err = kl_mm_malloc(&kl2_dev->mm, args.kernel.param_dword_size * sizeof(u32), XPU_MEM_PARAM,
                           sess, &param_addr, NULL, NULL);
        END_PROFILING(PROF_launch_param_alloc, kl2_dev->kdev);
        if (err)
            goto err_free_task;

        START_PROFILING(PROF_launch_param_dma);
        err = kl2_dma_ddma_from_host(&kl2_dev->dma_engine, param_addr, args.param_addr,
                                     args.kernel.param_dword_size * sizeof(u32), &time_ns);
        END_PROFILING(PROF_launch_param_dma, kl2_dev->kdev);
        PROF_ADD_DATA(PROF_launch_param_dma_pure, kl2_dev->kdev, time_ns);
        if (err)
            goto err_release_param;

        task->flag |= KL2_TASKFLAG_FREE_PARAM;
    } else { // run_sse_tc passes a xpu pointer
        param_addr = args.param_addr;
    }

    START_PROFILING(PROF_launch_preparextask);
    token = atomic_add_return(2, &kl2_dev->task_token) - 2;

    task->desc.kernel.type       = args.kernel.type;
    task->desc.kernel.nclusters  = args.nclusters;
    task->desc.kernel.ncores     = args.ncores;
    task->desc.kernel.token      = token;
    task->desc.kernel.codelen    = (args.kernel.code_byte_size + 3) & ~0x3;
    task->desc.kernel.code_addr  = args.kernel.code_addr;
    task->desc.kernel.param_addr = param_addr;
    // for compatibility
    task->desc.kernel.param0 = args.kernel.param_dword_size * sizeof(u32);
    task->type               = KL2_TASKTYPE_KERNEL;

    strncpy(task->kernel_name, args.name, XPU_MAX_STRLEN);
    task->kernel_name[XPU_MAX_STRLEN - 1] = '\0';
    set_task_params(kl2_dev, task, &args);
    END_PROFILING(PROF_launch_preparextask, kl2_dev->kdev);

    START_PROFILING(PROF_launch_sesslaunch);
    err = kl2_session_add_task(sess, task);
    END_PROFILING(PROF_launch_sesslaunch, kl2_dev->kdev);

    END_PROFILING(PROF_launch, kl2_dev->kdev);
    if (err)
        goto err_release_param;

    args.kernel_enter_cycle = __xpurt_prflr_PROF_launch_t0__;
    args.kernel_exit_cycle  = __xpurt_prflr_PROF_launch_t1__;
    args.param_addr         = param_addr;

    if (copy_to_user(argp, &args, sizeof(args))) {
        err = -EFAULT;
    }

    trace_kernel_launch(sess, &args, token);

    return 0;

err_release_param:
    if (args.params[0] == 1)
        kl_mm_free(&kl2_dev->mm, param_addr, sess, NULL);

err_free_task:
    kfree(task);

    return err;
}

static int ioctl_proflaunch(struct kl2_session *sess, void __user *argp)
{
    struct XPUProfLaunch       args;
    struct XPULaunchIoctlArgs *launch  = &args.launch;
    struct kl2_device         *kl2_dev = sess->kl2_dev;
    struct kl2_task           *task;
    u64                        param_addr;
    u32                        token;
    u32                        cost;
    int                        err;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    task = kzalloc(sizeof(*task), GFP_KERNEL);
    if (!task)
        return -ENOMEM;

    if (launch->params[0] == 1) { // api launch kernel passes a cpu user-space pointer
        // TODO(miaotianxiang): 后续更新XPU_MEM_PARAM占用内存统计
        err = kl_mm_malloc(&kl2_dev->mm, launch->kernel.param_dword_size * sizeof(u32),
                           XPU_MEM_PARAM, sess, &param_addr, NULL, NULL);
        if (err)
            goto err_free_task;

        err = kl2_dma_ddma_from_host(&kl2_dev->dma_engine, param_addr, launch->param_addr,
                                     launch->kernel.param_dword_size * sizeof(u32), NULL);
        if (err)
            goto err_release_param;

        task->flag |= KL2_TASKFLAG_FREE_PARAM;
    } else { // run_sse_tc passes a xpu pointer
        param_addr = launch->param_addr;
    }

    token = atomic_add_return(2, &kl2_dev->task_token) - 2;

    task->desc.kernel.type       = launch->kernel.type;
    task->desc.kernel.nclusters  = launch->nclusters;
    task->desc.kernel.ncores     = launch->ncores;
    task->desc.kernel.token      = token;
    task->desc.kernel.codelen    = (launch->kernel.code_byte_size + 3) & ~0x3;
    task->desc.kernel.code_addr  = launch->kernel.code_addr;
    task->desc.kernel.param_addr = param_addr;
    // for compatibility
    task->desc.kernel.param0 = launch->kernel.param_dword_size * sizeof(u32);
    task->type               = KL2_TASKTYPE_KERNEL;

    strncpy(task->kernel_name, launch->name, XPU_MAX_STRLEN);
    task->kernel_name[XPU_MAX_STRLEN - 1] = '\0';
    set_task_params(kl2_dev, task, launch);

    err = kl2_session_bind_hwq(sess);
    if (err)
        goto err_release_param;

    // clean RC reg
    kl2_sse_hwq_last_cycles(sess->hwq);

    err = kl2_session_add_task(sess, task);
    if (err)
        goto err_release_param;

#ifndef USE_POLL_WAIT
    err = wait_session_event_timeout_interruptible(sess, IOCTL_WAIT_TIMEOUT);
#else
    err = wait_session_timeout_interruptible(sess, IOCTL_WAIT_TIMEOUT);
#endif

    cost = kl2_sse_hwq_last_cycles(sess->hwq);

    args.cycles = cost;
    if (copy_to_user(argp, &args, sizeof(args)))
        err = -EFAULT;

    return err;

err_release_param:
    if (launch->params[0] == 1)
        kl_mm_free(&kl2_dev->mm, param_addr, sess, NULL);

err_free_task:
    kfree(task);

    return err;
}

int kl2_ioctl_wait(struct kl2_session *sess, void __user *argp)
{
    int ret;
#ifndef USE_POLL_WAIT
    ret = wait_session_event_timeout_interruptible(sess, IOCTL_WAIT_TIMEOUT);
#else
    ret = wait_session_timeout_interruptible(sess, IOCTL_WAIT_TIMEOUT);
#endif
    trace_xpu_wait(sess);
    return ret;
}

static int ioctl_debug_master(struct kl2_session *sess, void __user *argp)
{
    struct XPUDBGMIoctlArgs args;
    int                     ret = 0;

    if (copy_from_user(&args, argp, sizeof(args))) {
        return -EFAULT;
    }

    switch (args.type) {
    case DBGM_IOC_ENABLE:
        ret = kl2_dbgm_enable(sess, (void __user *)args.args, args.sz);
        break;
    case DBGM_IOC_DISABLE:
        ret = kl2_dbgm_disable(sess);
        break;
    case DBGM_IOC_START:
        ret = kl2_dbgm_start(sess->uproc);
        break;
    case DBGM_IOC_STOP:
        ret = kl2_dbgm_stop(sess->uproc);
        break;
    case DBGM_IOC_MBOX_WRITE:
        ret = kl2_dbgm_mbox_write(sess, (void __user *)args.args, args.sz);
        break;
    case DBGM_IOC_MBOX_READ:
        ret = kl2_dbgm_mbox_read(sess, (void __user *)args.args, args.sz);
        break;
    case DBGM_IOC_RELAX:
        ret = kl2_dbgm_sleep_until_new_cmd(sess);
        break;
    case DBGM_IOC_INUSE:
        ret = kl2_dbgm_in_use(sess, (void __user *)args.args, args.sz);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static int ioctl_prof_clear(struct file *file, struct kl2_device *kl2_dev, void __user *argp)
{
    struct kl_device *kdev = kl2_dev->kdev;
    int               i    = 0;

    for (i = 0; i < PROFILER_COUNT; ++i) {
        kdev->profiler[i].cost  = 0;
        kdev->profiler[i].count = 0;
    }

    return 0;
}

static int ioctl_dev_soft_reset(struct kl2_device *kl2_dev)
{
    return kl2_dev_soft_reset(kl2_dev, 1, 0);
}

static int ioctl_dev_set_numvfs(void __user *argp, struct kl2_device *kl2_dev)
{
    int num_vfs;

    if (copy_from_user(&num_vfs, argp, 4))
        return -EFAULT;

    if (num_vfs < 0 || num_vfs > KL2_SRIOV_MAX_NUM_VFS)
        return -EINVAL;

    return kl2_dev_set_numvfs(kl2_dev, num_vfs);
}

static int ioctl_ioc_version(void __user *argp)
{
    int ioc_version = IOC_VERSION;
    if (copy_to_user(argp, &ioc_version, sizeof(int)))
        return -EFAULT;
    return 0;
}

static int ioctl_query_device_info(struct kl2_device *kl2_dev, void __user *argp)
{
    int                     ret = 0;
    struct xpu_device_info *di  = vzalloc(sizeof(*di));
    if (!di) {
        return -ENOMEM;
    }

    switch (kl2_dev->dev_info.board) {
    case KL2_BOARD_ID_R100:
        di->model = R100;
        break;
    case KL2_BOARD_ID_R200:
        di->model = R200;
        break;
    case KL2_BOARD_ID_R300:
        di->model = R300;
        break;
    case KL2_BOARD_ID_R200_8F:
        di->model = R200_8F;
        break;
    case KL2_BOARD_ID_R200_8FS:
        di->model = R200_8FS;
        break;
    case KL2_BOARD_ID_R200_DEBUG_BOARD:
        di->model = R200_DEBUG_BOARD;
        break;
    case KL2_BOARD_ID_R420:
        di->model = R420;
        break;
    case KL2_BOARD_ID_RG800:
        di->model = RG800;
        break;
    case KL2_BOARD_ID_RG800_PRO:
        di->model = RG800_PRO;
        break;
    case KL2_BOARD_ID_RM80:
        di->model = RM80;
        break;
    default:
        ret = -XPUERR_NOSUPPORT;
        goto free_di;
    }

    di->id = kl2_dev->kinode->devfile_id;

    di->board_idx = kl2_dev->kdev->idx;

    di->chip_idx = 0;

    di->domain = kl2_dev->kdev->domain;
    di->bus    = kl2_dev->kdev->bus;
    di->slot   = kl2_dev->kdev->slot;
    di->func   = kl2_dev->kdev->func;

    // FIXME: Cannot handle situation where different regions of the same mem have
    //        different page sizes. Move this query into control_node and use
    //        a total byte size instead to fix it.
    di->cache_mem_page_all  = kl_mm_get_pg_all(&kl2_dev->mm, XPU_MEM_L3);
    di->cache_mem_page_size = kl_mm_get_pgsz(&kl2_dev->mm, XPU_MEM_L3);

    di->main_mem_page_all  = kl_mm_get_pg_all(&kl2_dev->mm, XPU_MEM_MAIN);
    di->main_mem_page_size = kl_mm_get_pgsz(&kl2_dev->mm, XPU_MEM_MAIN);

    if (copy_to_user(argp, di, sizeof(*di))) {
        ret = -EFAULT;
    }

free_di:
    vfree(di);
    return ret;
}

static int ioctl_driver_version(void __user *argp)
{
    struct XPUDriverVersionIoctlArgs args;
    args.major = XPURT_VERSION_MAJOR;
    args.minor = XPURT_VERSION_MINOR;
    strncpy(args.commit, XPURT_COMMIT, XPU_MAX_STRLEN);

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    return 0;
}

static bool is_valid_cmd(struct kl2_device *kl2_dev, unsigned int cmd)
{
    int  sriov_func_id = kl2_dev->dev_info.sriov_func_id;
    bool is_valid      = false;

    if (is_pf_id(sriov_func_id)) {
        switch (cmd) {
        case IOCTL_LAUNCH:
        case IOCTL_PROFLAUNCH:
        case IOCTL_WAIT:
        case IOCTL_EVENT_CREATE:
        case IOCTL_EVENT_DESTROY:
        case IOCTL_EVENT_RECORD:
        case IOCTL_EVENT_WAIT:
        case IOCTL_STREAM_WAIT_EVENT:
            is_valid = false;
            break;
        case IOCTL_MEMORY_ALLOC:
        case IOCTL_MEMORY_FREE:
        case IOCTL_REG_READ:
        case IOCTL_REG_WRITE:
        case IOCTL_MEMCPY_H2D:
        case IOCTL_MEMCPY_H2D_EX:
        case IOCTL_MEMCPY_D2H:
        case IOCTL_MEMCPY_D2H_EX:
        case IOCTL_MEMCPY_D2D:
        case IOCTL_HOST_REGISTER:
        case IOCTL_HOST_UNREGISTER:
        case IOCTL_DBGM:
        case IOCTL_PROFCLR:
        case IOCTL_DEV_HARD_RESET:
        case IOCTL_DEV_SOFT_RESET:
        case IOCTL_DEV_SET_NUMVFS:
        case IOCTL_IOC_VERSION:
        case IOCTL_QUERY_DEVINFO:
        case XPUCTL_VERSION:
            is_valid = true;
            break;

        default:
            is_valid = false;
            break;
        }
    } else if (is_vf_id(sriov_func_id)) {
        if (cmd == IOCTL_DEV_SET_NUMVFS) {
            is_valid = false;
        } else {
            is_valid = true;
        }
    } else {
        is_valid = true;
    }

    if (!is_valid)
        KL2_LOGW("invalid ioctl cmd send to sriov function[%d]!\n", sriov_func_id);

    return is_valid;
}

long kl2_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct kl2_session *sess    = file->private_data;
    struct kl2_device  *kl2_dev = sess->kl2_dev;
    void __user        *argp    = (void __user *)arg;
    int                 err;

    if (!is_valid_cmd(kl2_dev, cmd)) {
        return -EINVAL;
    }

    switch (cmd) {
    case IOCTL_MEMORY_ALLOC:
        err = ioctl_malloc(sess, argp);
        break;
    case IOCTL_MEMORY_FREE:
        err = ioctl_free(sess, argp);
        break;
    case IOCTL_REG_READ:
        err = ioctl_br(kl2_dev, argp);
        break;
    case IOCTL_REG_WRITE:
        err = ioctl_bw(kl2_dev, argp);
        break;
    case IOCTL_MEMCPY_H2D:
    case IOCTL_MEMCPY_H2D_EX:
        err = kl2_ioctl_memcpy_h2d(sess, argp);
        break;
    case IOCTL_MEMCPY_D2H:
    case IOCTL_MEMCPY_D2H_EX:
        err = kl2_ioctl_memcpy_d2h(sess, argp);
        break;
    case IOCTL_MEMCPY_D2D:
        err = kl2_ioctl_memcpy_d2d(sess, argp);
        break;
    case IOCTL_HOST_REGISTER:
        err = ioctl_host_register(sess, argp);
        break;
    case IOCTL_HOST_UNREGISTER:
        err = ioctl_host_unregister(sess, argp);
        break;
    case IOCTL_MEMCPY_P2P:
        err = kl2_ioctl_memcpy_p2p(kl2_dev, argp);
        break;
    case IOCTL_MEMCPY_P2P_DIRECT:
        err = ioctl_memcpy_p2p_direct(kl2_dev, argp);
        break;
    case IOCTL_LAUNCH:
        err = kl2_ioctl_launch(sess, argp);
        break;
    case IOCTL_PROFLAUNCH:
        err = ioctl_proflaunch(sess, argp);
        break;
    case IOCTL_WAIT:
        err = kl2_ioctl_wait(sess, argp);
        break;
    case IOCTL_EVENT_CREATE:
        err = ioctl_event_create(sess, argp);
        break;
    case IOCTL_EVENT_DESTROY:
        err = ioctl_event_destroy(sess, argp);
        break;
    case IOCTL_EVENT_RECORD:
        err = ioctl_event_record(sess, argp);
        break;
    case IOCTL_EVENT_WAIT:
        err = ioctl_event_wait(sess, argp);
        break;
    case IOCTL_STREAM_WAIT_EVENT:
        err = ioctl_event_stream_wait(sess, argp);
        break;
    case IOCTL_DBGM:
        err = ioctl_debug_master(sess, argp);
        break;
    case IOCTL_PROFCLR:
        err = ioctl_prof_clear(file, kl2_dev, argp);
        break;
    case IOCTL_DEV_HARD_RESET:
    case IOCTL_DEV_SOFT_RESET:
        err = ioctl_dev_soft_reset(kl2_dev);
        break;
    case IOCTL_DEV_SET_NUMVFS:
        err = ioctl_dev_set_numvfs(argp, kl2_dev);
        break;

        // TODO(miaotianxiang):
    case IOCTL_IOC_VERSION:
        err = ioctl_ioc_version(argp);
        break;
    case IOCTL_QUERY_DEVINFO:
        err = ioctl_query_device_info(kl2_dev, argp);
        break;
    case XPUCTL_VERSION:
        err = ioctl_driver_version(argp);
        break;

    default:
#ifdef ENABLE_CODEC
        if (_IOC_TYPE(cmd) == IOCTL_VDEC_IOC_MAGIC) {
            err = vdec_process_ioctl(kl2_dev->video_dec, file, cmd, arg);
        } else if (_IOC_TYPE(cmd) == IOCTL_VENC_IOC_MAGIC) {
            err = venc_process_ioctl(kl2_dev->video_enc, file, cmd, arg);
        } else if (_IOC_TYPE(cmd) == IOCTL_IMGPROC_MAGIC) {
            err = imgproc_process_ioctl(kl2_dev->image_proc, file, cmd, arg);
        } else
#endif // ENABLE_CODEC
        {
            //LOGW("unknown ioctl type=%x nr=%d\n", _IOC_TYPE(cmd), _IOC_NR(cmd));
            err = -XPUERR_NOIOC;
        }
        break;
    }

    return err;
}
