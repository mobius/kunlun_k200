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

#include "kl2/kl2.h"

#include <linux/errno.h>
#include <linux/kref.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/pid_namespace.h>

void kl2_mm_malloc_update_stat_cb(u64 addr, u64 size, int kind, void *owner, struct kl_memory *mem)
{
    struct kl2_session        *sess           = owner;
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    u64                        page_needed    = (size + mem->page_size - 1) >> mem->page_bits;

    spin_lock(&sess->uproc->lock);
    atomic64_add(page_needed, &sess->uproc->mem_used_pgcnt[mem->kind]);
    spin_unlock(&sess->uproc->lock);
    KL2_LOGD("kind= %d, page_needed= %llu, mem_used_pgcnt= %llu\n", kind, page_needed,
             atomic64_read(&sess->uproc->mem_used_pgcnt[mem->kind]));
}

void kl2_mm_free_update_stat_cb(u64 addr, u64 size, int kind, void *owner, struct kl_memory *mem)
{
    struct kl2_session        *sess           = owner;
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    u64                        page_freed     = (size + mem->page_size - 1) >> mem->page_bits;

    spin_lock(&sess->uproc->lock);
    atomic64_sub(page_freed, &sess->uproc->mem_used_pgcnt[mem->kind]);
    kl_cxpu_instance_sub_mem_used(sess->uproc->cxpu_instance, mem->kind, page_freed);
    spin_unlock(&sess->uproc->lock);
    KL2_LOGD("kind= %d, page_freed= %llu, mem_used_pgcnt= %llu\n", kind, page_freed,
             atomic64_read(&sess->uproc->mem_used_pgcnt[mem->kind]));
}

static int kl2_session_bind_hwq_locked(struct kl2_session *sess)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    int                        i, min_hwq_id, count;
    u32                        min;

    if (sess->hwq)
        return 0;

    mutex_lock(&kl2_dev->hwq_binding_lock);
    min        = UINT_MAX;
    min_hwq_id = 0;

    count = KL2_HWQ_CNT;
    for (i = 0; i < count; ++i) {
        // this hwq is disabled
        if (test_bit(i, &kl2_dev->hwq_bitmap))
            continue;

        if (kl2_dev->hwq[i].session_cnt < min) {
            min        = kl2_dev->hwq[i].session_cnt;
            min_hwq_id = i;
        }
    }
    sess->hwq = &kl2_dev->hwq[min_hwq_id];

    list_add_tail(&sess->hwq_node, &sess->hwq->session_list);
    ++sess->hwq->session_cnt;
    mutex_unlock(&kl2_dev->hwq_binding_lock);

    KL2_LOGD("bind pid_%d sess_%d to hwq_%d\n", sess->uproc->pid, sess->id, sess->hwq->id);
    return 0;
}

int kl2_session_bind_hwq(struct kl2_session *sess)
{
    int err;

    if (sess->hwq)
        return 0;

    mutex_lock(&sess->lock);
    err = kl2_session_bind_hwq_locked(sess);
    mutex_unlock(&sess->lock);

    return err;
}

static int kl2_session_release_hwq_locked(struct kl2_session *sess)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;

    if (!sess->hwq)
        return 0;

    mutex_lock(&kl2_dev->hwq_binding_lock);
    list_del(&sess->hwq_node);
    --sess->hwq->session_cnt;
    mutex_unlock(&kl2_dev->hwq_binding_lock);

    KL2_LOGD("unbind pid_%d sess_%d to hwq_%d\n", sess->uproc->pid, sess->id, sess->hwq->id);
    return 0;
}

static int kl2_create_userprocess_locked(struct kl2_device       *kl2_dev,
                                         struct kl2_userprocess **uproc_ptr)
{
    struct kl2_userprocess *uproc;
    int                     pid    = task_tgid_nr(current);
    pid_t                   ns_pid = task_tgid_nr_ns(current, task_active_pid_ns(current));
    int err, i __maybe_unused;

    uproc = kzalloc(sizeof(*uproc), GFP_KERNEL);
    if (!uproc)
        return -ENOMEM;

    err = idr_alloc(&kl2_dev->uproc_idr, uproc, pid, pid + 1, GFP_KERNEL);
    if (err < 0) {
        KL2_LOGW("idr_alloc= %d, pid= %d\n", err, pid);
        goto err_out;
    }

