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
// xpu_proc.c - XPU Proc fs
//
#define __FILENAME__ "xpu_proc.c"

#include <linux/version.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "xpu/defs.h"
#include "xpu/version.h"
#include "xpurt_priv/ioctl.h"
#include "xpu_drv.h"
#include "../kl_proc.h"

// how to add new proc entry:
// 1. determine a name for the proc entry, let's say 'new'
// 2. define a show function for this entry, this is the handler function when
//    user cat this proc entry, the show function must meet the name:
//        xpu_proc_##name##_show,
//    which in our case is
//    `static int xpu_proc_new_show(struct seq_file *m, void *v)`
// 3. define a open function with DEFINE_SEQ_OPEN(new), this must be defined after
//    the show function
// 4. if the entry is RO:
//      1. add an entry in `xpu_proc_entries` with XPU_RO_PROC_ENTRY(new)
// 5. if the entry is RW:
//      1. impl a write function
//         `static ssize_t
//          xpu_proc_new_write(struct file *file, const char __user *buffer,
//                             size_t count, loff_t *pos)`
//      2. add an entry in `xpu_proc_entries` with
//         XPU_RW_PROC_ENTRY(new, xpu_proc_new_write)
//
#define MAX_PROC_BUF_SIZE 512 //max len of long in char is 20

#define func_show(procname) xpu_proc_##procname##_show
#define func_open(procname) xpu_proc_##procname##_open
#define func_write(procname) xpu_proc_##procname##_write

#define DEFINE_SEQ_OPEN(procname)                                                                  \
    static int func_open(procname)(struct inode * inode, struct file * file)                       \
    {                                                                                              \
        return single_open(file, func_show(procname), NV_PDE_DATA(inode));                         \
    }

static int func_show(info)(struct seq_file *m, void *v)
{
    struct xpu_pd     *xpd  = (struct xpu_pd *)m->private;
    struct xpu_device *xdev = xpd->xdev;

    seq_printf(m,
               "DeviceName: %s\n"
               "DeviceAddr: %04x:%02x:%02x.%x\n"
               "DeviceSN: %llx\n"
               "FirmwareVersion: %04d.%04d.%04d\n"
               "CPLDVersion: %6x\n"
               "DriverVersion: %u.%u.%u\n",

               (xdev->product_num & (0x1 << 24)) ? "K100" : "K200", xdev->domain & 0xffff,
               xdev->bus & 0xff, xdev->slot & 0x1f, xdev->func & 0x7, xdev->sn,
               xdev->flash_version[0], xdev->flash_version[1], xdev->flash_version[2],
               xdev->cpld_version, XPURT_VERSION_MAJOR, XPURT_VERSION_MINOR, XPURT_VERSION_THIRD);

    return 0;
}
DEFINE_SEQ_OPEN(info);

static int xpu_proc_sessions_show(struct seq_file *m, void *v)
{
    struct xpu_pd      *xpd = (struct xpu_pd *)m->private;
    struct xpu_context *xctx;
    struct xpu_session *xsess;
    unsigned long       flag;

    spin_lock_irqsave(&xpd->sessions_lock, flag);
    list_for_each_entry(xctx, &xpd->contexts, xpd_contexts_ent) {
        seq_printf(m, "pid= %d comm= %s sess_cnt= %d\n", xctx->pid, xctx->comm,
                   atomic_read(&xctx->sess_cnt));
        list_for_each_entry(xsess, &xpd->sessions, xpd_sessions_ent) {
            if (xsess->xctx->pid == xctx->pid)
                seq_printf(m, "  sess%u\n", xsess->id);
        }
    }
    spin_unlock_irqrestore(&xpd->sessions_lock, flag);

    return 0;
}
DEFINE_SEQ_OPEN(sessions);

static int xpu_proc_sn_show(struct seq_file *m, void *v)
{
    struct xpu_pd *xpd = (struct xpu_pd *)m->private;
    int            i   = 0;

    seq_printf(m, "---- PD status ----\n");
    seq_printf(m, "state= %s tqs_pending_finish= %x\n", xpd_state_str(xpd->state),
               (u32)atomic_read(&xpd->xtqs_pending_finish_flag));

    seq_printf(m, "\n"
                  "Profiler_name cnt cost(cycles)\n");
    for (i = 0; i < PROFILER_COUNT; ++i)
        if (xpd->profiler[i].count)
            seq_printf(m, "%32s  %8u  %10llu\n", xpd->profiler[i].name, xpd->profiler[i].count,
                       xpd->profiler[i].cost);

    return 0;
}
DEFINE_SEQ_OPEN(sn);

