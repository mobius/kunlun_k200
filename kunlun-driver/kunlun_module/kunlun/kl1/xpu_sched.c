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
// xpu_sched.c - XPU Task Scheduler
//
#define __FILENAME__ "xpu_sched.c"

#include <linux/kthread.h>
#include <linux/sched.h>
#include "xpu_drv.h"

// TODO: maybe not necessary
// Schedule in a RR way of all the sessions bound to the given xpu_tq
// @data: a pointer of struct xpu_tq
//
void xpu_tasklet_sched_tq(unsigned long data)
{
}

// Schedule in a RR way of all the sessions in the device
// @data: a pointer of struct xpu_device
//
void xpu_tasklet_sched_dev(unsigned long data)
{
}

// Schedule as many pending tasks as possible from the given session
// into the bound xpu_tq
// @data: a pointer of struct xpu_session
//
void xpu_sched_session(struct xpu_session *xsess)
{
}

void xpu_sched_tq_init(struct xpu_tq *xtq)
{
}

void xpu_sched_dev_init(struct xpu_device *xdev)
{
}

void xpu_sched_bind_sess_to_tq(struct xpu_session *xsess, u32 tq_id)
{
    if (tq_id >= KL1_SSE_TQ_COUNT)
        return;

    mutex_lock(&xsess->lock);
    xsess->xtq = &xsess->xpd->xtqs[tq_id];
    mutex_unlock(&xsess->lock);
}

void xpu_sched_bind_sess(struct xpu_session *xsess)
{
    // FIXME: rebind if xsess->unfinished_cnt == 0
    DECLARE_PROFILER(PROF_sched_bind_sess);
    struct xpu_pd *xpd = xsess->xpd;

    mutex_lock(&xsess->lock);

    if (xsess->binding_fixed) {
        mutex_unlock(&xsess->lock);

        return;
    }

    START_PROFILING(PROF_sched_bind_sess);
    if (xsess->xtq != NULL) {
        mutex_unlock(&xsess->lock);
        return;
    }

    xsess->xtq = &xpd->xtqs[xsess->id % 8];
    mutex_unlock(&xsess->lock);

    LOGL3("[xpu_%d] bind sess_%d to tq_%d\n", xsess->xpd->devfile_id, xsess->id, xsess->xtq->id);
    END_PROFILING(PROF_sched_bind_sess, xpd);
}
