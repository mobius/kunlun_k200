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

#include "xpu_drv.h"
#include "xpu_hw.h"
#include <linux/mutex.h>
#include <linux/version.h>

bool xpu_device_disabled_or_in_reset(struct xpu_device *xdev)
{
    if ((xdev->disabled) || (atomic_read(&xdev->in_reset)))
        return true;
    else
        return false;
}

static int xpu_mcu_reinit_after_reset(struct xpu_device *xdev)
{
    int mcutimeout      = 0;
    u32 mcu_status      = 0;
    int i               = 0;
    u32 redirect_ints[] = {
        0x1100,                                                         // pm_int
        0x1200,                                                         // temp_int
        0x2800, 0x2804, 0x2808, 0x280C, 0x2810, 0x2814, 0x2818, 0x281C, // hbm0 int
        0x3800, 0x3804, 0x3808, 0x380C, 0x3810, 0x3814, 0x3818, 0x381C, // hbm1 int
    };

    while (mcutimeout < 10) { // about 10s
        mcu_status = xreadl(xdev->syscon_base, RSYSCON_G_R1);

        if (mcu_status == MCU_STATUS_NORMAL)
            break;

        msleep(1000);
        ++mcutimeout;
    }

    if (mcu_status != MCU_STATUS_NORMAL) {
        LOGW("xdev%d: MCU is not ready, status=0x%x\n", xdev->kdev->idx, mcu_status);

#define CHECK_GR(v)                                                                                \
    do {                                                                                           \
        xreadl(xdev->syscon_base, RSYSCON_G_R##v);                                                 \
    } while (0)

        CHECK_GR(0);
        CHECK_GR(1);
        CHECK_GR(2);
        CHECK_GR(3);
        CHECK_GR(4);
        CHECK_GR(5);
        CHECK_GR(6);
        CHECK_GR(7);
#undef CHECK_GR

        return -XPUERR_MCUUNINIT;
    }

    //LOGI("redirect %u interrupts to msi-31\n", sizeof(redirect_ints) / sizeof(u32));
    // redirect interrupts to msi 31, avoid driver handling them
    for (i = 0; i < sizeof(redirect_ints) / sizeof(u32); ++i)
        xwritel(31, xdev->intc_base, redirect_ints[i]);

    return 0;
}

int xpu_reinit_after_reset(struct xpu_device *xdev)
{
    int err;
    int pd_id, tq;

#if defined(PLATFORM_KUNLUN)
    xpuhw_setup_iatu_for_setup(xdev);

    err = xpu_mcu_reinit_after_reset(xdev);
    if (err)
        goto err_out;

    err = xpuhw_setup_iatu(xdev);
    if (err)
        goto err_out;

    xpuhw_edma_init(xdev);
#endif

    // hw setup PCIe iATU
    //err = xpuhw_setup_iatu(xdev);
    //if (err)
    //    goto err_out;

    for (pd_id = 0; pd_id < XPU_PD_NUM; ++pd_id) {
        struct xpu_pd *xpd = &xdev->xpd[pd_id];

        if (xpd->state == XPDS_UNUSED)
            continue;

        xpuhw_sse_init(xpd);
        xpuhw_cluster_round_mode_init(xpd);

        xpd->state             = XPDS_RUNNING;
        xpd->need_clean_etasks = 1;
        xpd->errno             = 0;

        for (tq = 0; tq < KL1_SSE_TQ_COUNT; ++tq) {
            struct xpu_tq *xtq = &xpd->xtqs[tq];

            xtq->state        = XTQS_NORMAL;
            xtq->cnt_running  = 0;
            xtq->cnt_all      = 0;
            xtq->last_running = &xtq->tasks;
            atomic_set(&xtq->dispatch_pending, 0);
            xtq->st_all_intr_issued  = 0;
            xtq->st_all_intr_handled = 0;
            xtq->st_all_msi_received = 0;
            xtq->st_all_task_issued  = 0;
            xtq->enable              = 1;
        }

        //atomic_set(&xpd->except_tokens_cnt, 0);
        atomic_set(&xpd->xtqs_pending_finish_flag, 0);
    }

    xpuhw_intc_init(xdev);

    xdev->state = XDS_RUNNING;

    return 0;

err_out:
    return err;
}

static void lock_pcie_edma(struct xpu_device *xdev)
{
    int           pd, ch;
    unsigned long flags;

    spin_lock_irqsave(&xdev->edma_rr_lock, flags);
    spin_unlock_irqrestore(&xdev->edma_rr_lock, flags);

    spin_lock_irqsave(&xdev->edma_rw_lock, flags);
    spin_unlock_irqrestore(&xdev->edma_rw_lock, flags);

    for (pd = 0; pd < xdev->pd_num; ++pd) {
        struct xpu_pd *xpd = &xdev->xpd[pd];
        for (ch = 0; ch < KL1_EDMA_CHANNEL_NUM_ONE_PD; ++ch) {
            mutex_lock(&xpd->rdch_edma[ch].lock);
            xpd->rdch_edma[ch].enable = 0;
            mutex_unlock(&xpd->rdch_edma[ch].lock);

            mutex_lock(&xpd->wrch_edma[ch].lock);
            xpd->wrch_edma[ch].enable = 0;
            mutex_unlock(&xpd->wrch_edma[ch].lock);
        }
    }
}

static void unlock_pcie_edma(struct xpu_device *xdev)
{
    int pd, ch;

    for (pd = 0; pd < xdev->pd_num; ++pd) {
        for (ch = 0; ch < KL1_EDMA_CHANNEL_NUM_ONE_PD; ++ch) {
            xdev->xpd[pd].wrch_edma[ch].enable = 1;
            xdev->xpd[pd].rdch_edma[ch].enable = 1;
        }
    }
}

static void lock_and_clean_tqs(struct xpu_device *xdev)
{
    int           pd, tq;
    unsigned long flags;

    for (pd = 0; pd < xdev->pd_num; ++pd) {
        for (tq = 0; tq < KL1_SSE_TQ_COUNT; ++tq) {
            struct xpu_tq   *xtq = &xdev->xpd[pd].xtqs[tq];
            struct xpu_task *xtask, *safe;

            spin_lock_irqsave(&xtq->lock, flags);
            xtq->enable = 0;
            list_for_each_entry_safe(xtask, safe, &xtq->tasks, tq_tasks_ent) {
                list_del(&xtask->tq_tasks_ent);
                atomic_sub(1, &xtask->xsess->unfinished_cnt);
                --xtq->cnt_running;
                --xtq->cnt_all;
                kfree(xtask);
            }
            spin_unlock_irqrestore(&xtq->lock, flags);
        }
    }
}

static void unlock_tqs(struct xpu_device *xdev)
{
    int pd, tq;

    for (pd = 0; pd < xdev->pd_num; ++pd)
        for (tq = 0; tq < KL1_SSE_TQ_COUNT; ++tq)
            xdev->xpd[pd].xtqs[tq].enable = 1;
}

// make the device's pcie stop
static int xpu_device_reset_prepare(struct xpu_device *xdev)
{
    int           err;
    int           pd_id;
    unsigned long flags;

    // Move the State Machine
    xdev->state = XDS_PRERESET;

    for (pd_id = 0; pd_id < xdev->pd_num; ++pd_id) {
        spin_lock_irqsave(&xdev->xpd[pd_id].sessions_lock, flags);
        spin_unlock_irqrestore(&xdev->xpd[pd_id].sessions_lock, flags);

        spin_lock_bh(&xdev->xpd[pd_id].timer_worklock);
        spin_unlock_bh(&xdev->xpd[pd_id].timer_worklock);
    }

    // require lock to sync the in_reset state
    spin_lock(&xdev->brw_lock);
    spin_unlock(&xdev->brw_lock);

    lock_and_clean_tqs(xdev);
    LOGI("xdev%d: tq lock finish\n", xdev->kdev->idx);

    lock_pcie_edma(xdev);
    LOGI("xdev%d: pcie_dma lock finish\n", xdev->kdev->idx);

    sessions_prepare_reset(xdev);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 12, 0)
    xdev->irq_disable_done.done = 0;
#else
    reinit_completion(&xdev->irq_disable_done);
#endif

    // pause interrupt
    LOGI("xdev%d: sending UserIntr_11...\n", xdev->kdev->idx);
    xpuhw_intc_set_usrintr(xdev, PRERESET_USRINTR_IDX);
    err = wait_for_completion_timeout(&xdev->irq_disable_done, 10 * 1000);
    if (err == 0) {
        LOGW("xdev%d: INTR disable timeout\n", xdev->kdev->idx);
        return -XPUERR_UNEXPECT;
    }

    return 0;
}

