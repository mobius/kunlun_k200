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

// Copyright 2018 Baidu Inc. All Rights Reserved.
// authors: Han Jinchen hanjinche@baidu.com
//
// xpu_ioctl.c - XPU ioctl handler
//
#define __FILENAME__ "xpu_ioctl.c"

#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/timex.h>
#include <linux/uaccess.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "xpu/version.h"
#include "xpurt_priv/ioctl.h"
#include "xpu_drv.h"
#include "xpu_hw.h"
//#include "device.h"

static inline struct xpu_tq *__find_tq_by_id(struct xpu_pd *xpd, u32 id)
{
    if (id >= KL1_SSE_TQ_COUNT)
        return NULL;

    return &xpd->xtqs[id];
}

static inline struct xpu_session *__find_session_by_id(struct xpu_pd *xpd, u32 id)
{
    struct xpu_session *p = NULL;
    list_for_each_entry(p, &xpd->sessions, xpd_sessions_ent) {
        if (p->id == id)
            return p;
    }
    LOGW("cannot find session with id= %u\n", id);
    return NULL;
}

/***************
 * IOCTL Handler
 ***************/

//int ioctl_ioc_version(void __user *argp)
//{
//    int ioc_version = IOC_VERSION;
//
//    if (copy_to_user(argp, &ioc_version, sizeof(int)))
//        return -EFAULT;
//
//    return 0;
//}

//int ioctl_version(void __user *argp)
//{
//    struct XPUDriverVersionIoctlArgs args;
//    args.major = XPURT_VERSION_MAJOR;
//    args.minor = XPURT_VERSION_MINOR;
//    strncpy(args.commit, XPURT_COMMIT, XPU_MAX_STRLEN);
//
//    if (copy_to_user(argp, &args, sizeof(args))) {
//        return -EFAULT;
//    }
//
//    return 0;
//}

//int ioctl_changeset(void __user *argp)
//{
//    u32 changeset = 0;
//
//#ifdef XPURT_CHANGESET
//    changeset = XPURT_CHANGESET;
//#endif
//
//    if (copy_to_user(argp, &changeset, sizeof(changeset)))
//        return -EFAULT;
//
//    return 0;
//}

/* DMA read : host -> device */
int ioctl_reg_read(struct xpu_pd *xpd, void __user *argp)
{
    struct XPURegisterIoctlArgs args;
    struct xpu_device          *xdev = xpd->xdev;
    struct kl_device           *kdev = xdev->kdev;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    if ((args.bar < 0) || (args.bar >= PCIE_BAR_NUM) || (kdev->bar[args.bar] == NULL)) {
        LOGW("invalid bar %d\n", args.bar);
        return -XPUERR_INVALID_PARAM;
    }

    spin_lock(&xdev->brw_lock);
    if (xpu_device_disabled_or_in_reset(xdev)) {
        spin_unlock(&xdev->brw_lock);
        return -XPUERR_PEERRESET;
    }
    args.value = reg_readl(kdev->bar[args.bar] + args.addr);
    spin_unlock(&xdev->brw_lock);

    LOGL1("BR b= %d o= 0x%llx v= 0x%x\n", args.bar, args.addr, args.value);

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    return 0;
}

int ioctl_reg_write(struct xpu_pd *xpd, void __user *argp)
{
    struct XPURegisterIoctlArgs args;
    struct xpu_device          *xdev = xpd->xdev;
    struct kl_device           *kdev = xdev->kdev;

    if (copy_from_user(&args, argp, sizeof(args))) {
        return -EFAULT;
    }

    if ((args.bar < 0) || (args.bar >= PCIE_BAR_NUM) || (kdev->bar[args.bar] == NULL)) {
        LOGW("invalid bar %d\n", args.bar);
        return -XPUERR_INVALID_PARAM;
    }

    spin_lock(&xdev->brw_lock);
    if (xpu_device_disabled_or_in_reset(xdev)) {
        spin_unlock(&xdev->brw_lock);
        return -XPUERR_PEERRESET;
    }
    reg_writel(kdev->bar[args.bar] + args.addr, (uint32_t)args.value);
    spin_unlock(&xdev->brw_lock);

    LOGL1("BW b= %d o= 0x%llx v= 0x%x\n", args.bar, args.addr, args.value);
    return 0;
}

/* ioctl memory allocate interface */
int ioctl_memory_alloc(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    struct XPUMemoryAllocIoctlArgs args;
    struct xpu_session            *xsess = NULL;
    if (copy_from_user(&args, argp, sizeof(args))) {
        return -EFAULT;
    }

    xsess = (struct xpu_session *)file->private_data;

    if (args.size == 0)
        return -XPUERR_INVALID_PARAM;

    args.addr = xpu_mem_alloc(xsess, args.size, args.kind);

    // even the allocation is failed, args.addr need to be written
    if (copy_to_user(argp, &args, sizeof(args))) {
        xpu_mem_free(xsess, args.addr);
        return -EFAULT;
    }
    return (args.addr == 0UL) ? -XPUERR_NOMEM : 0;
}

/* ioctl memory free interface */
int ioctl_memory_free(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    u64 args;
    if (copy_from_user((void *)&args, argp, sizeof(args))) {
        return -EFAULT;
    }

    xpu_mem_free((struct xpu_session *)file->private_data, args);

    return 0;
}

