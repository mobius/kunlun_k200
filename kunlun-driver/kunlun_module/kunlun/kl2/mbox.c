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

#include "mbox.h"
#include "kl2/kl2_regs.h"
#include "kl2/hw.h"
#include <linux/kthread.h>

#define KL2_SRIOV_MBOX_BAR_IDX 2
#define KL2_SRIOV_MBOX_SRCADDR_REG_OFFSET (KL2_REG_MAILBOX_BAR2_BASE + 0x0)
#define KL2_SRIOV_MBOX_DR0_OFFSET (KL2_REG_MAILBOX_BAR2_BASE + 0x24)
#define KL2_SRIOV_MBOX_DR1_OFFSET (KL2_REG_MAILBOX_BAR2_BASE + 0x28)
#define KL2_SRIOV_MBOX_DR2_OFFSET (KL2_REG_MAILBOX_BAR2_BASE + 0x2c)

#define KL2_SRIOV_MBOX_CNT 3
static int kl2_sriov_mbox_offset_tbl[KL2_SRIOV_MBOX_CNT] = {
    KL2_SRIOV_MBOX_DR0_OFFSET, // for vf0
    KL2_SRIOV_MBOX_DR1_OFFSET, // for vf1
    KL2_SRIOV_MBOX_DR2_OFFSET, // for vf2
};

static void kl2_sriov_mbox_clear_msg(struct kl2_device *kl2_dev, int mbox_idx)
{
    struct kl_device *kdev = kl2_dev->kdev;

    kl2_writel(kl2_dev, 0, kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + kl2_sriov_mbox_offset_tbl[mbox_idx]);
}

static int kl2_sriov_mbox_wait_status(struct kl2_device *kl2_dev, int mbox_idx, int status,
                                      int timeout_ms)
{
    struct kl_device      *kdev = kl2_dev->kdev;
    int                    cnt  = 0;
    int                    cur_status;
    volatile void __iomem *mbox_addr;

    mbox_addr  = kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + kl2_sriov_mbox_offset_tbl[mbox_idx];
    cur_status = KL2_SRIOV_MBOX_GET_STATUS(kl2_readl(kl2_dev, mbox_addr));

    while (cnt < timeout_ms * 20 && cur_status != status) {
        cur_status = KL2_SRIOV_MBOX_GET_STATUS(kl2_readl(kl2_dev, mbox_addr));
        udelay(50);
        cnt++;
    }

    if (cur_status != status) {
        return -ETIMEDOUT;
    }

    return 0;
}

static int kl2_sriov_mbox_read_msg(struct kl2_device *kl2_dev, int mbox_idx, u32 *msg)
{
    struct kl_device      *kdev = kl2_dev->kdev;
    volatile void __iomem *mbox_addr;

    mbox_addr = kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + kl2_sriov_mbox_offset_tbl[mbox_idx];
    *msg      = kl2_readl(kl2_dev, mbox_addr);

    return 0;
}

static int kl2_sriov_mbox_read_response(struct kl2_device *kl2_dev, int mbox_idx, u32 *msg,
                                        int timeout_ms)
{
    struct kl_device      *kdev = kl2_dev->kdev;
    volatile void __iomem *mbox_addr;
    int                    ret;

    mbox_addr = kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + kl2_sriov_mbox_offset_tbl[mbox_idx];
    ret       = kl2_sriov_mbox_wait_status(kl2_dev, mbox_idx, KL2_SRIOV_MBOX_STATUS_MSG_ACKED,
                                           timeout_ms);
    if (ret == 0) {
        *msg = kl2_readl(kl2_dev, mbox_addr);
    } else {
        KL2_LOGW("SRIOV mailbox read response timeout!\n");
        return ret;
    }

    return 0;
}

/* check if the specific the response of specific msg is acked */
static bool kl2_sriov_mbox_is_msg_responsed(struct kl2_device *kl2_dev, int mbox_idx, u32 msg)
{
    struct kl_device      *kdev = kl2_dev->kdev;
    volatile void __iomem *mbox_addr;
    int                    cur_msg;

    mbox_addr = kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + kl2_sriov_mbox_offset_tbl[mbox_idx];
    cur_msg   = kl2_readl(kl2_dev, mbox_addr);

    if (KL2_SRIOV_MBOX_GET_MSG_TYPE(cur_msg) == KL2_SRIOV_MBOX_GET_MSG_TYPE(msg) &&
        KL2_SRIOV_MBOX_GET_STATUS(cur_msg) == KL2_SRIOV_MBOX_STATUS_MSG_ACKED)
        return true;

    return false;
}

