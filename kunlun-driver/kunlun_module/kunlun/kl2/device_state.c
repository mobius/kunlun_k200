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

#include <linux/delay.h>
#include <linux/version.h>
#include <linux/hardirq.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "kl2/kl2.h"
#include "kl2/hw.h"
#include "kl2/kl2_regs.h"

const char *kl2_device_state_str(int state)
{
    switch (state) {
    case KL2_RUNNING:
        return "RUNNING";
    case KL2_IN_RESET:
        return "IN_RESET";
    case KL2_ERROR:
        return "ERROR";
    //case KL2_IN_EXCEPTION:
    //    return "IN_EXCEPTION";
    default:
        return "INVALID";
    }
}

int kl2_get_state(struct kl2_device *kl2_dev)
{
    return atomic_read(&kl2_dev->state);
}

int kl2_get_in_reset_state(struct kl2_device *kl2_dev)
{
    return atomic_read(&kl2_dev->in_reset_state);
}

void kl2_set_state(struct kl2_device *kl2_dev, int state)
{
    atomic_set(&kl2_dev->state, state);
    KL2_LOGD("set state to %s(%d)\n", kl2_device_state_str(state), state);
    // 移除原先此处的memset(excp board, 0)，将各个excp borad的清零挪到excp work的reset后
}

static int kl2_reinit_after_reset(struct kl2_device *kl2_dev, bool regular_timer_toggle, int mode)
{
    int ret      = 0;
    u32 old_cuen = kl2_dev->cuen;

    kl2_intc_disable_msi(kl2_dev);
    //kl2_misc_setup(kl2_dev);
    kl2_sse_init(kl2_dev);
    // 用户曾禁用某些cu，此处恢复禁用
    if (old_cuen != kl2_dev->default_cuen) {
        kl2_sse_cuen_update(kl2_dev, old_cuen);
    }
    kl2_compute_unit_init(kl2_dev);

    //kl2_plda_dma_init(kl2_dev, kl2_dev->spec.dmach_bits);
    kl2_intc_setup(kl2_dev);

    // 重新初始化gddr，由mode决定
    if (mode != 1) {
        ret = kl2_dev_reinit_gddr(kl2_dev);
        if (ret) {
            return ret;
        }
    }

    // 恢复regular timer, 包含温度功耗读取/pt_list重试下发/超时检测 ...
    if (regular_timer_toggle) {
        hrtimer_start(&kl2_dev->regular_timer, kl2_dev->regular_ktime, HRTIMER_MODE_REL);
    }
    return 0;
}

