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
#include <linux/uaccess.h>

static struct kl2_event *kl2_event_find_and_get(struct kl2_device *kl2_dev, int evnt_id)
{
    struct kl2_event *evnt;

    mutex_lock(&kl2_dev->event_idr_lock);
    evnt = idr_find(&kl2_dev->event_idr, evnt_id);
    if (evnt) {
        xref_get(&evnt->xref);
    }
    mutex_unlock(&kl2_dev->event_idr_lock);

    return evnt;
}

static struct kl2_event *kl2_create_event(struct kl2_session *sess, int flags)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    struct kl2_event          *evnt;

    evnt = kzalloc(sizeof(*evnt), GFP_KERNEL);
    if (!evnt) {
        return NULL;
    }

    mutex_lock(&kl2_dev->event_idr_lock);
    evnt->id = idr_alloc(&kl2_dev->event_idr, evnt, 1, INT_MAX, GFP_KERNEL);
    if (evnt->id < 0) {
        mutex_unlock(&kl2_dev->event_idr_lock);
        kfree(evnt);
        return NULL;
    }

    evnt->uproc = sess->uproc;
    list_add_tail(&evnt->uproc_node, &evnt->uproc->event_list);
    atomic_set(&evnt->state, KL2_EVNT_NORMAL);
    mutex_unlock(&kl2_dev->event_idr_lock);

    mutex_init(&evnt->lock);

    atomic_set(&evnt->rec_cnt, 0);
    atomic_set(&evnt->fin_cnt, 0);
    xref_init(&evnt->xref);

    KL2_LOGD("create evnt, evnt->id= %d, sess->id= %d\n", evnt->id, sess->id);
    return evnt;
}

// XXX(miaotianxiang): 调用时需持有kl2_dev->event_idr_lock
void kl2_destroy_event_locked_ref(struct kref *kref)
{
    struct kl2_event *evnt = container_of((struct xref *)kref, struct kl2_event, xref);
    // evnt->uproc在event创建时赋值，uproc销毁时释放下属所有event，故evnt->uproc一定合法
    struct kl2_device *kl2_dev __maybe_unused = evnt->uproc->kl2_dev;

    list_del(&evnt->uproc_node);
    idr_remove(&kl2_dev->event_idr, evnt->id);

    KL2_LOGD("destroy evnt, evnt->id= %d\n", evnt->id);
    kfree(evnt);
}

void kl2_destroy_event_ref(struct kref *kref)
{
    struct kl2_event          *evnt = container_of((struct xref *)kref, struct kl2_event, xref);
    struct kl2_device *kl2_dev __maybe_unused = evnt->uproc->kl2_dev;

    mutex_lock(&kl2_dev->event_idr_lock);
    list_del(&evnt->uproc_node);
    idr_remove(&kl2_dev->event_idr, evnt->id);
    mutex_unlock(&kl2_dev->event_idr_lock);

    KL2_LOGD("destroy evnt, evnt->id= %d\n", evnt->id);
    kfree(evnt);
}

// TODO(miaotianxiang): evnt->id做成全局保序？
int ioctl_event_create(struct kl2_session *sess, void __user *argp)
{
    union XPUEventCreate       args;
    struct XPUEventCreate_in  *in;
    struct XPUEventCreate_out *out;
    struct kl2_event          *evnt;

    in  = (struct XPUEventCreate_in *)&args;
    out = (struct XPUEventCreate_out *)&args;

    if (copy_from_user(&args, argp, sizeof(args)))
        return -EFAULT;

    evnt = kl2_create_event(sess, in->flags);
    if (!evnt)
        return -ENOMEM;

    out->handle = evnt->id;

    // 如果copy_to_user失败，新创建evnt已被记录在uproc->event_list，将伴随uproc销毁而释放
    if (copy_to_user(argp, &args, sizeof(args)))
        return -EFAULT;

    return 0;
}

int ioctl_event_destroy(struct kl2_session *sess, void __user *argp)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    int                        evnt_id;
    struct kl2_event          *evnt;
    u32                        old_state;
    int                        err;

    if (copy_from_user(&evnt_id, argp, sizeof(evnt_id)))
        return -EFAULT;

    mutex_lock(&kl2_dev->event_idr_lock);

    evnt = idr_find(&kl2_dev->event_idr, evnt_id);
    if (!evnt) {
        err = -XPUERR_INVALID_PARAM;
        goto out;
    }

    // 错误访问其他进程资源
    if (evnt->uproc != sess->uproc) {
        err = -XPUERR_INVALID_PARAM;
        goto out;
    }

    // 仅第一次destroy返回成功，后续destroy返回错误
    do {
        old_state = atomic_read(&evnt->state);
        if (old_state == KL2_EVNT_DESTROYED) {
            err = -XPUERR_INVALID_PARAM;
            goto out;
        }
    } while (atomic_cmpxchg(&evnt->state, old_state, KL2_EVNT_DESTROYED) != old_state);

    xref_put(&evnt->xref, kl2_destroy_event_locked_ref);
    err = 0;