    uproc->kl2_dev = kl2_dev;
    uproc->pid     = pid;
    uproc->ns_pid  = ns_pid;
    // 某些低版本内核中PIDTYPE_TGID未定义，故改用PIDTYPE_PID，在此作用相同
    //uproc->task_pid = get_task_pid(current, PIDTYPE_TGID);
    uproc->task_pid = get_task_pid(current->group_leader, PIDTYPE_PID);
    // 使用主线程comm信息，与pid对应
    get_task_comm(uproc->comm, current->group_leader);
    xref_init(&uproc->xref);
    spin_lock_init(&uproc->lock);
    rwlock_init(&uproc->sg_minfo_lock);
    uproc->sg_minfo_rb = RB_ROOT;

    INIT_LIST_HEAD(&uproc->p2p_list);
    INIT_LIST_HEAD(&uproc->event_list);
    atomic_set(&uproc->state, KL2_UPROC_NORMAL);

    uproc->cxpu_instance =
            kl_cxpu_get_instance_by_token_locked(&kl2_dev->kdev->cxpu, (u64)current->cgroups);

    *uproc_ptr = uproc;
    KL2_LOGD("kl2_create_userprocess_locked, pid= %d\n", pid);
    return 0;

err_out:
    kfree(uproc);
    return err;
}

static void kl2_destroy_uproc_event_list(struct kl2_device *kl2_dev, struct kl2_userprocess *uproc)
{
    struct kl2_event *evnt, *safe;

    mutex_lock(&kl2_dev->event_idr_lock);
    list_for_each_entry_safe(evnt, safe, &uproc->event_list, uproc_node) {
        xref_put(&evnt->xref, kl2_destroy_event_locked_ref);
    }
    mutex_unlock(&kl2_dev->event_idr_lock);
}

static void kl2_destroy_uproc_p2p_list(struct kl2_device *kl2_dev, struct kl2_userprocess *uproc)
{
    struct kl2_p2p_info *p2p_info, *safe;

    write_lock(&uproc->sg_minfo_lock);
    list_for_each_entry_safe(p2p_info, safe, &uproc->p2p_list, uproc_node) {
        list_del(&p2p_info->uproc_node);
        kfree(p2p_info);
    }
    write_unlock(&uproc->sg_minfo_lock);
}

static void kl2_destroy_userprocess_ref(struct kref *kref)
{
    struct kl2_userprocess    *uproc;
    struct kl2_device *kl2_dev __maybe_unused;
    struct kl2_sg_minfo       *minfo;
    struct rb_node            *node = NULL;

    uproc   = container_of((struct xref *)kref, struct kl2_userprocess, xref);
    kl2_dev = uproc->kl2_dev;
    KL2_LOGD("kl2_destroy_userprocess_ref, pid= %d\n", uproc->pid);

    // 删除idr应为第一步，此后uproc全局不可见
    mutex_lock(&kl2_dev->uproc_session_lock);
    idr_remove(&kl2_dev->uproc_idr, uproc->pid);
    mutex_unlock(&kl2_dev->uproc_session_lock);

    write_lock(&uproc->sg_minfo_lock);
    while ((node = rb_first(&uproc->sg_minfo_rb))) {
        minfo = container_of(node, struct kl2_sg_minfo, uproc_node);
        kl2_minfo_rb_erase(&uproc->sg_minfo_rb, minfo);
        write_unlock(&uproc->sg_minfo_lock);

        xref_put(&minfo->xref, kl2_destroy_minfo_ref);
        write_lock(&uproc->sg_minfo_lock);
    }
    write_unlock(&uproc->sg_minfo_lock);

    kl2_destroy_uproc_event_list(kl2_dev, uproc);
    kl2_destroy_uproc_p2p_list(kl2_dev, uproc);

    put_pid(uproc->task_pid);

    kl_cxpu_put_instance_locked(&kl2_dev->kdev->cxpu, uproc->cxpu_instance);

    kfree(uproc);
}

