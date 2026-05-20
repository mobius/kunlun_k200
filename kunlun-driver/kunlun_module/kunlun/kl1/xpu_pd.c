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

#include <linux/bitmap.h>
#include <linux/hrtimer.h>
#include <linux/smp.h>
#include <linux/version.h>

#include "xpu_drv.h"

inline void __update_use_ratio(struct xpu_pd *xpd)
{
    unsigned long *use_ratio_bm = (unsigned long *)xpd->stat_use_ratio_bitmap;

    if (xpd_busy_tq_count(xpd) != 0) {
        bitmap_set(use_ratio_bm, xpd->stat_use_ratio_off, 1);
    } else {
        bitmap_clear(use_ratio_bm, xpd->stat_use_ratio_off, 1);
    }

    xpd->stat_use_ratio_off = (xpd->stat_use_ratio_off + 1) % USE_RATIO_PN;
}

inline void __clean_sessions(struct xpu_pd *xpd)
{
    struct xpu_session *xsess;
    struct xpu_session *safe_xsess;
    struct xpu_context *xctx;
    struct xpu_context *safe_xctx;
    unsigned long       flags;
    LIST_HEAD(xsess_free_list);
    LIST_HEAD(xctx_free_list);

    spin_lock_irqsave(&xpd->sessions_lock, flags);
    list_for_each_entry_safe(xsess, safe_xsess, &xpd->sessions, xpd_sessions_ent) {
        if (xsess->state != XSS_CLOSED)
            continue;

        if (atomic_read(&xsess->unfinished_cnt) != 0)
            continue;

        list_move_tail(&xsess->xpd_sessions_ent, &xsess_free_list);

        xctx = xsess->xctx;
        if (xctx) {
            int sess_cnt = atomic_sub_return(1, &xctx->sess_cnt);
            if (!sess_cnt) {
                list_move_tail(&xctx->xpd_contexts_ent, &xctx_free_list);
            }
        }
    }
    spin_unlock_irqrestore(&xpd->sessions_lock, flags);

    // XXX(miaotianxiang): 在spin_lock_irqsave(&xpd->sessions_lock,
    // flags)中xpu_release_mem可能导致hard lockup，考虑下面的执行流：
    //
    // CPU 0
    // -------------------------
    // ioctl_memory_free
    // - spin_lock(&mem->lock)
    //
    // <IRQ>
    // </IRQ>
    //
    // <BH timer>
    // __clean_sessions
    // - spin_lock_irqsave(&xpd->sessions_lock, flags);
    // - xpu_release_mem
    //   - spin_lock(&mem->lock)       <--- Oops!!! 关中断，且CPU 0永远自旋
    //
    // XXX(miaotianxiang):
    // list_del(&xsess->xctx->xpd_contexts_ent)要加锁保护，否则可能破坏链表数据integrity
    //
    // 以下list_del()均只操作栈上list，无需保护
    list_for_each_entry_safe(xsess, safe_xsess, &xsess_free_list, xpd_sessions_ent) {
        xpu_release_mem(xsess);
        list_del(&xsess->xpd_sessions_ent);
        kfree(xsess);
    }
    list_for_each_entry_safe(xctx, safe_xctx, &xctx_free_list, xpd_contexts_ent) {
        list_del(&xctx->xpd_contexts_ent);
        kfree(xctx);
    }
}

enum hrtimer_restart xpd_timer_handler(struct hrtimer *hrtimer)
{
    // @Do your work here.
    struct xpu_pd *xpd = container_of(hrtimer, struct xpu_pd, hrtimer);
    ++xpd->timer_counter;

    __update_use_ratio(xpd);

    // every 10s
    if (xpd->timer_counter % 1000 == 0) {
        __clean_sessions(xpd);
    }

    hrtimer_forward_now(&xpd->hrtimer, xpd->kt_periode);

    return HRTIMER_RESTART;
}

static void
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
xpd_ktimer_handler(unsigned long data)
{
    struct xpu_pd *xpd = (struct xpu_pd *)data;
#else
xpd_ktimer_handler(struct timer_list *ktimer)
{
    struct xpu_pd *xpd = container_of(ktimer, struct xpu_pd, ktimer);
#endif

    ++xpd->timer_counter;

    __update_use_ratio(xpd);

    if (xpd->timer_counter % 200 == 0)
        __clean_sessions(xpd);

    if (xpd->id == 0 && xpd->timer_counter % 100 == 0) {
        spin_lock_bh(&xpd->timer_worklock);
        if (!xpu_device_disabled_or_in_reset(xpd->xdev)) {
            xpu_read_temp_sensor(xpd->xdev);
            xpu_read_frequency(xpd->xdev);
            xpu_read_power(xpd->xdev);
            xpu_read_hbm_temp(xpd->xdev);
        }
        spin_unlock_bh(&xpd->timer_worklock);
    }

    xpd_init_ktimer(xpd);
}

void xpd_init_ktimer(struct xpu_pd *xpd)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
    init_timer(&xpd->ktimer);
    xpd->ktimer.function = xpd_ktimer_handler;
    xpd->ktimer.data     = (unsigned long)xpd;
#else
    timer_setup(&xpd->ktimer, xpd_ktimer_handler, 0);
#endif
    xpd->last_ktimer_jiffies = jiffies;
    xpd->ktimer.expires      = xpd->last_ktimer_jiffies + PDTIMER_FREQ_JIFFIES;
    add_timer(&xpd->ktimer);
}

void xpd_del_ktimer(struct xpu_pd *xpd)
{
    del_timer(&xpd->ktimer);
}

int xpd_busy_tq_count(struct xpu_pd *xpd)
{
    int i;
    int busy;

    busy = 0;
    for (i = 0; i < KL1_SSE_TQ_COUNT; ++i) {
        struct xpu_tq *xtq = &xpd->xtqs[i];
        if (xtq->cnt_all != 0)
            ++busy;
    }

    return busy;
}

void xpd_clean_etasks_locked(struct xpu_pd *xpd)
{
    struct xpu_task *xtask = NULL;
    struct xpu_task *safe  = NULL;

    xpd->sse_errsv = 0;
    list_for_each_entry_safe(xtask, safe, &xpd->etasks, tq_tasks_ent) {
        list_del(&xtask->tq_tasks_ent);
        kfree(xtask);
    }
}

inline void xpd_clean_etasks_ifneed(struct xpu_pd *xpd)
{
    if (xpd->need_clean_etasks) {
        unsigned long flags;
        spin_lock_irqsave(&xpd->etasks_lock, flags);
        if (xpd->need_clean_etasks)
            xpd_clean_etasks_locked(xpd);
        xpd->need_clean_etasks = 0;
        spin_unlock_irqrestore(&xpd->etasks_lock, flags);
    }
}