// mode0: reset noc, sdnn, cluster, video, A55, gddr     ... (except pcie, syscon, M3)
// mode1: reset noc, sdnn, cluster, video, A55           ... (except pcie, syscon, M3, gddr)
// mode2: reset noc, sdnn, cluster, video, A55, gddr, M3 ... (except pcie, syscon)
// XXX(liyunzheng): fast_reset = 1 用于重置ccix端口时的快速复位
int kl2_dev_soft_reset(struct kl2_device *kl2_dev, int mode, int fast_reset)
{
    struct kl2_userprocess *uproc;
    struct kl2_session     *sess;
    int                     pid, sess_id;
    int                     ret, hwq_id, i, old_state, old_in_reset_state;
    u32                     heartbeat0, heartbeat1;
    struct dma_engine      *dma        = &kl2_dev->dma_engine;
    int                     dma_ch_num = bitcount(kl2_dev->spec.dmach_bits);
    bool                    regular_timer_toggle;
    u32                     underway;
    ktime_t                 begin_ktime;
    bool                    hwq_underway_equals_zero_timeout = false;

    mutex_lock(&kl2_dev->big_global_lock);
    regular_timer_toggle = kl2_dev->regular_timer_toggle;
    mutex_unlock(&kl2_dev->big_global_lock);

    //old_state = atomic_cmpxchg(&kl2_dev->state, KL2_RUNNING, KL2_IN_RESET);
    //if (old_state != KL2_RUNNING) {
    //    old_state = atomic_cmpxchg(&kl2_dev->state, KL2_ERROR, KL2_IN_RESET);
    //    if (old_state != KL2_ERROR) {
    //        KL2_LOGW("state is neither KL2_RUNNING nor KL2_ERROR, "
    //                 "old_state= %s(%d), retry later ...\n",
    //                 kl2_device_state_str(old_state), old_state);
    //        return -EBUSY;
    //    }
    //}
    old_in_reset_state = atomic_cmpxchg(&kl2_dev->in_reset_state, 0, KL2_IN_RESET);
    if (old_in_reset_state != 0) {
        old_state = kl2_get_state(kl2_dev);
        KL2_LOGW("soft reset ongoing, state= %s(%d), in_reset_state= %s(%d)\n",
                 kl2_device_state_str(old_state), old_state,
                 kl2_device_state_str(old_in_reset_state), old_in_reset_state);
        return -EBUSY;
    }
    KL2_LOGI("reset begin ...\n");

    KL2_LOGI("kill all related processes ...\n");
    mutex_lock(&kl2_dev->uproc_session_lock);
    idr_for_each_entry(&kl2_dev->session_idr, sess, sess_id) {
        struct pid *pid_to_kill = sess->uproc->task_pid;
        if (pid_to_kill == task_tgid(current)) {
            continue;
        }

        KL2_LOGI("mark sess error(errno= XPUERR_DEVRESET), sess= %d, pid= %d, comm= %s ...\n",
                 sess_id, sess->uproc->pid, sess->uproc->comm);
        // 标记sess状态为TAINT hwq状态为TAINT，依赖timer+excp work实现状态清理
        kl2_session_taint(sess, XPUERR_DEVRESET);
        // uproc下不能创建新sess
        atomic_set(&sess->uproc->state, KL2_UPROC_ERROR);
        if (likely(sess->hwq)) {
            atomic_set(&sess->hwq->taint_state, KL2_HWQ_TAINT);
        }
    }
    mutex_unlock(&kl2_dev->uproc_session_lock);
    // 等待已有进程退出
    if (!fast_reset)
        msleep(500);
    else
        msleep(5);
    mutex_lock(&kl2_dev->uproc_session_lock);
    // 如仍有进程未退出，发送SIGKILL
    idr_for_each_entry(&kl2_dev->uproc_idr, uproc, pid) {
        struct pid *pid_to_kill = uproc->task_pid;
        if (pid_to_kill == task_tgid(current)) {
            KL2_LOGI("skip myself pid= %d, comm= %s ...\n", pid, uproc->comm);
            continue;
        }

        KL2_LOGI("send SIGKILL to pid= %d, comm= %s ...\n", pid, uproc->comm);
        kill_pid(pid_to_kill, SIGKILL, 1);
    }
    mutex_unlock(&kl2_dev->uproc_session_lock);
    // 等待已有进程退出
    if (!fast_reset)
        msleep(500);
    else
        msleep(5);

    // 禁止下发kernel和dma
    KL2_LOGI("flush dma(at most 20s), please wait ...\n");
    for (i = 0; i < dma_ch_num; ++i) {
        down(&dma->sema);
    }
    for (i = 0; i < dma_ch_num; ++i) {
        up(&dma->sema);
    }
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        struct kl2_hwq *hwq = &kl2_dev->hwq[hwq_id];
        unsigned long   flags;

        spin_lock_irqsave(&hwq->lock, flags);
        // do nothing
        spin_unlock_irqrestore(&hwq->lock, flags);
    }

    // 主动stall全部hwq
    kl2_sse_hwq_stall_all(kl2_dev);
    // 等待所有队列underway归零
    begin_ktime = ktime_get();
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        struct kl2_hwq *hwq = &kl2_dev->hwq[hwq_id];
        while (1) {
            kl2_reg_lock(kl2_dev);
            underway = kl2_sse_hwq_underway(hwq);
            kl2_reg_unlock(kl2_dev);
            // 尽早退出循环，节省时间
            if (!underway) {
                break;
            }

            // while循环退出条件，避免soft lockup
            if (ktime_to_ms(ktime_sub(ktime_get(), begin_ktime)) > 60000 /* 60s */) {
                KL2_LOGW("hwq_%d hwq_underway_equals_zero_timeout\n", hwq_id);
                hwq_underway_equals_zero_timeout = true;
                break;
            }
            udelay(100);
            cond_resched();
        }
    }
    if (hwq_underway_equals_zero_timeout) {
        KL2_LOGW("hwq_underway_equals_zero_timeout, but reset anyway ...\n");
    }

    // 关闭设备中断，isr不会再被调用
    kl2_intc_disable_msi(kl2_dev);
    // 等待isr全部结束
    synchronize_irq(kl2_dev->kdev->pdev->irq);
    // 等待可能残留kl2_handle_excp_work_func结束
    KL2_LOGI("flush handle_exception_work(at most 60s), please wait ...\n");
    flush_work(&kl2_dev->handle_exception_work);
    // 关闭regular timer, 包含温度功耗读取/pt_list重试下发/超时检测 ...
    if (regular_timer_toggle) {
        hrtimer_cancel(&kl2_dev->regular_timer);
    }
    flush_work(&kl2_dev->handle_exception_work);

    // 清理hwq/sess中的残留task
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        struct kl2_hwq  *hwq = &kl2_dev->hwq[hwq_id];
        unsigned long    flags;
        struct kl2_task *task, *safe;

        spin_lock_irqsave(&hwq->lock, flags);
        //x_list_move_all_tail(&hwq->ft_list, &hwq->pt_list);
        //x_list_move_all_tail(&hwq->ft_list, &hwq->rt_list);
        // task被强制清理到ft_list，需对应减少sess->unfinished_cnt，通知kl2_release尽快结束
        list_for_each_entry_safe(task, safe, &hwq->rt_list, hwq_node) {
            list_move_tail(&task->hwq_node, &hwq->ft_list);
            atomic_sub(1, &task->sess->unfinished_cnt);
        }
        list_for_each_entry_safe(task, safe, &hwq->pt_list, hwq_node) {
            list_move_tail(&task->hwq_node, &hwq->ft_list);
            atomic_sub(1, &task->sess->unfinished_cnt);
        }
        hwq->cnt_running = 0;
        hwq->cnt_all     = 0;
        spin_unlock_irqrestore(&hwq->lock, flags);

        queue_work(hwq->kl2_dev->hwq_wq, &hwq->finish_work);
        flush_work(&hwq->finish_work);
    }
    // 清理excp board
    memset(&kl2_dev->exception_board_hwq, 0, sizeof(kl2_dev->exception_board_hwq));
    memset(&kl2_dev->exception_board_cluster, 0, sizeof(kl2_dev->exception_board_cluster));
    memset(&kl2_dev->exception_board_sdnn, 0, sizeof(kl2_dev->exception_board_sdnn));

    // 等待noc空闲
    KL2_LOGI("flush noc(at most 20s), please wait ...\n");
    ret = kl_poll_cond_timeout((kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon0_base +
                                                           KL2_REG_SYSCON0_NOC_MAIN0_NO_PENDING) ==
                                0x3ffffffu),
                               10000, 10000000 /* 10s */);
    if (ret) {
        KL2_LOGW("noc_main0_pending, but reset anyway ...\n");
    }
    ret = kl_poll_cond_timeout((kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon1_base +
                                                           KL2_REG_SYSCON1_NOC_MAIN1_NO_PENDING) ==
                                0xffffffu),
                               10000, 10000000 /* 10s */);
    if (ret) {
        KL2_LOGW("noc_main1_pending, but reset anyway ...\n");
    }

    // 真正开始reset
    KL2_LOGI("send UserInt_0 to m3 ...\n");
    // reset期间禁止任何板卡访问，包括timer中寄存器操作
    kl2_reg_lock(kl2_dev);
    kl2_writel(kl2_dev, mode, kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_BUFFER0);
    kl2_writel(kl2_dev, BIT(13), kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD);
    kl2_writel(kl2_dev, BIT(0), kl2_dev->iomem_base.intc_base + KL2_REG_INTC_SET_7);

    KL2_LOGI("wait for reset done(at most 10s) ...\n");
    // XXX(miaotianxiang): msleep非常关键，reset完成前的时间窗口内noc不应有任何流量，否则可能死机
    if (!fast_reset)
        msleep(3000);
    else
        msleep(2000);
    kl2_reg_unlock(kl2_dev);
    ret = kl_poll_cond_timeout(
            ((kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD) & BIT(13)) ==
             0),
            10000, 10000000 /* 10s */);
    if (ret) {
        KL2_LOGW("reset timeout ...\n");
        goto err;
    }

    heartbeat0 = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_HEARTBEAT);
    msleep(1100);
    heartbeat1 = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_HEARTBEAT);
    if (heartbeat0 == heartbeat1) {
        KL2_LOGW("m3 stuck, it's weird, heartbeat0= %08x, heartbeat1= %08x ...\n", heartbeat0,
                 heartbeat1);
        // XXX(miaotianxiang): 某些版本FW修改过heartbeat更新频率，因此不将heartbeat停止更新作为失败原因
        //goto err;
    } else {
        KL2_LOGI("m3 alive ...\n");
    }

    ret = kl2_reinit_after_reset(kl2_dev, regular_timer_toggle, mode);
    if (ret) {
        KL2_LOGW("reinit failed ...\n");
        goto err;
    }

    KL2_LOGI("reset done ...\n");
    // 是否必要？？？
    kl2_sse_hwq_unstall_all(kl2_dev);
    atomic_set(&kl2_dev->in_reset_state, 0);
    kl2_set_state(kl2_dev, KL2_RUNNING);
    return 0;