int kl2_create_session(struct kl2_device *kl2_dev, struct kl2_session **sess_ptr)
{
    struct kl2_session     *sess;
    struct kl2_userprocess *uproc;
    int                     pid = task_tgid_nr(current);
    int                     err;

    sess = kzalloc(sizeof(*sess), GFP_KERNEL);
    if (!sess)
        return -ENOMEM;

    mutex_lock(&kl2_dev->uproc_session_lock);
    sess->kl2_dev = kl2_dev;
    sess->id      = idr_alloc(&kl2_dev->session_idr, sess, 1, INT_MAX, GFP_KERNEL);
    if (sess->id < 0) {
        err = sess->id;
        goto err_unlock;
    }
    mutex_init(&sess->lock);

    // get assosiated struct kl2_userprocess
    uproc = idr_find(&kl2_dev->uproc_idr, pid);
    if (!uproc) {
        err = kl2_create_userprocess_locked(kl2_dev, &uproc);
        if (err)
            goto err_idr_remove;
    } else {
        if (atomic_read(&uproc->state) != KL2_UPROC_NORMAL) {
            err = -XPUERR_EVENTWAIT;
            goto err_idr_remove;
        }
        xref_get(&uproc->xref);
    }

    sess->uproc = uproc;
    atomic_set(&sess->state, KL2_SESS_NORMAL);
    atomic_set(&sess->taint_state, KL2_SESS_NORMAL);
    sess->errno = 0;

    atomic_set(&sess->unfinished_cnt, 0);
    xref_init(&sess->xref);

    init_waitqueue_head(&sess->wait_queue);
    mutex_unlock(&kl2_dev->uproc_session_lock);

    *sess_ptr = sess;
    KL2_LOGD("kl2_create_session, pid= %d, sess->id= %d\n", pid, sess->id);
    return 0;

err_idr_remove:
    idr_remove(&kl2_dev->session_idr, sess->id);

err_unlock:
    mutex_unlock(&kl2_dev->uproc_session_lock);

    kfree(sess);
    return err;
}

void kl2_destroy_session_ref(struct kref *kref)
{
    struct kl2_session        *sess;
    struct kl2_device *kl2_dev __maybe_unused;

    sess    = container_of((struct xref *)kref, struct kl2_session, xref);
    kl2_dev = sess->kl2_dev;
    KL2_LOGD("kl2_destroy_session_ref, sess->id= %d\n", sess->id);

    if (sess->dbgm) {
        kl2_dbgm_disable(sess);
    }

    // 删除idr应为第一步，此后sess全局不可见
    mutex_lock(&kl2_dev->uproc_session_lock);
    idr_remove(&kl2_dev->session_idr, sess->id);
    mutex_unlock(&kl2_dev->uproc_session_lock);

    // XXX(miaotianxiang):
    // kl2_session_release_hwq_locked曾位于idr_remove前，以下执行流时触发BUG
    //     CPU0                                         CPU1
    // -----------------------------                -----------------------------
    // kl2_session_release_hwq_locked
    //                                              kl2_excp_taint_all
    //                                                kl2_excp_taint_uproc_sess
    //                                                  idr_for_each_entry
    //                                                  kl2_excp_taint_hwq
    //                                                    // 即将删除的sess又被插入hwq->session_list
    //                                                    kl2_session_bind_hwq
    // idr_remove
    // // sess被释放但hwq->session_list还
    // // 存在sess->hwq_node引用
    // kfree(sess)
    //
    mutex_lock(&sess->lock);
    kl2_session_release_hwq_locked(sess);
    mutex_unlock(&sess->lock);

    // 需放在destroy uproc前，回调函数中更新uproc中的内存使用计数
    // TODO(miaotianxiang): 增加内存使用计数清零assert
    kl_mm_free_by_owner(&kl2_dev->mm, sess, kl2_mm_free_update_stat_cb);

    xref_put(&sess->uproc->xref, kl2_destroy_userprocess_ref);

    kfree(sess);
}

