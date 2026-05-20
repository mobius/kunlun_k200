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
// xpu_irq.c - XPU interrupt handler
//
#define __FILENAME__ "xpu_irq.c"

#include <linux/atomic.h>
#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/version.h>

#include "xpurt_priv/ioctl.h"
#include "xpu_drv.h"
#include "xpu_regs.h"
#include "xpu_hw.h"
#include "device.h"

/*******************
 * Interrupt Handler
 *******************/

//static u64 s_cluster_except_bit[2][4] = {
//    {   // clusters on pd0
//        0x1ULL << (14),
//        0x1ULL << (15),
//        0x1ULL << (13 + 32),
//        0x1ULL << (14 + 32)
//    },
//    {   // cluster on pd1
//        0x1ULL << (16 + 14),
//        0x1ULL << (16 + 15),
//        0x1ULL << (16 + 13 + 32),
//        0x1ULL << (16 + 14 + 32)
//    }
//};
//static u64 s_cdnn_cluster_except_bit[2][4] = {
//    {   // clusters on pd0
//        0x1ULL << (6),
//        0x1ULL << (8),
//        0x1ULL << (10),
//        0x1ULL << (12)
//    },
//    {   // cluster on pd1
//        0x1ULL << (16 + 6),
//        0x1ULL << (16 + 8),
//        0x1ULL << (16 + 10),
//        0x1ULL << (16 + 12)
//    }
//};
//static u64 s_cdnn_except_bit[2][4] = {
//    {   // clusters on pd0
//        0x1ULL << (7),
//        0x1ULL << (9),
//        0x1ULL << (11),
//        0x1ULL << (13)
//    },
//    {   // cluster on pd1
//        0x1ULL << (16 + 7),
//        0x1ULL << (16 + 9),
//        0x1ULL << (16 + 11),
//        0x1ULL << (16 + 13)
//    }
//};
static u64 s_mcu_int_bit[3] = {
    // in INTC_REG_3
    (0x00010000ULL << 32), // bit 16, usr_0 MCU FEEDBACK
    (0x00020000ULL << 32), // bit 17, usr_1 MCU REQUEST
    (0x00040000ULL << 32)  // bit 18, usr_2 MCU WARNING
};
static u64 s_test_intr_bit[4] = {
    // in INTC_REG_3
    (0x10000000ULL << 32), // bit 28, usr_12
    (0x20000000ULL << 32), // bit 29, usr_13
    (0x40000000ull << 32), // bit 30, usr_14
    (0x80000000ull << 32), // bit 31, usr_15, trigger pause
};
static const u64 s_prepare_reset_bit = 0x1ull << (PRERESET_USRINTR_IDX + 48);

static void isr_handle_prepare_reset(struct xpu_device *xdev)
{
    xpuhw_intc_disable_msi(xdev);
    //xdev->state = XDS_PRERESET;
    LOGI("xdev%d: isr_handle_prepare_reset finish\n", xdev->kdev->idx);
    complete(&xdev->irq_disable_done);
}

static void isr_handle_sse_intr(struct xpu_device *xdev, u64 st10, u64 *clr10)
{
    static struct {
        u64 testbit;
        int pd_idx;
        int tq_idx;
    } info[] = {
        {
                .testbit = 0x1ull << 32,
                .pd_idx  = 0,
                .tq_idx  = 0,
        },
        {
                .testbit = 0x1ull << 33,
                .pd_idx  = 0,
                .tq_idx  = 1,
        },
        {
                .testbit = 0x1ull << 34,
                .pd_idx  = 0,
                .tq_idx  = 2,
        },
        {
                .testbit = 0x1ull << 35,
                .pd_idx  = 0,
                .tq_idx  = 3,
        },
        {
                .testbit = 0x1ull << 36,
                .pd_idx  = 0,
                .tq_idx  = 4,
        },
        {
                .testbit = 0x1ull << 37,
                .pd_idx  = 0,
                .tq_idx  = 5,
        },
        {
                .testbit = 0x1ull << 38,
                .pd_idx  = 0,
                .tq_idx  = 6,
        },
        {
                .testbit = 0x1ull << 39,
                .pd_idx  = 0,
                .tq_idx  = 7,
        },
        {
                .testbit = 0x1ull << 48,
                .pd_idx  = 1,
                .tq_idx  = 0,
        },
        {
                .testbit = 0x1ull << 49,
                .pd_idx  = 1,
                .tq_idx  = 1,
        },
        {
                .testbit = 0x1ull << 50,
                .pd_idx  = 1,
                .tq_idx  = 2,
        },
        {
                .testbit = 0x1ull << 51,
                .pd_idx  = 1,
                .tq_idx  = 3,
        },
        {
                .testbit = 0x1ull << 52,
                .pd_idx  = 1,
                .tq_idx  = 4,
        },
        {
                .testbit = 0x1ull << 53,
                .pd_idx  = 1,
                .tq_idx  = 5,
        },
        {
                .testbit = 0x1ull << 54,
                .pd_idx  = 1,
                .tq_idx  = 6,
        },
        {
                .testbit = 0x1ull << 55,
                .pd_idx  = 1,
                .tq_idx  = 7,
        },
    };

    int i = 0;
    for (i = 0; i < 2 * KL1_SSE_TQ_COUNT; ++i) {
        int pd_idx = info[i].pd_idx;
        int tq_idx = info[i].tq_idx;
        if (st10 & info[i].testbit) {
            *clr10 |= info[i].testbit;
            ISR_xtq_on_finish(&xdev->xpd[pd_idx].xtqs[tq_idx],
                              xpuhw_sse_rc_tq_intr_cnt(&xdev->xpd[pd_idx], tq_idx));
        }
    }
}

