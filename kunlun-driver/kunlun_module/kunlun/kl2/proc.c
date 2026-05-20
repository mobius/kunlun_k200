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

#include "kl2/proc.h"
#include "kl2/kl2.h"
#include "kl2/exception.h"
#include "kl2/hw.h"
#include "kl2/kl2_regs.h"
#include "kl2/disable_reg_debug.h"
#include "kl_proc.h"

#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timex.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define IS_CMD_STR(buf, x) (strncmp(buf, (x), strlen((x))) == 0)

/*
 *   /proc/xpu/devX/XXX_debug entries
 */

DEFINE_DEVPROC_SHOW(cuen)
{
    struct kl2_device *kl2_dev = m->private;
    u32                i;
    u32                not_first = 0;
    u32                cuen      = kl2_dev->cuen;

    if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF) {
        return -EINVAL;
    }

    seq_printf(m, "cuen= %08x", cuen);
    seq_printf(m, "(");
    for (i = 0; i < KL2_CLUSTER_MAX_COUNT; ++i) {
        if (cuen & (0x1 << (i + 18))) {
            if (not_first)
                seq_printf(m, "|");
            seq_printf(m, "cl%d", i);
            not_first = 1;
        }
    }
    for (i = 0; i < KL2_SDNN_MAX_COUNT; ++i) {
        if (cuen & (0x1 << (i + 12))) {
            if (not_first)
                seq_printf(m, "|");
            seq_printf(m, "sd%d", i);
            not_first = 1;
        }
    }
    seq_printf(m, ")\n\n");

    seq_printf(m, "cuen stands for compute unit enable state.\n");
    seq_printf(m, "  cuen[%d:18] -> cl[%d:0]\n", fls(kl2_dev->spec.cl_bits) + 18 - 1,
               fls(kl2_dev->spec.cl_bits) - 1);
    seq_printf(m, "  cuen[%d:12] -> sdnn[%d:0]\n", fls(kl2_dev->spec.sdnn_bits) + 12 - 1,
               fls(kl2_dev->spec.sdnn_bits) - 1);
    seq_printf(m, "Only can these bits be modified.\n");
    return 0;
}

DEFINE_DEVPROC_WR(cuen)
{
    struct seq_file   *seqfile = file->private_data;
    struct kl2_device *kl2_dev = seqfile->private;
    char               user_str[64];
    u32                user_cuen;

    if ((count > sizeof(user_str) - 1) || *pos != 0)
        return -EINVAL;
    if (copy_from_user(user_str, buffer, count))
        return -EFAULT;
    user_str[count] = '\0';

    if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF) {
        return -EINVAL;
    }

    if (kstrtouint(user_str, 16, &user_cuen) || ((user_cuen >> 18) & kl2_dev->spec.cl_bits) == 0 ||
        ((user_cuen >> 12) & kl2_dev->spec.sdnn_bits) == 0) {
        KL2_LOGW("invalid conf, user_str= %s, user_cuen= %08x\n", user_str, user_cuen);
        return -EINVAL;
    } else {
        mutex_lock(&kl2_dev->big_global_lock);
        kl2_sse_cuen_update(kl2_dev, user_cuen);
        mutex_unlock(&kl2_dev->big_global_lock);
    }

    return count;
}

