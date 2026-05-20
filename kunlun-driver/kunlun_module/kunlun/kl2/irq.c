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

//#define DEBUG

#include "kl2/kl2.h"
#include "kl2/hw.h"
#include "kl2/kl2_regs.h"
#include "kl2/exception.h"

#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/version.h>

// 用于msix
static irqreturn_t irq_general(int irq, void *instance)
{
    //LOGD("irq_%d received\n", irq);
    return IRQ_RETVAL(IRQ_HANDLED);
}

// 用于msix
static irqreturn_t irq_sse(int irq, void *instance)
{
    struct kl2_hwq *hwq = (struct kl2_hwq *)instance;

    //LOGD("irq_%d received, sse q_%d\n", irq, hwq->id);

    kl2_handle_hwq_intr(hwq, kl2_sse_hwq_intr_count(hwq));

    kl2_intc_toggle_msix(hwq->kl2_dev, hwq->id + 1);

    return IRQ_RETVAL(IRQ_HANDLED);
}

static int kl2_handle_excp_irq(struct kl2_device *kl2_dev)
{
    u32 st1, st0;

    if (is_pf_id(kl2_dev->dev_info.sriov_func_id)) {
        return 0;
    }

    st1 = kl2_intc_status(kl2_dev, 1);
    st0 = kl2_intc_status(kl2_dev, 0);
    if (st1 || st0) {
        kl2_sse_hwq_stall_all(kl2_dev);
    }
    if (st1) {
        KL2_LOGD("cluster excp intr\n");
        // 在intc屏蔽该cluster产生的异常中断，等待reset后再解除屏蔽
        kl2_intc_int_mask(kl2_dev, 1, st1);
        kl2_cluster_disable_and_record_excp(kl2_dev, st1);
    }
    if (st0) {
        KL2_LOGD("sdnn exception intr\n");
        // 在intc屏蔽该sdnn产生的异常中断，等待reset后再解除屏蔽
        kl2_intc_int_mask(kl2_dev, 0, st0);
        kl2_sdnn_disable_and_record_excp(kl2_dev, st0);
    }
    if (st1 || st0) {
        queue_work(g_kunlun_wq, &kl2_dev->handle_exception_work);
    }

    return 0;
}

static int kl2_handle_user_intr_irq(struct kl2_device *kl2_dev, u32 st4)
{
    u32 st7;
    u32 l3_c0, l3_c4, l3_c8, sdnn_freq;
    int used;

    if (!(st4 & (0x1 << 17))) {
        return 0;
    }

    st7 = kl2_intc_status(kl2_dev, 7);
    if (st7) {
        kl2_writel(kl2_dev, st7, kl2_dev->iomem_base.intc_base + KL2_REG_INTC_CLR_7);
        KL2_LOGD("user int, st7= %08x\n", st7);
    }

    // 升频/降频
    if (st7 & (0x1 << 3)) {
        l3_c0     = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_DEV_BUFFER0);
        l3_c4     = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_DEV_BUFFER1);
        l3_c8     = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_DEV_BUFFER2);
        sdnn_freq = kl2_readl(kl2_dev,
                              kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_PLL0_MDIV5_CONF);

        used = atomic_cmpxchg(&kl2_dev->irq_printk_data_used, 0, 1);
        if (!used) {
            kl2_dev->irq_printk_reason      = KL2_IRQ_PRINTK_DOWN_UP_CLOCKING;
            kl2_dev->irq_printk_u32_data[0] = st7;
            kl2_dev->irq_printk_u32_data[1] = l3_c0;
            kl2_dev->irq_printk_u32_data[2] = l3_c4;
            kl2_dev->irq_printk_u32_data[3] = l3_c8;
            kl2_dev->irq_printk_u32_data[4] = sdnn_freq;

            queue_work(g_kunlun_wq, &kl2_dev->irq_printk_work);
        } else {
            // :( info lost, any better solution?
        }
    }

    return 0;
}

static int kl2_handle_sse_irq(struct kl2_device *kl2_dev, u32 st4)
{
    int i;

    if (is_pf_id(kl2_dev->dev_info.sriov_func_id)) {
        return 0;
    }

    if (st4 & 0xfff) {
        // sse
        for (i = 0; i < KL2_HWQ_CNT; ++i) {
            if (st4 & (0x1 << i)) {
                struct kl2_hwq *hwq = &kl2_dev->hwq[i];
                kl2_handle_hwq_intr(hwq, kl2_sse_hwq_intr_count(hwq));
            }
        }
    }

    return 0;
}

