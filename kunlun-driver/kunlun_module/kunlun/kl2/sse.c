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
#include "kl2/kl2_regs.h"
#include "kl2/hw.h"
#include <linux/io.h>

int kl2_sse_init(struct kl2_device *kl2_dev)
{
    void __iomem           *base = kl2_dev->iomem_base.sse_base;
    u32                     cl_disable_mask, sd_disable_mask;
    int                     hwq_id;
    union kl2_sse_task_desc rec0 = { { 0 } };

    if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_SRIOV_OFF ||
        is_pf_id(kl2_dev->dev_info.sriov_func_id)) {
#ifdef PLATFORM_KUNLUN
        // otp info is merged to kl2_dev->spec struct in kl2_get_df_spec
        kl2_dev->default_cuen = (kl2_dev->spec.cl_bits << 18) | (kl2_dev->spec.sdnn_bits << 12);
#elif defined PLATFORM_PLD
        kl2_dev->default_cuen = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon0_base + 0x000c);
#else
#error "Fix me !!!"
#endif

        // cluster base
        writeq(KL2_REG_CLUSTER0_NOC_BASE, base + KL2_REG_SSE_CLUSTER0_BASE_ADDR_L);
        writeq(KL2_REG_CLUSTER1_NOC_BASE, base + KL2_REG_SSE_CLUSTER1_BASE_ADDR_L);
        writeq(KL2_REG_CLUSTER2_NOC_BASE, base + KL2_REG_SSE_CLUSTER2_BASE_ADDR_L);
        writeq(KL2_REG_CLUSTER3_NOC_BASE, base + KL2_REG_SSE_CLUSTER3_BASE_ADDR_L);
        writeq(KL2_REG_CLUSTER4_NOC_BASE, base + KL2_REG_SSE_CLUSTER4_BASE_ADDR_L);
        writeq(KL2_REG_CLUSTER5_NOC_BASE, base + KL2_REG_SSE_CLUSTER5_BASE_ADDR_L);
        writeq(KL2_REG_CLUSTER6_NOC_BASE, base + KL2_REG_SSE_CLUSTER6_BASE_ADDR_L);
        writeq(KL2_REG_CLUSTER7_NOC_BASE, base + KL2_REG_SSE_CLUSTER7_BASE_ADDR_L);

        // sdnn base
        writeq(KL2_REG_SDNN_CLUSTER0_NOC_BASE, base + KL2_REG_SSE_SDNN0_BASE_ADDR_L);
        writeq(KL2_REG_SDNN_CLUSTER1_NOC_BASE, base + KL2_REG_SSE_SDNN1_BASE_ADDR_L);
        writeq(KL2_REG_SDNN_CLUSTER2_NOC_BASE, base + KL2_REG_SSE_SDNN2_BASE_ADDR_L);
        writeq(KL2_REG_SDNN_CLUSTER3_NOC_BASE, base + KL2_REG_SSE_SDNN3_BASE_ADDR_L);
        writeq(KL2_REG_SDNN_CLUSTER4_NOC_BASE, base + KL2_REG_SSE_SDNN4_BASE_ADDR_L);
        writeq(KL2_REG_SDNN_CLUSTER5_NOC_BASE, base + KL2_REG_SSE_SDNN5_BASE_ADDR_L);
    } else if (is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
        kl2_dev->default_cuen = (kl2_dev->spec.cl_bits << 18) | (kl2_dev->spec.sdnn_bits << 12);
    } else {
        KL2_LOGW("invalid dev_info.sriov_func_id= %d\n", kl2_dev->dev_info.sriov_func_id);
        return -EINVAL;
    }

    kl2_dev->cuen   = kl2_dev->default_cuen;
    cl_disable_mask = ~((kl2_dev->cuen >> 18) & kl2_dev->spec.cl_bits) & 0xffu;
    sd_disable_mask = ~((kl2_dev->cuen >> 12) & kl2_dev->spec.sdnn_bits) & 0x3fu;
    kl2_sse_set_cuen_cu_disable_mask(kl2_dev, (sd_disable_mask << 16) | cl_disable_mask);
    KL2_LOGI(
            "cuen= %08x(cl_bits << 18 | sdnn_bits << 12= %08x << 18 | %08x << 12), cl_disable_mask= %08x, sd_disable_mask= %08x, cuen_cu_disable_mask= %08x\n",
            kl2_dev->cuen, kl2_dev->spec.cl_bits, kl2_dev->spec.sdnn_bits, cl_disable_mask,
            sd_disable_mask, kl2_sse_get_cuen_cu_disable_mask(kl2_dev));

    rec0.ctrl.type       = KL2_SSE_TASKTYPE_EVNTREC;
    rec0.ctrl.record_seq = 0;
    for (hwq_id = 0; hwq_id < bitcount(kl2_dev->spec.hwq_bits); ++hwq_id) {
        void __iomem *qbase = KL2_REG_SSE_QDESC_BASE(base, hwq_id);
        writeq(rec0.r64[0], qbase + KL2_REG_SSE_QDESC_0);
        writeq(rec0.r64[1], qbase + KL2_REG_SSE_QDESC_2);
        writeq(rec0.r64[2], qbase + KL2_REG_SSE_QDESC_4);
        writeq(rec0.r64[3], qbase + KL2_REG_SSE_QDESC_6);
    }

    return 0;
}

