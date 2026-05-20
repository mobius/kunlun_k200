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
#include "kl2/hw.h"
#include "kl2/exception.h"
#include "kl2/kl2_regs.h"
#include "kl2/mbox.h"

static void kl2_hwq_finish_work_func(struct work_struct *work);

void kl2_hwq_init(struct kl2_device *kl2_dev)
{
    u32 hwq_bits, hwq_cnt;
    int i;

    hwq_bits = kl2_dev->spec.hwq_bits;
    hwq_cnt  = bitcount(hwq_bits);

    mutex_init(&kl2_dev->hwq_binding_lock);
    kl2_dev->hwq_bitmap = 0ul;

    for (i = 0; i < KL2_HWQ_CNT; ++i) {
        struct kl2_hwq *hwq = &kl2_dev->hwq[i];

        hwq->kl2_dev = kl2_dev;
        hwq->id      = i;
        if (hwq_bits & BIT(i)) {
            hwq->enable = true;
        } else {
            // this hwq is not available
            hwq->enable = false;
            bitmap_set(&kl2_dev->hwq_bitmap, i, 1);
        }

        spin_lock_init(&hwq->lock);
        INIT_LIST_HEAD(&hwq->pt_list);
        INIT_LIST_HEAD(&hwq->rt_list);
        INIT_LIST_HEAD(&hwq->ft_list);
        hwq->cnt_running = 0;
        hwq->cnt_all     = 0;

        INIT_LIST_HEAD(&hwq->session_list);
        hwq->session_cnt = 0;

        INIT_LIST_HEAD(&hwq->exception_list);

        atomic64_set(&hwq->evnt_seq, 0);

        INIT_WORK(&hwq->finish_work, kl2_hwq_finish_work_func);
    }

    KL2_LOGD("hwq_bits= %x bitmap(%px)= %lx\n", hwq_bits, &kl2_dev->hwq_bitmap,
             kl2_dev->hwq_bitmap);
}

void __kl2_hwq_dispatch_locked_nocheck(struct kl2_hwq *hwq)
{
    struct kl2_device *kl2_dev __maybe_unused = hwq->kl2_dev;
    struct kl2_task           *task;

    if (hwq->cnt_running >= KL2_HWQ_DEPTH) {
        KL2_LOGD("hwq_%d full\n", hwq->id);
        return;
    }

    while ((hwq->cnt_running < KL2_HWQ_DEPTH) &&
           (task = list_first_entry_or_null(&hwq->pt_list, struct kl2_task, hwq_node)) &&
           kl2_session_state_normal(task->sess)) {
        // move the first task in pending_list to the tail of running_list
        list_move_tail(&task->hwq_node, &hwq->rt_list);
        ++hwq->cnt_running;

        kl2_sse_write_desc_locked(kl2_dev, kl2_dev->iomem_base.sse_base, &task->desc, hwq->id);
        KL2_LOGD("write task_%u to hwq_%d\n", task->desc.kernel.token, hwq->id);
    }
}

void kl2_hwq_dispatch_locked(struct kl2_hwq *hwq)
{
    struct kl2_device *kl2_dev __maybe_unused = hwq->kl2_dev;
    int                        kl2_state      = kl2_get_state(kl2_dev);
    int                        in_reset_state = kl2_get_in_reset_state(kl2_dev);

    if (kl2_state != KL2_RUNNING || in_reset_state) {
        KL2_LOGD("kl2_state= %d, skip\n", kl2_state);
        return;
    }
    return __kl2_hwq_dispatch_locked_nocheck(hwq);
}