// 1. check which cunit issue an exception
// 2. get task token from cluster's 0x801c and 0x8020 regs
// 3. search xpu_task in xpu_tq's running tasks for this exception token
// 4. mark the xpu_tq as hangup and complete all intr tasks in it with an errno
static void isr_handle_cu_exception(struct xpu_device *xdev, u64 st10, u64 *clr10, u64 *msk10)
{
    int pd_idx = 0;
    int i, j;

    LOGW("xdev%d: INTC_STATUS01= 0x%llx\n", xdev->kdev->idx, st10 & INTC_EXCEPT_10);

    for (pd_idx = 0; pd_idx < XPU_PD_NUM; ++pd_idx) {
        struct xpu_pd *xpd       = &xdev->xpd[pd_idx];
        bool           has_error = false;

        // this pd is not enabled
        if (xpd->state == XPDS_UNUSED)
            continue;

        // clean is needed as long as there be an exception, no matter whether
        // the exception is on this PD
        xpd_clean_etasks_ifneed(xpd);

        for (i = 0; i < XPD_CLUSTER_COUNT; ++i) {
            struct exception_info *e = &g_clstr_dbgs[i];
            u64 cl_ebit              = 1ull << (e->status_idx * 32 + e->bit_idx + pd_idx * 16);
            u32 tk, rsn;

            if (!(st10 & cl_ebit))
                // exception on cluster_{pd_idx}_{i}
                continue;

            has_error = true;

            tk  = xpuhw_get_xpu_token(xpd, i);
            rsn = xpuhw_get_xpu_err(xpd, i);

            xpd->cu_error[i].token     = tk;
            xpd->cu_error[i].cl_reason = rsn;

            LOGW("xpu%d: %s exception token=%u reason=0x%x\n", xpd->devfile_id, e->name, tk, rsn);
            for (j = 0; j < 32; ++j)
                if ((rsn >> j) & 0x1)
                    LOGW("xpu%d: ..reason[%d] %s\n", xpd->devfile_id, j, e->reason_table[j]);
        }

        for (i = 0; i < XPD_CDNN_COUNT; ++i) {
            struct exception_info *e;
            u64                    cl_ebit, sd_ebit;
            u32                    tk, rsn;

            e       = &g_clstr_dbgs[4 + i];
            cl_ebit = 1ull << (e->status_idx * 32 + e->bit_idx + pd_idx * 16);

            e       = &g_clstr_dbgs[8 + i];
            sd_ebit = 1ull << (e->status_idx * 32 + e->bit_idx + pd_idx * 16);

            if (!(st10 & (cl_ebit | sd_ebit)))
                continue;

            has_error = true;

            tk                                         = xpuhw_get_cdnn_token(xpd, i);
            xpd->cu_error[XPD_CLUSTER_COUNT + i].token = tk;

            if (st10 & cl_ebit) {
                e                                              = &g_clstr_dbgs[4 + i];
                rsn                                            = xpuhw_get_cdnn_cl_err(xpd, i);
                xpd->cu_error[XPD_CLUSTER_COUNT + i].cl_reason = rsn;

                LOGW("xpu%d: %s exception token=%u reason=0x%x\n", xpd->devfile_id, e->name, tk,
                     rsn);
                for (j = 0; j < 32; ++j)
                    if ((rsn >> j) & 0x1)
                        LOGW("xpu%d: ..reason[%d] %s\n", xpd->devfile_id, j, e->reason_table[j]);
            }

            if (st10 & sd_ebit) {
                e                                              = &g_clstr_dbgs[8 + i];
                rsn                                            = xpuhw_get_cdnn_err(xpd, i);
                xpd->cu_error[XPD_CLUSTER_COUNT + i].sd_reason = rsn;

                LOGW("xpu%d: %s exception token=%u reason=0x%x\n", xpd->devfile_id, e->name, tk,
                     rsn);
                for (j = 0; j < 32; ++j)
                    if ((rsn >> j) & 0x1)
                        LOGW("xpu%d: ..reason[%d] %s\n", xpd->devfile_id, j, e->reason_table[j]);
            }
        }

        if (has_error)
            ISR_xpd_on_exception(xpd);
    }

    if (g_kl1_config_autoreset)
        schedule_work(&xdev->reset_work);

    *msk10 |= st10 & INTC_EXCEPT_10;
    *clr10 |= st10 & INTC_EXCEPT_10;

    xdev->exception_bits01 |= st10 & INTC_EXCEPT_10;
}