int ioctl_memcpy_h2d(struct xpu_pd *xpd, void __user *argp)
{
    struct XPUMemcpyIoctlArgs args;
    int                       ret = 0;

    if (copy_from_user(&args, argp, sizeof(args))) {
        LOGW("Copy from user error ERR=%d\n", -EFAULT);
        return -EFAULT;
    }

    ret = dma_host_to_device(xpd, args.dest, args.src, args.size, &args.cycles);
    if (ret == -XPUERR_DMATIMEOUT)
        xpd_state_to_error(xpd, XPUERR_DMATIMEOUT);

    if (ret < 0)
        return ret;

    if (copy_to_user(argp, &args, sizeof(args))) {
        LOGW("copy_to_user error, src= %px dst= %px\n", &args, argp);
        return -EFAULT;
    }

    return 0;
}

int ioctl_memcpy_d2h(struct xpu_pd *xpd, void __user *argp)
{
    struct XPUMemcpyIoctlArgs args;
    int                       ret = 0;

    if (copy_from_user(&args, argp, sizeof(args))) {
        LOGW("Copy from user error ERR=%d\n", -EFAULT);
        return -EFAULT;
    }

    ret = dma_device_to_host(xpd, args.dest, args.src, args.size, &args.cycles);
    if (ret == -XPUERR_DMATIMEOUT)
        xpd_state_to_error(xpd, XPUERR_DMATIMEOUT);

    if (ret < 0)
        return ret;

    if (copy_to_user(argp, &args, sizeof(args))) {
        LOGW("copy_to_user error, src= %px dst= %px\n", &args, argp);
        return -EFAULT;
    }

    return 0;
}

int ioctl_memcpy_d2d(struct xpu_pd *xpd, void __user *argp)
{
    struct XPUMemcpyIoctlArgs args;
    int                       ret = 0;

    if (copy_from_user(&args, argp, sizeof(args))) {
        LOGW("copy_from_user error, src= %px dst= %px\n", argp, &args);
        return -EFAULT;
    }

    ret = dma_device_to_device(xpd, args.dest, args.src, args.size, &args.cycles);
    if (ret < 0)
        return ret;

    if (copy_to_user(argp, &args, sizeof(args))) {
        LOGW("copy_to_user error, src= %px dst= %px\n", &args, argp);
        return -EFAULT;
    }

    return 0;
}

int ioctl_memcpy(struct xpu_pd *xpd, void __user *argp)
{
    struct XPUMemcpyIoctlArgs args;
    int                       err;

    if (copy_from_user(&args, argp, sizeof(args))) {
        LOGW("copy_from_user error, src= %px dst= %px\n", argp, &args);
        return -EFAULT;
    }

    if (args.size == 0)
        return -XPUERR_INVALID_PARAM;

    if (xpd->state == XPDS_PAUSING || xpd->state == XPDS_PAUSED) {
        err = xpu_poll_cond_timeout(xpd->state == XPDS_RUNNING, 10, 200000);
        if (err)
            return err;
    }

    switch (args.kind) {
    case XPU_DEVICE_TO_DEVICE:
        return ioctl_memcpy_d2d(xpd, argp);
    case XPU_HOST_TO_DEVICE:
        return ioctl_memcpy_h2d(xpd, argp);
    case XPU_DEVICE_TO_HOST:
        return ioctl_memcpy_d2h(xpd, argp);
    }

    return 0;
}

int ioctl_dev_hard_reset(struct xpu_pd *xpd)
{
    return 0;
}

int ioctl_dev_soft_reset(struct xpu_pd *xpd)
{
    int err = 0;
    err     = xpu_device_reset(xpd->xdev);
    return err;
}

static enum xpu_task_type __kernel_type_to_task_type(enum kernel_type ktype)
{
    switch (ktype) {
    case KT_CLUSTER:
        return XTT_CLUSTER;
    case KT_SDCDNN:
        return XTT_SDCDNN;
    default:
        LOGW("unknown kernel type %d\n", ktype);
        return XTT_NONE;
    }
}

int ioctl_session_bindtq(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    struct xpu_session *xsess = NULL;
    u64                 data  = (u64)argp;
    int                 ret;

    xsess = (struct xpu_session *)file->private_data;
    mutex_lock(&xsess->lock);

    if (atomic_read(&xsess->unfinished_cnt) != 0) {
        ret = -XPUERR_BUSY;
        goto out;
    }

    xsess->xtq = __find_tq_by_id(xpd, (u32)data);
    if (xsess->xtq == NULL) {
        ret = -XPUERR_INVALID_PARAM;
        goto out;
    }

    xsess->binding_fixed = 1;

    ret = 0;

out:
    mutex_unlock(&xsess->lock);
    return ret;
}