DEFINE_DEVPROC_SHOW(cu_debug)
{
    struct kl2_device *kl2_dev      = m->private;
    void __iomem      *cluster_base = kl2_dev->iomem_base.cluster_base;
    void __iomem      *sdnn_base    = kl2_dev->iomem_base.sdnn_base;
    u32                i;
    u32                val;

    kl2_reg_lock(kl2_dev);
    for_each_valid_cluster(kl2_dev, i) {
        seq_printf(m, "cl%d: ", i);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_DONE_STATUS0);
        seq_printf(m, "done0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_DONE_STATUS1);
        seq_printf(m, "done1= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXCEPTION_STATE);
        seq_printf(m, "excp= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
        seq_printf(m, "excp_mask= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXECUTE_CYCLE);
        seq_printf(m, "cycle= %08x ", val);
        val = kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_ID);
        seq_printf(m, "id= %08x ", val);
        val = kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_TOKEN);
        seq_printf(m, "tk= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_DMA0_DEBUG_INFO0);
        seq_printf(m, "dma0_d0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_DMA0_DEBUG_INFO1);
        seq_printf(m, "dma0_d1= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_DMA0_DEBUG_INFO2);
        seq_printf(m, "dma0_d2= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_DMA1_DEBUG_INFO0);
        seq_printf(m, "dma1_d0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_DMA1_DEBUG_INFO1);
        seq_printf(m, "dma1_d1= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_DMA1_DEBUG_INFO2);
        seq_printf(m, "dma1_d2= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_TOP_DEBUG_INFO0);
        seq_printf(m, "top_d0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_TOP_DEBUG_INFO1);
        seq_printf(m, "top_d1= %08x ", val);
        seq_printf(m, "\n");
    }
    seq_printf(m, "--\n");
    for_each_valid_sdnn(kl2_dev, i) {
        seq_printf(m, "sd%d: ", i);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_DONE_STATUS0);
        seq_printf(m, "done0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_DONE_STATUS1);
        seq_printf(m, "done1= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXCEPTION_STATE);
        seq_printf(m, "excp= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
        seq_printf(m, "excp_mask= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXECUTE_CYCLE);
        seq_printf(m, "cycle= %08x ", val);
        val = kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_ID);
        seq_printf(m, "id= %08x ", val);
        val = kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_TOKEN);
        seq_printf(m, "tk= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_DMA0_DEBUG_INFO0);
        seq_printf(m, "dma0_d0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_DMA0_DEBUG_INFO1);
        seq_printf(m, "dma0_d1= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_DMA0_DEBUG_INFO2);
        seq_printf(m, "dma0_d2= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_DMA1_DEBUG_INFO0);
        seq_printf(m, "dma1_d0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_DMA1_DEBUG_INFO1);
        seq_printf(m, "dma1_d1= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_DMA1_DEBUG_INFO2);
        seq_printf(m, "dma1_d2= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_TOP_DEBUG_INFO0);
        seq_printf(m, "top_d0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_TOP_DEBUG_INFO1);
        seq_printf(m, "top_d1= %08x ", val);
        seq_printf(m, "\n");
    }
    seq_printf(m, "--\n");
    kl2_reg_unlock(kl2_dev);
    return 0;
}

DEFINE_DEVPROC_SHOW(memory_frag)
{
    struct kl2_device *kl2_dev = m->private;
    struct kl_mm      *mm      = &kl2_dev->mm;
    struct kl_memory  *mem     = NULL;
    int                i;
    u64                j, k, max_contig_free_pages, contig_free_pages;

    for (i = 0; i < mm->mem_count; i++) {
        mem                   = &mm->mem[i];
        max_contig_free_pages = 0;

        spin_lock(&mem->lock);

        for (j = 0; j < mem->page_count; j++) {
            if (!test_bit(j, mem->page_bitmap)) {
                k                 = find_next_bit(mem->page_bitmap, mem->page_count, j);
                contig_free_pages = k - j;
                if (contig_free_pages > max_contig_free_pages) {
                    max_contig_free_pages = contig_free_pages;
                }
                j = k;
            } else {
                j = find_next_zero_bit(mem->page_bitmap, mem->page_count, j + 1) - 1;
            }
        }

        seq_printf(
                m,
                "kl_memory idx= %d kind= %d base= %016llx size= %016llx limit= %016llx page_size= %x page_used= %lld/%lld(used_ratio= %lld%%) \n",
                mem->idx, mem->kind, mem->base, mem->size, mem->limit, mem->page_size,
                mem->page_used, mem->page_count, mem->page_used * 100 / mem->page_count);
        seq_printf(m,
                   "kl_memory max_contig_free_pages= %lld/%llx max_contig_free_size= %lld/%llx\n",
                   max_contig_free_pages, max_contig_free_pages,
                   max_contig_free_pages * mem->page_size, max_contig_free_pages * mem->page_size);
        seq_printf(m, "kl_memory bitmap\n");
        for (j = 0; j < mem->page_count; j++) {
            if (j % 256 == 0) {
                seq_printf(m, "\n");
            } else if (j % 64 == 0) {
                seq_printf(m, " ");
            }

            if (test_bit(j, mem->page_bitmap)) {
                seq_printf(m, "*");
            } else {
                seq_printf(m, ".");
            }
        }
        seq_printf(m, "\n");

        spin_unlock(&mem->lock);
    }

    return 0;
}

DEFINE_DEVPROC_SHOW(perf_test)
{
    static u64 cl_treg64[8] = { 0x800008, 0x840008, 0x880008, 0x8C0008,
                                0x900008, 0x940008, 0x980008, 0x9C0008 };
    //static u64 cl_treg32[8] = {
    //    0x800004, 0x840004, 0x880004, 0x8C0004,
    //    0x900004, 0x940004, 0x980004, 0x9C0004};

    struct kl2_device *kl2_dev = m->private;
    struct kl_device  *kdev    = kl2_dev->kdev;
    void __iomem      *ptr;
    NvU64              t0, t1;
    int                i;

#define PERF_RUN_CNT (16384)
#define PERF_BEGIN                                                                                 \
    do {                                                                                           \
        t0 = os_get_current_tick_hr();                                                             \
    } while (0)
#define PERF_END                                                                                   \
    do {                                                                                           \
        t1 = os_get_current_tick_hr();                                                             \
    } while (0)
#define PERF_PRINT(title)                                                                          \
    seq_printf(m, "%-32s: cnt= %5d, avg= %4llu ns\n", title, PERF_RUN_CNT, (t1 - t0) / PERF_RUN_CNT)

    kl2_reg_lock(kl2_dev);
    // L3 MMIO Seq Write64
    ptr = kdev->bar[4] + 0x1000;
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; ++i) {
        writeq(i, ptr);
        ptr += 8;
    }
    PERF_END;
    PERF_PRINT("L3 MMIO Seq Write64");

    // L3 MMIO Same Write64
    ptr = kdev->bar[4] + 0x1000;
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; ++i) {
        writeq(i, ptr);
    }
    PERF_END;
    PERF_PRINT("L3 MMIO Same Write64");

    // L3 MMIO Seq Read64
    ptr = kdev->bar[4] + 0x1000;
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; ++i) {
        readq(ptr);
        ptr += 8;
    }
    PERF_END;
    PERF_PRINT("L3 MMIO Seq Read64");

    // L3 MMIO Same Read64
    ptr = kdev->bar[4] + 0x1000;
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; ++i) {
        readq(ptr);
    }
    PERF_END;
    PERF_PRINT("L3 MMIO Same Read64");

    // Reg MMIO Discrete Write64
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; ++i) {
        writeq(i, kdev->bar[2] + cl_treg64[i % 8]);
    }
    PERF_END;
    PERF_PRINT("Reg MMIO Discrete Write64");

    // Reg MMIO Same Write64
    ptr = kdev->bar[2] + cl_treg64[0];
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; ++i) {
        writeq(i, ptr);
    }
    PERF_END;
    PERF_PRINT("Reg MMIO Same Write64");

    // Reg MMIO Discrete Read64
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; ++i) {
        readq(kdev->bar[2] + cl_treg64[i % 8]);
    }
    PERF_END;
    PERF_PRINT("Reg MMIO Discrete Read64");

    // Reg MMIO Same Read64
    ptr = kdev->bar[2] + cl_treg64[0];
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; ++i) {
        readq(ptr);
    }
    PERF_END;
    PERF_PRINT("Reg MMIO Same Read64");

    // Reg MMIO Write64 on Bar0 Ping-pong
    // use PCIe DMA ch0 SRCPARAM DESTPARAM as test registers
    ptr = kdev->bar[0] + 0x400;
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; i += 2) {
        writeq(0, ptr + 0x8);
        writeq(0, ptr + 0x10);
    }
    PERF_END;
    PERF_PRINT("Reg MMIO Bar0 Write64 Ping-pong");

    // Reg MMIO Write32 on Bar0
    // use PCIe DMA ch0 SRCPARAM, DESTPARAM, SRCADDR and DESTADDR as test registers
    ptr = kdev->bar[0] + 0x400;
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; i += 6) {
        kl2_writel(kl2_dev, 0, ptr);
        kl2_writel(kl2_dev, 0, ptr + 0x4);
        kl2_writel(kl2_dev, 0, ptr + 0x8);
        kl2_writel(kl2_dev, 0, ptr + 0xC);
        kl2_writel(kl2_dev, 0, ptr + 0x10);
        kl2_writel(kl2_dev, 0, ptr + 0x14);
    }
    PERF_END;
    PERF_PRINT("Reg MMIO Bar0 Write32");

    // Reg MMIO Read64 on Bar0 Ping-pong
    // use PCIe DMA ch0 SRCPARAM DESTPARAM as test registers
    ptr = kdev->bar[0] + 0x400;
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; i += 2) {
        readq(ptr + 0x8);
        readq(ptr + 0x10);
    }
    PERF_END;
    PERF_PRINT("Reg MMIO Bar0 Read64 Ping-pong");

    // Reg MMIO Read32 on Bar0
    // use PCIe DMA ch0 SRCPARAM, DESTPARAM, SRCADDR and DESTADDR as test registers
    ptr = kdev->bar[0] + 0x400;
    PERF_BEGIN;
    for (i = 0; i < PERF_RUN_CNT; i += 6) {
        kl2_readl(kl2_dev, ptr);
        kl2_readl(kl2_dev, ptr + 0x4);
        kl2_readl(kl2_dev, ptr + 0x8);
        kl2_readl(kl2_dev, ptr + 0xC);
        kl2_readl(kl2_dev, ptr + 0x10);
        kl2_readl(kl2_dev, ptr + 0x14);
    }
    PERF_END;
    PERF_PRINT("Reg MMIO Bar0 Read32");
    kl2_reg_unlock(kl2_dev);

#undef PERF_RUN_CNT
#undef PERF_BEGIN
#undef PERF_END
#undef PERF_PRINT

    return 0;
}

DEFINE_DEVPROC_SHOW(soft_reset)
{
    return 0;
}

DEFINE_DEVPROC_WR(soft_reset)
{
    struct seq_file   *seqfile = file->private_data;
    struct kl2_device *kl2_dev = seqfile->private;
    int                ret;

    ret = kl2_dev_soft_reset(kl2_dev, 1, 0);
    if (ret) {
        return ret;
    }
    return count;
}

DEFINE_DEVPROC_SHOW(soft_reset_mode0_with_gddr)
{
    return 0;
}

DEFINE_DEVPROC_WR(soft_reset_mode0_with_gddr)
{
    struct seq_file   *seqfile = file->private_data;
    struct kl2_device *kl2_dev = seqfile->private;
    int                ret;

    if (kl2_dev->dev_info.fw[2] < 29) {
        KL2_LOGI("FW should be x.x.29 or later, now it's %04u.%04u.%04u\n", kl2_dev->dev_info.fw[0],
                 kl2_dev->dev_info.fw[1], kl2_dev->dev_info.fw[2]);
        return -EBUSY;
    }

    ret = kl2_dev_soft_reset(kl2_dev, 0, 0);
    if (ret) {
        return ret;
    }
    return count;
}

DEFINE_DEVPROC_SHOW(sse_debug)
{
    struct kl2_device *kl2_dev  = m->private;
    void __iomem      *sse_base = kl2_dev->iomem_base.sse_base;
    int                i, hwq_id;
    u32                val;
    unsigned long      val_ul;

    kl2_reg_lock(kl2_dev);
    val    = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_EXCEPT_STATUS);
    val_ul = val;
    seq_printf(m, "ex_st           = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for_each_set_bit(i, &val_ul, 32) {
            seq_printf(m, "%s", KL_BITWISE_DESC(KL2_REG_SSE_EXCEPT_STATUS, i));
            if (i != fls(val) - 1)
                seq_printf(m, "|");
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");

    val    = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_EXCEPT_MASK);
    val_ul = val;
    seq_printf(m, "ex_mask         = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for_each_set_bit(i, &val_ul, 32) {
            seq_printf(m, "%s", KL_BITWISE_DESC(KL2_REG_SSE_EXCEPT_MASK, i));
            if (i != fls(val) - 1)
                seq_printf(m, "|");
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");

    val    = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_STREAM_OVERFLOW_ERR_MAP);
    val_ul = val;
    seq_printf(m, "hwq_of          = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for_each_set_bit(i, &val_ul, 32) {
            seq_printf(m, "%d", i);
            if (i != fls(val) - 1)
                seq_printf(m, "|");
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");

    val    = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_QUEUE_DSC_ERR_MAP);
    val_ul = val;
    seq_printf(m, "hwq_desc_err    = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for_each_set_bit(i, &val_ul, 32) {
            seq_printf(m, "%d", i);
            if (i != fls(val) - 1)
                seq_printf(m, "|");
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");

    val    = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_WRESP_ERR_MAP);
    val_ul = val;
    seq_printf(m, "wresp_err       = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for_each_set_bit(i, &val_ul, 32) {
            seq_printf(m, "%s", KL_BITWISE_DESC(KL2_REG_SSE_WRESP_ERR_MAP, i));
            if (i != fls(val) - 1)
                seq_printf(m, "|");
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");

    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_LAST_SCH_CLUSTER_DSC_TOKEN);
    seq_printf(m, "user0_last_cl_tk= %08x(%u)", val, val);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_LAST_SCH_SDNN_DSC_TOKEN);
    seq_printf(m, "user0_last_sd_tk= %08x(%u)", val, val);
    seq_printf(m, "\n");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER1_LAST_SCH_CLUSTER_DSC_TOKEN);
    seq_printf(m, "user1_last_cl_tk= %08x(%u)", val, val);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER1_LAST_SCH_SDNN_DSC_TOKEN);
    seq_printf(m, "user1_last_sd_tk= %08x(%u)", val, val);
    seq_printf(m, "\n");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER2_LAST_SCH_CLUSTER_DSC_TOKEN);
    seq_printf(m, "user2_last_cl_tk= %08x(%u)", val, val);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER2_LAST_SCH_SDNN_DSC_TOKEN);
    seq_printf(m, "user2_last_sd_tk= %08x(%u)", val, val);
    seq_printf(m, "\n");

    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_CLUSTER_MAPPING_LIST);
    seq_printf(m, "user0_cl_map    = %08x", val);
    //if (val) {
    seq_printf(m, "(");
    for (i = 0; i < KL2_CLUSTER_MAX_COUNT; ++i) {
        seq_printf(m, "%2d", (val >> (i * 4)) & 0xf);
        if (i != KL2_CLUSTER_MAX_COUNT - 1) {
            seq_printf(m, "|");
        }
    }
    seq_printf(m, ")");
    //}
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_SDNN_MAPPING_LIST);
    seq_printf(m, "user0_sd_map    = %08x", val);
    //if (val) {
    seq_printf(m, "(");
    for (i = 0; i < KL2_SDNN_MAX_COUNT; ++i) {
        seq_printf(m, "%2d", (val >> (i * 4)) & 0xf);
        if (i != KL2_SDNN_MAX_COUNT - 1) {
            seq_printf(m, "|");
        }
    }
    seq_printf(m, ")");
    //}
    seq_printf(m, "\n");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER1_CLUSTER_MAPPING_LIST);
    seq_printf(m, "user1_cl_map    = %08x", val);
    //if (val) {
    seq_printf(m, "(");
    for (i = 0; i < KL2_CLUSTER_MAX_COUNT; ++i) {
        seq_printf(m, "%2d", (val >> (i * 4)) & 0xf);
        if (i != KL2_CLUSTER_MAX_COUNT - 1) {
            seq_printf(m, "|");
        }
    }
    seq_printf(m, ")");
    //}
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER1_SDNN_MAPPING_LIST);
    seq_printf(m, "user1_sd_map    = %08x", val);
    //if (val) {
    seq_printf(m, "(");
    for (i = 0; i < KL2_SDNN_MAX_COUNT; ++i) {
        seq_printf(m, "%2d", (val >> (i * 4)) & 0xf);
        if (i != KL2_SDNN_MAX_COUNT - 1) {
            seq_printf(m, "|");
        }
    }
    seq_printf(m, ")");
    //}
    seq_printf(m, "\n");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER2_CLUSTER_MAPPING_LIST);
    seq_printf(m, "user2_cl_map    = %08x", val);
    //if (val) {
    seq_printf(m, "(");
    for (i = 0; i < KL2_CLUSTER_MAX_COUNT; ++i) {
        seq_printf(m, "%2d", (val >> (i * 4)) & 0xf);
        if (i != KL2_CLUSTER_MAX_COUNT - 1) {
            seq_printf(m, "|");
        }
    }
    seq_printf(m, ")");
    //}
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER2_SDNN_MAPPING_LIST);
    seq_printf(m, "user2_sd_map    = %08x", val);
    //if (val) {
    seq_printf(m, "(");
    for (i = 0; i < KL2_SDNN_MAX_COUNT; ++i) {
        seq_printf(m, "%2d", (val >> (i * 4)) & 0xf);
        if (i != KL2_SDNN_MAX_COUNT - 1) {
            seq_printf(m, "|");
        }
    }
    seq_printf(m, ")");
    //}
    seq_printf(m, "\n");

    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_LAST_DISPATCHED_TOKEN);
    seq_printf(m, "last_disp_tk    = %08x(%u)\n", val, val);

    val    = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_SSE_BUSY_STATUS);
    val_ul = val;
    seq_printf(m, "sse_busy_st     = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for_each_set_bit(i, &val_ul, 32) {
            seq_printf(m, "%s", KL_BITWISE_DESC(KL2_REG_SSE_SSE_BUSY_STATUS, i));
            if (i != fls(val) - 1)
                seq_printf(m, "|");
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");

    val    = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_XPU_BUSY_STATUS);
    val_ul = val;
    seq_printf(m, "xpu_busy_st     = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for_each_set_bit(i, &val_ul, 32) {
            seq_printf(m, "%s", KL_BITWISE_DESC(KL2_REG_SSE_XPU_BUSY_STATUS, i));
            if (i != fls(val) - 1)
                seq_printf(m, "|");
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");

    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_STREAM_QUEUE_INFO);
    seq_printf(m, "user0_hwq   = %08x(%u)", val, val);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_SCHEDULER_XPU_INFO);
    seq_printf(m, "user0_cl_sd   = %08x(cl= %u, sd= %u)", val, val & 0xffffu, val >> 16);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    seq_printf(m, "user0_disable   = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for (i = 0; i < KL2_CLUSTER_MAX_COUNT; ++i) {
            if (val & (0x1 << i)) {
                seq_printf(m, "cl%d", i);
                if (i != fls(val) - 1)
                    seq_printf(m, "|");
            }
        }
        for (i = 0; i < KL2_SDNN_MAX_COUNT; ++i) {
            if (val & (0x1 << (i + 16))) {
                seq_printf(m, "sd%d", i);
                if (i + 16 != fls(val) - 1)
                    seq_printf(m, "|");
            }
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER1_STREAM_QUEUE_INFO);
    seq_printf(m, "user1_hwq   = %08x(%u)", val, val);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER1_SCHEDULER_XPU_INFO);
    seq_printf(m, "user1_cl_sd   = %08x(cl= %u, sd= %u)", val, val & 0xffffu, val >> 16);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER1_XPU_DISABLE);
    seq_printf(m, "user1_disable   = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for (i = 0; i < KL2_CLUSTER_MAX_COUNT; ++i) {
            if (val & (0x1 << i)) {
                seq_printf(m, "cl%d", i);
                if (i != fls(val) - 1)
                    seq_printf(m, "|");
            }
        }
        for (i = 0; i < KL2_SDNN_MAX_COUNT; ++i) {
            if (val & (0x1 << (i + 16))) {
                seq_printf(m, "sd%d", i);
                if (i + 16 != fls(val) - 1)
                    seq_printf(m, "|");
            }
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER2_STREAM_QUEUE_INFO);
    seq_printf(m, "user2_hwq   = %08x(%u)", val, val);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER2_SCHEDULER_XPU_INFO);
    seq_printf(m, "user2_cl_sd   = %08x(cl= %u, sd= %u)", val, val & 0xffffu, val >> 16);
    seq_printf(m, "\t");
    val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER2_XPU_DISABLE);
    seq_printf(m, "user2_disable   = %08x", val);
    if (val) {
        seq_printf(m, "(");
        for (i = 0; i < KL2_CLUSTER_MAX_COUNT; ++i) {
            if (val & (0x1 << i)) {
                seq_printf(m, "cl%d", i);
                if (i != fls(val) - 1)
                    seq_printf(m, "|");
            }
        }
        for (i = 0; i < KL2_SDNN_MAX_COUNT; ++i) {
            if (val & (0x1 << (i + 16))) {
                seq_printf(m, "sd%d", i);
                if (i + 16 != fls(val) - 1)
                    seq_printf(m, "|");
            }
        }
        seq_printf(m, ")");
    }
    seq_printf(m, "\n");
    seq_printf(m, "--\n");

    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        seq_printf(m, "hwq_%-2d ", hwq_id);
        val = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_QUEUE0_LAST_ERR_TOKEN + hwq_id * 4);
        seq_printf(m, "last_err_tk= %08x(%u) ", val, val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SSE_QCTRL_BASE(sse_base, hwq_id) + KL2_REG_SSE_QCTRL_STALL);
        seq_printf(m, "stall= %x ", val);
        val = kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) +
                                         KL2_REG_SSE_QSTATUS_TASK_RP_CNT);
        seq_printf(m, "underway= %d pending= %-2d ", (val >> 16) & 0xffff, val & 0xffff);
        //val = kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_TASK_F_CNT);
        //seq_printf(m, "finish= %d ", val);
        val    = kl2_readl(kl2_dev,
                           KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_QERR);
        val_ul = val;
        seq_printf(m, "err= %08x ", val);
        if (val) {
            for_each_set_bit(i, &val_ul, 32) {
                seq_printf(m, "%s", KL_BITWISE_DESC(KL2_REG_SSE_QUEUE0_STATUS_3, i));
                if (i != fls(val) - 1)
                    seq_printf(m, "|");
            }
        }
        //val = kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_QTMR_0);
        //seq_printf(m, "tmr0= %d ", val);
        //val = kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_QTMR_1);
        //seq_printf(m, "tmr1= %d ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_REC_0);
        seq_printf(m, "rec0= %08x ", val);
        val = kl2_readl(kl2_dev,
                        KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_REC_1);
        seq_printf(m, "rec1= %08x ", val);
        //val = kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_TRACE_LOST);
        //seq_printf(m, "tlost= %d ", val);
        //val = kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_INTR_TMR);
        //seq_printf(m, "itmr= %d ", val);
        //val = kl2_readl(kl2_dev, KL2_REG_SSE_QSTATUS_BASE(sse_base, hwq_id) + KL2_REG_SSE_QSTATUS_TASK_TMR);
        //seq_printf(m, "tcycle= %d ", val);
        seq_printf(m, "taint= %d ", atomic_read(&kl2_dev->hwq[hwq_id].taint_state));
        seq_printf(m, "tmr= %d ", atomic_read(&kl2_dev->hwq[hwq_id].regular_timer_state));
        seq_printf(m, "\n");
    }
    seq_printf(m, "--\n");

    seq_printf(m, "timer_seq       = %08x   excp_work_seq   = %08x   excp_work_running = %x\n",
               kl2_dev->exception_stash.regular_timer_seq, kl2_dev->exception_stash.excp_work_seq,
               kl2_dev->exception_stash.excp_work_running);
    seq_printf(m, "excp_board_hwq  = ");
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        u32 excp_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
        u32 taint_2_reset_cnt =
                atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_2_reset_cnt);
        u32 taint_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_cnt);
        u32 token     = kl2_dev->exception_board_hwq[hwq_id].token;
        if (excp_cnt || taint_2_reset_cnt || taint_cnt)
            seq_printf(m, "hwq_%d(excp= %u, t2r= %u, taint= %u, tk= %08x) ", hwq_id, excp_cnt,
                       taint_2_reset_cnt, taint_cnt, token);
    }
    seq_printf(m, "\n");
    seq_printf(m, "excp_board_cl   = ");
    for_each_valid_cluster(kl2_dev, i) {
        u32 excp_cnt          = atomic_read(&kl2_dev->exception_board_cluster[i].excp_cnt);
        u32 reset_cnt         = atomic_read(&kl2_dev->exception_board_cluster[i].reset_cnt);
        u32 timeout_cnt       = atomic_read(&kl2_dev->exception_board_cluster[i].timeout_cnt);
        u32 timeout_reset_cnt = atomic_read(&kl2_dev->exception_board_cluster[i].timeout_reset_cnt);
        if ((excp_cnt != reset_cnt) | (timeout_cnt != timeout_reset_cnt))
            seq_printf(m, "cl%d(excp= %u|%u, tmout= %u|%u) ", i, excp_cnt, reset_cnt, timeout_cnt,
                       timeout_reset_cnt);
    }
    seq_printf(m, "\n");
    seq_printf(m, "excp_board_sd   = ");
    for_each_valid_sdnn(kl2_dev, i) {
        u32 excp_cnt          = atomic_read(&kl2_dev->exception_board_sdnn[i].excp_cnt);
        u32 reset_cnt         = atomic_read(&kl2_dev->exception_board_sdnn[i].reset_cnt);
        u32 timeout_cnt       = atomic_read(&kl2_dev->exception_board_sdnn[i].timeout_cnt);
        u32 timeout_reset_cnt = atomic_read(&kl2_dev->exception_board_sdnn[i].timeout_reset_cnt);
        if ((excp_cnt != reset_cnt) | (timeout_cnt != timeout_reset_cnt))
            seq_printf(m, "sd%d(excp= %u|%u, tmout= %u|%u) ", i, excp_cnt, reset_cnt, timeout_cnt,
                       timeout_reset_cnt);
    }
    seq_printf(m, "\n");
    seq_printf(m, "--\n");

    seq_printf(m, "support: stall_hwq_x unstall_hwq_x\n");
    kl2_reg_unlock(kl2_dev);
    return 0;
}

