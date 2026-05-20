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
#include "kl2/kl2_regs.h"

// IRQ mapping
#define KL_IRQ_GENERAL 0
#define KL_IRQ_SSEQ(qid) (1 + (qid))

int kl2_intc_setup(struct kl2_device *kl2_dev)
{
    void __iomem *base;
    int           main_msi_vector = kl2_dev->main_msi_vector;

    base = kl2_dev->iomem_base.intc_base;

#if defined(ENABLE_MSIX)
    // Setup SSEQ mapping (MSIX)
    kl2_writel(kl2_dev, 1, base + KL2_REG_INTC_MSIX_MAP_SSEQ_0);
    kl2_writel(kl2_dev, 2, base + KL2_REG_INTC_MSIX_MAP_SSEQ_1);
    kl2_writel(kl2_dev, 3, base + KL2_REG_INTC_MSIX_MAP_SSEQ_2);
    kl2_writel(kl2_dev, 4, base + KL2_REG_INTC_MSIX_MAP_SSEQ_3);
    kl2_writel(kl2_dev, 5, base + KL2_REG_INTC_MSIX_MAP_SSEQ_4);
    kl2_writel(kl2_dev, 6, base + KL2_REG_INTC_MSIX_MAP_SSEQ_5);
    kl2_writel(kl2_dev, 7, base + KL2_REG_INTC_MSIX_MAP_SSEQ_6);
    kl2_writel(kl2_dev, 8, base + KL2_REG_INTC_MSIX_MAP_SSEQ_7);
    kl2_writel(kl2_dev, 9, base + KL2_REG_INTC_MSIX_MAP_SSEQ_8);
    kl2_writel(kl2_dev, 10, base + KL2_REG_INTC_MSIX_MAP_SSEQ_9);
    kl2_writel(kl2_dev, 11, base + KL2_REG_INTC_MSIX_MAP_SSEQ_10);
    kl2_writel(kl2_dev, 12, base + KL2_REG_INTC_MSIX_MAP_SSEQ_11);

    // Setup user int mapping (MSIX)
    kl2_writel(kl2_dev, 13, base + KL2_REG_INTC_MSIX_MAP_USER_INT_0);
    kl2_writel(kl2_dev, 13, base + KL2_REG_INTC_MSIX_MAP_USER_INT_1);
    kl2_writel(kl2_dev, 13, base + KL2_REG_INTC_MSIX_MAP_USER_INT_2);
    kl2_writel(kl2_dev, 13, base + KL2_REG_INTC_MSIX_MAP_USER_INT_3);
#else
    // 为避免出现VF中断失效, VF的SSEQ与user int使用vector 1触发中断
    // Setup SSEQ mapping (MSI)
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_0);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_1);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_2);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_3);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_4);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_5);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_6);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_7);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_8);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_9);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_10);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_SSEQ_11);

    // Setup user int mapping (MSI)
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_USER_INT_0);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_USER_INT_1);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_USER_INT_2);
    kl2_writel(kl2_dev, main_msi_vector, base + KL2_REG_INTC_MSI_MAP_USER_INT_3);
