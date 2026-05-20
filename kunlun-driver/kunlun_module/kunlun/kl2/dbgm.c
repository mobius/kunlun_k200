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

#include "kl2/kl2.h"
#include "kl2/hw.h"
#include "kl2/dbgm.h"

void kl2_dbgm_init(struct kl2_device *kl2_dev)
{
    struct kl2_debug_master *dbgm = &kl2_dev->dbgm;
    memset(dbgm, 0, sizeof(*dbgm));
    mutex_init(&dbgm->lock);
    init_waitqueue_head(&dbgm->wait_queue);
}

int kl2_dbgm_enable(struct kl2_session *sess, void __user *argp, u64 sz)
{
    struct XPUDebugMasterConfig args;
    struct kl2_device          *kl2_dev = sess->kl2_dev;
    struct kl2_debug_master    *dbgm    = &kl2_dev->dbgm;
    u64                         pinned_ptr, noc_addr;
    u32                         desc[5];
    int                         ret = 0;

    if (sz != sizeof(args)) {
        // ioc 版本不一致
        return -EINVAL;
    }
    if (copy_from_user(&args, argp, sizeof(args))) {
        return -EFAULT;
    }
    pinned_ptr = (u64)args.pinned_ptr;
    if (!args.size || !pinned_ptr || !args.is_devmem) {
        // kl2 不支持通过 pcie slave 往 host memory 写 trace 的模式
        return -EINVAL;
    }
    if (args.size % sizeof(u32)) {
        // dma_size 需要和DW对齐
        return -EINVAL;
    }

    /* sgdma desc generate */
    ret = kl_mm_malloc(&kl2_dev->mm, KL2_DBGM_SGDESC_DWORD * sizeof(u32), XPU_MEM_PARAM, sess,
                       &noc_addr, NULL, NULL);
    if (ret) {
        return ret;
    }
    /* host desc */
    desc[0] = (args.size / sizeof(u32)) << 1;
    desc[1] = low32(pinned_ptr);
    desc[2] = high32(pinned_ptr);
    desc[3] = low32(noc_addr);
    desc[4] = high32(noc_addr);
    ret     = kl2_dma_ddma_from_host_kernel(&kl2_dev->dma_engine, noc_addr, (u64)desc,
                                            KL2_DBGM_SGDESC_DWORD * sizeof(u32));
    if (ret) {
        goto err_disable_dbgm;
    }

    mutex_lock(&dbgm->lock);
    if (dbgm->sess) {
        // dbmg 已经被 enable 过，执行 enable 的 session 为 dbgm->sess
        mutex_unlock(&dbgm->lock);
        ret = -EINVAL;
        goto err_disable_dbgm;
    } else {
        // 拥有 dbgm 的 sess 需要负责关闭 dbgm
        sess->dbgm             = dbgm;
        dbgm->sess             = sess;
        dbgm->time_stamp       = args.time_stamp;
        dbgm->stamp_interval   = args.stamp_interval;
        dbgm->lost_interval    = args.lost_interval;
        dbgm->port             = args.port;
        dbgm->sgdesc.noc_addr  = noc_addr;
        dbgm->sgdesc.size      = KL2_DBGM_SGDESC_DWORD * sizeof(u32);
        dbgm->sgdesc.is_device = true;
        dbgm->mbox.enable      = args.enable_mbox;
        dbgm->mbox.message     = 0;
    }
    mutex_unlock(&dbgm->lock);

    return 0;

err_disable_dbgm:
    kl_mm_free(&kl2_dev->mm, noc_addr, sess, NULL);

    return ret;
}

int kl2_dbgm_disable(struct kl2_session *sess)
{
    struct kl2_device       *kl2_dev     = sess->kl2_dev;
    struct kl2_debug_master *dbgm        = &kl2_dev->dbgm;
    u64                      sgdesc_addr = dbgm->sgdesc.noc_addr;
    int                      ret         = 0;

    mutex_lock(&dbgm->lock);
    if (!dbgm->sess || !sess->dbgm) {
        mutex_unlock(&dbgm->lock);
        return -EINVAL;
    } else {
        // 必须已经被初始化 且 只有 owner 才能调用 disable
        sess->dbgm = NULL;
        kl2_debug_master_port_disable(kl2_dev);
        if (dbgm->in_use) {
            ret = kl2_debug_master_disable(kl2_dev);
        }
        dbgm->sess         = NULL;
        dbgm->in_use       = false;
        dbgm->mbox.enable  = false;
        dbgm->mbox.message = 0; /* 清空 mail box */
    }
    mutex_unlock(&dbgm->lock);

    kl_mm_free(&kl2_dev->mm, sgdesc_addr, sess, NULL);

    wake_up_interruptible_all(&dbgm->wait_queue);

    return ret;
}

int kl2_dbgm_start(struct kl2_userprocess *uproc)
{
    struct kl2_device       *kl2_dev = uproc->kl2_dev;
    struct kl2_debug_master *dbgm    = &kl2_dev->dbgm;
    int                      ret     = 0;

    mutex_lock(&dbgm->lock);
    if (!dbgm->sess) {
        /* 未初始化直接返回报错 */
        ret = -EINVAL;
    } else if (dbgm->in_use) {
        /* 已经处于 start 状态，返回成功 */
        ret = 0;
    } else {
        /* 保证一张卡只能被 start 一次 */
        dbgm->in_use = true;
        /* debug master dma reg config */
        kl2_debug_master_enable(kl2_dev, dbgm->time_stamp, dbgm->stamp_interval,
                                dbgm->lost_interval, dbgm->sgdesc.noc_addr, dbgm->port);

        trace_xpu_clock_sync(kl2_dev->kdev->idx, 0ULL);
    }
    mutex_unlock(&dbgm->lock);

    return ret;
}