int ioctl_launch(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    DECLARE_PROFILER(PROF_launch);
    DECLARE_PROFILER(PROF_launch_usercpy);
    DECLARE_PROFILER(PROF_launch_preparextask);
    DECLARE_PROFILER(PROF_launch_sesslaunch);

    struct XPULaunchIoctlArgs args;
    struct xpu_session       *xsess = NULL;
    struct xpu_task          *xtask = NULL;
    int                       ret   = 0;

    START_PROFILING(PROF_launch);
    START_PROFILING(PROF_launch_usercpy);
    if (copy_from_user(&args, argp, sizeof(args))) {
        LOGW("Copy args from user error ERR=%d\n", -EFAULT);
        return -EFAULT;
    }

    if (args.kernel.param_dword_size > MAX_PARAM_DWORD_SIZE_KL1) {
        return -XPUERR_INVALID_DEVICE;
    }

    xsess = (struct xpu_session *)file->private_data;
    END_PROFILING(PROF_launch_usercpy, xpd);

    if (xpd->state == XPDS_PAUSING || xpd->state == XPDS_PAUSED) {
        ret = xpu_poll_cond_timeout(xpd->state == XPDS_RUNNING, 10, 200000);
        if (ret)
            return ret;
    }

    START_PROFILING(PROF_launch_preparextask);
    // freed in x_sess.session_finish_task
    xtask = (struct xpu_task *)kzalloc(sizeof(*xtask), GFP_KERNEL);
    if (xtask == NULL) {
        LOGW("kmalloc xpu_task fail\n");
        return -XPUERR_NOCPUMEM;
    }

    xtask->token      = atomic_add_return(1, &xpd->task_token_counter);
    xtask->type       = __kernel_type_to_task_type(args.kernel.type);
    xtask->free_by_tq = 1;
    xtask->kernel     = args.kernel;
    xtask->xsess      = xsess;
    xtask->xsess_id   = xsess->id;
    xtask->xpd        = xpd;
    xtask->nclusters  = args.nclusters;
    xtask->ncores     = args.ncores;

    strncpy(xtask->kernel_name, args.name, XPU_MAX_STRLEN);
    xtask->kernel_name[XPU_MAX_STRLEN - 1] = '\0';
    memmove(xtask->params, args.params, args.kernel.param_dword_size * sizeof(u32));

    END_PROFILING(PROF_launch_preparextask, xpd);

    LOGL2("[xpu_%d sess_%d] launch kernel tk= %u name=%s ty=%d addr=0x%llx p_dword_size=%u\n",
          xsess->xpd->devfile_id, xsess->id, xtask->token, xtask->kernel_name, args.kernel.type,
          args.kernel.code_addr, args.kernel.param_dword_size);

    START_PROFILING(PROF_launch_sesslaunch);
    ret = session_launch_tasks(xsess, &xtask, 1);
    END_PROFILING(PROF_launch_sesslaunch, xpd);

    END_PROFILING(PROF_launch, xpd);

    args.kernel_enter_cycle = __xpurt_prflr_PROF_launch_t0__;
    args.kernel_exit_cycle  = __xpurt_prflr_PROF_launch_t1__;

    if (copy_to_user(argp, &args, sizeof(args)))
        return -EFAULT;

    return ret;
}

int ioctl_batchlaunch(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    DECLARE_PROFILER(PROF_batchlaunch);
    DECLARE_PROFILER(PROF_batchlaunch_prepare);

    struct XPUBatchLaunchIoctlArgs args;
    struct xpu_session            *xsess    = (struct xpu_session *)file->private_data;
    struct XPULaunchIoctlArgs     *cmds     = NULL;
    struct xpu_task              **xtasks   = NULL;
    u32                            newtoken = 0;
    int                            i        = 0;
    int                            ret      = 0;

    START_PROFILING(PROF_batchlaunch);
    START_PROFILING(PROF_batchlaunch_prepare);
    if (copy_from_user(&args, argp, sizeof(args))) {
        LOGW("Copy args from user error ERR=%d\n", -EFAULT);
        return -EFAULT;
    }

    if (args.cnt == 0)
        return -XPUERR_INVALID_PARAM;

    if (args.cnt > MAX_LAUNCH_BATCH_COUNT) {
        LOGW("[xpu_%d sess_%d] exceed max batch launch limit. cnt= %d\n", xpd->devfile_id,
             xsess->id, args.cnt);
        return -XPUERR_INVALID_PARAM;
    }

    if (xpd->state == XPDS_PAUSING || xpd->state == XPDS_PAUSED) {
        ret = xpu_poll_cond_timeout(xpd->state == XPDS_RUNNING, 10, 200000);
        if (ret)
            return ret;
    }

    cmds = vmalloc(args.cnt * sizeof(*cmds));
    if (cmds == NULL) {
        LOGW("vmalloc for cmds fail\n");
        return -XPUERR_NOCPUMEM;
    }

    if (copy_from_user(cmds, args.cmds, args.cnt * sizeof(*cmds))) {
        LOGW("Copy args.cmds from user error ERR=%d\n", -EFAULT);
        ret = -EFAULT;
        goto batchlaunch_free_cmds;
    }

    for (i = 0; i < args.cnt; ++i) {
        if (cmds[i].kernel.param_dword_size > MAX_PARAM_DWORD_SIZE_KL1) {
            ret = -XPUERR_INVALID_DEVICE;
            goto batchlaunch_free_cmds;
        }
    }

    //LOGI("args.cnt= %u sizeof xtask= %u\n", args.cnt, sizeof(struct xpu_task));
    xtasks = kmalloc(args.cnt * sizeof(*xtasks), GFP_KERNEL);
    if (xtasks == NULL) {
        LOGW("kmalloc for xtasks fail\n");
        ret = -XPUERR_NOCPUMEM;
        goto batchlaunch_free_cmds;
    }

    newtoken = atomic_add_return(args.cnt, &xpd->task_token_counter) - args.cnt + 1;
    //LOGI("token= %u ~ %u\n", newtoken, newtoken + args.cnt - 1);
    for (i = 0; i < args.cnt; ++i) {
        xtasks[i] = (struct xpu_task *)kmalloc(sizeof(struct xpu_task), GFP_KERNEL);
        if (xtasks[i] == NULL) {
            LOGW("kmalloc for xtasks[%d] fail\n", i);
            ret = -XPUERR_NOCPUMEM;
            goto batchlaunch_err_free_xtasks;
        }

        xtasks[i]->token       = newtoken + i;
        xtasks[i]->type        = __kernel_type_to_task_type(cmds[i].kernel.type);
        xtasks[i]->free_by_tq  = 1;
        xtasks[i]->malloc_type = 0;
        xtasks[i]->kernel      = cmds[i].kernel;
        xtasks[i]->xsess       = xsess;
        xtasks[i]->xsess_id    = xsess->id;
        xtasks[i]->xpd         = xpd;
        xtasks[i]->nclusters   = cmds[i].nclusters;
        xtasks[i]->ncores      = cmds[i].ncores;

        strncpy(xtasks[i]->kernel_name, cmds[i].name, XPU_MAX_STRLEN);
        xtasks[i]->kernel_name[XPU_MAX_STRLEN - 1] = '\0';

        memmove(xtasks[i]->params, cmds[i].params, cmds[i].kernel.param_dword_size * sizeof(u32));
    }

    xsess->last_token = xtasks[args.cnt - 1]->token;

    END_PROFILING(PROF_batchlaunch_prepare, xpd);
    ret = session_launch_tasks(xsess, xtasks, args.cnt);
    END_PROFILING(PROF_batchlaunch, xpd);

    args.kernel_enter_cycle = __xpurt_prflr_PROF_batchlaunch_t0__;
    args.kernel_exit_cycle  = __xpurt_prflr_PROF_batchlaunch_t1__;

    if (copy_to_user(argp, &args, sizeof(args)))
        ret = -EFAULT;

    goto batchlaunch_free_cmds;

batchlaunch_err_free_xtasks:
    for (i = i - 1; i >= 0; --i)
        kfree(xtasks[i]);

batchlaunch_free_cmds:
    kfree(xtasks);
    vfree(cmds);

    return ret;
}