out:
    mutex_unlock(&kl2_dev->event_idr_lock);
    return err;
}

int ioctl_event_record(struct kl2_session *sess, void __user *argp)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    int                        evnt_id;
    struct kl2_event          *evnt;
    struct kl2_task           *task;
    u32                        token;
    int                        err;

    if (copy_from_user(&evnt_id, argp, sizeof(evnt_id)))
        return -EFAULT;

    evnt = kl2_event_find_and_get(kl2_dev, evnt_id);
    if (!evnt) {
        return -XPUERR_INVALID_PARAM;
    }

    // 错误访问其他进程资源
    if (evnt->uproc != sess->uproc) {
        err = -XPUERR_INVALID_PARAM;
        goto err_put_evnt;
    }

    // 获取某个hwq->evnt_seq，故需提早bind
    //err = kl2_session_bind_hwq(sess);
    //if (err) {
    //    goto err_put_evnt;
    //}

    task = kzalloc(sizeof(*task), GFP_KERNEL);
    if (!task) {
        err = -ENOMEM;
        goto err_put_evnt;
    }

    // 原先evnt_seq获取位于ioctl_event_record，但可能被同hwq其他sess event wait超车，
    // 极端情况下全部hwq被超车将造成连环死锁。故将evnt_seq获取延后至hwq->lock cs内，
    // 保证evnt rec可以尽早下发。
    //evnt_seq = atomic64_add_return(1, &sess->hwq->evnt_seq);
    token = atomic_add_return(2, &kl2_dev->task_token) - 2;

    task->desc.ctrl.type  = KL2_SSE_TASKTYPE_EVNTREC;
    task->desc.ctrl.token = token;
    //task->desc.ctrl.record_seq = evnt_seq;
    task->type = KL2_TASKTYPE_EVNTREC;
    // kl2_session_add_task中将再次增加evnt引用计数
    task->evnt = evnt;

    err = kl2_session_add_task(sess, task);
    if (err) {
        goto err_free_task;
    }

    KL2_LOGD(
            "record evnt, evnt->id= %d, sess->id= %d, hwq->id= %u, evnt->rec_hwq_evnt_seq= %llx, evnt->rec_cnt= %u\n",
            evnt->id, sess->id, sess->hwq->id, evnt->rec_hwq_evnt_seq, atomic_read(&evnt->rec_cnt));

    trace_event_record(sess, evnt, token);
    // 至此无需再访问evnt，对应减少kl2_event_find_and_get中增加的引用计数
    xref_put(&evnt->xref, kl2_destroy_event_ref);

    return 0;

err_free_task:
    kfree(task);
err_put_evnt:
    xref_put(&evnt->xref, kl2_destroy_event_ref);

    return err;
}