static int kl2_handle_codec_irq(struct kl2_device *kl2_dev, u32 st4)
{
#ifdef ENABLE_CODEC
    u32        st;
    int        i;
    vdecdev_t *pvdec_device = NULL;

    if (is_pf_id(kl2_dev->dev_info.sriov_func_id)) {
        return 0;
    }

    // video dec
    if (st4 & (0x1 << 12)) {
        st = kl2_intc_status(kl2_dev, 2);
        st &= 0x3ffff;
        for (i = 0; i < 18; i++) {
            if (st & (0x1 << i)) {
                if ((i % 2) == 0) {
                    vdec_isr(kl2_dev->video_dec);
                } else {
                    pvdec_device = (vdecdev_t *)kl2_dev->video_dec;
                    vcache_isr(pvdec_device->cache_device);
                }
            }
        }
    }
    // video enc
    if (st4 & (0x1 << 13)) {
        st = kl2_intc_status(kl2_dev, 3);
        if (st) {
            venc_isr(kl2_dev->video_enc);
        }
    }
    // image proc
    if (st4 & (0x1 << 14)) {
        st = kl2_intc_status(kl2_dev, 5);
        if (st) {
            imgproc_isr(kl2_dev->image_proc);
        }
    }
#endif

    return 0;
}

static int kl2_handle_other_irq(struct kl2_device *kl2_dev, u32 st4)
{
    u32 st;

    if (!(st4 & (0x1 << 17))) {
        return 0;
    }

    // cluster/sdnn exception
    {
        kl2_handle_excp_irq(kl2_dev);
    }

    // gddr/noc/etc.
    st = kl2_intc_status(kl2_dev, 13);
    if (st) {
        if (st & 0xff) {
            // gddr
            kl2_handle_gddr_excp(kl2_dev, st);
        }
        if (st & 0x001c0000u) {
            // vmu
            u32 vmu_irq_gddr, vmu_irq_l3, vmu_irq_l3ro;

            vmu_irq_gddr =
                    kl2_readl(kl2_dev, kl2_dev->kdev->bar[2] + KL2_REG_VMUGDDR_BAR2_BASE + 0x0010);
            if (vmu_irq_gddr)
                kl2_writel(kl2_dev, 0x4,
                           kl2_dev->kdev->bar[2] + KL2_REG_VMUGDDR_BAR2_BASE + 0x0010);
            vmu_irq_l3 =
                    kl2_readl(kl2_dev, kl2_dev->kdev->bar[2] + KL2_REG_VMUL3_BAR2_BASE + 0x0010);
            if (vmu_irq_l3)
                kl2_writel(kl2_dev, 0x4, kl2_dev->kdev->bar[2] + KL2_REG_VMUL3_BAR2_BASE + 0x0010);
            vmu_irq_l3ro =
                    kl2_readl(kl2_dev, kl2_dev->kdev->bar[2] + KL2_REG_VMUL3RO_BAR2_BASE + 0x0010);
            if (vmu_irq_l3ro)
                kl2_writel(kl2_dev, 0x4,
                           kl2_dev->kdev->bar[2] + KL2_REG_VMUL3RO_BAR2_BASE + 0x0010);
            KL2_LOGI(
                    "vmu irq, st13= %08x, vmu_irq_gddr= %08x, vmu_irq_l3= %08x, vmu_irq_l3ro= %08x\n",
                    st, vmu_irq_gddr, vmu_irq_l3, vmu_irq_l3ro);
        }
        if (st & 0x00200000u) {
            // vac
            u32 vac_irq;

            vac_irq = kl2_readl(kl2_dev, kl2_dev->kdev->bar[2] + KL2_REG_VAC_BAR2_BASE + 0x0010);
            if (vac_irq)
                kl2_writel(kl2_dev, 0x4, kl2_dev->kdev->bar[2] + KL2_REG_VAC_BAR2_BASE + 0x0010);
            KL2_LOGI("vac irq, st13= %08x, vac_irq= %08x\n", st, vac_irq);
        }
        if (st & 0x00001000u) {
            // sse
            u32 sse_status;

            // TODO(liyunzheng): 需添加sse exception的详细处理逻辑
            sse_status =
                    kl2_readl(kl2_dev, kl2_dev->iomem_base.sse_base + KL2_REG_SSE_EXCEPT_STATUS);
            if (sse_status) {
                kl2_handle_sse_excp(kl2_dev, sse_status);
                kl2_writel(kl2_dev, sse_status,
                           kl2_dev->iomem_base.sse_base + KL2_REG_SSE_EXCEPT_CLR);
            }
        }
    }

    // pcie/ccix
    st = kl2_intc_status(kl2_dev, 14);
    if (st) {
        KL2_LOGD("pcie/ccix int, st14= %08x\n", st);
    }

    return 0;
}