static int wait_session_timeout_interruptable(struct xpu_session *xsess, u64 timeout_us)
{
    unsigned long sleep_us = 400;
    ktime_t       __timeout;
    int           last_unfinished = 0;
    ktime_t       __busy_wait_timeout;
    unsigned long busy_wait_us = 500 * 1000; // 500ms

    unsigned long hybrid_busy_time_us = 500;
    ktime_t       __hybrid_wait_timeout;
    int           hybrid_ite = 0;

    might_sleep();

    __timeout             = ktime_add_us(ktime_get(), timeout_us);
    __busy_wait_timeout   = ktime_add_us(ktime_get(), busy_wait_us);
    __hybrid_wait_timeout = ktime_add_us(ktime_get(), hybrid_busy_time_us);

    for (;;) {
        ktime_t __current  = ktime_get();
        int     unfinished = atomic_read(&xsess->unfinished_cnt);
        if (unfinished == 0)
            return -xsess->errno;

        if (unfinished != last_unfinished) {
            last_unfinished = unfinished;
            __timeout       = ktime_add_us(__current, timeout_us);
        }

        if (timeout_us && ktime_compare(__current, __timeout) > 0) {
            LOGI("xpu%d sess%u %s: wait timeout\n", xsess->xpd->devfile_id, xsess->id,
                 xsess->xctx->comm);
            return -XPUERR_TIMEOUT;
        }

        if (signal_pending_state(TASK_INTERRUPTIBLE, current)) {
            // 直接返回-ERESTARTSYS，令glibc重新发起ioctl syscall或及时处理SIGINT/SIGTERM等终止信号
            // 20220517 xpu_wait可能被SIGCHLD打断，原返回-XPUERR_INTERRUPTED不正确
            return -ERESTARTSYS;
        }

        if (xsess->errno) {
            LOGI("xpu%d sess%u %s: session error %s\n", xsess->xpd->devfile_id, xsess->id,
                 xsess->xctx->comm, xpu_strerror(xsess->errno));
            return -xsess->errno;
        }

        switch (g_kl1_config_wait_mode) {
        case 0: {
            // normal wait
            usleep_range((sleep_us >> 2) + 1, sleep_us);
            break;
        }
        case 1: {
            // busy wait
            if (ktime_compare(__current, __busy_wait_timeout) > 0) {
                //LOGI("try to sleep for a while in busy wait\n");
                usleep_range(4, 16);
                // wakeup, update busy wait timeout
                __busy_wait_timeout = ktime_add_us(ktime_get(), busy_wait_us);
            }
            cpu_relax();
            break;
        }
        case 2: {
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
            break;
        }
        }
    }
}

int ioctl_proflaunch(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    struct XPUProfLaunch       args;
    struct XPULaunchIoctlArgs *launch;
    struct xpu_kernel         *kern;
    struct xpu_session        *xsess;
    struct xpu_task           *xtask;
    u64                        cost = 0;
    u32                        token;
    int                        ret;

    if (copy_from_user(&args, argp, sizeof(args))) {
        LOGW("Copy from user error ERR=%d\n", -EFAULT);
        return -EFAULT;
    }

    if (args.launch.kernel.param_dword_size > MAX_PARAM_DWORD_SIZE_KL1) {
        return -XPUERR_INVALID_DEVICE;
    }

    launch = &args.launch;
    kern   = &launch->kernel;

    xsess = (struct xpu_session *)file->private_data;

    xtask = kzalloc(sizeof(*xtask), GFP_KERNEL);
    if (xtask == NULL) {
        ret = -XPUERR_NOCPUMEM;
        goto err_freextask;
    }

    token = atomic_add_return(1, &xpd->task_token_counter);

    xtask->token       = token;
    xtask->type        = __kernel_type_to_task_type(kern->type);
    xtask->free_by_tq  = 1;
    xtask->malloc_type = 0;
    xtask->kernel      = *kern;
    xtask->xpd         = xpd;
    xtask->xsess       = xsess;
    xtask->xsess_id    = xsess->id;
    xtask->nclusters   = launch->nclusters;
    xtask->ncores      = launch->ncores;

    strncpy(xtask->kernel_name, launch->name, XPU_MAX_STRLEN);
    xtask->kernel_name[XPU_MAX_STRLEN - 1] = '\0';
    memmove(xtask->params, launch->params, kern->param_dword_size * sizeof(u32));

    //LOGI("prof launch tk %u, %u\n", xtask[0]->token, xtask[1]->token);

    xpu_sched_bind_sess(xsess);

    xpuhw_sse_last_cycles(xpd, xsess->xtq->id);

    ret = session_launch_tasks(xsess, &xtask, 1);
    if (ret < 0)
        goto err_freextask;

    ret = wait_session_timeout_interruptable(xsess, 20 * 1000 * 1000);
    if (ret >= 0) {
        cost = xpuhw_sse_last_cycles(xpd, xsess->xtq->id);
        //LOGI("sse cost: %lu\n", cost);
        ret = 0;
    }

    args.cycles = cost;
    if (copy_to_user(argp, &args, sizeof(args))) {
        ret = -EFAULT;
    }

    return ret;

err_freextask:
    kfree(xtask);

    return ret;
}

