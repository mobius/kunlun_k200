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

// Copyright 2014 Baidu Inc. All Rights Reserved.
// authors: Han Jinchen hanjinche@baidu.com
//
// Stream Scheduling Engine manager
// SSE related haredware interface
//
#define __FILENAME__ "xpu_sse.c"

#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include "xpu_regs.h"
#include "xpu_drv.h"
#include "xpu_hw.h"

// user can specify 31 params for each kernel launch, $param_id ranges [1, 31],
// RSSE_TQ_PARAM(tq_id, 0) is reserved as `load_param 0` gives the logic cluster id,
// so the first kernel param need to write into RSSE_TQ_PARAM(tq_id, 1) and load
// with `load_param 1`
#define RSSE_TQ_PARAM(tq_id, param_id) (0x30000 + (tq_id)*0x1000 + (param_id)*0x4)

// Actually, NoC address is 36-bit, so for HI address, we only need 4 bits
#define ADDR_HI(addr) (((addr) >> 32) & 0xFFFFFFFF)
#define ADDR_LO(addr) ((addr)&0xFFFFFFFF)

#define sse_regwl(reg, val)                                                                        \
    do {                                                                                           \
        reg_writel(xpd->sse_base + (reg), (val));                                                  \
        LOGL1(#reg "(%px)= 0x%x\n", xpd->sse_base + (reg), (u32)(val));                            \
    } while (0)
#define sse_regwq(reg, val)                                                                        \
    do {                                                                                           \
        reg_writeq(xpd->sse_base + (reg), (val));                                                  \
        LOGL1(#reg "(%px)= 0x%llx\n", xpd->sse_base + (reg), (u64)(val));                          \
    } while (0)

// Read the SSE version register
// maybe we need to parse the value in the furture
u32 xpuhw_sse_version(struct xpu_pd *xpd)
{
    return reg_readl(xpd->sse_base + RSSE_VERSION);
}

// Initialize SSE
void xpuhw_sse_init(struct xpu_pd *xpd)
{
    u64 pd_offset = 0;
    u32 cunits    = xpd->xdev->cunit_bits;
    u32 cldisable = 0;

    LOGL4("xpd->sse_base = %px\n", xpd->sse_base);

    // sse disable bits 0-3 XPU0-3
    // sse disable bits 4-7 CDNN0-3
    //   1 means corresponding Cluster or XPU is disabled
    // 0x20042244
    //   bit 12-15 XPU0-3 on pd1
    //   bit  8-11 XPU0-3 on pd0
    //   bit  4- 7 CDNN0-3 on pd1
    //   bit  0- 4 CDNN0-3 on pd0
    cldisable = (((~(cunits >> (8 + xpd->id * 4))) & 0xf) |        // xpu disable bits
                 (((~(cunits >> (0 + xpd->id * 4))) & 0xf)) << 4); // cdnn disable bits
    sse_regwl(RSSE_CLDISABLE, cldisable);
    LOGL4("sse_cldisable= 0x%x\n", cldisable);

    // FIXME
    if (xpd->id == 1)
        pd_offset = PD1_OFFSET;

    sse_regwl(RSSE_XPU0_BASE_LO, ADDR_LO(pd_offset + RXPU0_BASE));
    sse_regwl(RSSE_XPU0_BASE_HI, ADDR_HI(pd_offset + RXPU0_BASE));
    sse_regwl(RSSE_XPU1_BASE_LO, ADDR_LO(pd_offset + RXPU1_BASE));
    sse_regwl(RSSE_XPU1_BASE_HI, ADDR_HI(pd_offset + RXPU1_BASE));
    sse_regwl(RSSE_XPU2_BASE_LO, ADDR_LO(pd_offset + RXPU2_BASE));
    sse_regwl(RSSE_XPU2_BASE_HI, ADDR_HI(pd_offset + RXPU2_BASE));
    sse_regwl(RSSE_XPU3_BASE_LO, ADDR_LO(pd_offset + RXPU3_BASE));
    sse_regwl(RSSE_XPU3_BASE_HI, ADDR_HI(pd_offset + RXPU3_BASE));
    sse_regwl(RSSE_CDNN0_BASE_LO, ADDR_LO(pd_offset + RCDNN0CL_BASE));
    sse_regwl(RSSE_CDNN0_BASE_HI, ADDR_HI(pd_offset + RCDNN0CL_BASE));
    sse_regwl(RSSE_CDNN1_BASE_LO, ADDR_LO(pd_offset + RCDNN1CL_BASE));
    sse_regwl(RSSE_CDNN1_BASE_HI, ADDR_HI(pd_offset + RCDNN1CL_BASE));
    sse_regwl(RSSE_CDNN2_BASE_LO, ADDR_LO(pd_offset + RCDNN2CL_BASE));
    sse_regwl(RSSE_CDNN2_BASE_HI, ADDR_HI(pd_offset + RCDNN2CL_BASE));
    sse_regwl(RSSE_CDNN3_BASE_LO, ADDR_LO(pd_offset + RCDNN3CL_BASE));
    sse_regwl(RSSE_CDNN3_BASE_HI, ADDR_HI(pd_offset + RCDNN3CL_BASE));
}

// Push a task into the specific task queue
// this func will not touch task->state, caller need to maintain the state machine.
// must acquire xpu_tq->lock to call this funcion
int xpuhw_sse_enqueue_task_locked(struct xpu_task *xtask, int tq_id)
{
    DECLARE_PROFILER(PROF_sse_enqueue_task_locked);
    struct xpu_pd *xpd = NULL;
    int            i   = 0;

    if (xtask == NULL) {
        LOGW("xtask is null\n");
        return -XPUERR_INVALID_PARAM;
    }

    if (tq_id < 0 || tq_id >= KL1_SSE_TQ_COUNT) {
        LOGW("tq_id %d not in [0, %d)\n", tq_id, KL1_SSE_TQ_COUNT);
        return -XPUERR_INVALID_PARAM;
    }

    START_PROFILING(PROF_sse_enqueue_task_locked);
    xpd = xtask->xpd;

    // constraint: 1. write params before the control registers
    //             2. must write RSSE_TQ_PARAM(tq_id, 31) after all params
    //             3. must write RSSE_TQ_KSIZE(tq_id) after all control regs
    // note that we need to write params[0] into RSSE_TQ_PARAM(tq_id, 3)
    sse_regwl(RSSE_TQ_PARAM(tq_id, 1), xtask->nclusters);

    if (xtask->kernel.param_dword_size != 0)
        sse_regwq(RSSE_TQ_PARAM(tq_id, 2), makeu64(xtask->params[0], xtask->ncores));
    else
        sse_regwl(RSSE_TQ_PARAM(tq_id, 2), xtask->ncores);

    for (i = 1; i + 1 < xtask->kernel.param_dword_size; i += 2)
        sse_regwq(RSSE_TQ_PARAM(tq_id, 3 + i), makeu64(xtask->params[i + 1], xtask->params[i]));

    for (; i < xtask->kernel.param_dword_size; ++i)
        sse_regwl(RSSE_TQ_PARAM(tq_id, 3 + i), xtask->params[i]);

    // must write RSSE_TQ_PARAM(tq_id, 31) to enable all params
    if ((3 + i) != KL1_SSE_TQ_PARAM_COUNT)
        sse_regwl(RSSE_TQ_PARAM(tq_id, KL1_SSE_TQ_PARAM_COUNT - 1), 0);

    sse_regwq(RSSE_TQ_MAXID(tq_id), makeu64(xtask->token, xtask->nclusters - 1));
    sse_regwq(RSSE_TQ_KTYPE(tq_id),
              makeu64(ADDR_LO(xtask->kernel.code_addr), 0xffffff00 | xtask->type));

    //LOGL6("[xpu_%d tq_%u] before last sse rw, tk= %u\n",
    //        xpd->devfile_id, tq_id, xtask->token);

    sse_regwq(RSSE_TQ_KADDR_HI(tq_id), makeu64(align_up(xtask->kernel.code_byte_size, 64),
                                               ADDR_HI(xtask->kernel.code_addr)));

    // write an INT immediately
    sse_regwl(RSSE_TQ_KTYPE(tq_id), XTT_INTR);
    sse_regwl(RSSE_TQ_KSIZE(tq_id), 0);

    END_PROFILING(PROF_sse_enqueue_task_locked, xpd);
    return 0;
}

// Get token of the last dispatched task on given task queue
inline u32 xpuhw_sse_last_dispatched(struct xpu_pd *xpd, int tq_id)
{
    return xreadl(xpd->sse_base, 0x04000 + tq_id * 0x4);
}

// Get token of the last finished task on given task queue
inline u32 xpuhw_sse_last_finished(struct xpu_pd *xpd, int tq_id)
{
    return xreadl(xpd->sse_base, 0x04020 + tq_id * 0x4);
}

inline u64 xpuhw_sse_last_cycles(struct xpu_pd *xpd, int tq_id)
{
    return xreadq(xpd->sse_base, 0x12000 + tq_id * 0x8) & (~(0x3ull << 62));
}

inline u64 xpuhw_sse_rc_error(struct xpu_pd *xpd)
{
    u32 lo, hi;
    u64 err;
    lo  = xreadl(xpd->sse_base, 0x8);
    hi  = xreadl(xpd->sse_base, 0xC);
    err = ((u64)hi << 32) | lo;
    LOGI("hi=%x lo=%x err=%llx\n", hi, lo, err);
    return err;
}

// Read and clear the interrupt counter on given task queue
// this register will be set to 0 after every read
u32 xpuhw_sse_rc_tq_intr_cnt(struct xpu_pd *xpd, int tq_id)
{
    return reg_readl(xpd->sse_base + RSSE_TQ_INTR_CNT(tq_id));
}

void xpu_sse_print_errmsg(struct xpu_pd *xpd, u64 err)
{
    static const char *sse_errmsg[64] = {
        /* 00 ~ 07 */
        "xtq0: kernel addr not 64B aligned", "xtq1: kernel addr not 64B aligned",
        "xtq2: kernel addr not 64B aligned", "xtq3: kernel addr not 64B aligned",
        "xtq4: kernel addr not 64B aligned", "xtq5: kernel addr not 64B aligned",
        "xtq6: kernel addr not 64B aligned", "xtq7: kernel addr not 64B aligned",
        /* 08 ~ 15 */
        "xtq0: kernel size not 64B aligned", "xtq1: kernel size not 64B aligned",
        "xtq2: kernel size not 64B aligned", "xtq3: kernel size not 64B aligned",
        "xtq4: kernel size not 64B aligned", "xtq5: kernel size not 64B aligned",
        "xtq6: kernel size not 64B aligned", "xtq7: kernel size not 64B aligned",
        /* 16 ~ 23 */
        "xtq0: task fifo overflow", "xtq1: task fifo overflow", "xtq2: task fifo overflow",
        "xtq3: task fifo overflow", "xtq4: task fifo overflow", "xtq5: task fifo overflow",
        "xtq6: task fifo overflow", "xtq7: task fifo overflow",
        /* 24 ~ 31 */
        "xtq0: param fifo overflow", "xtq1: param fifo overflow", "xtq2: param fifo overflow",
        "xtq3: param fifo overflow", "xtq4: param fifo overflow", "xtq5: param fifo overflow",
        "xtq6: param fifo overflow", "xtq7: param fifo overflow",
        /* 32 ~ 39 */
        "xtq0: kernel size exceed 16K", "xtq1: kernel size exceed 16K",
        "xtq2: kernel size exceed 16K", "xtq3: kernel size exceed 16K",
        "xtq4: kernel size exceed 16K", "xtq5: kernel size exceed 16K",
        "xtq6: kernel size exceed 16K", "xtq7: kernel size exceed 16K",
        /* 40 ~ 47 */
        "xtq0: kernel size is zero", "xtq1: kernel size is zero", "xtq2: kernel size is zero",
        "xtq3: kernel size is zero", "xtq4: kernel size is zero", "xtq5: kernel size is zero",
        "xtq6: kernel size is zero", "xtq7: kernel size is zero",
        /* 48 ~ 55 */
        ": axi ch0 read slave exception, maybe ecc", ": axi ch0 read noc exception, wrong address",
        ": axi ch1 read slave exception, maybe ecc", ": axi ch1 read noc exception, wrong address",
        ": axi ch0 write slave exception, maybe ecc",
        ": axi ch0 write noc exception, wrong address",
        ": axi ch1 write slave exception, maybe ecc",
        ": axi ch1 write noc exception, wrong address",
        /* 56 ~ 63 */
        ": reserve", ": reserve", ": reserve", ": reserve", ": reserve", ": reserve", ": reserve",
        ": reserve"
    };
    int i;
    for (i = 0; i < 64; ++i) {
        if ((err >> i) & 0x1)
            LOGW("xpu%d %s\n", xpd->devfile_id, sse_errmsg[i]);
    }
}