// 被msi使用
irqreturn_t kl2_handle_irq(int irq, void *instance)
{
    struct kl2_device *kl2_dev = instance;
    struct pci_dev    *pdev    = kl2_dev->kdev->pdev;
    int                irq_idx = irq - pdev->irq;
    u32                st4;

    st4 = kl2_intc_status(kl2_dev, 4);

    if (!kl2_dev->multi_msi_vector) {
        kl2_handle_sse_irq(kl2_dev, st4);
        kl2_handle_codec_irq(kl2_dev, st4);
        kl2_handle_user_intr_irq(kl2_dev, st4);
        kl2_handle_other_irq(kl2_dev, st4);
    } else {
        if (irq_idx == kl2_dev->main_msi_vector) {
            kl2_handle_sse_irq(kl2_dev, st4);
            kl2_handle_user_intr_irq(kl2_dev, st4);
        }
        if (irq_idx == 0) {
            kl2_handle_codec_irq(kl2_dev, st4);
            kl2_handle_other_irq(kl2_dev, st4);
        }
    }

    // 强制将中断信号拉低一个cycle，使intc能采集到信号上升沿，防止中断丢失
    kl2_intc_toggle_msi(kl2_dev, irq_idx);

    return IRQ_RETVAL(IRQ_HANDLED);
}

#define KL2_MIN_NIRQ 14
static int __try_enable_msix(struct kl2_device *kl2_dev, int nvec)
{
    struct kl_device *kdev = kl2_dev->kdev;
    struct pci_dev   *pdev = kdev->pdev;
    int               i    = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 14, 0)
    int errsv = 0;
#endif
    // FIXME: this sould
    // set Table of MSI-X capability to Bar2 + 0x184B000;
    // which is 0x2304B000
    kl2_writel(kl2_dev, 0x184B002, kdev->bar[2] + KL2_REG_PCIEAXILT_BAR2_BASE + 0xAC);
    kl2_writel(kl2_dev, 0x184B402, kdev->bar[2] + KL2_REG_PCIEAXILT_BAR2_BASE + 0xB0);

    KL2_LOGD("msi_register, msix_cap=%u\n", pdev->msix_cap);
    for (i = 0; i < nvec; ++i)
        kl2_dev->msix_entries[i].entry = i;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 14, 0)
    while (nvec >= KL2_MIN_NIRQ) {
        kl2_dev->msix_nvec = nvec;
        errsv              = pci_enable_msix(pdev, kl2_dev->msix_entries, nvec);
        if (errsv > 0)
            nvec = errsv;
        else
            return nvec;
    }
    return -ENOSPC;
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
    return pci_enable_msix_range(pdev, kl2_dev->msix_entries, nvec, nvec);
#else
    return pci_alloc_irq_vectors(pdev, nvec, nvec, PCI_IRQ_MSIX);
#endif
}