int ioctl_wait(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    DECLARE_PROFILER(PROF_wait);
    struct xpu_session *xsess = NULL;
    int                 err   = 0;

    START_PROFILING(PROF_wait);

    xsess = (struct xpu_session *)file->private_data;

    err = wait_session_timeout_interruptable(xsess, 20 * 1000 * 1000);

    if (err == -XPUERR_TIMEOUT) {
        unsigned long  flags;
        struct xpu_tq *xtq;

        xtq = xsess->xtq;

        xpd_state_to_error(xpd, XPUERR_TIMEOUT);

        LOGI("[xpu%d sess%u] Kernel execution timeout. bind on xtq%u\n", xpd->devfile_id, xsess->id,
             xsess->xtq->id);

        xpd_clean_etasks_ifneed(xpd);

        spin_lock_irqsave(&xtq->lock, flags);
        xtq->state = XTQS_HANGUP;

        if (!list_empty(&xtq->tasks)) {
            int              k;
            struct xpu_task *xtask = list_first_entry(&xtq->tasks, struct xpu_task, tq_tasks_ent);
            LOGI("xpu%d tq%u: head task, tk=%u .name=%s\n", xpd->devfile_id, xtq->id, xtask->token,
                 xtask->kernel_name);
            for (k = 0; k < xtask->kernel.param_dword_size; ++k)
                LOGI(" ..param[%d]= 0x%x\n", k, xtask->params[k]);

            xtq_save_etask_locked(xtq, xtask);

            if (xtask->xsess->errno == 0) {
                xtask->xsess->state = XSS_ERROR;
                xtask->xsess->errno = XPUERR_TIMEOUT;
            }
        }
        spin_unlock_irqrestore(&xtq->lock, flags);
    }

    END_PROFILING(PROF_wait, xsess->xpd);

    return err;
}

int ioctl_prof_clear(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    int i;
    for (i = 0; i < PROFILER_COUNT; ++i) {
        xpd->profiler[i].cost  = 0;
        xpd->profiler[i].count = 0;
    }
    return 0;
}

