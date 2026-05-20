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
// xpu_tq.c - XPU Task Queue manager
// Implement hardware interface for xtq mangement
//
#define __FILENAME__ "xpu_tq.c"

#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/timex.h>
#include <linux/types.h>

#include "xpu_drv.h"
#include "xpu_hw.h"

void xtq_init(struct xpu_pd *xpd)
{
    int i;
    for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
        struct xpu_tq *xtq = &xpd->xtqs[i];
        spin_lock_init(&xtq->lock);
        xtq->id                     = i;
        xtq->xpd                    = xpd;
        xtq->state                  = XTQS_NORMAL;
        xtq->capacity               = KL1_SSE_TQ_CAPACITY;
        xtq->cnt_running            = 0;
        xtq->cnt_all                = 0;
        xtq->cnt_successive_kernels = 0;
        INIT_LIST_HEAD(&xtq->tasks);
        xtq->last_running = &xtq->tasks;
        tasklet_init(&xtq->tasklet_dispatch, tasklet_xtq_dispatch, (unsigned long)xtq);
        atomic_set(&xtq->dispatch_pending, 0);
        xtq->nwi_for_pending = (struct xpu_task *)kmalloc(sizeof(struct xpu_task), GFP_ATOMIC);

        xtq->st_all_intr_issued  = 0;
        xtq->st_all_intr_handled = 0;
        xtq->st_all_msi_received = 0;
        xtq->st_all_task_issued  = 0;
        xtq->enable              = 1;
    }
}

int xtq_free_count(struct xpu_tq *tq)
{
    return tq->capacity - tq->cnt_running;
}

static int __xtq_add_tasks_locked(struct xpu_tq *xtq, struct xpu_task **xtasks, int cnt)
{
    int i = 0;

    if (cnt <= 0)
        return -EINVAL;

    switch (xtq->xpd->state) {
    case XPDS_RUNNING:
    case XPDS_PAUSING:
    case XPDS_PAUSED:
        break;
    case XPDS_LOWPOWER:
        // cdnn clock up
        break;
    case XPDS_UNUSED:
    case XPDS_ERROR:
        return -XPUERR_ABNORMAL;
    }

    if (xtq->state == XTQS_HANGUP) {
        LOGW("xpu%d: tq%u hang up, fail to add tasks\n", xtq->xpd->devfile_id, xtq->id);
        return -XPUERR_ABNORMAL;
    }

    for (i = 0; i < cnt; ++i) {
        struct xpu_task *xtask = xtasks[i];
        xtask->xtq_id          = xtq->id;
        list_add_tail(&xtask->tq_tasks_ent, &xtq->tasks);
        ++xtq->cnt_all;
    }

    return 0;
}

// Insert a task into the task queue, waiting for scheduling
//
int xtq_add_tasks(struct xpu_tq *xtq, struct xpu_task **xtasks, int cnt)
{
    DECLARE_PROFILER(PROF_xtq_add_pending_task);
    unsigned long flags;
    int           err = 0;

    START_PROFILING(PROF_xtq_add_pending_task);

    spin_lock_irqsave(&xtq->lock, flags);
    err = __xtq_add_tasks_locked(xtq, xtasks, cnt);
    spin_unlock_irqrestore(&xtq->lock, flags);

    END_PROFILING(PROF_xtq_add_pending_task, xtq->xpd);

    return err;
}