int kl2_msix_register(struct kl2_device *kl2_dev)
{
    int errsv, i, irq;

    errsv = __try_enable_msix(kl2_dev, KL2_MIN_NIRQ);
    if (errsv < 0)
        return errsv;

    kl2_dev->msix_nvec = errsv;
    KL2_LOGD("nvec=%d\n", kl2_dev->msix_nvec);

    for (i = 0; i < kl2_dev->msix_nvec; ++i) {
        irqreturn_t (*handler)(int irq, void *instance);
        void *data;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
        irq = kl2_dev->msix_entries[i].vector;
#else
        irq = pci_irq_vector(kl2_dev->kdev->pdev, i);
#endif
        if (i == 0 || i == 13) {
            data    = (void *)kl2_dev;
            handler = &irq_general;
        } else {
            data    = (void *)&kl2_dev->hwq[i - 1];
            handler = &irq_sse;
        }

        errsv = request_irq(irq, handler, IRQF_SHARED, kl2_dev->kdev->name, data);
        if (errsv) {
            KL2_LOGW("request_irq(%d, %d) = %d\n", i, irq, errsv);
            goto err_out;
        }

        KL2_LOGD("enable msix[%d] irq_%d\n", i, irq);
    }
    return 0;

err_out:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
    for (; i >= 0; --i) {
        KL2_LOGD("free irq %d\n", kl2_dev->msix_entries[i].vector);
        free_irq(kl2_dev->msix_entries[i].vector, kl2_dev);
    }
    pci_disable_msix(kl2_dev->kdev->pdev);
#else
    for (; i >= 0; --i) {
        KL2_LOGD("free irq %d\n", pci_irq_vector(kl2_dev->kdev->pdev, i));
        free_irq(pci_irq_vector(kl2_dev->kdev->pdev, i), kl2_dev);
    }
    pci_free_irq_vectors(kl2_dev->kdev->pdev);
#endif

    return errsv;
}

void kl2_msix_unregister(struct kl2_device *kl2_dev)
{
    int i;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
    for (i = 0; i < kl2_dev->msix_nvec; ++i) {
        void *data;
        if (i == 0 || i == 13)
            data = (void *)kl2_dev;
        else
            data = (void *)&kl2_dev->hwq[i - 1];
        KL2_LOGD("free irq %d\n", kl2_dev->msix_entries[i].vector);
        free_irq(kl2_dev->msix_entries[i].vector, data);
    }
    pci_disable_msix(kl2_dev->kdev->pdev);
#else
    for (i = 0; i < kl2_dev->msix_nvec; ++i) {
        void *data;
        if (i == 0 || i == 13)
            data = (void *)kl2_dev;
        else
            data = (void *)&kl2_dev->hwq[i - 1];
        KL2_LOGD("free irq %d\n", pci_irq_vector(kl2_dev->kdev->pdev, i));
        free_irq(pci_irq_vector(kl2_dev->kdev->pdev, i), data);
    }
    pci_free_irq_vectors(kl2_dev->kdev->pdev);
#endif
}

int kl2_msi_register(struct kl2_device *kl2_dev)
{
    struct pci_dev *pdev = kl2_dev->kdev->pdev;
    int             ret;
    int             i;
    int             msi_nvec_alloc;
    int             msi_nvec_req;

    if (kl2_dev->multi_msi_vector) {
        // VF需要分配全部32个MSI vector，否则即使更换vector 1也会出现中断丢失。
        msi_nvec_alloc = KL2_MAX_MSI_VECTOR_CNT;
        msi_nvec_req   = kl2_dev->main_msi_vector + 1;
        if (msi_nvec_req < 1 || msi_nvec_req > msi_nvec_alloc) {
            KL2_LOGE("invalid msi vector requested, main_msi_vector= %d\n",
                     kl2_dev->main_msi_vector);
            ret = -EINVAL;
            goto err_out;
        }
    } else {
        msi_nvec_alloc = 1;
        msi_nvec_req   = 1;
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 14, 0)
    ret = pci_enable_msi_block(pdev, msi_nvec_alloc);
    if (ret != 0) {
        KL2_LOGW("pci_enable_msi_block() = %d\n", ret);
        ret = -ENOSPC;
        goto err_out;
    }
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
    ret = pci_enable_msi_range(pdev, msi_nvec_alloc, msi_nvec_alloc);
    if (ret != msi_nvec_alloc) {
        KL2_LOGW("pci_enable_msi_range() = %d\n", ret);
        ret = -ENOSPC;
        goto err_out;
    }
#else
    ret = pci_alloc_irq_vectors(pdev, msi_nvec_alloc, msi_nvec_alloc, PCI_IRQ_MSI);
    if (ret != msi_nvec_alloc) {
        KL2_LOGW("pci_alloc_irq_vectors() = %d\n", ret);
        ret = -ENOSPC;
        goto err_out;
    }
#endif

    kl2_dev->msi_nvec = msi_nvec_req;
    for (i = 0; i < kl2_dev->msi_nvec; i++) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
        ret = request_irq(pdev->irq + i, &kl2_handle_irq, IRQF_SHARED, kl2_dev->kdev->name,
                          kl2_dev);
        if (ret) {
            KL2_LOGW("request_irq(%d) = %d\n", pdev->irq + i, ret);
            goto err_free_irq;
        }
#else
        ret = request_irq(pci_irq_vector(pdev, i), &kl2_handle_irq, IRQF_SHARED,
                          kl2_dev->kdev->name, kl2_dev);
        if (ret) {
            KL2_LOGW("request_irq(%d) = %d\n", pci_irq_vector(pdev, i), ret);
            goto err_free_irq;
        }
#endif
    }

    kl2_dev->msi_en = 1;
    KL2_LOGD("enable msi irq=%d\n", pdev->irq);

    return 0;