// Put whatever you wanna test of the driver into this ioctl
int ioctl_test(struct file *file, struct xpu_device *dev, void __user *argp)
{
    u64 data = (u64)(uintptr_t)argp;
    int type = (int)(data & 0xff);

    switch (type) {
    case 5:
        kl1_dma_direct = (int)((data >> 8) & 1);
        LOGI("S4: kl1_dma_direct=%d (IOCTL_TEST)\n", kl1_dma_direct);
        break;
    default:
        break;
    }

#if 0
    u64 data = (u64) argp;
    int type = data & 0xff;
    void __iomem *base = dev->bar_spaces[0] + 0x600000;

    switch (type) {
    case 0: {
        // Register read write performance test
        u64 nclusters = (u64)(data >> 8);
        unsigned long t0, t1;
        u32 v32 = 0;
        u64 v64 = 0;
        int i = 0;
        int test_count_v64 = nclusters;
        int test_count_v32 = nclusters * 2;

        reg_writel(base + 0x4, 0x30004000U);
        reg_writel(base + 0x8, 0x10002000U);
        v32 = reg_readl(base + 0x4);
        LOGI("0x4 4B value=%x\n", v32);

        v32 = reg_readl(base + 0x8);
        LOGI("0x8 4B value=%x\n", v32);

        v64 = reg_readq(base + 0x4);
        LOGI("0x4 8B value=%llx\n", v64);

        reg_writeq(base + 0x4, 0xf000e000d000c000ULL);
        LOGI("write 0x4 8B value=0xf000e000d000c000\n");

        v64 = reg_readq(base + 0x4);
        LOGI("0x4 8B value=%llx\n", v64);

        v32 = reg_readl(base + 0x4);
        LOGI("value=%x\n", v32);

        v32 = reg_readl(base + 0x8);
        LOGI("value=%x\n", v32);

        t0 = get_cycles();
        for (i = 0; i < test_count_v32; ++i)
            reg_writel(base + 0x4, i);
        t1 = get_cycles();
        LOGI("write 4B w/  mb, cnt= %d cycles= %lu\n", test_count_v32, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v32; ++i)
            xwritel(i, base, 0x4);
        t1 = get_cycles();
        LOGI("write 4B w/o mb, cnt= %d cycles= %lu\n", test_count_v32, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v64; ++i)
            reg_writeq(base + 0x4, i);
        t1 = get_cycles();
        LOGI("write 8B w/  mb, cnt= %d cycles= %lu\n", test_count_v64, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v64; ++i)
            xwriteq(i, base, 0x4);
        t1 = get_cycles();
        LOGI("write 8B w/o mb, cnt= %d cycles= %lu\n", test_count_v64, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v32; ++i)
            v32 = reg_readl(base + 0x4);
        t1 = get_cycles();
        LOGI("read  4B w/  mb, cnt= %d cycles= %lu\n", test_count_v32, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v32; ++i)
            v32 = readl(base + 0x4);
        t1 = get_cycles();
        LOGI("read  4B w/o mb, cnt= %d cycles= %lu\n", test_count_v32, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v64; ++i)
            v64 = reg_readq(base + 0x4);
        t1 = get_cycles();
        LOGI("read  8B w/  mb, cnt= %d cycles= %lu\n", test_count_v64, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v64; ++i)
            v64 = readq(base + 0x4);
        t1 = get_cycles();
        LOGI("read  8B w/o mb, cnt= %d cycles= %lu\n", test_count_v64, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v64; ++i) {
            reg_writeq(base + 0x4, i);
            v64 = reg_readq(base + 0x4);
        }
        t1 = get_cycles();
        LOGI("wr/rd 8B w/  mb, cnt= %d cycles= %lu\n", test_count_v64, (t1 - t0));

        msleep(10);

        t0 = get_cycles();
        for (i = 0; i < test_count_v64; ++i) {
            xwriteq(i, base, 0x4);
            v64 = readq(base + 0x4);
        }
        t1 = get_cycles();
        LOGI("wr/rd 8B w/o mb, cnt= %d cycles= %lu\n", test_count_v64, (t1 - t0));
        break;
    }
    case 1:
        // do nothing test, pure ioctl overhead
        //printk("%lu\n", get_cycles());
        ((void)0);
        break;
    case 2: {
        u64 timeout_us = 20 * 1000 * 1000; // 20s
        unsigned long sleep_us = 20;
        ktime_t __timeout;
        might_sleep();
        __timeout = ktime_add_us(ktime_get(), timeout_us);
        for (;;) {
            if (timeout_us && ktime_compare(ktime_get(), __timeout) > 0) {
                LOGI("timed out\n");
                break;
            }

            if (signal_pending_state(TASK_INTERRUPTIBLE, current)) {
                LOGI("interrupted\n");
                break;
            }

            usleep_range((sleep_us >> 2) + 1, sleep_us);
        }
        break;
    }
    case 3: {
        struct xpu_session *xsess = (struct xpu_session *)file->private_data;
        struct xpu_task *xtask = NULL;

        xtask = (struct xpu_task *)kzalloc(sizeof(*xtask), GFP_KERNEL);

        xtask->token       = atomic_add_return(1, &xsess->xpd->task_token_counter);
        xtask->type        = XTT_CLUSTER;
        xtask->free_by_tq  = 1;
        xtask->xsess       = xsess;
        xtask->xsess_id    = xsess->id;
        xtask->xpd         = xsess->xpd;
        xtask->nclusters   = 1;
        xtask->ncores      = 3;

        xtask->kernel.type = KT_CLUSTER;
        xtask->kernel.place = KP_XPU;
        xtask->kernel.code_addr = 0xFF00000000ull;
        xtask->kernel.code_byte_size = 0x100000u;

        strncpy(xtask->kernel_name, "tk_sse_err", XPU_MAX_STRLEN);

        LOGI("launch a kernel that would cause sse exception.\n");

        session_launch_tasks(xsess, &xtask, 1);

        break;
    }
    case 4: {
        struct xpu_session *xsess = (struct xpu_session *)file->private_data;
        struct xpu_task *xtask = NULL;

        xtask = (struct xpu_task *)kzalloc(sizeof(*xtask), GFP_KERNEL);

        xtask->token       = atomic_add_return(1, &xsess->xpd->task_token_counter);
        xtask->type        = XTT_CLUSTER;
        xtask->free_by_tq  = 1;
        xtask->xsess       = xsess;
        xtask->xsess_id    = xsess->id;
        xtask->xpd         = xsess->xpd;
        xtask->nclusters   = 1;
        xtask->ncores      = 3;

        xtask->kernel.type = KT_CLUSTER;
        xtask->kernel.place = KP_XPU;
        xtask->kernel.code_addr = 0xFF00000000ull;
        xtask->kernel.code_byte_size = 0x100u;

        strncpy(xtask->kernel_name, "tk_sse_err", XPU_MAX_STRLEN);

        LOGI("launch a kernel that would cause sse exception.\n");

        session_launch_tasks(xsess, &xtask, 1);

        break;
    }

    default:
        LOGI("unknown test type=%d\n", type);
        break;
    }
#endif
    return 0;
}

// Launch use SSE interface only, no session and no scheduler.
// for debug and test use only.
// be sure you know what you are doing before using this ioctl
int ioctl_sse_launch(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    return 0;
}

int ioctl_sse_wait(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    return 0;
}

// Temporary test purpose, DO NOT use this func
int ioctl_sd_launch(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
    return 0;
}

int ioctl_start_cunits(struct file *file, struct xpu_pd *xpd, void __user *argp)
{
#if 0
    struct XPUStartCunits args;
    struct xpu_pd *xpd0;
    struct xpu_pd *xpd1;
    int i = 0;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    xpd0 = &xpd->xdev->xpd[0];
    xpd1 = &xpd->xdev->xpd[1];

    for (i = 0; i < args.count; ++i) {
        switch (args.cunits[i]) {
        case 0:
            xwritel(0xffff, xpd0->cluster_base[0], 0x8020);
            break;
        case 1:
            xwritel(0xffff, xpd0->cluster_base[1], 0x8020);
            break;
        case 2:
            xwritel(0xffff, xpd0->cluster_base[2], 0x8020);
            break;
        case 3:
            xwritel(0xffff, xpd0->cluster_base[3], 0x8020);
            break;
        case 4:
            xwritel(0xff, xpd0->cdnn_cluster_base[0], 0x8020);
            break;
        case 5:
            xwritel(0xff, xpd0->cdnn_cluster_base[1], 0x8020);
            break;
        case 6:
            xwritel(0xff, xpd0->cdnn_cluster_base[2], 0x8020);
            break;
        case 7:
            xwritel(0xff, xpd0->cdnn_cluster_base[3], 0x8020);
            break;
        default:
            LOGW("cunit %d not supported\n", args.cunits[i]);
        }
    }
#endif
    return 0;
}