static int xpu_proc_state_show(struct seq_file *m, void *v)
{
    struct xpu_pd     *xpd  = (struct xpu_pd *)m->private;
    struct xpu_device *xdev = xpd->xdev;

    switch (xdev->state) {
    case XDS_PRERESET:
    case XDS_POSTRESET:
        seq_printf(m, "RECOVERING\n");
        return 0;
    case XDS_ERRPROBE:
        seq_printf(m, "PROBE_ERROR\n");
        return 0;
    case XDS_ERROR:
        seq_printf(m, "ERROR\n");
        return 0;
    default:
        break;
    }

    switch (xpd->state) {
    case XPDS_RUNNING:
        seq_printf(m, "RUNNING\n");
        return 0;
    case XPDS_LOWPOWER:
        seq_printf(m, "LOWPOWER\n");
        return 0;
    case XPDS_PAUSING:
    case XPDS_PAUSED:
        seq_printf(m, "PAUSE\n");
        return 0;
    case XPDS_ERROR:
        seq_printf(m, "ERROR\n");
        return 0;
    default:
        seq_printf(m, "UNKNOWN\n");
        break;
    }

    return 0;
}
DEFINE_SEQ_OPEN(state);

static int xpu_proc_errinfo_show(struct seq_file *m, void *v)
{
    struct xpu_pd *xpd = (struct xpu_pd *)m->private;

    if (xpd->state != XPDS_ERROR) {
        seq_printf(m, "no error\n");
        return 0;
    }

    switch (xpd->errno) {
    case XPUERR_UCECC:
        seq_printf(m, "Uncorrectable ECC\n");
        break;
    case XPUERR_OVERHEAT:
        seq_printf(m, "Overheat\n");
        break;
    case XPUERR_KEXCEPTION:
        seq_printf(m, "Exception in kernel execution\n");
        break;
    case XPUERR_TIMEOUT:
        seq_printf(m, "Kernel execution timed out\n");
        break;
    case XPUERR_HWEXCEPTION:
        seq_printf(m, "Hardware module exception\n");
        break;
    case XPUERR_DMATIMEOUT:
        seq_printf(m, "DMA timed out\n");
        break;
    default:
        break;
    }

    return 0;
}
DEFINE_SEQ_OPEN(errinfo);

DEFINE_DEVPROC_SHOW(reset_count)
{
    struct xpu_pd *xpd = (struct xpu_pd *)m->private;
    seq_printf(m, "%u\n", xpd->xdev->reset_count);
    return 0;
}

DEFINE_DEVPROC_WR(reset_count)
{
    struct seq_file   *m    = (struct seq_file *)file->private_data;
    struct xpu_pd     *xpd  = (struct xpu_pd *)m->private;
    struct xpu_device *xdev = xpd->xdev;

    char _buf = 0;
    if (count < 1)
        return count;

    if (copy_from_user(&_buf, buffer, 1)) {
        LOGW("copy_from_user failed\n");
        return -EFAULT;
    }

    if (_buf == '0')
        xdev->reset_count = 0;

    return count;
}

static void __print_e_rsn(struct seq_file *m, struct exception_info *e, u32 tk, u32 rsn)
{
    int j;

    seq_printf(m, "%s token=%u reason=0x%x\n", e->name, tk, rsn);

    for (j = 0; j < 32; ++j)
        if ((rsn >> j) & 0x1)
            seq_printf(m, "..reason[%d]: %s\n", j, e->reason_table[j]);
}