static int kl2_sriov_mbox_write_msg(struct kl2_device *kl2_dev, int mbox_idx, u32 msg,
                                    int timeout_ms)
{
    struct kl_device      *kdev = kl2_dev->kdev;
    volatile void __iomem *mbox_addr;
    int                    ret;
    unsigned long          flags;

    spin_lock_irqsave(&kl2_dev->sriov_mbox_lock, flags);
    ret = kl2_sriov_mbox_wait_status(kl2_dev, mbox_idx, KL2_SRIOV_MBOX_STATUS_EMPTY, timeout_ms);
    if (ret != 0) {
        KL2_LOGW("SRIOV mailbox is busy now!\n");
        spin_unlock_irqrestore(&kl2_dev->sriov_mbox_lock, flags);
        return ret;
    }
    KL2_SRIOV_MBOX_SET_STATUS(msg, KL2_SRIOV_MBOX_STATUS_MSG_NEW);
    mbox_addr = kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + kl2_sriov_mbox_offset_tbl[mbox_idx];
    kl2_writel(kl2_dev, msg, mbox_addr);
    spin_unlock_irqrestore(&kl2_dev->sriov_mbox_lock, flags);

    return 0;
}

static int kl2_sriov_mbox_write_response(struct kl2_device *kl2_dev, int mbox_idx, u32 msg)
{
    struct kl_device      *kdev = kl2_dev->kdev;
    volatile void __iomem *mbox_addr;

    KL2_SRIOV_MBOX_SET_STATUS(msg, KL2_SRIOV_MBOX_STATUS_MSG_ACKED);
    mbox_addr = kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + kl2_sriov_mbox_offset_tbl[mbox_idx];
    kl2_writel(kl2_dev, msg, mbox_addr);

    return 0;
}

int kl2_sriov_mbox_reset_cluster_vf(struct kl2_device *kl2_dev, int user_id, int vf_cluster_id,
                                    int timeout_ms)
{
    u32 msg = 0;

    KL2_SRIOV_MBOX_SET_MSG_TYPE(msg, KL2_SRIOV_MSGTYPE_RESET_CLUSTER);
    KL2_SRIOV_MBOX_SET_SRC_USER_ID(msg, user_id);
    KL2_SRIOV_MBOX_SET_CLUSTER_ID(msg, vf_cluster_id);

    kl2_sriov_mbox_write_msg(kl2_dev, user_id, msg, timeout_ms);
    kl2_sriov_mbox_read_response(kl2_dev, user_id, &msg, timeout_ms);
    kl2_sriov_mbox_clear_msg(kl2_dev, user_id);

    return 0;
}

int kl2_sriov_mbox_reset_sdnn_vf(struct kl2_device *kl2_dev, int user_id, int vf_sdnn_id,
                                 int timeout_ms)
{
    u32 msg = 0;

    KL2_SRIOV_MBOX_SET_MSG_TYPE(msg, KL2_SRIOV_MSGTYPE_RESET_SDNN);
    KL2_SRIOV_MBOX_SET_SRC_USER_ID(msg, user_id);
    KL2_SRIOV_MBOX_SET_SDNN_ID(msg, vf_sdnn_id);

    kl2_sriov_mbox_write_msg(kl2_dev, user_id, msg, timeout_ms);
    kl2_sriov_mbox_read_response(kl2_dev, user_id, &msg, timeout_ms);
    kl2_sriov_mbox_clear_msg(kl2_dev, user_id);

    return 0;
}