void isr_handle_sse_exception(struct xpu_device *xdev, u64 st10, u64 *clr10, u64 *msk10)
{
    if ((st10 >> 41) & 0x1)
        isr_xpd_on_sse_exception(&xdev->xpd[0]);

    if ((st10 >> 57) & 0x1)
        isr_xpd_on_sse_exception(&xdev->xpd[1]);

    if (g_kl1_config_autoreset)
        schedule_work(&xdev->reset_work);

    *msk10 |= st10 & INTC_SSE_EXCEPTION_10;
    *clr10 |= st10 & INTC_SSE_EXCEPTION_10;

    xdev->exception_bits01 |= st10 & INTC_SSE_EXCEPTION_10;
}

static void isr_resume_xpd(struct xpu_pd *xpd)
{
    int i = 0;

    if (xpd->state != XPDS_PAUSED) {
        LOGW("[xpu_%d] reject RESUME rqst, current state is %s\n", xpd->devfile_id,
             xpd_state_str(xpd->state));
        return;
    }

    xpd_state_to_running(xpd);

    for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
        struct xpu_tq *xtq = &xpd->xtqs[i];
        tasklet_schedule(&xtq->tasklet_dispatch);
    }
}

static void isr_pausing_xpd(struct xpu_pd *xpd)
{
    int i = 0;

    if (xpd->state != XPDS_RUNNING) {
        LOGW("[xpu_%d] reject PAUSING rqst, current state is %s\n", xpd->devfile_id,
             xpd_state_str(xpd->state));
        return;
    }

    xpd_state_to_pausing(xpd);

    atomic_set(&xpd->xtqs_pending_finish_flag, 0);

    for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
        unsigned long  flags = 0;
        struct xpu_tq *xtq   = &xpd->xtqs[i];
        spin_lock_irqsave(&xtq->lock, flags);
        if (xtq->cnt_running == 0) {
            __atomic_or((0x1 << xtq->id), &xpd->xtqs_pending_finish_flag);
        }
        spin_unlock_irqrestore(&xtq->lock, flags);
    }

    if (atomic_read(&xpd->xtqs_pending_finish_flag) == ((0x1 << KL1_SSE_TQ_COUNT) - 1))
        xpd_state_to_paused(xpd);
}

