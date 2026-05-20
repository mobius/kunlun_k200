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
// xpu_session.c - XPU Session manager
//
#define __FILENAME__ "xpu_session.c"

#include <linux/file.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/types.h>
#include "xpu_drv.h"

//
// Layout of session->tasks
//
// |--- XTS_FINISHED tasks ---|--- XTS_RUNNING tasks ---|--- XTS_PENDING tasks --|
//
// - inserting is performed at the tail of pending tasks
// - scheduling is performed at the head of pending tasks
// - intr handling is performed at the head of running tasks
// - task clearing is performed at the head of finished tasks

void session_init(struct xpu_pd *xpd)
{
    INIT_LIST_HEAD(&xpd->contexts);
    INIT_LIST_HEAD(&xpd->sessions);
    spin_lock_init(&xpd->sessions_lock);
    atomic_set(&xpd->session_id_counter, 100);
}

struct xpu_session *session_create(struct xpu_pd *xpd, struct file *file)
{
    struct xpu_session *sess    = NULL;
    struct xpu_context *ctx_new = NULL;
    struct xpu_context *ctx_ite = NULL;
    int                 pid     = (int)task_tgid_nr(current);
    int                 err;
    unsigned long       flags;

    sess = kzalloc(sizeof(*sess), GFP_KERNEL);
    if (sess == NULL) {
        LOGW("vmalloc fail\n");
        return NULL;
    }

    sess->id    = atomic_add_return(1, &xpd->session_id_counter);
    sess->state = XSS_NORMAL;
    mutex_init(&sess->lock);
    sess->file = file;
    sess->xpd  = xpd;
    INIT_LIST_HEAD(&sess->tasks);
    atomic_set(&sess->unfinished_cnt, 0);

    spin_lock_irqsave(&xpd->sessions_lock, flags);

    if (xpu_device_disabled_or_in_reset(xpd->xdev)) {
        LOGI("wait for in_reset\n");
        err = xpu_poll_cond_timeout_spinlocked_irqsave(!xpu_device_disabled_or_in_reset(xpd->xdev),
                                                       100 * 1000, 20 * 1000 * 1000,
                                                       &xpd->sessions_lock, flags);
        if (err) {
            spin_unlock_irqrestore(&xpd->sessions_lock, flags);
            kfree(sess);
            LOGW("[xpu%d] open xpu fail, reset timeout\n", xpd->devfile_id);
            return NULL;
        }
    }

    list_add_tail(&sess->xpd_sessions_ent, &xpd->sessions);
    // try to find an existing context
    list_for_each_entry(ctx_ite, &xpd->contexts, xpd_contexts_ent) {
        if (ctx_ite->pid == pid)
            break;
    }
    if (&ctx_ite->xpd_contexts_ent == &xpd->contexts) {
        // first open
        ctx_new = kzalloc(sizeof(struct xpu_context), GFP_ATOMIC);
        if (ctx_new == NULL) {
            kfree(sess);
            return NULL;
        }
        ctx_new->pid = pid;
        strncpy(ctx_new->comm, current->comm, TASK_COMM_LEN);
        atomic_set(&ctx_new->sess_cnt, 1);
        atomic_set(&ctx_new->cache_mem_page_used, 0);
        atomic_set(&ctx_new->main_page_used, 0);
        list_add_tail(&ctx_new->xpd_contexts_ent, &xpd->contexts);
        sess->xctx = ctx_new;
    } else {
        // already opened
        atomic_add(1, &ctx_ite->sess_cnt);
        sess->xctx = ctx_ite;
    }
    spin_unlock_irqrestore(&xpd->sessions_lock, flags);

    return sess;
}

// FIXME: be careful about destroy without wait
void session_destroy(struct xpu_pd *xpd, struct xpu_session *sess)
{
    if (sess == NULL)
        return;

    // CLOSED session will be freed by a regular timer
    sess->state = XSS_CLOSED;
}

// Bind xsess to a task queue if not and insert the xtask into the session and the xtq.
// note this func does not acquire xsess->lock and xtq->lock at the same time, so it is
// NOT THEAD SAFE if two threads share the same session.
//
static inline int __session_add_tasks_locked(struct xpu_session *xsess, struct xpu_task **xtasks,
                                             int cnt)
{
    int ret;

    // add to xpu_tq's task fifo queue
    ret = xtq_add_tasks(xsess->xtq, xtasks, cnt);
    if (ret < 0)
        return ret;

    atomic_add(cnt, &xsess->unfinished_cnt);

    return XPU_SUCCESS;
}

int session_launch_tasks(struct xpu_session *xsess, struct xpu_task **xtasks, int cnt)
{
    int ret = 0;
    DECLARE_PROFILER(PROF_sess_launch_task);

    xpu_sched_bind_sess(xsess);

    START_PROFILING(PROF_sess_launch_task);

    //mutex_lock(&xsess->lock);
    ret = __session_add_tasks_locked(xsess, xtasks, cnt);
    //mutex_unlock(&xsess->lock);

    if (ret < 0)
        return ret;

    // schedule a tasklet to try to dispatch tasks from xtq's fifo into xpu
    if (xsess->xpd->state == XPDS_RUNNING)
        tasklet_xtq_dispatch((unsigned long)xsess->xtq);

    END_PROFILING(PROF_sess_launch_task, xsess->xpd);
    return XPU_SUCCESS;
}

int sessions_prepare_reset(struct xpu_device *xdev)
{
    int           i;
    unsigned long flag;

    for (i = 0; i < xdev->pd_num; ++i) {
        struct xpu_pd      *xpd;
        struct xpu_session *xsess;

        xpd = &xdev->xpd[i];
        spin_lock_irqsave(&xpd->sessions_lock, flag);
        list_for_each_entry(xsess, &xpd->sessions, xpd_sessions_ent) {
            if (xsess->state != XSS_NORMAL)
                continue;

            if (xsess->state == XSS_NORMAL) {
                xsess->state = XSS_ERROR;
                xsess->errno = XPUERR_PEERRESET;
            }
        }
        spin_unlock_irqrestore(&xpd->sessions_lock, flag);
    }

    return 0;
}