// Push as many tasks as possible from xtq pending list into XPU sse
//
void tasklet_xtq_dispatch(unsigned long data)
{
    DECLARE_PROFILER(PROF_xtq_dispatch);
    struct xpu_tq *xtq   = (struct xpu_tq *)data;
    unsigned long  flags = 0;

    //LOGL6("[xpu_%d tq_%u] start dispatcher\n",
    //        xtq->xpd->devfile_id, xtq->id);

    //LOGI("start dispatcher %llu\n", get_cycles());

    if (xtq->cnt_running >= xtq->capacity) {
        LOGL2("[xpu_%d tq_%u] no space in hwq, running=%u capacity=%u\n", xtq->xpd->devfile_id,
              xtq->id, xtq->cnt_running, xtq->capacity);
        return;
    }

    // no pending tasks need to dispatch
    // FIXME: is this safe?
    if (xtq->cnt_running == xtq->cnt_all) {
        LOGL2("[xpu_%d tq_%u] no pending, running=%u all=%u\n", xtq->xpd->devfile_id, xtq->id,
              xtq->cnt_running, xtq->cnt_all);
        return;
    }

    START_PROFILING(PROF_xtq_dispatch);
    spin_lock_irqsave(&xtq->lock, flags);

    if (unlikely(xtq->xpd->state != XPDS_RUNNING)) {
        LOGW("[xpu_%d tq_%u] dispatch cancelled as XPD state is %s\n", xtq->xpd->devfile_id,
             xtq->id, xpd_state_str(xtq->xpd->state));
        goto out;
    }

    if (xpu_device_disabled_or_in_reset(xtq->xpd->xdev)) {
        LOGW("Error schedule tasks to hwq, disabled or in_reset\n");
        goto out;
    }

    // dispatch the first pending task into xpu if
    // 1. there is some available hw sse tq slots
    // 2. there is some pending tasks
    while ((xtq->cnt_running < xtq->capacity) && !list_is_last(xtq->last_running, &xtq->tasks)) {
        struct xpu_task *xtask = list_entry(xtq->last_running->next, struct xpu_task, tq_tasks_ent);
        ++xtq->cnt_running;
        ++xtq->st_all_task_issued;

        xtask->state = XTS_RUNNING;

        xtq->last_running = xtq->last_running->next;

        // write to hw
        xpuhw_sse_enqueue_task_locked(xtask, xtq->id);

        LOGL2("[xpu_%d tq_%u] write to sse token= %u cnt_running=%d last_running=%px\n",
              xtq->xpd->devfile_id, xtq->id, xtask->token, xtq->cnt_running, xtq->last_running);
    }

out:
    spin_unlock_irqrestore(&xtq->lock, flags);
    END_PROFILING(PROF_xtq_dispatch, xtq->xpd);

    //LOGI("  end dispatcher %llu\n", get_cycles());
}