static void isr_mcu_request(struct xpu_device *xdev)
{
    struct kl_device *kdev = xdev->kdev;
    void *buffer_addr = kdev->bar[0] + (reg_readl(xdev->syscon_base + RSYSCON_G_R0) - RSRAM_BASE);
    u32   request_reg = reg_readl(buffer_addr + 0x4);
    int   num_pd      = (xdev->product_num & (0x1 << 24)) ? 1 : 2;
    int   pd          = 0;
    if (request_reg & DEVICE_CMD_HOST_PAUSE) {
        int num_paused = 0;
        LOGW("mcu DEVICE_CMD_HOST_PAUSE request, 0x%x\n", request_reg);
        for (pd = 0; pd < num_pd; ++pd) {
            isr_pausing_xpd(&xdev->xpd[pd]);
            if (xdev->xpd[pd].state == XPDS_PAUSED) {
                // all xtqs are paused
                num_paused++;
            } else {
                int i;
                LOGI("[xpu_%d] xtqs_pending_finish_flag: 0x%x\n", xdev->xpd[pd].devfile_id,
                     (u32)atomic_read(&xdev->xpd[pd].xtqs_pending_finish_flag));

                for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
                    struct xpu_tq *xtq = &xdev->xpd[pd].xtqs[i];
                    LOGI("[xpu_%d tq_%d] cnt_running: %u\n", xtq->xpd->devfile_id, xtq->id,
                         xtq->cnt_running);
                    if (xtq->cnt_running > 0) {
                        struct xpu_task *lrt =
                                list_entry(xtq->last_running, struct xpu_task, tq_tasks_ent);
                        LOGI("[xpu_%d tq_%d] last_running: %u\n", xtq->xpd->devfile_id, xtq->id,
                             lrt->type);
                    }
                }
            }
        }
        if (num_paused == num_pd) {
            reg_writel(buffer_addr + 0x4, 0x0);
        }
    } else if (request_reg & DEVICE_CMD_HOST_RESUME) {
        LOGW("mcu DEVICE_CMD_HOST_RESUME request, 0x%x\n", request_reg);
        for (pd = 0; pd < num_pd; ++pd) {
            isr_resume_xpd(&xdev->xpd[pd]);
        }
        reg_writel(buffer_addr + 0x4, 0x0);
    } else {
        LOGW("Unknow mcu request, 0x%x\n", request_reg);
    }
}

static void isr_handle_hbm_ucecc_exception(struct xpu_device *xdev)
{
    int pd_id;

    for (pd_id = 0; pd_id < XPU_PD_NUM; ++pd_id) {
        struct xpu_pd *xpd = &xdev->xpd[pd_id];

        if (xpd->state == XPDS_UNUSED)
            continue;

        if (xpuhw_hbm_has_ucecc(xpd)) {
            LOGI("xpu%d: UnCorrectable ECC\n", xpd->devfile_id);
            xpd_state_to_error(xpd, XPUERR_UCECC);
        }
    }
}

static void isr_mcu_warning(struct xpu_device *xdev)
{
#define MCU_WARNING_PLL (0x1)
#define MCU_WARNING_HBM_ECC_2BITS (0x1 << 1)
#define MCU_WARNING_HBM0_CATASTROPHIC (0x1 << 2)
#define MCU_WARNING_HBM1_CATASTROPHIC (0x1 << 3)
#define MCU_WARNING_OTHER (0x1 << 4)

    struct kl_device *kdev = xdev->kdev;
    void *buffer_addr = kdev->bar[0] + (reg_readl(xdev->syscon_base + RSYSCON_G_R0) - RSRAM_BASE);
    u32   rsn         = reg_readl(buffer_addr + 0x8);
    switch (rsn) {
    case MCU_WARNING_PLL:
        // PLL down, could be ignored
        break;
    case MCU_WARNING_HBM_ECC_2BITS:
        isr_handle_hbm_ucecc_exception(xdev);
        break;
    case MCU_WARNING_HBM0_CATASTROPHIC:
        LOGI("xpu0: HBM overheat\n");
        xpd_state_to_error(&xdev->xpd[0], XPUERR_OVERHEAT);
        break;
    case MCU_WARNING_HBM1_CATASTROPHIC:
        LOGI("xpu1: HBM overheat\n");
        xpd_state_to_error(&xdev->xpd[1], XPUERR_OVERHEAT);
        break;
    case MCU_WARNING_OTHER:
        break;
    default:
        LOGI("xdev%d: unknown fw warning %u\n", xdev->kdev->idx, rsn);
        break;
    }
}