int ioctl_query_bar(struct file *file, struct xpu_device *xdev, void __user *argp)
{
    struct kl_device *kdev = xdev->kdev;

    if (copy_to_user(argp, &kdev->bar_info, sizeof(struct bar_info))) {
        return -EFAULT;
    }

    return 0;
}

int ioctl_query_iatu_region(struct file *file, struct xpu_device *xdev, void __user *argp)
{
    struct iatu_region_info iatu_info;
    if (copy_from_user(&iatu_info, argp, sizeof(iatu_info))) {
        return -EFAULT;
    }

    if (iatu_info.region_type == KL1_REGION_TYPE_OUTBOUND) {
        LOGI("copy outbound info\n");
        if (copy_to_user(argp, &xdev->iatu_outbound_info, sizeof(iatu_info))) {
            return -EFAULT;
        }
    } else if (iatu_info.region_type == KL1_REGION_TYPE_INBOUND) {
        LOGI("copy inbound info\n");
        if (copy_to_user(argp, &xdev->iatu_inbound_info, sizeof(iatu_info))) {
            return -EFAULT;
        }
    } else {
        return -XPUERR_INVALID_PARAM;
    }

    return 0;
}

int ioctl_query_device_attr(struct file *file, struct xpu_device *dev, void __user *argp)
{
    struct xpu_devfile_info *dfi;
    struct xpu_session      *xsess;
    struct xpu_pd           *xpd;
    struct xpu_device       *xdev;

    dfi   = (struct xpu_devfile_info *)vmalloc(sizeof(*dfi));
    xsess = (struct xpu_session *)file->private_data;
    xpd   = xsess->xpd;
    xdev  = xpd->xdev;

    if (dfi == NULL)
        return -XPUERR_NOCPUMEM;

    dfi->id          = xpd->devfile_id;
    dfi->board_id    = xdev->kdev->idx;
    dfi->onboard_idx = xpd->id;

    dfi->domain = xdev->domain;
    dfi->bus    = xdev->bus;
    dfi->slot   = xdev->slot;
    dfi->func   = xdev->func;

    dfi->sn          = xdev->sn;
    dfi->product_num = xdev->product_num;

    if (copy_to_user(argp, dfi, sizeof(*dfi))) {
        return -EFAULT;
    }

    return 0;
}

int ioctl_query_device_info(struct file *file, void __user *argp)
{
    struct xpu_device_info *di = NULL;
    struct xpu_session     *xsess;
    struct xpu_pd          *xpd;
    struct xpu_device      *xdev;
    struct xpu_context     *xctx;
    unsigned long           flag;
    int                     ret = 0;

    xsess = (struct xpu_session *)file->private_data;
    xpd   = xsess->xpd;
    xdev  = xpd->xdev;

    mutex_lock(&xpd->xdi_cache_lock);
    di = &xpd->xdi_cache;

    di->magic_version = DEVINFO_MAGIC_V0;

    if (xdev->product_num & (0x1 << 24))
        di->model = K100;
    else
        di->model = K200;

    di->id        = xpd->devfile_id;
    di->board_idx = xdev->kdev->idx;
    di->chip_idx  = xpd->id;

    di->domain = xdev->domain;
    di->bus    = xdev->bus;
    di->slot   = xdev->slot;
    di->func   = xdev->func;

    di->sn                  = low32(xdev->sn);
    di->hardware_version[1] = high32(xdev->sn);
    di->product_num         = xdev->product_num;

    di->cache_mem_page_used = xpd->mem[MMRGN_L3].page_used;
    di->cache_mem_page_all  = xpd->mem[MMRGN_L3].page_count;
    di->cache_mem_page_size = xpd->mem[MMRGN_L3].page_size;

    di->main_mem_page_used = xpd->mem[MMRGN_HBM_LO].page_used + xpd->mem[MMRGN_HBM_HI].page_used;
    di->main_mem_page_all  = xpd->mem[MMRGN_HBM_LO].page_count + xpd->mem[MMRGN_HBM_HI].page_count;
    di->main_mem_page_size = xpd->mem[MMRGN_HBM_LO].page_size;

    di->use_ratio_numerator   = bitmap_weight(xpd->stat_use_ratio_bitmap, USE_RATIO_PN);
    di->use_ratio_denominator = USE_RATIO_PN;

    di->io_rate_byte_size = 0;
    di->io_rate_time_us   = 0;

    di->dev_configs = 0;

    di->temperature[0] = xdev->monitor.temp[xpd->id * 2];
    di->temperature[0] = xdev->monitor.temp[xpd->id * 2 + 1];
    di->temperature[2] = xdev->monitor.hbm_temp[xpd->id];

    memcpy(&di->frequency[0], &xdev->monitor.freq[0], 6 * sizeof(uint32_t));

    memcpy(&di->firmware_version[0], xdev->flash_version, 3 * sizeof(u32));

    di->hardware_version[0] = xdev->cpld_version;

    di->power = xdev->monitor.power;

    di->process_count = 0;
    spin_lock_irqsave(&xpd->sessions_lock, flag);
    list_for_each_entry(xctx, &xpd->contexts, xpd_contexts_ent) {
        di->process[di->process_count].pid            = xctx->pid;
        di->process[di->process_count].stream_count   = atomic_read(&xctx->sess_cnt);
        di->process[di->process_count].main_page_used = atomic_read(&xctx->main_page_used);
        di->process[di->process_count].cache_mem_page_used =
                atomic_read(&xctx->cache_mem_page_used);
        ++di->process_count;
        if (di->process_count >= DEVICE_MAX_PROCESS_PRINT_COUNT)
            break;
    }
    spin_unlock_irqrestore(&xpd->sessions_lock, flag);

    if (copy_to_user(argp, di, sizeof(*di))) {
        ret = -EFAULT;
    }

    mutex_unlock(&xpd->xdi_cache_lock);

    return ret;
}