int kl2_dbgm_stop(struct kl2_userprocess *uproc)
{
    struct kl2_device       *kl2_dev = uproc->kl2_dev;
    struct kl2_debug_master *dbgm    = &kl2_dev->dbgm;
    int                      ret     = 0;

    mutex_lock(&dbgm->lock);
    if (!dbgm->sess) {
        /* 未初始化直接返回报错 */
        ret = -EINVAL;
    } else if (!dbgm->in_use) {
        /* 已经处于停止状态，直接返回成功 */
        ret = 0;
    } else {
        /* 只 disable debug master，而不对子部件的开关做操作 */
        ret          = kl2_debug_master_disable(kl2_dev);
        dbgm->in_use = false;
    }
    mutex_unlock(&dbgm->lock);

    return ret;
}

int kl2_dbgm_mbox_write(struct kl2_session *sess, void __user *argp, u64 sz)
{
    struct kl2_device        *kl2_dev = sess->kl2_dev;
    struct kl2_debug_master  *dbgm    = &kl2_dev->dbgm;
    struct XPUDebugMasterMbox args;
    int                       ret = 0;

    if (sz != sizeof(args)) {
        return -EINVAL;
    }
    if (copy_from_user(&args, argp, sizeof(args))) {
        return -EFAULT;
    }
    if (!args.write) {
        return -EINVAL;
    }

    if (!dbgm->mbox.enable) {
        return -EINVAL;
    }

    mutex_lock(&dbgm->lock);
    if (!dbgm->sess) {
        /* dbgm 没有初始化 */
        ret = -EINVAL;
    } else {
        /* ret 标识 ioctl 本身的错误码，没有遇到错误就返回 0 */
        ret = 0;
        if (dbgm->mbox.message) {
            /* mail box 内存在未被取走的信息, 无法写入新信息 */
            args.success = 0;
        } else {
            dbgm->mbox.message = args.message;
            args.success       = 1;
        }
    }
    mutex_unlock(&dbgm->lock);

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    wake_up_interruptible_all(&dbgm->wait_queue);

    return ret;
}

int kl2_dbgm_mbox_read(struct kl2_session *sess, void __user *argp, u64 sz)
{
    struct kl2_device        *kl2_dev = sess->kl2_dev;
    struct kl2_debug_master  *dbgm    = &kl2_dev->dbgm;
    struct XPUDebugMasterMbox args;
    int                       ret = 0;

    if (sz != sizeof(args)) {
        return -EINVAL;
    }
    if (copy_from_user(&args, argp, sizeof(args))) {
        return -EFAULT;
    }
    if (args.write) {
        return -EINVAL;
    }

    if (!dbgm->mbox.enable) {
        return -EINVAL;
    }

    mutex_lock(&dbgm->lock);
    if (!dbgm->sess) {
        /* dbgm 没有初始化 */
        ret = -EINVAL;
    } else {
        ret = 0;
        if (!dbgm->mbox.message) {
            /* mail box 内没有未被取走的信息 */
            args.message = 0;
            args.success = 0;
        } else {
            args.message       = dbgm->mbox.message;
            args.success       = 1;
            dbgm->mbox.message = 0;
        }
    }
    mutex_unlock(&dbgm->lock);

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    return ret;
}

static bool kl2_dbgm_wait_condition(struct kl2_debug_master *dbgm)
{
    bool ret = false;

    mutex_lock(&dbgm->lock);
    if (dbgm->mbox.message || !dbgm->mbox.enable) {
        ret = true;
    }
    mutex_unlock(&dbgm->lock);

    return ret;
}

int kl2_dbgm_sleep_until_new_cmd(struct kl2_session *sess)
{
    struct kl2_device       *kl2_dev = sess->kl2_dev;
    struct kl2_debug_master *dbgm    = &kl2_dev->dbgm;

    wait_event_interruptible_timeout(dbgm->wait_queue, kl2_dbgm_wait_condition(dbgm),
                                     usecs_to_jiffies(IOCTL_WAIT_TIMEOUT));

    return 0;
}

int kl2_dbgm_in_use(struct kl2_session *sess, void __user *argp, u64 sz)
{
    struct kl2_device       *kl2_dev = sess->kl2_dev;
    struct kl2_debug_master *dbgm    = &kl2_dev->dbgm;
    int                      in_use, ret = 0;

    if (sz != sizeof(in_use)) {
        return -EINVAL;
    }

    mutex_lock(&dbgm->lock);
    if (!dbgm->sess) {
        /* 未初始化直接返回报错 */
        ret = -EINVAL;
    }
    if (!dbgm->mbox.enable) {
        /* 在非 deamonize 模式下不可用 */
        ret = -EINVAL;
    }
    in_use = dbgm->in_use;
    mutex_unlock(&dbgm->lock);

    if (copy_to_user(argp, &in_use, sizeof(in_use))) {
        return -EFAULT;
    }

    return ret;
}