void kl2_sse_cuen_update(struct kl2_device *kl2_dev, u32 new_cuen)
{
    u32 cl_disable_mask, sd_disable_mask;

    kl2_dev->cuen   = new_cuen & kl2_dev->default_cuen;
    cl_disable_mask = ~((kl2_dev->cuen >> 18) & kl2_dev->spec.cl_bits) & 0xffu;
    sd_disable_mask = ~((kl2_dev->cuen >> 12) & kl2_dev->spec.sdnn_bits) & 0x3fu;
    kl2_sse_set_cuen_cu_disable_mask(kl2_dev, (sd_disable_mask << 16) | cl_disable_mask);
    KL2_LOGI(
            "cuen_update, cuen= %08x(new_cuen & default_cuen= %08x & %08x), cl_disable_mask= %08x, sd_disable_mask= %08x, cuen_cu_disable_mask= %08x\n",
            kl2_dev->cuen, new_cuen, kl2_dev->default_cuen, cl_disable_mask, sd_disable_mask,
            kl2_sse_get_cuen_cu_disable_mask(kl2_dev));
}

#include "kl2/disable_reg_debug.h"
void kl2_sse_write_desc_locked(struct kl2_device *kl2_dev, void __iomem *sse_base,
                               union kl2_sse_task_desc *desc, int hwq_id)
{
    void __iomem           *base;
    union kl2_sse_task_desc wait = { { 0 } };

    base = KL2_REG_SSE_QDESC_BASE(sse_base, hwq_id);

    wait.ctrl.type  = KL2_SSE_TASKTYPE_INTR;
    wait.ctrl.token = desc->kernel.token + 1;

    writeq(desc->r64[0], base + KL2_REG_SSE_QDESC_0);
    writeq(desc->r64[1], base + KL2_REG_SSE_QDESC_2);
    writeq(desc->r64[2], base + KL2_REG_SSE_QDESC_4);
    writeq(desc->r64[3], base + KL2_REG_SSE_QDESC_6);

    KL2_LOGD("sse q_%d %x %x %x %x %x %x %x %x\n", hwq_id, desc->r32[0], desc->r32[1], desc->r32[2],
             desc->r32[3], desc->r32[4], desc->r32[5], desc->r32[6], desc->r32[7]);

    writeq(wait.r64[0], base + KL2_REG_SSE_QDESC_0);
    writeq(wait.r64[1], base + KL2_REG_SSE_QDESC_2);
    writeq(wait.r64[2], base + KL2_REG_SSE_QDESC_4);
    writeq(wait.r64[3], base + KL2_REG_SSE_QDESC_6);
}
#include "kl2/enable_reg_debug.h"

int kl2_sse_hwq_intr_count(struct kl2_hwq *hwq)
{
    struct kl2_device *kl2_dev __maybe_unused = hwq->kl2_dev;
    return kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(hwq->kl2_dev->iomem_base.sse_base, hwq->id) +
                                      KL2_REG_SSE_QSTATUS_INTR_CNT);
}