#endif

    // Setup MASK
    // sdnn int/excp
    kl2_intc_set_int_mask(kl2_dev, 0, 0xfffff555);
    // cluster int/excp
    kl2_intc_set_int_mask(kl2_dev, 1, 0xffff5555);
    // enable video decoder int
    kl2_intc_set_int_mask(kl2_dev, 2, 0xfffc0000);
    // enable video encoder int
    kl2_intc_set_int_mask(kl2_dev, 3, 0xffffffc0);
    // enable image proc int
    kl2_intc_set_int_mask(kl2_dev, 5, 0xffffffc0);
    // disable all edma int
    kl2_intc_set_int_mask(kl2_dev, 6, 0xffffffff);
    // several bits are enabled
    // [3]: user_int_3 down_up_clocking
    //
    // [0]: user_int_0 only_recved_by_m3
    // [1]: user_int_1 unused
    // [2]: user_int_2 unused
    kl2_intc_set_int_mask(kl2_dev, 7, 0xfffffff0);
    kl2_intc_set_host_mask(kl2_dev, 7, 0xfffffff7);
    // disable all aes int
    kl2_intc_set_int_mask(kl2_dev, 8, 0xffffffff);
    // disable all ccix int
    kl2_intc_set_int_mask(kl2_dev, 9, 0xffffffff);
    kl2_intc_set_int_mask(kl2_dev, 10, 0xffffffff);
    kl2_intc_set_int_mask(kl2_dev, 11, 0xffffffff);
    kl2_intc_set_int_mask(kl2_dev, 12, 0xffffffff);
    // several bits are enabled
    // [0]: gddr_int_blk0
    // [1]: gddr_int_blk1
    // [2]: gddr_int_blk2
    // [3]: gddr_int_blk3
    // [4]: gddr_int_blk4
    // [5]: gddr_int_blk5
    // [6]: gddr_int_blk6
    // [7]: gddr_int_blk7
    // [8]: noc_int_blk0
    // [9]: noc_int_blk1
    // [10]: noc_int_blk2
    // [11]: noc_int_blk3
    // [12]: sse_fault_int
    //
    // [18]: vmu_irq_blk0
    // [19]: vmu_irq_blk1
    // [20]: vmu_irq_blk2
    // [21]: vac_irq_blk0
    kl2_intc_set_int_mask(kl2_dev, 13, 0xffc3e000);
    // disable all pcie int except PRST/HotReset in intc source mask
    // disable all pcie int in intc host mask
    // [1] pcie_plrst_int
    kl2_intc_set_int_mask(kl2_dev, 14, 0xfffffffd);
    kl2_intc_set_host_mask(kl2_dev, 14, 0xffffffff);

#if defined(ENABLE_MSIX)
    kl2_writel(kl2_dev, 0, base + KL2_REG_INTC_MSI_EN);
    kl2_writel(kl2_dev, 1, base + KL2_REG_INTC_MSIX_EN);
#else
    kl2_writel(kl2_dev, 0, base + KL2_REG_INTC_MSIX_EN);
    kl2_writel(kl2_dev, 1, base + KL2_REG_INTC_MSI_EN);
#endif

    return 0;
}

void kl2_intc_unsetup(struct kl2_device *kl2_dev)
{
    void __iomem *base = kl2_dev->iomem_base.intc_base;

    // disable MSI
    kl2_writel(kl2_dev, 0, base + KL2_REG_INTC_MSI_EN);

    // disable MSIX
    kl2_writel(kl2_dev, 0, base + KL2_REG_INTC_MSIX_EN);
}

inline void kl2_intc_disable_msi(struct kl2_device *kl2_dev)
{
    kl2_writel(kl2_dev, 0, kl2_dev->iomem_base.intc_base + KL2_REG_INTC_MSI_EN);
}

inline void kl2_intc_enable_msi(struct kl2_device *kl2_dev)
{
    kl2_writel(kl2_dev, 1, kl2_dev->iomem_base.intc_base + KL2_REG_INTC_MSI_EN);
}

inline void kl2_intc_toggle_msi(struct kl2_device *kl2_dev, int irq)
{
    kl2_writel(kl2_dev, 0x1u << irq, kl2_dev->iomem_base.intc_base + KL2_REG_INTC_MSI_TOGGLE);
}

inline void kl2_intc_toggle_msix(struct kl2_device *kl2_dev, int irq)
{
    writeq(0x1ull << irq, kl2_dev->iomem_base.intc_base + KL2_REG_INTC_MSIX_TOGGLE);
}

inline u32 kl2_intc_status(struct kl2_device *kl2_dev, int idx)
{
    if (idx < 0 || idx > 14)
        return 0;
    return kl2_readl(kl2_dev, kl2_dev->iomem_base.intc_base + KL2_REG_INTC_STAT[idx]);
}