// tasklet_xtq_finish - finish tasks in xpu task queue
// runs in ISR context
//
// Finish the first several INT tasks and all the normal tasks between on the given task queue.
// The count of INTs needed to be finished is indicated by xtq->finish_cnt, which is set by
// the top half of intr handler.
void ISR_xtq_on_finish(struct xpu_tq *xtq, int cnt_finish)
{
    DECLARE_PROFILER(PROF_xtq_finish);
    struct xpu_task *xtask     = NULL;
    struct xpu_task *safe      = NULL;
    int              countdown = 0;
    unsigned long    flags     = 0;

    START_PROFILING(PROF_xtq_finish);

    LOGL2("[xpu_%d tq_%u] start finisher\n", xtq->xpd->devfile_id, xtq->id);
    //LOGL6("start finisher\n");
    ++xtq->st_all_msi_received;

    if (cnt_finish == 0)
        return;

    spin_lock_irqsave(&xtq->lock, flags);
    if (xpu_device_disabled_or_in_reset(xtq->xpd->xdev)) {
        spin_unlock_irqrestore(&xtq->lock, flags);
        return;
    }

    countdown = cnt_finish;
    LOGL2("[xpu_%d tq_%u] intr_to_handle=%d\n", xtq->xpd->devfile_id, xtq->id, cnt_finish);

    // mark all tasks before the first INT as finished, the tasks may comes from
    // different sessions, but only one session will be notified of the interrupt.
    list_for_each_entry_safe(xtask, safe, &xtq->tasks, tq_tasks_ent) {
        int ufcnt;

        if (xtask->state == XTS_PENDING) {
            struct xpu_task *lrt = NULL;
            if (xtq->last_running != &xtq->tasks) {
                lrt = list_entry(xtq->last_running, struct xpu_task, tq_tasks_ent);
            }

            LOGW("[xpu_%d tq_%u] finish pending task_%u cnt_fnsh=%d "
                 "cntdwn=%d cnt_all=%u cnt_running=%u lrt=%u\n",
                 xtq->xpd->devfile_id, xtq->id, xtask->token, cnt_finish, countdown, xtq->cnt_all,
                 xtq->cnt_running, ((lrt == NULL) ? 0 : lrt->token));
        }

        list_del(&xtask->tq_tasks_ent);
        xtask->state = XTS_FINISHED;
        --xtq->cnt_running;
        --xtq->cnt_all;

        LOGL2("[xpu_%d tq_%u] finish xtask.token=%u ty=%d running=%u all=%u\n",
              xtq->xpd->devfile_id, xtq->id, xtask->token, xtask->type, xtq->cnt_running,
              xtq->cnt_all);

        if (&xtask->tq_tasks_ent == xtq->last_running)
            xtq->last_running = &xtq->tasks;

        if (xtask->xsess) {
            ufcnt = atomic_sub_return(1, &xtask->xsess->unfinished_cnt);
            LOGL2("[xpu_%d sess_%u] unfinished_count= %d\n", xtq->xpd->devfile_id, xtask->xsess->id,
                  ufcnt);
        }

        if (xtask->free_by_tq) {
            LOGL2("[xpu_%d tq_%u] auto free task.tk= %u\n", xtq->xpd->devfile_id, xtq->id,
                  xtask->token);
            kfree(xtask);
        }

        --countdown;
        if (countdown == 0)
            break;
    }

    if (xtq->xpd->state == XPDS_PAUSING) {
        if (xtq->cnt_running == 0) {
            LOGI("[xpu_%d tq_%d] paused\n", xtq->xpd->devfile_id, xtq->id);
            __atomic_or((0x1 << xtq->id), &xtq->xpd->xtqs_pending_finish_flag);
            if (atomic_read(&xtq->xpd->xtqs_pending_finish_flag) ==
                ((0x1 << KL1_SSE_TQ_COUNT) - 1)) {
                // all xtqs are paused
                xpd_state_to_paused(xtq->xpd);
            }
        }
    }

    spin_unlock_irqrestore(&xtq->lock, flags);

    if (countdown != 0) {
        LOGW("[xpu_%d tq_%u] unexpected tq is empty but cnt_finish=%d countdown=%d\n",
             xtq->xpd->devfile_id, xtq->id, cnt_finish, countdown);
    }

    // after finishing, there is still pending tasks, schedule a scheduler routine
    if ((xtq->cnt_running < xtq->cnt_all) && (xtq->xpd->state == XPDS_RUNNING)) {
        //tasklet_schedule(&xtq->tasklet_dispatch);
        tasklet_xtq_dispatch((unsigned long)xtq);
    }

    LOGL2("[xpu_%d tq_%u] exit finisher\n", xtq->xpd->devfile_id, xtq->id);

    END_PROFILING(PROF_xtq_finish, xtq->xpd);
}

int xtq_save_etask_locked(struct xpu_tq *xtq, struct xpu_task *xtask)
{
    struct xpu_pd   *xpd;
    struct xpu_task *etask;
    unsigned long    flags;

    if (!xtq || !xtask)
        return -EINVAL;

    xpd = xtq->xpd;

    strncpy(xtq->error_xtask_name, xtask->kernel_name, XPU_MAX_STRLEN);

    etask = kzalloc(sizeof(*etask), GFP_ATOMIC);
    if (!etask) {
        LOGW("xpu%d: malloc for saved err task fail\n", xpd->devfile_id);
        return -ENOMEM;
    }

    memcpy(etask, xtask, sizeof(*etask));

    spin_lock_irqsave(&xpd->etasks_lock, flags);
    list_add_tail(&etask->tq_tasks_ent, &xpd->etasks);
    spin_unlock_irqrestore(&xpd->etasks_lock, flags);

    return 0;
}