DEFINE_DEVPROC_WR(sse_debug)
{
    struct seq_file   *seqfile      = file->private_data;
    struct kl2_device *kl2_dev      = seqfile->private;
    unsigned long      flags        = 0;
    int                hwq_id       = -1;
    char               user_str[64] = { 0 };

    const char *stall_sse_cmd   = "stall_hwq_";
    const char *unstall_ssd_cmd = "unstall_hwq_";

    if ((count > sizeof(user_str) - 1) || *pos != 0)
        return -EINVAL;
    if (copy_from_user(user_str, buffer, count))
        return -EFAULT;
    user_str[count] = '\0';

    if (IS_CMD_STR(user_str, stall_sse_cmd)) {
        if (sscanf(user_str + strlen(stall_sse_cmd), "%d", &hwq_id) <= 0) {
            KL2_LOGI("stall_hwq: invalid param:%s\n", user_str);
            return -EINVAL;
        }

        if (hwq_id < 0 || hwq_id > KL2_HWQ_CNT) {
            return -EINVAL;
        }

        KL2_LOGI("stall sse queue %d\n", hwq_id);

        spin_lock_irqsave(&kl2_dev->hwq[hwq_id].lock, flags);
        kl2_sse_hwq_stall(kl2_dev, hwq_id);
        spin_unlock_irqrestore(&kl2_dev->hwq[hwq_id].lock, flags);

    } else if (IS_CMD_STR(user_str, unstall_ssd_cmd)) {
        if (sscanf(user_str + strlen(unstall_ssd_cmd), "%d", &hwq_id) <= 0) {
            KL2_LOGI("stall_hwq: invalid param:%s\n", user_str);
            return -EINVAL;
        }

        if (hwq_id < 0 || hwq_id > KL2_HWQ_CNT) {
            return -EINVAL;
        }

        KL2_LOGI("unstall sse queue %d\n", hwq_id);

        spin_lock_irqsave(&kl2_dev->hwq[hwq_id].lock, flags);
        kl2_sse_hwq_unstall(kl2_dev, hwq_id);
        spin_unlock_irqrestore(&kl2_dev->hwq[hwq_id].lock, flags);

    } else {
        return -EINVAL;
    }

    return count;
}