int xpu_device_reset(struct xpu_device *xdev)
{
    struct kl_device *kdev = xdev->kdev;
    u64               mcu_buffer;
    int               err;

    if (xdev->flash_version[2] < 20)
        return -XPUERR_NOSUPPORT;

    err = atomic_cmpxchg(&xdev->in_reset, 0, 1);
    if (err) {
        LOGW("xdev%d: Cannot reset XPU cause it is being reset.\n", xdev->kdev->idx);
        return -XPUERR_PEERRESET;
    }

    LOGI("xdev%d: start reset\n", xdev->kdev->idx);

    ++xdev->reset_count;

    err = xpu_device_reset_prepare(xdev);
    if (err)
        return err;

    LOGL4("xdev%d: Prepare finish\n", xdev->kdev->idx);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 12, 0)
    xdev->reset_done.done = 0;
#else
    reinit_completion(&xdev->reset_done);
#endif

    mcu_buffer = xreadl(xdev->syscon_base, RSYSCON_G_R0) - RSRAM_BASE;

    // Perform hardware soft-reset
    mutex_lock(&xdev->firmware_lock);
    xwritel(0x200, kdev->bar[0], mcu_buffer);

    //err = xpu_poll_cond_timeout(xdev->state == XDS_POSTRESET, 10, 20000);
    err = wait_for_completion_timeout(&xdev->reset_done, 20 * 1000);
    if (err == 0) {
        LOGE("xdev%d: Reset timeout.\n", xdev->kdev->idx);
        mutex_unlock(&xdev->firmware_lock);
        return -XPUERR_UNEXPECT;
    }
    mutex_unlock(&xdev->firmware_lock);

    msleep(1000);

    LOGL4("xdev%d: Device Soft Reset Finished\n", xdev->kdev->idx);

    // Re-init
    err = xpu_reinit_after_reset(xdev);
    if (err) {
        LOGE("xdev%d: Error reinit after reset\n", xdev->kdev->idx);
        return err;
    }

    unlock_tqs(xdev);
    unlock_pcie_edma(xdev);

    atomic_set(&xdev->in_reset, 0);

    LOGI("xdev%d: Reset DONE\n", xdev->kdev->idx);

    return 0;
}

void xpu_device_reset_work(struct work_struct *work)
{
    struct xpu_device *xdev = container_of(work, struct xpu_device, reset_work);
    int                err;

    LOGI("xdev%d: reset work start\n", xdev->kdev->idx);

    err = xpu_device_reset(xdev);

    LOGI("xdev%d: reset work returns %d\n", xdev->kdev->idx, err);
}