err_free_irq:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
    for (i = 0; i < kl2_dev->msi_nvec; i++) {
        free_irq(pdev->irq + i, kl2_dev);
    }
    pci_disable_msi(pdev);
#else
    for (i = 0; i < kl2_dev->msi_nvec; i++) {
        free_irq(pci_irq_vector(pdev, i), kl2_dev);
    }
    pci_free_irq_vectors(pdev);
#endif

err_out:
    return ret;
}

void kl2_msi_unregister(struct kl2_device *kl2_dev)
{
    struct pci_dev *pdev = kl2_dev->kdev->pdev;
    int             i;

    if (kl2_dev->msi_en) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 7, 0)
        for (i = 0; i < kl2_dev->msi_nvec; i++) {
            free_irq(pdev->irq + i, kl2_dev);
        }
        pci_disable_msi(pdev);
#else
        for (i = 0; i < kl2_dev->msi_nvec; i++) {
            free_irq(pci_irq_vector(pdev, i), kl2_dev);
        }
        pci_free_irq_vectors(pdev);
#endif
    }
    kl2_dev->msi_en = 0;
}

void kl2_irq_printk_work_func(struct work_struct *work)
{
    struct kl2_device *kl2_dev = container_of(work, struct kl2_device, irq_printk_work);

    u32 st7, l3_c0, l3_c4, l3_c8, sdnn_freq, val;

    if (kl2_dev->irq_printk_reason == KL2_IRQ_PRINTK_DOWN_UP_CLOCKING) {
        //l3_c0     = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + 0xc0);
        //l3_c4     = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + 0xc4);
        //l3_c8     = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + 0xc8);
        //sdnn_freq = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon0_base + 0x128);
        st7       = kl2_dev->irq_printk_u32_data[0];
        l3_c0     = kl2_dev->irq_printk_u32_data[1];
        l3_c4     = kl2_dev->irq_printk_u32_data[2];
        l3_c8     = kl2_dev->irq_printk_u32_data[3];
        sdnn_freq = kl2_dev->irq_printk_u32_data[4];

        // 升频/降频
        if (st7 & (0x1 << 3)) {
            if (sdnn_freq > 0 && sdnn_freq < 256) {
                sdnn_freq = 5200 / sdnn_freq;
            } else {
                sdnn_freq = 0;
            }

            if (l3_c0 & (0x1 << 0)) {
                // 降频
                if (l3_c4 & (0x1 << 0)) {
                    // 温度超限
                    val = (l3_c8 * -2454 + 3668120) / 10000;
                    KL2_LOG_XID(
                            XPU_XID2,
                            "auto downclocking, as temperature reaches limit, freq= %u, temp= %u\n",
                            sdnn_freq, val);
                }
                if (l3_c4 & (0x1 << 1)) {
                    // 功耗超限
                    val = l3_c8;
                    KL2_LOG_XID(XPU_XID2,
                                "auto downclocking, as power reaches limit, freq= %u, power= %u\n",
                                sdnn_freq, val);
                }
            }

            if (l3_c0 & (0x1 << 1)) {
                // 升频
                KL2_LOGI("auto upclocking, freq= %u\n", sdnn_freq);
            }
        }
    }

    atomic_set(&kl2_dev->irq_printk_data_used, 0);
    return;
}