void kl2_handle_hwq_intr(struct kl2_hwq *hwq, int cnt)
{
    struct kl2_device *kl2_dev __maybe_unused = hwq->kl2_dev;
    unsigned long              flags;
    struct kl2_task           *task;
    struct kl2_task           *safe;

    if (!cnt)
        return;

    spin_lock_irqsave(&hwq->lock, flags);

    // 压测时曾观测到该现象，默认修正cnt值，容忍该硬件异常
    if (hwq->cnt_running < cnt) {
        KL2_LOGW("hwq_%d hwq->cnt_running < cnt, cnt_running= %d, cnt= %d\n", hwq->id,
                 hwq->cnt_running, cnt);
        cnt = hwq->cnt_running;
    }

    hwq->cnt_running -= cnt;
    hwq->cnt_all -= cnt;

    list_for_each_entry_safe(task, safe, &hwq->rt_list, hwq_node) {
        if (cnt <= 0)
            break;

        list_move_tail(&task->hwq_node, &hwq->ft_list);

#ifndef USE_POLL_WAIT
        if (atomic_sub_return(1, &task->sess->unfinished_cnt) == 0)
            wake_up_interruptible(&task->sess->wait_queue);
#else
        atomic_sub(1, &task->sess->unfinished_cnt);
#endif
        --cnt;
    }

    kl2_hwq_dispatch_locked(hwq);

    spin_unlock_irqrestore(&hwq->lock, flags);

    queue_work(hwq->kl2_dev->hwq_wq, &hwq->finish_work);
}

static void finish_task(struct kl2_task *task)
{
    struct kl2_device *kl2_dev __maybe_unused = task->sess->kl2_dev;
    u32                        fin_cnt;

    switch (task->type) {
    case KL2_TASKTYPE_KERNEL:
        if (task->flag & KL2_TASKFLAG_FREE_PARAM) {
            kl_mm_free(&task->sess->kl2_dev->mm, task->desc.kernel.param_addr, task->sess, NULL);
        }
        break;
    case KL2_TASKTYPE_EVNTREC:
        fin_cnt = atomic_add_return(1, &task->evnt->fin_cnt);
        KL2_LOGD("record sess_%d task_%x seq_%llx on hwq_%d done, rec_cnt= %x, fin_cnt= %x\n",
                 task->sess->id, task->desc.kernel.token, task->desc.ctrl.record_seq, task->hwq_id,
                 atomic_read(&task->evnt->rec_cnt), fin_cnt);
        xref_put(&task->evnt->xref, kl2_destroy_event_ref);
        break;
    case KL2_TASKTYPE_EVNTWAIT:
        KL2_LOGD("wait sess_%d task_%x seq_%llx of hwq_%d on hwq_%d done\n", task->sess->id,
                 task->desc.kernel.token, task->desc.ctrl.record_seq,
                 task->desc.ctrl.wait_vstream_id, task->hwq_id);
        break;
    default:
        break;
    }
}

static void kl2_hwq_finish_work_func(struct work_struct *work)
{
    struct kl2_hwq  *hwq = container_of(work, struct kl2_hwq, finish_work);
    struct kl2_task *task, *safe;
    unsigned long    flags;
    LIST_HEAD(free_list);

    spin_lock_irqsave(&hwq->lock, flags);

    list_for_each_entry_safe(task, safe, &hwq->exception_list, hwq_node) {
        list_move_tail(&task->hwq_node, &free_list);

#ifndef USE_POLL_WAIT
        if (atomic_sub_return(1, &task->sess->unfinished_cnt) == 0)
            wake_up_interruptible(&task->sess->wait_queue);
#else
        atomic_sub(1, &task->sess->unfinished_cnt);
#endif
    }
    x_list_move_all_tail(&free_list, &hwq->ft_list);

    spin_unlock_irqrestore(&hwq->lock, flags);

    list_for_each_entry_safe(task, safe, &free_list, hwq_node) {
        finish_task(task);
        list_del(&task->hwq_node);
        kl2_session_free_task(task);
    }
}

static int kl2_copy_hw_info_to_vf(struct kl2_device *kl2_dev, u32 data, int hw_info_offset)
{
    struct kl_device *kdev = kl2_dev->kdev;

    kl2_writel(kl2_dev, data, kl2_dev->iomem_base.l3_base + hw_info_offset);

    if (kdev->num_vfs == 2) {
        kl2_writel(kl2_dev, data,
                   kl2_dev->iomem_base.l3_base + KL2_SRIOV_2VFS_L3_SIZE + hw_info_offset);
    } else if (kdev->num_vfs == 3) {
        kl2_writel(kl2_dev, data,
                   kl2_dev->iomem_base.l3_base + KL2_SRIOV_3VFS_L3_SIZE + hw_info_offset);
        kl2_writel(kl2_dev, data,
                   kl2_dev->iomem_base.l3_base + KL2_SRIOV_3VFS_L3_SIZE * 2 + hw_info_offset);
    }

    return 0;
}