err:
    kl2_dev->errno = XPUERR_DEVRESET;
    atomic_set(&kl2_dev->in_reset_state, 0);
    kl2_set_state(kl2_dev, KL2_ERROR);
    KL2_LOGE("something unexpected happened, hardware stuck now, "
             "please check/reset manually !!!\n");
    return -EBUSY;
}

int kl2_dev_reinit_gddr(struct kl2_device *kl2_dev)
{
    int ret;
    u32 l3_c0;

    KL2_LOGI("gddr reinit begin ...\n");
    kl2_writel(kl2_dev, BIT(14), kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD);
    kl2_writel(kl2_dev, 0xabcd2020 /* magic number */,
               kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_BUFFER0);
    kl2_writel(kl2_dev, BIT(0), kl2_dev->iomem_base.intc_base + KL2_REG_INTC_SET_7);
    ret = kl_poll_cond_timeout(
            ((kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD) & BIT(14)) ==
             0),
            10000, 10000000 /* 10s */);
    if (ret) {
        KL2_LOGW("gddr reinit timeout ...\n");
        return -EBUSY;
    }

    kl2_gddr_interrupt_mask_init(kl2_dev);

    l3_c0 = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_DEV_BUFFER0);
    KL2_LOGI("gddr reinit done, l3_c0= %08x ...\n", l3_c0);
    return 0;
}