DEFINE_DEVPROC_SHOW(task_timeout_detect_threshold_in_ms)
{
    struct kl2_device *kl2_dev = m->private;
    seq_printf(m, "%d\n", kl2_dev->task_timeout_detect.detect_threshold_in_ms);
    return 0;
}

DEFINE_DEVPROC_WR(task_timeout_detect_threshold_in_ms)
{
    struct seq_file   *seqfile = file->private_data;
    struct kl2_device *kl2_dev = seqfile->private;
    char               user_str[64];
    int                user_int = 0;
    int                old;

    if ((count > sizeof(user_str) - 1) || *pos != 0)
        return -EINVAL;
    if (copy_from_user(user_str, buffer, count))
        return -EFAULT;
    user_str[count] = '\0';

    if (kstrtoint(user_str, 10, &user_int) || user_int < 3000) {
        KL2_LOGW("invalid conf, user_str= %s, user_int= %d\n", user_str, user_int);
        return -EINVAL;
    } else {
        old = kl2_dev->task_timeout_detect.detect_threshold_in_ms;
        kl2_dev->task_timeout_detect.detect_threshold_in_ms = user_int;
        KL2_LOGI("set task_timeout_detect.detect_threshold_in_ms= %d, old= %d\n",
                 kl2_dev->task_timeout_detect.detect_threshold_in_ms, old);
    }

    return count;
}