DEFINE_DEVPROC_SHOW(errtask)
{
    struct xpu_pd   *xpd = (struct xpu_pd *)m->private;
    struct xpu_task *xtask;
    unsigned long    flags;
    int              i;

    for (i = 0; i < XPD_CLUSTER_COUNT; ++i) {
        if (xpd->cu_error[i].cl_reason == 0)
            continue;

        __print_e_rsn(m, &g_clstr_dbgs[i], xpd->cu_error[i].token, xpd->cu_error[i].cl_reason);
    }

    for (i = 0; i < XPD_CDNN_COUNT; ++i) {
        if (xpd->cu_error[4 + i].cl_reason)
            __print_e_rsn(m, &g_clstr_dbgs[4 + i], xpd->cu_error[4 + i].token,
                          xpd->cu_error[4 + i].cl_reason);

        if (xpd->cu_error[4 + i].sd_reason)
            __print_e_rsn(m, &g_clstr_dbgs[8 + i], xpd->cu_error[4 + i].token,
                          xpd->cu_error[4 + i].sd_reason);
    }

    spin_lock_irqsave(&xpd->etasks_lock, flags);

    list_for_each_entry(xtask, &xpd->etasks, tq_tasks_ent) {
        seq_printf(m, "ETASK tk=%u .name=%s .ncl=%u .nco=%u .addr=0x%llx .ksz=0x%x\n", xtask->token,
                   xtask->kernel_name, xtask->nclusters, xtask->ncores, xtask->kernel.code_addr,
                   xtask->kernel.code_byte_size);
        for (i = 0; i < xtask->kernel.param_dword_size; ++i)
            seq_printf(m, "..param[%d]= 0x%x\n", i, xtask->params[i]);
    }

    spin_unlock_irqrestore(&xpd->etasks_lock, flags);

    return 0;
}

static int xpu_proc_tqinfo_show(struct seq_file *m, void *v)
{
    struct xpu_pd *xpd = (struct xpu_pd *)m->private;
    int            i;

    for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
        struct xpu_tq   *xtq = &xpd->xtqs[i];
        struct xpu_task *xtask;
        unsigned long    flags = 0;
        int              j;

        seq_printf(m, "tq%d: state=%s #all=%u #running=%u\n", i,
                   (xpd->xtqs[i].state) ? "HANG" : "NORMAL", xtq->cnt_all, xtq->cnt_running);

        spin_lock_irqsave(&xtq->lock, flags);
        j = 1;
        list_for_each_entry(xtask, &xtq->tasks, tq_tasks_ent) {
            seq_printf(m, "  TASK[%d] sess_id=%u tk=%u .name=%s .addr=0x%llx .ksz=0x%x\n", j,
                       xtask->xsess_id, xtask->token, xtask->kernel_name, xtask->kernel.code_addr,
                       xtask->kernel.code_byte_size);
            if (j == 1) {
                int kk;
                for (kk = 0; kk < xtask->kernel.param_dword_size; ++kk)
                    seq_printf(m, "..param[%d]= 0x%x\n", kk, xtask->params[kk]);
            }
            ++j;
            if (j > (xtq->cnt_running + 5))
                break;
        }
        spin_unlock_irqrestore(&xtq->lock, flags);
    }

    seq_printf(m, "\n");
    seq_printf(m, "sse_errsv= %llx\n", xpd->sse_errsv);

    seq_printf(m, "\n");
    seq_printf(m, "      %s / %s / %s / %s / %s / %s / %s\n", "state", "cnt_all", "cnt_running",
               "task_issued", "intr_issued", "msi_cnt", "intr_handled");

    for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
        seq_printf(m, "tq%d : %s / %u / %u / %llu / %llu / %llu / %llu\n", i,
                   (xpd->xtqs[i].state) ? "HANG" : "NORMAL", xpd->xtqs[i].cnt_all,
                   xpd->xtqs[i].cnt_running, xpd->xtqs[i].st_all_task_issued,
                   xpd->xtqs[i].st_all_intr_issued, xpd->xtqs[i].st_all_msi_received,
                   xpd->xtqs[i].st_all_intr_handled);
    }

    return 0;
}
DEFINE_SEQ_OPEN(tqinfo);

struct xpu_proc_entry {
    const char *name;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
    const struct proc_ops fops;
#else
    const struct file_operations fops;
#endif
};

static struct xpu_proc_entry xpu_proc_entries[] = {
    //{ "sn", {
    //        .owner = THIS_MODULE,
    //        .open = xpu_proc_sn_open,
    //        .read = seq_read,
    //        .release = single_release, } },
    XPU_RO_PROC_ENTRY(errtask),     XPU_RO_PROC_ENTRY(errinfo),  XPU_RO_PROC_ENTRY(info),
    XPU_RW_PROC_ENTRY(reset_count), XPU_RO_PROC_ENTRY(sessions), XPU_RO_PROC_ENTRY(sn),
    XPU_RO_PROC_ENTRY(state),       XPU_RO_PROC_ENTRY(tqinfo),
};