const u32 KL2_R300_CCIX_TOPOLOGY[8] = {
    /* 32 bit divied into 4groups, each group represent one port */
    /*                      PORT3  PORT2  PORT1  PORT0  */
    0x02020808, /* dev0     BIT(1) BIT(1) BIT(3) BIT(3) */
    0x01012004, /* dev1     BIT(0) BIT(0) BIT(5) BIT(2) */
    0x08204002, /* dev2     BIT(3) BIT(5) BIT(6) BIT(1) */
    0x04100101, /* dev3     BIT(2) BIT(4) BIT(0) BIT(0) */
    0x20088080, /* dev4     BIT(5) BIT(3) BIT(7) BIT(7) */
    0x10040240, /* dev5     BIT(4) BIT(2) BIT(1) BIT(6) */
    0x80800420, /* dev6     BIT(7) BIT(7) BIT(2) BIT(5) */
    0x40401010, /* dev7     BIT(6) BIT(6) BIT(4) BIT(4) */
};

int kl2_ccix_link_speed_check(struct kl2_device *kl2_dev, u32 r300_bitmap)
{
    u32 ltssm_state = 0x10; // L0 state
    u32 speed       = 0;
    u32 test_in_ori, test_in_new, test_out_pcie;
    u32 card_id;
    int i;
    int ret = 0;

    if (kl2_dev->dev_info.board != KL2_BOARD_ID_R300) {
        return -EINVAL;
    }

    card_id = kl2_dev->ccix_info.card_id;
    if (card_id >= 8) {
        LOGW("invalid r300 card id %d...\n", card_id);
        return -EINVAL;
    }

    // check ltssm status
    for (i = 0; i < KL2_R300_CCIX_PORT_NUM; i++) {
        if (r300_bitmap & KL2_R300_CCIX_TOPOLOGY[card_id] & (0xff << i * 8)) {
            // set value of test_in [27:24] as 0x3 to choose test_out_pcie [127:96]
            // don't change config info in test_in [19:0]
            test_in_ori = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon1_base +
                                                     KL2_REG_SYSCON1_CCIX0_TEST_IN + i * 0x10);
            test_in_new = (0x3 << 24) | (test_in_ori & 0xFFFFF);
            kl2_writel(kl2_dev, test_in_new,
                       kl2_dev->iomem_base.syscon1_base + KL2_REG_SYSCON1_CCIX0_TEST_IN + i * 0x10);
            test_out_pcie = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon1_base +
                                                       KL2_REG_SYSCON1_CCIX0_TEST_OUT + i * 0x10);
            // restore test_in
            kl2_writel(kl2_dev, test_in_ori,
                       kl2_dev->iomem_base.syscon1_base + KL2_REG_SYSCON1_CCIX0_TEST_IN + i * 0x10);
            // check whether ltssm state is L0
            ltssm_state = ltssm_state & (test_out_pcie & 0x1F);
            if (ltssm_state != 0x10) {
                ret                                   = -EINVAL;
                kl2_dev->ccix_info.port_link_valid[i] = false;
            } else {
                // DVSEC, ESM Status Register 0x2C4
                speed = kl2_readl(kl2_dev,
                                  KL2_REG_CCIX_PORT_BASE(kl2_dev->iomem_base.ccix_base, i) +
                                          KL2_REG_CCIX_PCIE_DEVSEC_ESM_STATUS);
                if (speed == 0) {
                    speed = kl2_readl(kl2_dev,
                                      KL2_REG_CCIX_PORT_BASE(kl2_dev->iomem_base.ccix_base, i) +
                                              KL2_REG_CCIX_PCIE_BASIC_STATUS);
                    speed = (speed >> 8) & 0x1F;
                }
                kl2_dev->ccix_info.port_link_speed[i] = speed;
                kl2_dev->ccix_info.port_link_valid[i] = true;
                if (speed != KL2_CCIX_LINK_SPEED_ESM_25_0_GTS) {
                    ret = -EINVAL;
                }
            }
        } else {
            // remote device is not exist, port invalid
            kl2_dev->ccix_info.port_link_valid[i] = false;
        }
    }

    return ret;
}