static struct kl_proc_entry kl2_proc_debug_entries[] = {
    XPU_RW_PROC_ENTRY(cuen),        XPU_RO_PROC_ENTRY(cu_debug),
    XPU_RO_PROC_ENTRY(memory_frag), XPU_RO_PROC_ENTRY(perf_test),
    XPU_RW_PROC_ENTRY(soft_reset),  XPU_RW_PROC_ENTRY(soft_reset_mode0_with_gddr),
    XPU_RW_PROC_ENTRY(sse_debug),   XPU_RW_PROC_ENTRY(task_timeout_detect_threshold_in_ms),
};

/*
 *   /proc/xpu/devX/ entries
 */

DEFINE_DEVPROC_SHOW(eccinfo)
{
    struct kl2_device *kl2_dev = m->private;

    seq_printf(m,
               "SBE(Single Bit Error ):\t%llx\n"
               "DBE(Double Bits Error):\t%llx\n",
               kl2_dev->dev_info.ecc_sbe_count, kl2_dev->dev_info.ecc_dbe_count);

    return 0;
}

DEFINE_DEVPROC_SHOW(errinfo)
{
    struct kl2_device *kl2_dev   = m->private;
    int                kl2_state = kl2_get_state(kl2_dev);

    if (kl2_state != KL2_ERROR) {
        seq_printf(m, "No error\n");
        return 0;
    }

    switch (kl2_dev->errno) {
    case XPUERR_KEXCEPTION:
        seq_printf(m, "XPUERR_KEXCEPTION\n");
        break;
    case XPUERR_DEVRESET:
        seq_printf(m, "XPUERR_DEVRESET\n");
        break;
    case XPUERR_DEVINIT:
        seq_printf(m, "XPUERR_DEVINIT\n");
        break;
    default:
        seq_printf(m, "XPUERR_UNKNOWN\n");
        break;
    }

    return 0;
}