int kl2_sriov_mbox_request_cluster_hwqid_vf(struct kl2_device *kl2_dev, int user_id,
                                            int vf_cluster_id, int timeout_ms)
{
    u32 msg = 0;
    int hwq_id;

    KL2_SRIOV_MBOX_SET_MSG_TYPE(msg, KL2_SRIOV_MSGTYPE_REQ_CLUSTER_HWQ_ID);
    KL2_SRIOV_MBOX_SET_SRC_USER_ID(msg, user_id);
    KL2_SRIOV_MBOX_SET_CLUSTER_ID(msg, vf_cluster_id);

    kl2_sriov_mbox_write_msg(kl2_dev, user_id, msg, timeout_ms);
    kl2_sriov_mbox_read_response(kl2_dev, user_id, &msg, timeout_ms);
    hwq_id = KL2_SRIOV_MBOX_GET_HWQ_ID(msg);
    kl2_sriov_mbox_clear_msg(kl2_dev, user_id);

    return hwq_id;
}

int kl2_sriov_mbox_request_sdnn_hwqid_vf(struct kl2_device *kl2_dev, int user_id, int vf_sdnn_id,
                                         int timeout_ms)
{
    u32 msg = 0;
    int hwq_id;

    KL2_SRIOV_MBOX_SET_MSG_TYPE(msg, KL2_SRIOV_MSGTYPE_REQ_SDNN_HWQ_ID);
    KL2_SRIOV_MBOX_SET_SRC_USER_ID(msg, user_id);
    KL2_SRIOV_MBOX_SET_SDNN_ID(msg, vf_sdnn_id);

    kl2_sriov_mbox_write_msg(kl2_dev, user_id, msg, timeout_ms);
    kl2_sriov_mbox_read_response(kl2_dev, user_id, &msg, timeout_ms);
    hwq_id = KL2_SRIOV_MBOX_GET_HWQ_ID(msg);
    kl2_sriov_mbox_clear_msg(kl2_dev, user_id);

    return hwq_id;
}

static u32 make_vf_xpu_busy_status(struct kl2_device *kl2_dev, int user_id,
                                   u32 phy_sse_xpu_busy_status)
{
    u32 cluster_num_per_user = kl2_sse_get_cluster_number(kl2_dev, 0);
    u32 sdnn_num_per_user    = kl2_sse_get_sdnn_number(kl2_dev, 0);
    u32 vf_sse_xpu_busy_status;
    u32 vf_cluster_status;
    u32 vf_sdnn_status;

    vf_cluster_status = (phy_sse_xpu_busy_status & 0xff) >> (user_id * cluster_num_per_user);
    vf_sdnn_status    = (phy_sse_xpu_busy_status & 0x3f00) >> (user_id * sdnn_num_per_user);

    vf_sse_xpu_busy_status = vf_sdnn_status | vf_cluster_status;

    return vf_sse_xpu_busy_status;
}

/* If the busy status in mailbox is ready, this func will return immediately, if not,
 * you need to call this func again to get xpu busy status. It may take more 10ms to
 * get xpu_busy_status from PF for VF.
 */
int kl2_sriov_mbox_req_xpu_busy_status_vf(struct kl2_device *kl2_dev, int user_id,
                                          u32 *sse_xpu_busy_status)
{
    u32 msg = 0;
    int ret;

    KL2_SRIOV_MBOX_SET_MSG_TYPE(msg, KL2_SRIOV_MSGTYPE_REQ_XPU_BUSY_STATUS);
    KL2_SRIOV_MBOX_SET_SRC_USER_ID(msg, user_id);

    if (kl2_sriov_mbox_is_msg_responsed(kl2_dev, user_id, msg)) {
        kl2_sriov_mbox_read_response(kl2_dev, user_id, &msg, 50);
        kl2_sriov_mbox_clear_msg(kl2_dev, user_id);
        *sse_xpu_busy_status = KL2_SRIOV_MBOX_GET_XPU_BUSY_STATUS(msg);
        ret                  = 0;
    } else {
        kl2_sriov_mbox_write_msg(kl2_dev, user_id, msg, 50);
        ret = -EAGAIN;
    }

    return ret;
}

int kl2_sriov_mbox_response_xpu_busy_status_pf(struct kl2_device *kl2_dev, int msg)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    u32           user_id;
    u32           phy_sse_xpu_busy_status = 0x0;
    u32           vf_sse_xpu_busy_status  = 0x0;

    user_id = KL2_SRIOV_MBOX_GET_SRC_USER_ID(msg);

    phy_sse_xpu_busy_status = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_XPU_BUSY_STATUS);
    vf_sse_xpu_busy_status  = make_vf_xpu_busy_status(kl2_dev, user_id, phy_sse_xpu_busy_status);
    KL2_LOGD("Phy xpu_busy_status[0x%x], vf xpu_busy_status[0x%x]\n", phy_sse_xpu_busy_status,
             vf_sse_xpu_busy_status);

    KL2_SRIOV_MBOX_SET_XPU_BUSY_STATUS(msg, vf_sse_xpu_busy_status);

    kl2_sriov_mbox_write_response(kl2_dev, user_id, msg);

    return 0;
}

