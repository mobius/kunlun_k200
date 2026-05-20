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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_HW_H
#define BAIDU_XPU_RUNTIME_MODULE_KL2_HW_H

#include "kl2/kl2.h"
#include <linux/bitops.h>

#define for_each_cluster(x) for (x = 0; x < KL2_CLUSTER_MAX_COUNT; ++x)

#define for_each_valid_cluster(k, x)                                                               \
    for ((x) = 0; (x) < KL2_CLUSTER_MAX_COUNT; ++(x))                                              \
        if (unlikely(cluster_invalid((k), (x)))) {                                                 \
        } else

#define for_each_sdnn(x) for (x = 0; x < KL2_SDNN_MAX_COUNT; ++x)

#define for_each_valid_sdnn(k, x)                                                                  \
    for ((x) = 0; (x) < KL2_SDNN_MAX_COUNT; ++(x))                                                 \
        if (unlikely(sdnn_invalid((k), (x)))) {                                                    \
        } else

#define for_each_sse_queue(x) for (x = 0; x < KL2_HWQ_CNT; ++x)

#define for_each_valid_sse_queue(k, x)                                                             \
    for (x = 0; x < KL2_HWQ_CNT; ++x)                                                              \
        if (unlikely(hwq_invalid((k), (x)))) {                                                     \
        } else

#define NOC_MAIN0_PENDING (syscon0_base + 0x0C14)
#define NOC_MAIN1_PENDING (syscon1_base + 0x0288)
#define NOC_CPU_PENDING (syscon0_base + 0x0C18)
#define NOC_VIDEO_PENDING (syscon1_base + 0x0284)
// 26 bits
#define NOC_MAIN0_MASK (0x03FFFFFF)
// 24 bits
#define NOC_MAIN1_MASK (0x00FFFFFF)
// exclude M3
#define NOC_CPU_MASK (0x18)
// 18 bits
#define NOC_VIDEO_MASK (0x3FFFF)

#define WAIT_NOC_TIMEOUT (1000)

static inline int hwq_invalid(struct kl2_device *kl2_dev, int hwq_id)
{
    return test_bit(hwq_id, &kl2_dev->hwq_bitmap);
}

static inline int cluster_invalid(struct kl2_device *kl2_dev, int cluster_id)
{
    return (!((kl2_dev->default_cuen >> 18) & (0x1u << cluster_id)));
}

static inline int sdnn_invalid(struct kl2_device *kl2_dev, int cluster_id)
{
    return (!((kl2_dev->default_cuen >> 12) & (0x1u << cluster_id)));
}

void cluster_stop_dma(struct kl2_device *kl2_dev, int idx);
void sdnn_stop_dma(struct kl2_device *kl2_dev, int idx);
void cluster_reset(struct kl2_device *kl2_dev, int idx);
void sdnn_reset(struct kl2_device *kl2_dev, int idx);
int  wait_for_noc_cluster(struct kl2_device *kl2_dev, int idx);
int  wait_for_noc_sdnn(struct kl2_device *kl2_dev, int idx);

void cluster_clear_exception(struct kl2_device *kl2_dev, int idx, u32 mask);
void sdnn_cl_clear_exception(struct kl2_device *kl2_dev, int idx, u32 mask);
void sdnn_clear_exception(struct kl2_device *kl2_dev, int idx, u32 mask);

u32  kl2_intc_get_int_mask(struct kl2_device *kl2_dev, int i);
void kl2_intc_set_int_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_intc_int_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_intc_int_unmask(struct kl2_device *kl2_dev, int i, u32 unmask);

u32  kl2_intc_get_host_mask(struct kl2_device *kl2_dev, int i);
void kl2_intc_set_host_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_intc_host_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_intc_host_unmask(struct kl2_device *kl2_dev, int i, u32 unmask);

u32  kl2_cluster_get_excp_mask(struct kl2_device *kl2_dev, int i);
void kl2_cluster_set_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_cluster_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_cluster_excp_unmask(struct kl2_device *kl2_dev, int i, u32 unmask);

u32  kl2_sdnn_get_cl_excp_mask(struct kl2_device *kl2_dev, int i);
void kl2_sdnn_set_cl_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_sdnn_cl_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_sdnn_cl_excp_unmask(struct kl2_device *kl2_dev, int i, u32 unmask);

u32  kl2_sdnn_get_sd_excp_mask(struct kl2_device *kl2_dev, int i);
void kl2_sdnn_set_sd_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_sdnn_sd_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask);
void kl2_sdnn_sd_excp_unmask(struct kl2_device *kl2_dev, int i, u32 unmask);

u32  kl2_sse_get_cuen_cu_disable_mask(struct kl2_device *kl2_dev);
void kl2_sse_set_cuen_cu_disable_mask(struct kl2_device *kl2_dev, u32 mask);
void kl2_sse_excp_disable_clusters(struct kl2_device *kl2_dev, u32 mask);
void kl2_sse_excp_enable_clusters(struct kl2_device *kl2_dev, u32 mask);
void kl2_sse_excp_disable_sdnns(struct kl2_device *kl2_dev, u32 mask);
void kl2_sse_excp_enable_sdnns(struct kl2_device *kl2_dev, u32 mask);

void kl2_debug_master_enable(struct kl2_device *kl2_dev, u32 time_stamp, u32 stamp_interval,
                             u32 lost_interval, u64 sgdesc_addr, u32 port);
int  kl2_debug_master_disable(struct kl2_device *kl2_dev);
void kl2_debug_master_port_disable(struct kl2_device *kl2_dev);

#define RDMA_SRCPARAM 0x0
#define RDMA_DESTPARAM 0x4
#define RDMA_SRCADDR 0x8
#define RDMA_DESTADDR 0x10
#define RDMA_LENGTH 0x18
#define RDMA_CONTROL 0x1c
#define RDMA_STATUS 0x20
#define RDMA_PRC_LENGTH 0x24
#define RDMA_SHARE_ACCESS 0x28

#define PARAMSRC_PCIE 0x0
#define PARAMSRC_AXI 0x4

#define DIRECT_DMA_CTRL_REG_VAL (BIT(0) | BIT(5) | BIT(7))
#define SG_DMA_BASIC_CTRL_REG_VAL                                                                  \
    (BIT(0) | BIT(3) | BIT(7) | BIT(8) | BIT(9) | BIT(13) | BIT(23) | BIT(24) | BIT(25))
#define SG_DMA_ADVANCED_CTRL_REG_VAL                                                               \
    (BIT(0) | BIT(3) | BIT(7) | BIT(8) | BIT(9) | BIT(13) | BIT(23) | BIT(24) | BIT(25))

#endif