// tasklet_xtq_exception - handle exception on this queue
// runs in ISR context
void ISR_xpd_on_exception(struct xpu_pd *xpd)
{
    int i;

    xpd_state_to_error(xpd, XPUERR_KEXCEPTION);

    // hang all the SSE tqs on this PD
    for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
        struct xpu_tq   *xtq = &xpd->xtqs[i];
        struct xpu_task *xtask, *safe;
        unsigned long    flags = 0;

        spin_lock_irqsave(&xtq->lock, flags);

        // mark this TQ as hanged up so there will be no further launch on this TQ
        xtq->state = XTQS_HANGUP;
        list_for_each_entry_safe(xtask, safe, &xtq->tasks, tq_tasks_ent) {
            int j = 0;
            LOGL2("[xpu_%d tq_%u] task token=%u state=%d\n", xpd->devfile_id, i, xtask->token,
                  xtask->state);

            // find if current task triggers the exception
            for (j = 0; j < XPD_CLUSTER_COUNT + XPD_CDNN_COUNT; ++j) {
                int  k;
                char xsess_comm[TASK_COMM_LEN] = { 0 };

                if (xtask->token != xpd->cu_error[j].token)
                    continue;

                if (xtask->xsess) {
                    strncpy(xsess_comm, xtask->xsess->xctx->comm, TASK_COMM_LEN);
                }

                LOGI("xpu%d: err task, sess_id=%u comm=%s tq=%u "
                     "tk=%u .name=%s .ncl=%u .nco=%u .addr=0x%llx .ksz=0x%x\n",
                     xpd->devfile_id, xtask->xsess_id, xsess_comm, xtq->id, xtask->token,
                     xtask->kernel_name, xtask->nclusters, xtask->ncores, xtask->kernel.code_addr,
                     xtask->kernel.code_byte_size);
                for (k = 0; k < xtask->kernel.param_dword_size; ++k)
                    LOGI("xpu%d: ..param[%d]= 0x%x\n", xpd->devfile_id, k, xtask->params[k]);

                xtq_save_etask_locked(xtq, xtask);

                break;
            }

            if (xtask->xsess->errno == 0) {
                xtask->xsess->state = XSS_ERROR;
                xtask->xsess->errno = XPUERR_KEXCEPTION;
            }
        }

        spin_unlock_irqrestore(&xtq->lock, flags);
    }
}

void isr_xpd_on_sse_exception(struct xpu_pd *xpd)
{
    int i;
    xpd_state_to_error(xpd, XPUERR_HWEXCEPTION);

    xpd->sse_errsv = xpuhw_sse_rc_error(xpd);
    LOGW("xpu%d: sse exception err= %llx\n", xpd->devfile_id, xpd->sse_errsv);

    for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
        struct xpu_tq   *xtq = &xpd->xtqs[i];
        struct xpu_task *xtask, *safe;
        unsigned long    flags = 0;

        spin_lock_irqsave(&xtq->lock, flags);
        xtq->state = XTQS_HANGUP;

        list_for_each_entry_safe(xtask, safe, &xtq->tasks, tq_tasks_ent) {
            LOGL2("[xpu_%d tq_%u] task token=%u state=%d\n", xpd->devfile_id, i, xtask->token,
                  xtask->state);
            if (xtask->xsess->errno == 0) {
                xtask->xsess->state = XSS_ERROR;
                xtask->xsess->errno = XPUERR_HWEXCEPTION;
            }
        }

        spin_unlock_irqrestore(&xtq->lock, flags);
    }
    xpu_sse_print_errmsg(xpd, xpd->sse_errsv);
}