int kl2_sriov_mbox_reset_cluster_pf(struct kl2_device *kl2_dev, int msg)
{
    u32 user_id;
    u32 vf_cluster_id;
    u32 phy_cluster_id;
    u32 cluster_num_per_user = kl2_sse_get_cluster_number(kl2_dev, 0);

    user_id        = KL2_SRIOV_MBOX_GET_SRC_USER_ID(msg);
    vf_cluster_id  = KL2_SRIOV_MBOX_GET_CLUSTER_ID(msg);
    phy_cluster_id = vf_cluster_id + user_id * cluster_num_per_user;

    wait_for_noc_cluster(kl2_dev, phy_cluster_id);
    udelay(10);
    cluster_reset(kl2_dev, phy_cluster_id);
    KL2_LOGD("Received msg[0x%x] from vf[%d], reset vf cluster[%d]\n", msg, user_id, vf_cluster_id);

    kl2_sriov_mbox_write_response(kl2_dev, user_id, msg);

    return 0;
}

int kl2_sriov_mbox_reset_sdnn_pf(struct kl2_device *kl2_dev, int msg)
{
    u32 user_id;
    u32 vf_sdnn_id;
    u32 phy_sdnn_id;
    u32 sdnn_num_per_user = kl2_sse_get_sdnn_number(kl2_dev, 0);

    user_id     = KL2_SRIOV_MBOX_GET_SRC_USER_ID(msg);
    vf_sdnn_id  = KL2_SRIOV_MBOX_GET_SDNN_ID(msg);
    phy_sdnn_id = vf_sdnn_id + user_id * sdnn_num_per_user;

    wait_for_noc_sdnn(kl2_dev, phy_sdnn_id);
    udelay(10);
    sdnn_reset(kl2_dev, phy_sdnn_id);
    KL2_LOGD("Received msg[0x%x] from vf[%d], reset vf sdnn[%d]\n", msg, user_id, vf_sdnn_id);

    kl2_sriov_mbox_write_response(kl2_dev, user_id, msg);

    return 0;
}

int kl2_sriov_mbox_response_cluster_hwqid(struct kl2_device *kl2_dev, int msg)
{
    u32 user_id;
    u32 vf_cluster_id;
    u32 phy_cluster_id;
    u32 hwq_id;
    u32 hwq_num_per_user     = kl2_sse_get_hwq_number(kl2_dev, 0);
    u32 cluster_num_per_user = kl2_sse_get_cluster_number(kl2_dev, 0);

    user_id        = KL2_SRIOV_MBOX_GET_SRC_USER_ID(msg);
    vf_cluster_id  = KL2_SRIOV_MBOX_GET_CLUSTER_ID(msg);
    phy_cluster_id = vf_cluster_id + user_id * cluster_num_per_user;
    hwq_id         = kl2_sse_cluster_map_hwq(kl2_dev, user_id, phy_cluster_id);
    KL2_LOGD("Get cluster phy hwq_id[%d] for phy cluster[%d]\n", hwq_id, phy_cluster_id);
    KL2_SRIOV_MBOX_SET_HWQ_ID(msg, hwq_id - (user_id * hwq_num_per_user));

    kl2_sriov_mbox_write_response(kl2_dev, user_id, msg);

    return 0;
}

int kl2_sriov_mbox_response_sdnn_hwqid(struct kl2_device *kl2_dev, int msg)
{
    u32 user_id;
    u32 vf_sdnn_id;
    u32 phy_sdnn_id;
    u32 hwq_id;
    u32 hwq_num_per_user  = kl2_sse_get_hwq_number(kl2_dev, 0);
    u32 sdnn_num_per_user = kl2_sse_get_sdnn_number(kl2_dev, 0);

    user_id     = KL2_SRIOV_MBOX_GET_SRC_USER_ID(msg);
    vf_sdnn_id  = KL2_SRIOV_MBOX_GET_SDNN_ID(msg);
    phy_sdnn_id = vf_sdnn_id + user_id * sdnn_num_per_user;
    hwq_id      = kl2_sse_sdnn_map_hwq(kl2_dev, user_id, phy_sdnn_id);
    KL2_LOGD("Get sdnn phy hwq_id[%d] for phy sdnn[%d]\n", hwq_id, phy_sdnn_id);
    KL2_SRIOV_MBOX_SET_HWQ_ID(msg, hwq_id - (user_id * hwq_num_per_user));

    kl2_sriov_mbox_write_response(kl2_dev, user_id, msg);

    return 0;
}