int xpu_proc_entries_create(struct xpu_pd *xpd, struct proc_dir_entry *xpu_proc_root)
{
    int i = 0;
    int n = (int)ARRAY_SIZE(xpu_proc_entries);

    xpd->proc_root = proc_mkdir(xpd->proc_name, xpu_proc_root);
    if (xpd->proc_root == NULL) {
        LOGE("Create dir /proc/%s/%s error!\n", DEVICE_NAME, xpd->proc_name);
        return -1;
    }

    for (i = 0; i < n; i++) {
        const struct xpu_proc_entry *entry = &xpu_proc_entries[i];
        if (proc_create_data(entry->name, 0666, xpd->proc_root, &entry->fops, xpd) == NULL) {
            LOGE("create entry /proc/%s/%s/%s error!\n", DEVICE_NAME, xpd->dev_name, entry->name);
            goto err_xpu_proc_entry;
        }
    }

    return 0;

err_xpu_proc_entry:
    for (--i; i >= 0; --i) {
        remove_proc_entry(xpu_proc_entries[i].name, xpd->proc_root);
    }

    remove_proc_entry(xpd->proc_name, xpu_proc_root);

    return -1;
}

void xpu_proc_entries_destroy(struct xpu_pd *xpd, struct proc_dir_entry *xpu_proc_root)
{
    int i = 0;
    int n = (int)ARRAY_SIZE(xpu_proc_entries);

    if (!xpd->proc_root)
        return;

    for (i = 0; i < n; i++)
        remove_proc_entry(xpu_proc_entries[i].name, xpd->proc_root);
    remove_proc_entry(xpd->proc_name, xpu_proc_root);
}

//// Driver level procfs

DEFINE_DRVPROC_SHOW(auto_reset)
{
    seq_printf(m, "%d\n", g_kl1_config_autoreset);
    return 0;
}

DEFINE_DRVPROC_WR(auto_reset)
{
    char _buf = 0;
    if (count < 1)
        return count;

    if (copy_from_user(&_buf, buffer, 1)) {
        pr_err("xpu_drvproc_auto_reset_write copy_from_user failed\n");
        return -EFAULT;
    }

    pr_info("buf=%c cnt=%zu\n", _buf, count);

    if (_buf == '0')
        g_kl1_config_autoreset = 0;
    else if (_buf == '1')
        g_kl1_config_autoreset = 1;
    else
        return -EINVAL;

    return count;
}

DEFINE_DRVPROC_SHOW(wait_mode)
{
    switch (g_kl1_config_wait_mode) {
    case 0:
        seq_printf(m, "normal\n");
        break;
    case 1:
        seq_printf(m, "busy\n");
        break;
    case 2:
        seq_printf(m, "hybrid\n");
        break;
    }
    return 0;
}

DEFINE_DRVPROC_WR(wait_mode)
{
    char _buf[17] = { '\0' };
    if (count > 16) {
        count = 16;
    }
    if (copy_from_user(_buf, buffer, count)) {
        printk("xpu_drvproc_wait_mode_write copy from user failed\n");
        return -1;
    }

    if (strncmp(_buf, "normal", 6) == 0) {
        g_kl1_config_wait_mode = 0;
    } else if (strncmp(_buf, "busy", 4) == 0) {
        g_kl1_config_wait_mode = 1;
    } else if (strncmp(_buf, "hybrid", 6) == 0) {
        g_kl1_config_wait_mode = 2;
    } else {
        printk("xpu_drvproc_wait_mode_write unknown wait mode:%s\n", _buf);
        return -1;
    }
    return count;
}

static struct xpu_proc_entry xpu_driver_proc_entries[] = {
    XPU_RW_DRVPROC_ENTRY(auto_reset),
    XPU_RW_DRVPROC_ENTRY(wait_mode),
};

int xpu_drvproc_entries_create()
{
    int i = 0;
    int n = (int)ARRAY_SIZE(xpu_driver_proc_entries);

    for (i = 0; i < n; i++) {
        const struct xpu_proc_entry *entry = &xpu_driver_proc_entries[i];
        if (proc_create_data(entry->name, 0666, g_proc_root, &entry->fops, NULL) == NULL) {
            LOGE("create entry /proc/%s/%s error!\n", DEVICE_NAME, entry->name);
            goto err_out;
        }
    }

    return 0;

err_out:
    for (--i; i >= 0; --i) {
        remove_proc_entry(xpu_driver_proc_entries[i].name, g_proc_root);
    }
    return -1;
}

void xpu_drvproc_entries_destroy()
{
    int i = 0;
    int n = (int)ARRAY_SIZE(xpu_driver_proc_entries);

    if (g_proc_root == NULL)
        return;

    for (i = 0; i < n; i++) {
        remove_proc_entry(xpu_driver_proc_entries[i].name, g_proc_root);
    }
}