/*!
 * ioctl_host_register_kl1 - KL1 stub for host memory pinning
 *
 * KL1 DMA uses bounce-buffer (copy_from_user), not SG-DMA like KL2.
 * Return 0 so the XPURT library believes the memory is registered;
 * actual DMA bandwidth is unchanged (still limited by copy_from_user).
 */
/*!
 * ioctl_memcpy_p2p_kl1 - Peer-to-peer DMA between two PDs on the same K200
 *
 * Extracts device IDs from top 4 bits of addresses (same encoding as KL2).
 * Both PDs must belong to the same xpu_device (same physical card).
 */
int ioctl_memcpy_p2p_kl1(struct xpu_pd *xpd, void __user *argp)
{
    struct XPUMemcpyExIoctlArgs args;
    struct xpu_pd              *dst_xpd;
    int                         src_devid, dst_devid;
    u64                         cycles = 0;
    int                         ret;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    LOGI("KL1_P2P_V5 enter devfile=%d\n", xpd->devfile_id);

    args.time_ns = 0;
    src_devid = (args.src >> 60) & 0xf;
    dst_devid = (args.dest >> 60) & 0xf;
    args.src  = args.src & (~(0xfULL << 60));
    args.dest = args.dest & (~(0xfULL << 60));

    if (src_devid != xpd->devfile_id) {
        LOGW("P2P src_devid %d != session devfile %d\n", src_devid, xpd->devfile_id);
        return -EINVAL;
    }
    if (src_devid == dst_devid) {
        LOGW("P2P src == dst (%d), use D2D instead\n", src_devid);
        return -EINVAL;
    }

    if (dst_devid / XPU_PD_NUM != xpd->devfile_id / XPU_PD_NUM) {
        LOGW("P2P cross-card not supported on KL1 (src=%d dst=%d)\n",
             src_devid, dst_devid);
        return -XPUERR_NOIOC;
    }

    {
        int dst_pd_idx = dst_devid % XPU_PD_NUM;

        dst_xpd = &xpd->xdev->xpd[dst_pd_idx];
    }

    LOGI("KL1_P2P_V5 lookup src=%d dst=%d dst_pd=%d src=0x%llx dst=0x%llx sz=0x%llx\n",
         src_devid, dst_devid, dst_xpd->devfile_id, args.src, args.dest, args.size);

    if (kl1_p2p_stub) {
        LOGI("KL1_P2P_V5 ioctl stub success\n");
        args.time_ns = 0;
        if (copy_to_user(argp, &args, sizeof(args)))
            return -EFAULT;
        return 0;
    }

    /* EDMA uses PD-relative device addresses (same as H2D/D2H ioctl paths). */
    ret = kl1_dma_peer_to_peer(xpd, dst_xpd, args.dest, args.src, args.size, &cycles);
    LOGI("[xpu_%d] P2P ioctl done ret=%d cycles=%llu\n", xpd->devfile_id, ret, cycles);
    args.time_ns = cycles;

    if (copy_to_user(argp, &args, sizeof(args)))
        return -EFAULT;

    return ret;
}

int ioctl_host_register_kl1(struct xpu_pd *xpd, void __user *argp)
{
    struct XPUHostRegisterIoctlArgs args;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    LOGI("[xpu_%d] host_register ptr=0x%llx size=0x%llx (KL1 stub, returning 0)\n",
         xpd->devfile_id, args.ptr, args.size);

    /* KL1 host_alloc uses hugepage mmap + optional S4 direct EDMA.
     * No extra pinning; return success for XPURT registration. */
    return 0;
}

int ioctl_host_unregister_kl1(struct xpu_pd *xpd, void __user *argp)
{
    struct XPUHostRegisterIoctlArgs args;
    struct mm_struct             *mm = current->mm;
    struct vm_area_struct        *vma;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    if (!args.ptr)
        return -EINVAL;

    /* XPURT host_free ioctl must return mapping size for munmap(). */
    mmap_read_lock(mm);
    vma = find_vma(mm, args.ptr);
    if (!vma || args.ptr < vma->vm_start || args.ptr != vma->vm_start) {
        mmap_read_unlock(mm);
        LOGW("[xpu_%d] host_unregister invalid vma ptr=0x%llx\n", xpd->devfile_id, args.ptr);
        return -EINVAL;
    }

    args.size = vma->vm_end - vma->vm_start;
    mmap_read_unlock(mm);

    LOGI("[xpu_%d] host_unregister ptr=0x%llx size=0x%llx\n", xpd->devfile_id, args.ptr,
         args.size);

    if (copy_to_user(argp, &args, sizeof(args)))
        return -EFAULT;

    return 0;
}