static int kl2_sriov_mbox_check_msg(struct kl2_device *kl2_dev)
{
    u32 msg;
    u16 msg_type;
    int i;

    for (i = 0; i < KL2_SRIOV_MBOX_CNT; i++) {
        kl2_sriov_mbox_read_msg(kl2_dev, i, &msg);
        if (KL2_SRIOV_MBOX_GET_STATUS(msg) != KL2_SRIOV_MBOX_STATUS_MSG_NEW) {
            continue;
        }

        msg_type = KL2_SRIOV_MBOX_GET_MSG_TYPE(msg);

        KL2_LOGD("Check mailbox[%d], msg type: %d", i, msg_type);

        switch (msg_type) {
        case KL2_SRIOV_MSGTYPE_RESET_CLUSTER:
            kl2_sriov_mbox_reset_cluster_pf(kl2_dev, msg);
            break;
        case KL2_SRIOV_MSGTYPE_RESET_SDNN:
            kl2_sriov_mbox_reset_sdnn_pf(kl2_dev, msg);
            break;
        case KL2_SRIOV_MSGTYPE_REQ_CLUSTER_HWQ_ID:
            kl2_sriov_mbox_response_cluster_hwqid(kl2_dev, msg);
            break;
        case KL2_SRIOV_MSGTYPE_REQ_SDNN_HWQ_ID:
            kl2_sriov_mbox_response_sdnn_hwqid(kl2_dev, msg);
            break;
        case KL2_SRIOV_MSGTYPE_REQ_XPU_BUSY_STATUS:
            kl2_sriov_mbox_response_xpu_busy_status_pf(kl2_dev, msg);
            break;
        default:
            break;
        }
    }

    return 0;
}

static int kl2_sriov_mbox_kthread_fn(void *args)
{
    struct kl2_device *kl2_dev       = args;
    int                sriov_func_id = kl2_dev->dev_info.sriov_func_id;

    while (!kthread_should_stop()) {
        if (is_pf_id(sriov_func_id)) {
            kl2_sriov_mbox_check_msg(kl2_dev);
        }

        msleep(10);
    }

    return 0;
}

void kl2_sriov_mbox_init(struct kl2_device *kl2_dev)
{
    spin_lock_init(&kl2_dev->sriov_mbox_lock);
}

void kl2_sriov_mbox_enable_pf(struct kl2_device *kl2_dev)
{
    struct kl_device *kdev = kl2_dev->kdev;

    /* make mailbox data register writable */
    kl2_writel(kl2_dev, 1, kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + KL2_SRIOV_MBOX_SRCADDR_REG_OFFSET);

    kl2_dev->sriov_mbox_kthread =
            kthread_create(kl2_sriov_mbox_kthread_fn, kl2_dev, "kl2_sriov/%d", kl2_dev->kdev->idx);
    if (IS_ERR(kl2_dev->sriov_mbox_kthread)) {
        kl2_dev->sriov_mbox_kthread = NULL;
        KL2_LOGW("Create sriov mailbox kthread failed!\n");
        return;
    }

    wake_up_process(kl2_dev->sriov_mbox_kthread);
}

void kl2_sriov_mbox_disable_pf(struct kl2_device *kl2_dev)
{
    struct kl_device *kdev = kl2_dev->kdev;

    kthread_stop(kl2_dev->sriov_mbox_kthread);

    /* clear src addr register, and mailbox dr0~2 and doorbell
     * reg will be cleared automatically.
     */
    kl2_writel(kl2_dev, 0, kdev->bar[KL2_SRIOV_MBOX_BAR_IDX] + KL2_SRIOV_MBOX_SRCADDR_REG_OFFSET);
}