u32 kl2_sse_hwq_last_cycles(struct kl2_hwq *hwq)
{
    struct kl2_device *kl2_dev __maybe_unused = hwq->kl2_dev;
    return kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(hwq->kl2_dev->iomem_base.sse_base, hwq->id) +
                                      KL2_REG_SSE_QSTATUS_TASK_TMR);
}

u32 kl2_sse_hwq_underway(struct kl2_hwq *hwq)
{
    struct kl2_device *kl2_dev __maybe_unused = hwq->kl2_dev;
    u32                        val            = kl2_readl(kl2_dev,
                                                          KL2_REG_SSE_QSTATUS_BASE(hwq->kl2_dev->iomem_base.sse_base, hwq->id) +
                                                                  KL2_REG_SSE_QSTATUS_TASK_RP_CNT);
    return (val >> 16);
}

void kl2_sse_hwq_reset(struct kl2_device *kl2_dev, int hwq_id)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    kl2_writel(kl2_dev, 1, KL2_REG_SSE_QCTRL_BASE(sse_base, hwq_id) + KL2_REG_SSE_QCTRL_RESET);
    udelay(10);
    KL2_LOGD("reset hwq_%d\n", hwq_id);
}

void kl2_sse_hwq_stall(struct kl2_device *kl2_dev, int hwq_id)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    kl2_writel(kl2_dev, 1, KL2_REG_SSE_QCTRL_BASE(sse_base, hwq_id) + KL2_REG_SSE_QCTRL_STALL);
}

void kl2_sse_hwq_stall_all(struct kl2_device *kl2_dev)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    int           hwq_id;

    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        kl2_writel(kl2_dev, 1, KL2_REG_SSE_QCTRL_BASE(sse_base, hwq_id) + KL2_REG_SSE_QCTRL_STALL);
    }
}

void kl2_sse_hwq_unstall(struct kl2_device *kl2_dev, int hwq_id)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    kl2_writel(kl2_dev, 0, KL2_REG_SSE_QCTRL_BASE(sse_base, hwq_id) + KL2_REG_SSE_QCTRL_STALL);
}

void kl2_sse_hwq_unstall_all(struct kl2_device *kl2_dev)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    int           hwq_id;

    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        kl2_writel(kl2_dev, 0, KL2_REG_SSE_QCTRL_BASE(sse_base, hwq_id) + KL2_REG_SSE_QCTRL_STALL);
    }
}

void kl2_sse_cluster_force_done(struct kl2_device *kl2_dev, int cl_id)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    kl2_writel(kl2_dev, (1u << cl_id), sse_base + KL2_REG_SSE_USER0_XPU_FORCE_DONE);
}

void kl2_sse_sdnn_force_done(struct kl2_device *kl2_dev, int sdnn_id)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    kl2_writel(kl2_dev, (1u << (sdnn_id + 16)), sse_base + KL2_REG_SSE_USER0_XPU_FORCE_DONE);
}

u32 kl2_sse_cluster_map_hwq(struct kl2_device *kl2_dev, u32 user, u32 cl_id)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    u32 vmap = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_CLUSTER_MAPPING_LIST + (0x8 * user));
    return (vmap >> (cl_id << 2)) & 0xfu;
}

u32 kl2_sse_sdnn_map_hwq(struct kl2_device *kl2_dev, u32 user, u32 sdnn_id)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    u32 vmap = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_SDNN_MAPPING_LIST + (0x8 * user));
    return (vmap >> (sdnn_id << 2)) & 0xfu;
}

u32 kl2_sse_get_cluster_number(struct kl2_device *kl2_dev, u32 user)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    u32           user_xpu_info =
            kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_SCHEDULER_XPU_INFO + (0x10 * user));
    return user_xpu_info & 0xfu;
}

u32 kl2_sse_get_sdnn_number(struct kl2_device *kl2_dev, u32 user)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    u32           user_xpu_info =
            kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_SCHEDULER_XPU_INFO + (0x10 * user));
    return (user_xpu_info >> 16) & 0xfu;
}

u32 kl2_sse_get_hwq_number(struct kl2_device *kl2_dev, u32 user)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    u32           user_queue_info =
            kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_STREAM_QUEUE_INFO + (0x10 * user));
    return user_queue_info & 0xfu;
}