DEFINE_DEVPROC_SHOW(errtask)
{
    struct kl2_device *kl2_dev = m->private;
    u32                cur, begin, end;
    struct kl2_etask  *etask;

    spin_lock(&kl2_dev->etasks_lock);
    cur = atomic_read(&kl2_dev->etasks_cur);
    if (cur <= KL2_ETASK_SAVE_CNT) {
        begin = 0;
        end   = cur;
    } else {
        begin = cur % KL2_ETASK_SAVE_CNT;
        end   = begin + KL2_ETASK_SAVE_CNT;
    }
    for (cur = begin; cur < end; ++cur) {
        etask = &kl2_dev->etasks[cur % KL2_ETASK_SAVE_CNT];
#define KL2_PRINT_FMT_STR(fmt_str, ...) seq_printf(m, fmt_str, ##__VA_ARGS__)
        kl2_print_etask(kl2_dev, etask, /* debug_info= */ 1);
#undef KL2_PRINT_FMT_STR
    }
    spin_unlock(&kl2_dev->etasks_lock);

    return 0;
}

DEFINE_DEVPROC_SHOW(info)
{
    struct kl2_device *kl2_dev = m->private;
    struct kl_device  *kdev    = kl2_dev->kdev;

    seq_printf(m,
               "DeviceName:\t%s\n"
               "DeviceAddr:\t%04x:%02x:%02x.%x\n"
               "DeviceSN:\t%.16s\n"
               "DriverVersion:\t%u.%u.%u\n",
               "Kunlun2", //TODO: placeholder
               kdev->domain & 0xffff, kdev->bus & 0xff, kdev->slot & 0x1f, kdev->func & 0x7,
               (const char *)&kl2_dev->dev_info.sn[0], XPURT_VERSION_MAJOR, XPURT_VERSION_MINOR,
               XPURT_VERSION_THIRD);

    return 0;
}

DEFINE_DEVPROC_WR(info)
{
    struct seq_file      *seqfile = file->private_data;
    struct kl2_device    *kl2_dev = seqfile->private;
    char                  user_str[64];
    int                   i;
    struct kl_proc_entry *entry;

    const char *whosyourdaddy = "whosyourdaddy";
    const char *godblessyou   = "godblessyou";

    if ((count > sizeof(user_str) - 1) || *pos != 0)
        return -EINVAL;
    if (copy_from_user(user_str, buffer, count))
        return -EFAULT;
    user_str[count] = '\0';

    mutex_lock(&kl2_dev->big_global_lock);
    if (IS_CMD_STR(user_str, whosyourdaddy)) {
        if (!kl2_dev->proc_debug_toggle) {
            for (i = 0; i < ARRAY_SIZE(kl2_proc_debug_entries); ++i) {
                entry = &kl2_proc_debug_entries[i];
                proc_create_data(entry->name, 0666, kl2_dev->proc_root, &entry->fops, kl2_dev);
            }
            kl2_dev->proc_debug_toggle = true;
        }
    } else if (IS_CMD_STR(user_str, godblessyou)) {
        if (kl2_dev->proc_debug_toggle) {
            for (i = 0; i < ARRAY_SIZE(kl2_proc_debug_entries); ++i) {
                entry = &kl2_proc_debug_entries[i];
                remove_proc_entry(entry->name, kl2_dev->proc_root);
            }
            kl2_dev->proc_debug_toggle = false;
        }
    }
    mutex_unlock(&kl2_dev->big_global_lock);
    return count;
}

DEFINE_DEVPROC_SHOW(limits)
{
    struct kl2_device *kl2_dev = m->private;
    kl_cxpu_t         *cxpu    = &kl2_dev->kdev->cxpu;

    kl_cxpu_proc_show(cxpu, m);

    return 0;
}

DEFINE_DEVPROC_SHOW(mminfo)
{
    struct kl2_device *kl2_dev = m->private;
    struct kl_mm      *mm      = &kl2_dev->mm;
    u64                l3_used, l3_size, main_used, main_size;

    l3_used   = kl_mm_get_bytes_used(mm, XPU_MEM_L3);
    l3_size   = kl_mm_get_bytes_all(mm, XPU_MEM_L3);
    main_used = kl_mm_get_bytes_used(mm, XPU_MEM_MAIN);
    main_size = kl_mm_get_bytes_all(mm, XPU_MEM_MAIN);

    seq_printf(m, "L3: %llu / %llu MB\n", DIV_ROUND_UP(l3_used, 0x100000),
               DIV_ROUND_UP(l3_size, 0x100000));

    seq_printf(m, "MAIN: %llu / %llu MB\n", DIV_ROUND_UP(main_used, 0x100000),
               DIV_ROUND_UP(main_size, 0x100000));

    return 0;
}

DEFINE_DEVPROC_SHOW(profile)
{
    struct kl2_device *kl2_dev = m->private;
    struct kl_device  *kdev    = kl2_dev->kdev;
    int                i       = 0;

    seq_printf(m, "\n"
                  "\tProfiler_name\t\t\tcnt\tcost(cycles)\n");
    for (i = 0; i < PROFILER_COUNT; ++i) {
        if (kdev->profiler[i].count)
            seq_printf(m, "%32s %8u\t%10llu\n", kdev->profiler[i].name, kdev->profiler[i].count,
                       kdev->profiler[i].cost);
    }
    return 0;
}

