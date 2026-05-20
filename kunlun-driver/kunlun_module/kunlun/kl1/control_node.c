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

#define __FILENAME__ "control_node.c"

#include "xpu_drv.h"

// ctrlnode iocv5
int kl1_query_device_info_v1(struct kl_device *kdev, union xpu_device_info_v1 *i)
{
    struct xpu_pd     *xpd        = NULL;
    struct xpu_device *xdev       = NULL;
    int                devfile_id = 0;

    if (i->in.type != QDIT_PROC_INCG) {
        devfile_id = i->in.arg[0];

        xpd = get_xpd_by_devfile_id(devfile_id);
        if (!xpd)
            return -ENODEV;

        xdev = xpd->xdev;
    }

    switch (i->in.type) {
    case QDIT_MODEL_SN: {
        if (xdev->product_num & (0x1 << 24))
            i->out.ret = K100;
        else
            i->out.ret = K200;
        i->out.v64[0] = xdev->sn;
        break;
    }
    case QDIT_COORDINATE: {
        i->out.v32    = xpd->devfile_id;
        i->out.v64[0] = makeu64(xdev->kdev->idx, xpd->id);
        i->out.v64[1] = makeu64(xdev->domain, xdev->bus);
        i->out.v64[2] = makeu64(xdev->slot, xdev->func);
        break;
    }
    case QDIT_HS_MEM: {
        i->out.v32    = xpd->mem[MMRGN_L3].page_size;
        i->out.v64[0] = xpd->mem[MMRGN_L3].page_used;
        i->out.v64[1] = xpd->mem[MMRGN_L3].page_count;
        break;
    }
    case QDIT_MAIN_MEM: {
        i->out.v64[0] = xpd->mem[MMRGN_HBM_LO].page_used + xpd->mem[MMRGN_HBM_HI].page_used;
        i->out.v64[1] = xpd->mem[MMRGN_HBM_LO].page_count + xpd->mem[MMRGN_HBM_HI].page_count;
        i->out.v32    = xpd->mem[MMRGN_HBM_LO].page_size;
        break;
    }
    case QDIT_USE_RATIO: {
        i->out.v32    = bitmap_weight(xpd->stat_use_ratio_bitmap, USE_RATIO_PN);
        i->out.v64[0] = USE_RATIO_PN;
        break;
    }
    case QDIT_DMA_RATIO: {
        i->out.v32    = 0; // time_us
        i->out.v64[0] = 0; // read bytes
        i->out.v64[1] = 0; // write bytes
        i->out.v64[2] = 0;
        break;
    }
    case QDIT_TEMPERATURE: {
        i->out.v32 = xdev->monitor.hbm_temp[xpd->id];
        i->out.v64[0] =
                makeu64(xdev->monitor.temp[xpd->id * 2], xdev->monitor.temp[xpd->id * 2 + 1]);
        break;
    }
    case QDIT_STATE: {
        if (xdev->state == XDS_RUNNING)
            i->out.ret = xpd->state;
        else
            i->out.ret = xdev->state;
        break;
    }
    case QDIT_FREQUENCY: {
        i->out.v64[0] = makeu64(xdev->monitor.freq[0], xdev->monitor.freq[1]);
        i->out.v64[1] = makeu64(xdev->monitor.freq[2], xdev->monitor.freq[3]);
        i->out.v64[2] = makeu64(xdev->monitor.freq[4], xdev->monitor.freq[5]);
        break;
    }
    case QDIT_POWER: {
        i->out.v32 = xdev->monitor.power;
        break;
    }
    case QDIT_FIRMWARE_VERSION: {
        i->out.v32    = xdev->flash_version[0];
        i->out.v64[0] = makeu64(xdev->flash_version[1], xdev->flash_version[2]);
        i->out.v64[1] = xdev->cpld_version;
        break;
    }
    case QDIT_CU: {
        i->out.v64[0] = 4;
        i->out.v64[1] = 4;
        i->out.v64[2] = KL1_EDMA_CHANNEL_NUM_ONE_PD;
        break;
    }
    case QDIT_CODEC: {
        i->out.v64[0] = 0;
        i->out.v64[1] = 0;
        i->out.v64[2] = 0;
        break;
    }
    case QDIT_SRIOV_INFO: {
        i->out.v32    = 0;
        i->out.v64[0] = 0;
        i->out.v64[1] = 0;
        i->out.v64[2] = 0;
        break;
    }
    default:
        return -XPUERR_NOSUPPORT;
    }

    return 0;
}

int kl1_query_device_proc_info(struct kl_device *kdev, struct ioc_qproc_info_in *i,
                               struct xpu_device_processes *dp)
{
    struct xpu_pd      *xpd;
    struct xpu_context *xctx;
    unsigned long       flags;
    int                 devfile_id = i->devfile_id;

    xpd = get_xpd_by_devfile_id(devfile_id);
    if (!xpd)
        return -ENODEV;

    dp->magic_version = PROCINFO_MAGIC_V0;
    dp->process_count = 0;
    dp->devfile_id    = devfile_id;
    dp->dma_time_us   = 0;

    spin_lock_irqsave(&xpd->sessions_lock, flags);
    list_for_each_entry(xctx, &xpd->contexts, xpd_contexts_ent) {
        struct xpu_proc *proc = &dp->proc[dp->process_count];

        proc->pid          = xctx->pid;
        proc->root_ns_pid  = xctx->pid;
        proc->stream_count = atomic_read(&xctx->sess_cnt);
        memcpy(proc->comm, xctx->comm, PROCESS_COMM_LEN);
        proc->cgtoken             = 0;
        proc->hs_mem_pgused       = atomic_read(&xctx->cache_mem_page_used);
        proc->hs_mem_pgall        = 0;
        proc->main_mem_pgused     = atomic_read(&xctx->main_page_used);
        proc->main_mem_pgall      = 0;
        proc->dma_read_byte_size  = 0;
        proc->dma_write_byte_size = 0;
        proc->ur_weight           = bitmap_weight(xpd->stat_use_ratio_bitmap, USE_RATIO_PN);
        proc->level               = 0;
        ++dp->process_count;
        if (dp->process_count >= DEVICE_MAX_PROCESS_PRINT_COUNT)
            break;
    }
    spin_unlock_irqrestore(&xpd->sessions_lock, flags);

    return 0;
}