int ioctl_event_stream_wait(struct kl2_session *sess, void __user *argp)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    int                        evnt_id;
    struct kl2_event          *evnt;
    struct kl2_task           *task;
    u32                        token;
    int                        err;

    if (copy_from_user(&evnt_id, argp, sizeof(evnt_id)))
        return -EFAULT;

    evnt = kl2_event_find_and_get(kl2_dev, evnt_id);
    if (!evnt) {
        return -XPUERR_INVALID_PARAM;
    }

    // 错误访问其他进程资源
    if (evnt->uproc != sess->uproc) {
        err = -XPUERR_INVALID_PARAM;
        goto err_put_evnt;
    }

    //err = kl2_session_bind_hwq(sess);
    //if (err) {
    //    goto err_put_evnt;
    //}

    task = kzalloc(sizeof(*task), GFP_KERNEL);
    if (!task) {
        err = -ENOMEM;
        goto err_put_evnt;
    }

    token = atomic_add_return(2, &kl2_dev->task_token) - 2;

    // 并发条件下，确保wait_vstream_id + record_seq与record时一致
    mutex_lock(&evnt->lock);
    // 该evnt尚未record
    if (!evnt->rec_hwq) {
        mutex_unlock(&evnt->lock);
        err = -XPUERR_INVALID_PARAM;
        goto err_free_task;
    }

    task->desc.ctrl.type            = KL2_SSE_TASKTYPE_EVNTWAIT;
    task->desc.ctrl.wait_vstream_id = evnt->rec_hwq->id;
    task->desc.ctrl.token           = token;
    task->desc.ctrl.record_seq      = evnt->rec_hwq_evnt_seq;
    task->type                      = KL2_TASKTYPE_EVNTWAIT;
    mutex_unlock(&evnt->lock);

    err = kl2_session_add_task(sess, task);
    if (err) {
        goto err_free_task;
    }

    KL2_LOGD(
            "stream wait evnt, evnt->id= %d, wait_sess->id= %d, wait_hwq->id= %d, rec_hwq->id= %d, evnt->rec_hwq_evnt_seq= %llx, evnt->rec_cnt= %u\n",
            evnt->id, sess->id, sess->hwq->id, evnt->rec_hwq->id, evnt->rec_hwq_evnt_seq,
            atomic_read(&evnt->rec_cnt));

    trace_event_stream_wait(sess, evnt, token);

    // 至此无需再访问evnt，对应减少kl2_event_find_and_get中增加的引用计数
    xref_put(&evnt->xref, kl2_destroy_event_ref);

    return 0;

err_free_task:
    kfree(task);
err_put_evnt:
    xref_put(&evnt->xref, kl2_destroy_event_ref);

    return err;
}

static int __maybe_unused wait_evnt_timeout_interruptible(struct kl2_session *sess,
                                                          struct kl2_event *evnt, u64 timeout_us)
{
    unsigned long sleep_us = 400;
    ktime_t       __timeout;

    unsigned long hybrid_busy_time_us = 500;
    ktime_t       __hybrid_wait_timeout;
    int           hybrid_ite = 0;

    might_sleep();

    __timeout             = ktime_add_us(ktime_get(), timeout_us);
    __hybrid_wait_timeout = ktime_add_us(ktime_get(), hybrid_busy_time_us);

    for (;;) {
        ktime_t __current = ktime_get();
        u32     rec_cnt   = atomic_read(&evnt->rec_cnt);
        u32     fin_cnt   = atomic_read(&evnt->fin_cnt);

        // 该evnt尚未record 或 所有record均已结束（此判断条件强于最近record结束）
        if (rec_cnt == 0 || rec_cnt <= fin_cnt)
            return -sess->errno;

        if (timeout_us && ktime_compare(__current, __timeout) > 0)
            return -XPUERR_TIMEOUT;

        if (signal_pending_state(TASK_INTERRUPTIBLE, current))
            return -ERESTARTSYS;

        // 该evnt record的sess前部task发生错误，传递到该evnt，该evnt将不被hwq record
        if (atomic_read(&evnt->state) == KL2_EVNT_REVOKED)
            return -XPUERR_EVENTWAIT;
        // 当前wait sess发生错误，但不一定是该evnt record的sess
        if (sess->errno)
            return -sess->errno;

        {
            // hybrid wait
            if (ktime_compare(__current, __hybrid_wait_timeout) > 0) {
                if (hybrid_ite < 20) {
                    __hybrid_wait_timeout = ktime_add_us(__current, hybrid_busy_time_us);
                    usleep_range(4, 16);
                } else {
                    usleep_range((sleep_us >> 2) + 1, sleep_us);
                }
                ++hybrid_ite;
            }
            cpu_relax();
        }
    }

    return 0;
}

int ioctl_event_wait(struct kl2_session *sess, void __user *argp)
{
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    int                        evnt_id;
    struct kl2_event          *evnt;
    int                        err;

    if (copy_from_user(&evnt_id, argp, sizeof(evnt_id)))
        return -EFAULT;

    evnt = kl2_event_find_and_get(kl2_dev, evnt_id);
    if (!evnt)
        return -XPUERR_INVALID_PARAM;

    // 错误访问其他进程资源
    if (evnt->uproc != sess->uproc) {
        err = -XPUERR_INVALID_PARAM;
        goto err_put_evnt;
    }

    err = wait_evnt_timeout_interruptible(sess, evnt, IOCTL_WAIT_TIMEOUT);

    trace_event_wait(sess, evnt, evnt_id);

err_put_evnt:
    xref_put(&evnt->xref, kl2_destroy_event_ref);

    return err;
}