int kl2_session_add_task(struct kl2_session *sess, struct kl2_task *task)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    int                        kl2_state      = kl2_get_state(kl2_dev);
    int                        err;
    unsigned long              flags;
    struct kl2_event          *evnt = NULL;
    u32                        rec_cnt;
    u64                        evnt_seq = 0;
    int                        type     = -1;

    if (!kl2_session_state_normal(sess) || kl2_state == KL2_ERROR) {
        KL2_LOGD("try to add task_%d to error session_%d(st:%d taint_st:%d) xpu(st:%d)\n",
                 task->desc.kernel.token, sess->id, atomic_read(&sess->state),
                 atomic_read(&sess->taint_state), kl2_state);
        return -XPUERR_NOSUPPORT;
    }

    if (!sess->hwq) {
        err = kl2_session_bind_hwq(sess);
        if (err)
            return err;
    }

    task->sess    = sess;
    task->hwq_id  = sess->hwq->id;
    task->sess_id = sess->id;
    task->pid     = sess->uproc->pid;

    spin_lock_irqsave(&sess->hwq->lock, flags);
    // kl2_session_mark_error需放在revoke_task_from_err_session_locked前，
    // 并且kl2_session_add_task需持有spinlock检查sess->state，
    // 从而保证sess被标记为ERROR且revoke后不会添加新task待执行。
    if (!kl2_session_state_normal(sess)) {
        err = -XPUERR_NOSUPPORT;
    } else {
        // 根据不同task->type，维护各种引用计数和完成判断计数
        type = task->type;
        switch (type) {
        case KL2_TASKTYPE_KERNEL:
            KL2_LOGD("add sess_%d task_%x to hwq_%d\n", sess->id, task->desc.kernel.token,
                     sess->hwq->id);
            break;
        case KL2_TASKTYPE_EVNTREC:
            if (!task->evnt) {
                BUG();
            }
            evnt = task->evnt;

            // 原先evnt_seq获取位于ioctl_event_record，但可能被同hwq其他sess event wait超车，
            // 极端情况下全部hwq被超车将造成连环死锁。故将evnt_seq获取延后至hwq->lock cs内，
            // 保证evnt rec可以尽早下发。
            evnt_seq                   = atomic64_add_return(1, &sess->hwq->evnt_seq);
            task->desc.ctrl.record_seq = evnt_seq;

            // 如果该task为EVNTREC，需增加evnt引用计数，因finish_task中需
            // 操作evnt->fin_cnt用于ioctl_event_wait，finish_task中对应减少
            rec_cnt = atomic_add_return(1, &evnt->rec_cnt);
            xref_get(&evnt->xref);
            KL2_LOGD("record sess_%d task_%x seq_%llx on hwq_%d, rec_cnt= %x\n", sess->id,
                     task->desc.kernel.token, task->desc.ctrl.record_seq, sess->hwq->id, rec_cnt);
            break;
        case KL2_TASKTYPE_EVNTWAIT:
            KL2_LOGD("wait sess_%d task_%x seq_%llx of hwq_%d on hwq_%d\n", sess->id,
                     task->desc.kernel.token, task->desc.ctrl.record_seq,
                     task->desc.ctrl.wait_vstream_id, sess->hwq->id);
            break;
        }
        // 所有类型task，均需维护sess->unfinished_cnt，xpu_wait等待全部类型task执行完返回
        atomic_add(1, &sess->unfinished_cnt);
        // 增加sess引用计数，kl2_session_free_task中对应减少
        xref_get(&sess->xref);

        list_add_tail(&task->hwq_node, &sess->hwq->pt_list);
        sess->hwq->cnt_all += 1;
        kl2_hwq_dispatch_locked(sess->hwq);
        err = 0;
    }
    spin_unlock_irqrestore(&sess->hwq->lock, flags);

    if (!err) {
        switch (type) {
        case KL2_TASKTYPE_EVNTREC:
            // 对外暴露该evnt已record，应放在kl2_session_add_task最后
            mutex_lock(&evnt->lock);
            evnt->rec_hwq          = sess->hwq;
            evnt->rec_hwq_evnt_seq = evnt_seq;
            mutex_unlock(&evnt->lock);
            break;
        }
    }

    return err;
}

void kl2_session_free_task(struct kl2_task *task)
{
    xref_put(&task->sess->xref, kl2_destroy_session_ref);
    kfree(task);
}

void kl2_session_mark_error(struct kl2_session *sess, int errno, int taint)
{
    atomic_set(&sess->state, KL2_SESS_ERROR);
    if (!taint) {
        // taint为0说明sess中task真正发生了异常，无条件覆盖errno
        sess->errno = errno;
    } else {
        // 否则，该sess系被异常扩散污染，优先级低于sess中task真正发生了异常，不覆盖已有errno
        if (!sess->errno)
            sess->errno = errno;
    }

    wake_up_interruptible_all(&sess->wait_queue);
}

void kl2_session_taint(struct kl2_session *sess, int errno)
{
    atomic_set(&sess->taint_state, KL2_SESS_TAINT);
    sess->taint_errno = errno;
}

bool kl2_session_state_normal(struct kl2_session *sess)
{
    if (atomic_read(&sess->state) == KL2_SESS_NORMAL &&
        atomic_read(&sess->taint_state) == KL2_SESS_NORMAL) {
        return true;
    } else {
        return false;
    }
}

int kl2_session_wait_until_finished(struct kl2_session *sess)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;

    if (atomic_read(&sess->unfinished_cnt) == 0) {
        return 0;
    }
    kl_poll_cond_timeout((atomic_read(&sess->unfinished_cnt) == 0), 10000, 500000 /* 0.5s */);
    if (atomic_read(&sess->unfinished_cnt) == 0) {
        return 0;
    }

    // 如有task未结束，标记sess状态为TAINT hwq状态为TAINT，依赖timer+excp work实现状态清理
    kl2_session_taint(sess, XPUERR_TIMEOUT);
    if (likely(sess->hwq)) {
        atomic_set(&sess->hwq->taint_state, KL2_HWQ_TAINT);
    }

    // TODO(miaotianxiang): 60s是不是太长了
    return kl_poll_cond_timeout((atomic_read(&sess->unfinished_cnt) == 0), 10000,
                                60000000 /* 60s */);
}