DEFINE_DEVPROC_WR(profile)
{
    struct seq_file   *seqfile      = file->private_data;
    struct kl2_device *kl2_dev      = seqfile->private;
    struct kl_device  *kdev         = kl2_dev->kdev;
    char               user_str[64] = { 0 };
    int                i            = 0;

    if ((count > sizeof(user_str) - 1) || *pos != 0)
        return -EINVAL;
    if (copy_from_user(user_str, buffer, count))
        return -EFAULT;
    user_str[count] = '\0';

    if (strncmp(user_str, "clear", 5) == 0) {
        for (i = 0; i < PROFILER_COUNT; ++i) {
            kdev->profiler[i].cost  = 0;
            kdev->profiler[i].count = 0;
        }
    }

    return count;
}

DEFINE_DEVPROC_SHOW(regular_timer_toggle)
{
    struct kl2_device *kl2_dev = m->private;

    mutex_lock(&kl2_dev->big_global_lock);
    if (kl2_dev->regular_timer_toggle) {
        seq_printf(m, "1\n");
    } else {
        seq_printf(m, "0\n");
    }
    mutex_unlock(&kl2_dev->big_global_lock);
    return 0;
}

DEFINE_DEVPROC_WR(regular_timer_toggle)
{
    struct seq_file   *seqfile      = file->private_data;
    struct kl2_device *kl2_dev      = seqfile->private;
    char               user_str[64] = { 0 };

    if ((count > sizeof(user_str) - 1) || *pos != 0)
        return -EINVAL;
    if (copy_from_user(user_str, buffer, count))
        return -EFAULT;
    user_str[count] = '\0';

    mutex_lock(&kl2_dev->big_global_lock);
    if (strncmp(user_str, "0", 1) == 0) {
        if (kl2_dev->regular_timer_toggle) {
            hrtimer_cancel(&kl2_dev->regular_timer);
            kl2_dev->regular_timer_toggle = false;
            KL2_LOGI("set regular_timer_toggle to 0\n");
        }
    } else if (strncmp(user_str, "1", 1) == 0) {
        if (!kl2_dev->regular_timer_toggle) {
            hrtimer_start(&kl2_dev->regular_timer, kl2_dev->regular_ktime, HRTIMER_MODE_REL);
            kl2_dev->regular_timer_toggle = true;
            KL2_LOGI("set regular_timer_toggle to 1\n");
        }
    }
    mutex_unlock(&kl2_dev->big_global_lock);

    return count;
}

DEFINE_DEVPROC_SHOW(stash_for_ltloop)
{
    return 0;
}

DEFINE_DEVPROC_WR(stash_for_ltloop)
{
    struct seq_file   *seqfile      = file->private_data;
    struct kl2_device *kl2_dev      = seqfile->private;
    char               user_str[64] = { 0 };
    int                ret;

    if ((count > sizeof(user_str) - 1) || *pos != 0)
        return -EINVAL;
    if (copy_from_user(user_str, buffer, count))
        return -EFAULT;
    user_str[count] = '\0';

    mutex_lock(&kl2_dev->big_global_lock);
    if (strncmp(user_str, "1", 1) == 0) {
        kl2_writel(kl2_dev, 0x800000, kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD);
        kl2_writel(kl2_dev, BIT(0), kl2_dev->iomem_base.intc_base + KL2_REG_INTC_SET_7);
        ret = kl_poll_cond_timeout(
                (kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD) == 0), 10000,
                10000000 /* 10s */);
        if (ret) {
            KL2_LOGW("stash_for_ltloop timeout ...\n");
        } else {
            KL2_LOGI("stash_for_ltloop done ...\n");
        }
    }
    mutex_unlock(&kl2_dev->big_global_lock);

    return count;
}

DEFINE_DEVPROC_WR(state)
{
    struct seq_file   *seqfile = file->private_data;
    struct kl2_device *kl2_dev = seqfile->private;
    char               user_str[64];

    const char *st_error   = "ERROR";
    const char *st_running = "RUNNING";

    if ((count > sizeof(user_str) - 1) || *pos != 0)
        return -EINVAL;
    if (copy_from_user(user_str, buffer, count))
        return -EFAULT;
    user_str[count] = '\0';

    mutex_lock(&kl2_dev->big_global_lock);
    if (IS_CMD_STR(user_str, st_error)) {
        kl2_set_state(kl2_dev, KL2_ERROR);
    } else if (IS_CMD_STR(user_str, st_running)) {
        kl2_set_state(kl2_dev, KL2_RUNNING);
    }
    mutex_unlock(&kl2_dev->big_global_lock);

    return count;
}

DEFINE_DEVPROC_SHOW(state)
{
    struct kl2_device *kl2_dev   = m->private;
    int                kl2_state = kl2_get_state(kl2_dev);
    seq_printf(m, "%s\n", kl2_device_state_str(kl2_state));
    return 0;
}

static void seq_printf_kernel_task(struct seq_file *m, struct kl2_task *task)
{
    union kl2_sse_task_desc *desc = &task->desc;

    seq_printf(m, "  TASK tk=%08x pid=%d sess=%d", desc->kernel.token, task->pid, task->sess_id);

    switch (desc->kernel.type) {
    case KL2_SSE_TASKTYPE_CL:
        seq_printf(m, " ty=CL");
        break;
    case KL2_SSE_TASKTYPE_SDNN:
        seq_printf(m, " ty=SD");
        break;
    default:
        seq_printf(m, " ty=Er\n");
        return;
    }

    seq_printf(m,
               " name=%s code=0x%llx argv=0x%llx argsz=%u ksz=0x%x"
               " ncl=%u nco=%u\n",
               task->kernel_name, (u64)desc->kernel.code_addr, (u64)desc->kernel.param_addr,
               desc->kernel.param0, desc->kernel.codelen, desc->kernel.nclusters,
               desc->kernel.ncores);
}

static void seq_printf_kernel_task_param(struct seq_file *m, struct kl2_task *task)
{
    u32 kk;
    for (kk = 0; kk < task->desc.kernel.param0 / sizeof(u32); ++kk)
        seq_printf(m, "  ..param[%d]= 0x%x\n", kk, task->params[kk]);
}

static void seq_printf_ctrl_task(struct seq_file *m, struct kl2_task *task)
{
    union kl2_sse_task_desc *desc = &task->desc;

    seq_printf(m, "  TASK tk=%08x pid=%d sess=%d", desc->kernel.token, task->pid, task->sess_id);

    switch (desc->ctrl.type) {
    case KL2_SSE_TASKTYPE_EVNTREC:
        seq_printf(m, " ty=EVNTREC seq=%llx\n", desc->ctrl.record_seq);
        break;
    case KL2_SSE_TASKTYPE_EVNTWAIT:
        seq_printf(m, " ty=EVNTWAIT hwq=%u seq=%llx\n", desc->ctrl.wait_vstream_id,
                   desc->ctrl.record_seq);
        break;
    default:
        seq_printf(m, " ty=Er\n");
    }
}