// interrupt handling general process:
// 1. read all status (0/1/2/3) from intc registers
// 2. for each bit that is non-zero, process it in pre-defined way
// 3. mask rrm bits if any and clear all processed bits
// 4. read all status again, and go to step2 if any is non-zero
//
// step4 guaranteed that there is a point in isr and all status are zero, which
// is required for intc to work properly.
//
// there are 3 ways of process interrupt bits:
// normal - for bits indicating sse task queues
// ignore - for bits that should always be ignored
// rrm    - mostly bits that indicating an exception,
//          these intrs will be receive, record and masked at the first time
irqreturn_t xpu_interrupt(int irq, void *instance)
{
    DECLARE_PROFILER(PROF_isr);
    struct xpu_device *xdev       = (struct xpu_device *)instance;
    u64                st10       = 1; // status_1 and status_0
    u64                st32       = 1; // status_3 and status_2
    int                loop_count = 0; // make sure that isr will not run forever

    //LOGL6("receive irq\n");

    if (unlikely(irq != xdev->pdev->irq))
        return IRQ_RETVAL(IRQ_HANDLED);

    if (unlikely(xdev->disabled))
        return IRQ_RETVAL(IRQ_HANDLED);

    START_PROFILING(PROF_isr);

    if (unlikely(xdev->state == XDS_POSTRESET))
        return IRQ_RETVAL(IRQ_HANDLED);

    // get all interrupt status
    st10 = xpuhw_intc_statusq(xdev, 0);
    st32 = xpuhw_intc_statusq(xdev, 2);

    if (unlikely((st10 == ~0x0ull) || (st32 == ~0x0ull)))
        goto out;

    if (unlikely(xdev->state == XDS_PRERESET)) {
        if (st32 & s_mcu_int_bit[0]) {
            xdev->state = XDS_POSTRESET;
            xpuhw_intc_clearq(xdev, 2, st32);
            LOGI("xdev%d: RESET FINISH received\n", xdev->kdev->idx);
            complete(&xdev->reset_done);
        } else if (st32 & s_prepare_reset_bit) {
            isr_handle_prepare_reset(xdev);
            return IRQ_RETVAL(IRQ_HANDLED);
        } else {
            LOGW("xdev%d: Intr received during reset st3210=%llx,%llx\n", xdev->kdev->idx, st32,
                 st10);
            xpuhw_intc_mask_and_clearq(xdev, 0, st10);
            xpuhw_intc_mask_and_clearq(xdev, 2, st32);
            xpuhw_intc_toggle_msi(xdev);
        }
        return IRQ_RETVAL(IRQ_HANDLED);
    }

    while ((st10 || st32) & (loop_count < 10)) {
        u64 msk10 = 0; // store bits need to mask in step3
        u64 msk32 = 0; // store bits need to mask in step3
        u64 clr10 = 0; // store bits need to clear in step3
        u64 clr32 = 0; // store bits need to clear in step3
        u64 rrm10 = 0; // rrm bits in status_1 and status_0
        u64 rrm32 = 0; // rrm bits in status_3 and status_2

        ++loop_count;
        LOGL4("loop_%d st10= %llx st32= %llx\n", loop_count, st10, st32);

        if (unlikely((st10 == ~0x0ull) || (st32 == ~0x0ull)))
            goto out;

        // handle mcu intr first, as HBM has highest priority
        if (unlikely(st32 & INTC_MCU_32)) {
            if (st32 & s_mcu_int_bit[1]) {
                // request
                isr_mcu_request(xdev);
            } else {
                // warning
                isr_mcu_warning(xdev);
            }
            clr32 |= INTC_MCU_32;
        }

        // SSE priority is higher than CU exception
        if (st10 & INTC_SSE_EXCEPTION_10)
            isr_handle_sse_exception(xdev, st10, &clr10, &msk10);

        // handle execution exception from cluster or cdnn
        if (st10 & INTC_EXCEPT_10)
            isr_handle_cu_exception(xdev, st10, &clr10, &msk10);

        // handle normal sse interrupt
        if (st10 & INTC_NORMAL_10)
            isr_handle_sse_intr(xdev, st10, &clr10);

        // Trigger driver pausing and resume, test and temporary code
        if (st32 & INTC_TEST_32) {
            if (st32 & s_test_intr_bit[0])
                // place holder
                isr_resume_xpd(&xdev->xpd[0]);

            if (st32 & s_test_intr_bit[1])
                // place holder
                isr_resume_xpd(&xdev->xpd[1]);

            if (st32 & s_test_intr_bit[2])
                // place holder
                isr_pausing_xpd(&xdev->xpd[0]);

            if (st32 & s_test_intr_bit[3])
                // trigger pause
                isr_pausing_xpd(&xdev->xpd[1]);

            clr32 |= INTC_TEST_32;
        }

        // this should not happen, as all IRNOGED bits should already be masked
        // most likely the XPU has been reseted if it did happen
        if (st10 & INTC_IGNORED_10) {
            msk10 |= INTC_IGNORED_10;
            clr10 |= INTC_IGNORED_10;
            LOGL4("ignored intr received, st10= 0x%llx\n", st10 & INTC_IGNORED_10);
        }

        if (st32 & INTC_IGNORED_32) {
            msk32 |= INTC_IGNORED_32;
            clr32 |= INTC_IGNORED_32;
            LOGL4("ignored intr received, st32= 0x%llx\n", st32 & INTC_IGNORED_32);
        }

        // received an RRM intr the first time, record it and mask it later
        if ((rrm10 = st10 & INTC_RRM_10) != 0) {
            xdev->exception_bits01 |= rrm10;
            msk10 |= rrm10;
            clr10 |= rrm10;
            LOGW("RRM exception received st10= 0x%llx\n", rrm10);
        }

        if ((rrm32 = st32 & INTC_RRM_32) != 0) {
            xdev->exception_bits23 |= rrm32;
            msk32 |= rrm32;
            clr32 |= rrm32;
            LOGW("RRM exception received st32= 0x%llx\n", rrm32);
        }

        // do the mask and clear
        if (msk10)
            xpuhw_intc_maskq(xdev, 0, msk10);

        if (clr10)
            xpuhw_intc_clearq(xdev, 0, clr10);

        if (msk32)
            xpuhw_intc_maskq(xdev, 2, msk32);

        if (clr32)
            xpuhw_intc_clearq(xdev, 2, clr32);

        // read again to make sure that all intr sources have been handled
        st10 = xpuhw_intc_statusq(xdev, 0);
        st32 = xpuhw_intc_statusq(xdev, 2);
    }

    ++xdev->st_all_msi_received;

    // toggle INTC if status still not zero
    if (st10 || st32)
        xpuhw_intc_toggle_msi(xdev);

    END_PROFILING(PROF_isr, &xdev->xpd[0]);
    END_PROFILING(PROF_isr, &xdev->xpd[1]);

out:
    return IRQ_RETVAL(IRQ_HANDLED);
}