int kl2_ccix_link_width_check(struct kl2_device *kl2_dev, u32 r300_bitmap)
{
    u32 ltssm_state = 0x10; // L0 state
    u32 test_in_ori, test_in_new, test_out_pcie;
    u32 link_width;
    u32 card_id;
    int i;
    int ret = 0;

    if (kl2_dev->dev_info.board != KL2_BOARD_ID_R300) {
        return -EINVAL;
    }

    card_id = kl2_dev->ccix_info.card_id;
    if (card_id >= 8) {
        LOGW("invalid r300 card id %d...\n", card_id);
        return -EINVAL;
    }

    // check ltssm status
    for (i = 0; i < KL2_R300_CCIX_PORT_NUM; i++) {
        if (r300_bitmap & KL2_R300_CCIX_TOPOLOGY[card_id] & (0xff << i * 8)) {
            // set value of test_in [27:24] as 0x3 to choose test_out_pcie [127:96]
            // don't change config info in test_in [19:0]
            test_in_ori = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon1_base +
                                                     KL2_REG_SYSCON1_CCIX0_TEST_IN + i * 0x10);
            test_in_new = (0x3 << 24) | (test_in_ori & 0xFFFFF);
            kl2_writel(kl2_dev, test_in_new,
                       kl2_dev->iomem_base.syscon1_base + KL2_REG_SYSCON1_CCIX0_TEST_IN + i * 0x10);
            test_out_pcie = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon1_base +
                                                       KL2_REG_SYSCON1_CCIX0_TEST_OUT + i * 0x10);
            // restore test_in
            kl2_writel(kl2_dev, test_in_ori,
                       kl2_dev->iomem_base.syscon1_base + KL2_REG_SYSCON1_CCIX0_TEST_IN + i * 0x10);
            // check whether ltssm state is L0
            ltssm_state = ltssm_state & (test_out_pcie & 0x1F);
            if (ltssm_state != 0x10) {
                ret                                   = -EINVAL;
                kl2_dev->ccix_info.port_link_valid[i] = false;
            } else {
                // Link Control, link_width Register 0x90
                link_width = kl2_readl(kl2_dev,
                                       KL2_REG_CCIX_PORT_BASE(kl2_dev->iomem_base.ccix_base, i) +
                                               KL2_REG_CCIX_PCIE_LINK_STATUS);
                link_width = (link_width >> KL2_CCIX_LINK_WIDTH_SHIFT) & KL2_CCIX_LINK_WIDTH_MASK;
                kl2_dev->ccix_info.port_link_width[i] = link_width;
                kl2_dev->ccix_info.port_link_valid[i] = true;
                if (link_width != 8) {
                    ret = -EINVAL;
                }
            }
        } else {
            kl2_dev->ccix_info.port_link_valid[i] = false;
        }
    }

    return ret;
}