DEFINE_DEVPROC_SHOW(tqinfo)
{
    struct kl2_device *kl2_dev = m->private;
    int                i;

    for (i = 0; i < KL2_HWQ_CNT; ++i) {
        struct kl2_hwq  *hwq = &kl2_dev->hwq[i];
        struct kl2_task *task;
        unsigned long    flags;
        bool             is_first_kernel;
        int              cnt;

        if (!hwq->enable)
            continue;

        seq_printf(m, "hwq%d: state=N/A #all=%u #run=%u seq=%llx\n", hwq->id, hwq->cnt_all,
                   hwq->cnt_running, (u64)atomic64_read(&hwq->evnt_seq));

        spin_lock_irqsave(&hwq->lock, flags);

        // running list
        is_first_kernel = true;
        list_for_each_entry(task, &hwq->rt_list, hwq_node) {
            if (task->type == KL2_TASKTYPE_KERNEL) {
                seq_printf_kernel_task(m, task);
                if (is_first_kernel) {
                    seq_printf_kernel_task_param(m, task);
                    is_first_kernel = false;
                }
            } else {
                seq_printf_ctrl_task(m, task);
            }
        }

        seq_printf(m, "  --\n");

        // pending list
        cnt = 0;
        list_for_each_entry(task, &hwq->pt_list, hwq_node) {
            if (task->type == KL2_TASKTYPE_KERNEL)
                seq_printf_kernel_task(m, task);
            else
                seq_printf_ctrl_task(m, task);

            ++cnt;
            if (cnt >= 5)
                break;
        }

        spin_unlock_irqrestore(&hwq->lock, flags);
    }

    return 0;
}

DEFINE_DEVPROC_SHOW(use_ratio)
{
    struct kl2_device *kl2_dev = m->private;
    seq_printf(m, "%d\n", ur_weight(&kl2_dev->ur));
    return 0;
}

#ifdef ENABLE_CODEC
#define VDEC_REG_READ(core_id, reg_offset)                                                         \
    ioread32((void *)(pvdec_device->reg_base_virt_addr[core_id] + reg_offset))
DEFINE_DEVPROC_SHOW(video_state)
{
    struct kl2_device *kl2_dev      = m->private;
    int                i            = 0;
    int                j            = 0;
    u32                reg_value    = 0;
    enc_dev_t         *pvenc_device = kl2_dev->video_enc;
    vdecdev_t         *pvdec_device = kl2_dev->video_dec;

    kl2_reg_lock(kl2_dev);
    seq_printf(m, "video enc");
    for (i = 0; i < (sizeof(pvenc_device->enc_data) / sizeof(pvenc_device->enc_data[0])); i++) {
        enc_data_t *penc_data = &pvenc_device->enc_data[i];
        seq_printf(m,
                   "\n    subsys_id: %i"
                   "\n              is_valid: %i"
                   "\n              subsys_data.core_info.type_info:  0x%x",
                   i, penc_data->is_valid, penc_data->subsys_data.core_info.type_info);

        for (j = 0; j < (sizeof(penc_data->is_reserved) / sizeof(penc_data->is_reserved[0])); j++) {
            seq_printf(m,
                       "\n              core_type: %i"
                       "\n                  is_reserved: %i"
                       "\n                  pid: %i"
                       "\n                  enc_owner: %p"
                       "\n                  irq_received: %i"
                       "\n                  irq_status: 0x%x",
                       j, penc_data->is_reserved[j], penc_data->pid[j], penc_data->enc_owner[j],
                       penc_data->irq_received[j], penc_data->irq_status[j]);
        }
    }

    seq_printf(m, "\nvideo dec");
    for (i = 0; i < VDEC_REGS_NUM; i++) {
        if (0 == i) {
            seq_printf(m, "\nreg\\core:");
            for (j = 0; j < VDEC_MAX_CORES; j++) {
                if (0 == j) {
                    seq_printf(m, "%6i", j);
                } else {
                    seq_printf(m, "%10i", j);
                }
            }
        }
        seq_printf(m, "\n%4i:", i);
        for (j = 0; j < VDEC_MAX_CORES; j++) {
            reg_value = VDEC_REG_READ(j, i * 4);
            seq_printf(m, "%10x", reg_value);
        }
    }

    seq_putc(m, '\n');
    kl2_reg_unlock(kl2_dev);
    return 0;
}
#endif

// clang-format off
static struct kl_proc_entry kl2_proc_entries[] = {
    XPU_RO_PROC_ENTRY(eccinfo),
    XPU_RO_PROC_ENTRY(errinfo),
    XPU_RO_PROC_ENTRY(errtask),
    XPU_RW_PROC_ENTRY(info),
    XPU_RO_PROC_ENTRY(limits),
    XPU_RO_PROC_ENTRY(mminfo),
    XPU_RW_PROC_ENTRY(profile),
    XPU_RW_PROC_ENTRY(regular_timer_toggle),
    XPU_RW_PROC_ENTRY(stash_for_ltloop),
    XPU_RW_PROC_ENTRY(state),
    XPU_RO_PROC_ENTRY(tqinfo),
    XPU_RO_PROC_ENTRY(use_ratio),
#ifdef ENABLE_CODEC
    XPU_RO_PROC_ENTRY(video_state),
#endif
};
// clang-format on

int kl2_device_proc_create(struct proc_dir_entry *proc_root, struct kl2_device *kl2_dev)
{
    snprintf(kl2_dev->proc_name, XPU_MAX_STRLEN, "dev%d", kl2_dev->kinode->devfile_id);
    kl2_dev->proc_root = proc_entries_create(proc_root, kl2_dev->proc_name, kl2_proc_entries,
                                             ARRAY_SIZE(kl2_proc_entries), kl2_dev);
    if (kl2_dev->proc_root == NULL) {
        return -EFAULT;
    }

    return 0;
}

void kl2_device_proc_destroy(struct proc_dir_entry *proc_root, struct kl2_device *kl2_dev)
{
    struct kl_proc_entry *entry;
    int                   i;

    mutex_lock(&kl2_dev->big_global_lock);
    if (kl2_dev->proc_debug_toggle) {
        for (i = 0; i < ARRAY_SIZE(kl2_proc_debug_entries); ++i) {
            entry = &kl2_proc_debug_entries[i];
            remove_proc_entry(entry->name, kl2_dev->proc_root);
        }
        kl2_dev->proc_debug_toggle = false;
    }
    mutex_unlock(&kl2_dev->big_global_lock);

    proc_entries_destroy(proc_root, kl2_dev->proc_name, kl2_dev->proc_root, kl2_proc_entries,
                         ARRAY_SIZE(kl2_proc_entries));
}