static int update_base_hw_info(struct kl2_device *kl2_dev)
{
    if (is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
        kl2_dev->dev_info.temperature =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_TEMPERATURE_L3_OFFSET);
        kl2_dev->dev_info.sdnn_freq =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_SDNN_FREQ_L3_OFFSET);
        kl2_dev->dev_info.cluster_freq =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_CLUSTER_FREQ_L3_OFFSET);
        kl2_dev->dev_info.power =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_POWER_L3_OFFSET);

        kl2_dev->dev_info.decoder_freq =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_DECODER_FREQ_L3_OFFSET);
        kl2_dev->dev_info.encoder_freq =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_ENCODER_FREQ_L3_OFFSET);
        kl2_dev->dev_info.image_proc_freq =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_IMAGE_PROC_FREQ_L3_OFFSET);
    } else {
        kl2_dev->dev_info.temperature =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_GEN_R2);
        kl2_dev->dev_info.sdnn_freq    = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon0_base +
                                                                    KL2_REG_SYSCON0_PLL0_MDIV5_CONF);
        kl2_dev->dev_info.cluster_freq = kl2_readl(
                kl2_dev, kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_PLL5_MDIV3_CONF);
        kl2_dev->dev_info.power =
                kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_GEN_R1);

        kl2_dev->dev_info.decoder_freq = kl2_readl(
                kl2_dev, kl2_dev->iomem_base.syscon1_base + KL2_REG_SYSCON1_PLL1_MDIV2_CONF);
        kl2_dev->dev_info.encoder_freq = kl2_readl(
                kl2_dev, kl2_dev->iomem_base.syscon1_base + KL2_REG_SYSCON1_PLL1_MDIV3_CONF);
        kl2_dev->dev_info.image_proc_freq = kl2_readl(
                kl2_dev, kl2_dev->iomem_base.syscon1_base + KL2_REG_SYSCON1_PLL1_MDIV4_CONF);
    }

    if (is_pf_id(kl2_dev->dev_info.sriov_func_id)) {
        kl2_copy_hw_info_to_vf(kl2_dev, kl2_dev->dev_info.temperature, KL2_TEMPERATURE_L3_OFFSET);
        kl2_copy_hw_info_to_vf(kl2_dev, kl2_dev->dev_info.sdnn_freq, KL2_SDNN_FREQ_L3_OFFSET);
        kl2_copy_hw_info_to_vf(kl2_dev, kl2_dev->dev_info.cluster_freq, KL2_CLUSTER_FREQ_L3_OFFSET);
        kl2_copy_hw_info_to_vf(kl2_dev, kl2_dev->dev_info.power, KL2_POWER_L3_OFFSET);
        kl2_copy_hw_info_to_vf(kl2_dev, kl2_dev->dev_info.decoder_freq, KL2_DECODER_FREQ_L3_OFFSET);
        kl2_copy_hw_info_to_vf(kl2_dev, kl2_dev->dev_info.encoder_freq, KL2_ENCODER_FREQ_L3_OFFSET);
        kl2_copy_hw_info_to_vf(kl2_dev, kl2_dev->dev_info.image_proc_freq,
                               KL2_IMAGE_PROC_FREQ_L3_OFFSET);
    }

    return 0;
}

static int get_xpu_busy_status(struct kl2_device *kl2_dev, u32 *sse_xpu_busy_status)
{
    void __iomem *sse_base      = kl2_dev->iomem_base.sse_base;
    int           sriov_func_id = kl2_dev->dev_info.sriov_func_id;
    int           user_id       = sriov_func_id - KL2_SRIOV_FUNC_ID_VF_0;
    int           ret;

    if (is_vf_id(sriov_func_id)) {
        ret = kl2_sriov_mbox_req_xpu_busy_status_vf(kl2_dev, user_id, sse_xpu_busy_status);
    } else {
        *sse_xpu_busy_status = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_XPU_BUSY_STATUS);
        ret                  = 0;
    }

    return ret;
}