int xpu_msi_register(struct xpu_device *xdev)
{
    int             ret     = 0;
    struct pci_dev *pdev    = xdev->pdev;
    int             pos     = 0;
    u16             control = 0;

    // TODO: handle case, positive return value
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 14, 0)
    xdev->irq_enabled = 1;
    while ((ret = pci_enable_msi_block(pdev, xdev->irq_enabled)) > 0)
        xdev->irq_enabled = ret;

    if (ret < 0) {
        LOGE("pci_enable_msi_block() = %d\n", ret);
        return -XPUERR_DEVINIT;
    }
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
    ret = pci_enable_msi_range(pdev, 1, 1);
    if (ret < 1) {
        LOGE("pci_enable_msi_range() = %d\n", ret);
        return -XPUERR_DEVINIT;
    }
    xdev->irq_enabled = ret;
#else
    ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI);
    if (ret < 1) {
        LOGE("pci_alloc_irq_vectors() = %d\n", ret);
        return -XPUERR_DEVINIT;
    }
    xdev->irq_enabled = ret;
#endif

    // TODO: here IRQF_DISABLED may not be needed, as long as different irq handler
    // acquire different spin_lock
    ret = request_irq(pdev->irq + 0, &xpu_interrupt, IRQF_SHARED, xdev->kdev->name, xdev);
    if (ret) {
        LOGE("request_irq(%d) = %d\n", pdev->irq, ret);
        return -XPUERR_DEVINIT;
    }

    pos = pci_find_capability(pdev, PCI_CAP_ID_MSI);
    pci_read_config_dword(pdev, pos + PCI_MSI_ADDRESS_LO, &xdev->msi_lo);
    pci_read_config_dword(pdev, pos + PCI_MSI_ADDRESS_HI, &xdev->msi_hi);
    pci_read_config_word(pdev, pos + PCI_MSI_FLAGS, &control);
    pci_read_config_word(pdev, pos + ((control & PCI_MSI_FLAGS_64BIT) ? 12 : 8), &xdev->msi_data);

    LOGI("Setup MSI, irq=%d~%d, addr=%x_%x, data=%x\n", pdev->irq,
         pdev->irq + xdev->irq_enabled - 1, xdev->msi_hi, xdev->msi_lo, xdev->msi_data);
    return 0;
}

void xpu_msi_unregister(struct xpu_device *xdev)
{
#ifndef XPU_DRIVER_TEST
    int             i    = 0;
    struct pci_dev *pdev = xdev->pdev;
    for (i = 0; i < xdev->irq_enabled; ++i)
        free_irq(pdev->irq + i, xdev);
    pci_disable_msi(pdev);
#endif
}