static void kl2_ccix_link_width_sync_remote_port(struct kl2_device *kl2_dev, u32 local_id,
                                                 u32 local_port, struct kl2_device **r300_devs,
                                                 int r300_count)
{
    struct kl2_device *remote_kl2_dev = NULL;
    // (liyunzheng): R300 CCIX 端口连接遵从0<-->0，1<-->1，... ，因此remote_port = local_port
    u32 remote_id, remote_port = local_port;
    int i;

    remote_id = __ffs((KL2_R300_CCIX_TOPOLOGY[local_id] >> (local_port * 8)) & 0xff);
    for (i = 0; i < r300_count; i++) {
        if (r300_devs[i]->ccix_info.card_id == remote_id) {
            remote_kl2_dev = r300_devs[i];
            break;
        }
    }
    if (!remote_kl2_dev) {
        KL2_LOGW("remote_kl2_dev not found, local_id= %u, local_port= %u, remote_id= %u\n",
                 local_id, local_port, remote_id);
        return;
    }

    // 检测port远端的状态是否也不达标，如果达标强制设为不达标，进行port mode switch
    if (remote_kl2_dev->ccix_info.port_link_valid[remote_port] &&
        remote_kl2_dev->ccix_info.port_link_width[remote_port] == 8) {
        KL2_LOGI(
                "local/remote port link width not sync, it's weird, local_id= %u, local_port= %u, local_width= %u, remote_id= %u, remote_port= %u, remote_width= %u\n",
                local_id, local_port, kl2_dev->ccix_info.port_link_width[local_port], remote_id,
                remote_port, remote_kl2_dev->ccix_info.port_link_width[remote_port]);
        remote_kl2_dev->ccix_info.port_link_width[remote_port] = 0;
    }
}

// 遍历kl2_dev的端口，将 link_width ！= x8 的端口进行 RC <--> EP 的模式切换，并检测远端设备，确保两端均进行模式切换
int kl2_ccix_link_port_mode_switch_if_necessary(struct kl2_device *kl2_dev, u32 r300_bitmap,
                                                struct kl2_device **r300_devs, int r300_count)
{
    u32 card_id;
    int i;
    int ret = 0;

    if (kl2_dev->dev_info.board != KL2_BOARD_ID_R300) {
        return -EINVAL;
    }

    card_id = kl2_dev->ccix_info.card_id;
    if (card_id >= 8) {
        LOGW("invalid r300 card id %d...\n", card_id);
        return -EINVAL;
    }

    for (i = 0; i < KL2_R300_CCIX_PORT_NUM; i++) {
        if (r300_bitmap & KL2_R300_CCIX_TOPOLOGY[card_id] & (0xff << i * 8)) {
            if (kl2_dev->ccix_info.port_link_valid[i] &&
                kl2_dev->ccix_info.port_link_width[i] != 8) {
                kl2_ccix_link_width_sync_remote_port(kl2_dev, card_id, i, r300_devs, r300_count);
            }
        }
    }
    for (i = 0; i < KL2_R300_CCIX_PORT_NUM; i++) {
        if (r300_bitmap & KL2_R300_CCIX_TOPOLOGY[card_id] & (0xff << i * 8)) {
            if (kl2_dev->ccix_info.port_link_valid[i] &&
                kl2_dev->ccix_info.port_link_width[i] != 8) {
                kl2_writel(kl2_dev, 0x0 /* 0 -> switch port mode, 1 -> get port mode*/,
                           kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_BUFFER0);
                kl2_writel(kl2_dev, i /* port num */,
                           kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_BUFFER1);
                kl2_writel(kl2_dev, BIT(28), kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD);
                kl2_writel(kl2_dev, BIT(0), kl2_dev->iomem_base.intc_base + KL2_REG_INTC_SET_7);

                // 等待MCU命令执行完成
                ret = kl_poll_cond_timeout(
                        ((kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD) &
                          BIT(28)) == 0),
                        10000, 1000000 /* 1s */);
                if (ret) {
                    LOGW("%s port_mode_switch kl_poll_cond_timeout() failed, ret= %d\n",
                         kl2_dev->kdev->name, ret);
                    return ret;
                }
            }
        }
    }

    return ret;
}