enum hrtimer_restart kl2_regular_timer_func(struct hrtimer *hrtimer)
{
    struct kl2_device *kl2_dev = container_of(hrtimer, struct kl2_device, regular_timer);
    int                hwq_id, i, err;
    unsigned long      flags;
    void __iomem      *cluster_base = kl2_dev->iomem_base.cluster_base;
    void __iomem      *sdnn_base    = kl2_dev->iomem_base.sdnn_base;
    ktime_t            cur_ktime;
    bool               hwq_timeout[KL2_HWQ_CNT] = { false };
    u32                cl_timeout_st = 0, sd_timeout_st = 0;
    u32               *hwq_timeout_token    = &kl2_dev->exception_stash.hwq_timeout_token[0];
    unsigned long     *hwq_timeout_cl_sd_st = &kl2_dev->exception_stash.hwq_timeout_cl_sd_st[0];
    u32               *cl_timeout_token     = &kl2_dev->exception_stash.cl_timeout_token[0];
    u32               *sd_timeout_token     = &kl2_dev->exception_stash.sd_timeout_token[0];
    bool               need_signal          = false;
    bool               need_stall           = false;
    u32                sse_xpu_busy_status  = 0;

    // 更新temperature/freq/power
    update_base_hw_info(kl2_dev);

    // 在非KL2_RUNNING状态，用户下发的task会被挂到hwq->pt_list上，此处应给予机会重新下发
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        struct kl2_hwq *hwq = &kl2_dev->hwq[hwq_id];
        spin_lock_irqsave(&hwq->lock, flags);
        kl2_hwq_dispatch_locked(hwq);
        spin_unlock_irqrestore(&hwq->lock, flags);
    }

    // 检测超时，cur_cu_timeout为真超时，需通知excp work
    memset(hwq_timeout_cl_sd_st, 0, sizeof(kl2_dev->exception_stash.hwq_timeout_cl_sd_st));
    cur_ktime = ktime_get();
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        struct kl2_hwq  *hwq = &kl2_dev->hwq[hwq_id];
        struct kl2_task *cur_task;
        u32              cur_token = 0;
        // cur_task位于hwq->rt_list头部且时间超过阈值，但可能是假超时，比如因缺少cu或hwq stall而无法调度
        bool cur_timeout = false;
        // cur_task被要求尽快结束，比如cur_task已被判定为真超时，或sess已被标记
        bool cur_earlyout = false;
        // cur_task占用cu时间超过阈值，是真超时
        bool cur_cu_timeout         = false;
        int  detect_threshold_in_ms = kl2_dev->task_timeout_detect.detect_threshold_in_ms;

        spin_lock_irqsave(&hwq->lock, flags);
        cur_task = list_first_entry_or_null(&hwq->rt_list, struct kl2_task, hwq_node);
        if (cur_task) {
            cur_token = cur_task->desc.kernel.token;

            // cur_task被要求尽快结束
            if (!kl2_session_state_normal(cur_task->sess)) {
                cur_earlyout = true;
            }
            // 如果hwq被标记为ERROR，尽早强制当前task退出
            //if (atomic_read(&hwq->taint_state) != KL2_HWQ_NORMAL) {
            //    cur_earlyout = true;
            //}
            // 如果cur_task为EVNTWAIT，不占用cluster/sdnn计算资源，
            // 等效于无task正在执行，需重置hwq簿记信息
            if (cur_task->type == KL2_TASKTYPE_EVNTWAIT) {
                cur_task = NULL;
            }
            // 如果用户调大了超时检测时间阈值，且excp work工作中，则恢复原阈值以使excp work尽快结束
            if (detect_threshold_in_ms > KL2_TASK_TIMEOUT_DETECT_THRESHOLD_IN_MS &&
                kl2_dev->exception_stash.excp_work_running) {
                detect_threshold_in_ms = KL2_TASK_TIMEOUT_DETECT_THRESHOLD_IN_MS;
            }
        }
        spin_unlock_irqrestore(&hwq->lock, flags);

        if (!cur_task) {
            kl2_dev->task_timeout_detect.hwq[hwq_id].busy         = false;
            kl2_dev->task_timeout_detect.hwq[hwq_id].token        = 0;
            kl2_dev->task_timeout_detect.hwq[hwq_id].record_ktime = cur_ktime;
        } else {
            if ((kl2_dev->task_timeout_detect.hwq[hwq_id].busy &&
                 kl2_dev->task_timeout_detect.hwq[hwq_id].token == cur_token) ||
                cur_earlyout) {
                ktime_t delta =
                        ktime_sub(cur_ktime, kl2_dev->task_timeout_detect.hwq[hwq_id].record_ktime);
                cur_timeout = ktime_to_ms(delta) > detect_threshold_in_ms;
                if (cur_timeout || cur_earlyout) {
                    // 超时
                    unsigned long sse_xpu_busy_status_ul;

                    if (!sse_xpu_busy_status) {
                        // 通过mailbox获取状态需要一定时间,为了避免在hrtimer里等待太久,这里
                        // 直接退出timer等下次进入hrtimer时直接获取mailbox读到的状态.
                        err = get_xpu_busy_status(kl2_dev, &sse_xpu_busy_status);
                        if (err) {
                            goto restart_timer;
                        }
                    }
                    sse_xpu_busy_status_ul = sse_xpu_busy_status;
                    // 下文token寄存器访问可能与soft_reset、cu reset等构成冲突，用reg_lock避免。
                    // timer处于中断上下文，无法长时间spin等待，如trylock失败则退出等待下次触发。
                    err = kl2_reg_trylock(kl2_dev);
                    if (err) {
                        //goto restart_timer;
                        continue;
                    }
                    for_each_set_bit(i, &sse_xpu_busy_status_ul, 32) {
                        if (i >= 0 && i < KL2_CLUSTER_MAX_COUNT && !cluster_invalid(kl2_dev, i)) {
                            // cluster
                            u32 busy_token =
                                    kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) +
                                                               KL2_REG_CLUSTER_TOKEN);
                            u32 virt_cl_id =
                                    kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) +
                                                               KL2_REG_CLUSTER_ID);
                            if ((busy_token == cur_token) &&
                                (virt_cl_id != kl2_dev->task_timeout_detect.hwq[hwq_id]
                                                       .last_virt_cl_id[i] ||
                                 cur_earlyout)) {
                                cur_cu_timeout = true;
                                cl_timeout_st |= 0x1u << (i * 2 + 1);
                                hwq_timeout_cl_sd_st[hwq_id] |= 0x1u << i;
                                cl_timeout_token[i] = cur_token;
                                kl2_dev->task_timeout_detect.hwq[hwq_id].last_virt_cl_id[i] =
                                        virt_cl_id;
                            }
                        } else if (i >= KL2_CLUSTER_MAX_COUNT &&
                                   i < KL2_CLUSTER_MAX_COUNT + KL2_SDNN_MAX_COUNT &&
                                   !sdnn_invalid(kl2_dev, i - KL2_CLUSTER_MAX_COUNT)) {
                            // sdnn
                            u32 busy_token = kl2_readl(
                                    kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base,
                                                                       i - KL2_CLUSTER_MAX_COUNT) +
                                                     KL2_REG_CLUSTER_TOKEN);
                            u32 virt_sdnn_cl_id = kl2_readl(
                                    kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base,
                                                                       i - KL2_CLUSTER_MAX_COUNT) +
                                                     KL2_REG_CLUSTER_ID);
                            if ((busy_token == cur_token) &&
                                (virt_sdnn_cl_id !=
                                         kl2_dev->task_timeout_detect.hwq[hwq_id]
                                                 .last_virt_sdnn_cl_id[i - KL2_CLUSTER_MAX_COUNT] ||
                                 cur_earlyout)) {
                                cur_cu_timeout = true;
                                sd_timeout_st |= 0x1u << ((i - KL2_CLUSTER_MAX_COUNT) * 2 + 1);
                                hwq_timeout_cl_sd_st[hwq_id] |= 0x1u << i;
                                sd_timeout_token[i - KL2_CLUSTER_MAX_COUNT] = cur_token;
                                kl2_dev->task_timeout_detect.hwq[hwq_id]
                                        .last_virt_sdnn_cl_id[i - KL2_CLUSTER_MAX_COUNT] =
                                        virt_sdnn_cl_id;
                            }
                        }
                    }
                    kl2_reg_unlock(kl2_dev);

                    if (cur_cu_timeout) {
                        KL2_LOGW("task timeout/task earlyout detected, hwq_id= %d, tk= %u\n",
                                 hwq_id, cur_token);
                        hwq_timeout[hwq_id]       = true;
                        hwq_timeout_token[hwq_id] = cur_token;
                        need_signal               = true;

                        // 如已确定cu执行task超时，且task仍位于rt_list头部，则该task真超时无疑，尽早标记task->sess为TAINT，后续子任务可走earlyout分支快速结束
                        spin_lock_irqsave(&hwq->lock, flags);
                        cur_task =
                                list_first_entry_or_null(&hwq->rt_list, struct kl2_task, hwq_node);
                        if (cur_task && cur_task->desc.kernel.token == cur_token) {
                            kl2_session_taint(cur_task->sess, XPUERR_TIMEOUT);
                        }
                        spin_unlock_irqrestore(&hwq->lock, flags);
                    }
                } else {
                    // 未到超时阈值，无需更新record_ktime
                }
            } else {
                // 该hwq开始执行一个新task
                kl2_dev->task_timeout_detect.hwq[hwq_id].busy         = true;
                kl2_dev->task_timeout_detect.hwq[hwq_id].token        = cur_token;
                kl2_dev->task_timeout_detect.hwq[hwq_id].record_ktime = cur_ktime;
                for (i = 0; i < KL2_CLUSTER_MAX_COUNT; ++i) {
                    kl2_dev->task_timeout_detect.hwq[hwq_id].last_virt_cl_id[i] = -1;
                }
                for (i = 0; i < KL2_SDNN_MAX_COUNT; ++i) {
                    kl2_dev->task_timeout_detect.hwq[hwq_id].last_virt_sdnn_cl_id[i] = -1;
                }
            }
        }
    }

    // 处理hwq->taint_state被excp work或kl2_release标记为taint，先设置hwq stall，等待underway变为0后增加taint_2_reset_cnt伺机reset
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        struct kl2_hwq *hwq = &kl2_dev->hwq[hwq_id];
        if (atomic_read(&hwq->taint_state) != KL2_HWQ_NORMAL) {
            need_stall = true;
            atomic_set(&hwq->regular_timer_state, KL2_HWQ_STALL);
            atomic_set(&hwq->taint_state, KL2_HWQ_NORMAL);
        }
        // 1. 万一timer被cancel后又重新start ...
        // 2. 万一stall后又被excp work unstall，需要再走一遍流程 ...
        // 3. 好处是可以尽量不影响该hwq正在运行的无关task，无关task可能需要很长时间才能结束，
        // hwq的TAINT状态便从hwq->taint_state转化到hwq->regular_timer_state
        //
        // 但即使这样，仍然无法保证queue work到excp work开始处理期间underway一直是0，但好在excp
        // work中第一个大循环要求所有hwq underway变为0，方可reset
        if (atomic_read(&hwq->regular_timer_state) != KL2_HWQ_NORMAL) {
            need_stall = true;
        }
    }
    if (need_stall) {
        kl2_sse_hwq_stall_all(kl2_dev);
    }
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        struct kl2_hwq *hwq = &kl2_dev->hwq[hwq_id];
        if (atomic_read(&hwq->regular_timer_state) == KL2_HWQ_STALL) {
            u32 underway;

            err = kl2_reg_trylock(kl2_dev);
            if (err) {
                //goto restart_timer;
                continue;
            }
            underway = kl2_sse_hwq_underway(hwq);
            kl2_reg_unlock(kl2_dev);

            // hwq underway变为0，且hwq已被stall，可以通知excp work安全reset
            if (!underway) {
                atomic_set(&hwq->regular_timer_state, KL2_HWQ_UNDERWAY_EQUALS_ZERO);
                need_signal = true;
            }
        }
    }

    // 通知excp work
    if (need_signal) {
        kl2_sse_hwq_stall_all(kl2_dev);

        // 标记需要处理的cu
        if (cl_timeout_st) {
            kl2_cluster_disable_and_record_timeout(kl2_dev, cl_timeout_st, cl_timeout_token);
        }
        if (sd_timeout_st) {
            kl2_sdnn_disable_and_record_timeout(kl2_dev, sd_timeout_st, sd_timeout_token);
        }
        // 标记需要处理的hwq
        for_each_valid_sse_queue(kl2_dev, hwq_id) {
            struct kl2_hwq *hwq = &kl2_dev->hwq[hwq_id];
            if (hwq_timeout[hwq_id]) {
                kl2_dev->exception_board_hwq[hwq_id].token = hwq_timeout_token[hwq_id];
                for_each_set_bit(i, &hwq_timeout_cl_sd_st[hwq_id], 32) {
                    if (i >= 0 && i < KL2_CLUSTER_MAX_COUNT) {
                        // cluster
                        kl2_dev->exception_board_hwq[hwq_id].cl_excp_st[i] =
                                kl2_dev->exception_board_cluster[i].cl_excp_st;
                        memcpy(&kl2_dev->exception_board_hwq[hwq_id].cl_debug_info[i],
                               &kl2_dev->exception_board_cluster[i].cl_debug_info,
                               sizeof(kl2_dev->exception_board_cluster[i].cl_debug_info));
                    } else if (i >= KL2_CLUSTER_MAX_COUNT &&
                               i < KL2_CLUSTER_MAX_COUNT + KL2_SDNN_MAX_COUNT) {
                        // sdnn
                        kl2_dev->exception_board_hwq[hwq_id]
                                .sdnn_cl_excp_st[i - KL2_CLUSTER_MAX_COUNT] =
                                kl2_dev->exception_board_sdnn[i - KL2_CLUSTER_MAX_COUNT]
                                        .sdnn_cl_excp_st;
                        kl2_dev->exception_board_hwq[hwq_id]
                                .sdnn_sd_excp_st[i - KL2_CLUSTER_MAX_COUNT] =
                                kl2_dev->exception_board_sdnn[i - KL2_CLUSTER_MAX_COUNT]
                                        .sdnn_sd_excp_st;
                        memcpy(&kl2_dev->exception_board_hwq[hwq_id]
                                        .sdnn_debug_info[i - KL2_CLUSTER_MAX_COUNT],
                               &kl2_dev->exception_board_sdnn[i - KL2_CLUSTER_MAX_COUNT]
                                        .sdnn_debug_info,
                               sizeof(kl2_dev->exception_board_sdnn[i - KL2_CLUSTER_MAX_COUNT]
                                              .sdnn_debug_info));
                    }
                }
                atomic_inc(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
            }
            // 这三行很重要，不增加excp_cnt而使用taint_2_reset_cnt是因为excp_cnt要求找到对应etask，
            // 而hwq被taint不一定因为etask
            if (atomic_read(&hwq->regular_timer_state) == KL2_HWQ_UNDERWAY_EQUALS_ZERO) {
                atomic_inc(&kl2_dev->exception_board_hwq[hwq_id].taint_2_reset_cnt);
                atomic_set(&hwq->regular_timer_state, KL2_HWQ_NORMAL);
            }
        }

        queue_work(g_kunlun_wq, &kl2_dev->handle_exception_work);
        KL2_LOGD("task timeout/task earlyout/hwq taint detected, queue_work\n");
    }

    kl2_dev->exception_stash.regular_timer_seq++;

restart_timer:
    hrtimer_forward_now(&kl2_dev->regular_timer, kl2_dev->regular_ktime);
    return HRTIMER_RESTART;
}
