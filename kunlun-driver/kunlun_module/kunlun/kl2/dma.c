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

#include "kl2/dma.h"
#include "kl2/kl2.h"
#include "kl2/hw.h" // For sgdma related macros
#include "kl2/disable_reg_debug.h"

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/hrtimer.h> // For ktime_get() in linux-3.10.0
#include <linux/ktime.h>   // For ktime_get() in higher version
#include <linux/vmalloc.h>

#define Kl2_MAX_DMA_DATA_LENGTH (4 * 1024 * 1024 * 1024ULL) // 4GB
// 最大支持的每desc传输大小，受desc.desc_page_size 24bit限制
#define KL2_MAX_DESC_PAGE_SIZE (0x1000000) // 16MB

//typedef struct {
//    unsigned long   page_count;
//    struct page   **user_pages;
//    struct sg_table sgt;
//    int             mapped_nents;
//} sg_dma_data_t;

typedef struct __attribute__((packed)) {
    union {
        struct {
            volatile u32 desc_status_num : 4; /* provides the Status Number of
                                                  the DMA Engine. This number is incremented, enabling the
                                                  application to determine the last processed Descriptor,
                                                  which is key in streaming flow between asynchronous devices.*/
            volatile u32 desc_prc_status : 4; /* • Bit 4: SG-DMA Descriptor has been processed.
                                                    • Bit 5: an error occurred during the processing of the current
                                                             SG-DMA Descriptor.
                                                    • Bit 6: an EOP (End of Packet) condition has been reported
                                                             by the source of the SG-DMA transfer.
                                                    • Bit 7: reserved
                                                    Note: It is recommended that the application clears
                                                    DESC_PRC_STATUS, so that it can determine when the
                                                    STATUS field has been updated by polling Bit 0.*/
            volatile u32 desc_prc_page_size : 24; /* provides the Processed
                                                    Page Size, which is the actual written or read page size.
                                                    It can be different from PAGE_SIZE if the Descriptor
                                                    processing has been shortened due to an error or EOP
                                                    detection (see SE_COND description).*/
        } desc_status_field;

        volatile u32 desc_status;
    };

    union {
        struct {
            u32 desc_status_req : 1; /* defines whether the DMA Engine
                                            provides a status report by writing to DESC_STATUS when the
                                            current SG-DMA Descriptor has been processed. This enables
                                            the application to easily monitor DMA progress without polling the
                                            Bridge’s DMA Engine registers.*/
            u32 desc_type : 3;       /* indicates the current SG-DMA Descriptor
                                            mapping. Its value is reserved and equal to 3’b000 for the current
                                            SG-DMA Descriptor Mapping.*/
            u32 desc_irq : 4;        /* defines when an interrupt should be issued:
                                            • Bit 4: an IRQ is issued when this SG-DMA Descriptor has
                                                        been processed.
                                            • Bit 5: an IRQ is issued if an error occurs.
                                            • Bit 6: an IRQ is issued if the source of the transfer reports an
                                                        EOP condition.
                                            • Bit 7: reserved*/
            u32 desc_page_size : 24; /* provides the Page Size in Bytes,
                                            from 1 to 16 Mbytes. If set to 24’h0, it specifies a value of 16
                                            Mbytes.*/
        } desc_control_field;

        u32 desc_control;
    };

    struct {
        u32 desc_se_cond : 4;          /* defines the Start and End conditions for
                                        SG-DMA Descriptor processing. It is composed of the following
                                        sub-fields:
                                        • Bit 0: End the DMA transfer after this SG-DMA Descriptor
                                                 has been processed (equivalent to an End Of Chain).
                                        • Bit 1: Abort this SG-DMA Descriptor processing if an error
                                                 occurs.
                                        • Bit 2:
                                            • If the destination of the SG-DMA is an AXI4 Stream
                                            Interface and SG-Type is 01, generate an EOP at the end of
                                            SG-DMA Descriptor processing. SE_COND[2] of the
                                            DMA_CONTROL parameter must be asserted to enable this
                                            feature.
                                            • If the source of the SG-DMA is an AXI-Stream Interface and
                                            SG-Type is 10, stop SG-DMA Descriptor processing if the
                                            source of the transfer reports an EOP condition.
                                        • Bit 3: Start this SG-DMA Descriptor processing when the
                                            source of the transfer reports an SOP reception (only
                                            relevant when the source is an AXI4 Stream Interface, on
                                            data following a TLAST assertion).
                                        If bits [2: 1] of the DESC_SE_COND field are cleared, DMA will
                                        stop once the PAGE_SIZE bytes are processed.
                                        If the DMA Transfer length is unknown, the application may want to
                                        build its SG-DMA Chain List dynamically. In this case, the
                                        application can allow the last built SG-DMA Descriptor to be
                                        cleared and both the DESC_NEXT_ADDR field and Bit 0 of the
                                        DESC_SE_COND field are considered irrelevant for the DMA
                                        Engine.
                                        Once the application has built the next Descriptor (or if the current
                                        SG-DMA Descriptor is the last one in the Chain List), it should
                                        update the current SG-DMA Descriptor, setting
                                        DESC_NEXT_ADDR and DESC_SE_COND’s Bit 0 to their defined
                                        values.
                                        The DMA Engine regularly checks if the SG-DMA Descriptor has
                                        been updated (every 1μs to 65μs, depending on the Core Constant
                                        configuration). However the application can set the DESC_UPDT
                                        field of the DMA Engine’s DMA_CONTROL register to 1b
                                        to indicate that a Descriptor has been updated.*/
        u32 desc_next_ready : 1;       /* indicates if the next SG-DMA Descriptor is
                                        ready (and fetchable). The application can set it to 0b to indicate
                                        that the chain list is not ended, but will be completed later.*/
        u32 desc_next_address_lo : 27; /* Next Descriptor Address. This field must be
                                        aligned on a 32-byte boundary.*/
    };

    u32 desc_next_address_hi;
    u32 desc_src_address_lo;
    u32 desc_src_address_hi; /* Source Address. If not used (SG_TYPE field
                                        set to 2’b10), the application can set this field to 64’h0.*/
    u32 desc_dst_address_lo;
    u32 desc_dst_address_hi; /* Destination Address. If not used (SG_TYPE
                                        field set to 2’b01), the application can set this field to 64’h0.*/
} plda_dma_descriptor_t;

#define SG_DMA_ADVANCED_PERF_MODE
#ifdef SG_DMA_ADVANCED_PERF_MODE

// 当前使用的每desc传输大小，调试时可更改
#define KL2_DESC_PAGE_SIZE KL2_MAX_DESC_PAGE_SIZE
// 每次提交sg_dma_update_desc需积攒的desc数量
const u32 SG_DMA_ADVANCED_NR_DESC_IN_BATCH = 10;
// 可被使用的desc总size
const u32 SG_DMA_ADVANCED_TOTAL_DESC_SIZE = KL2_DMA_KBUF_SIZE;

#else // SG_DMA_ADVANCED_TORTURE_MODE

#define KL2_DESC_PAGE_SIZE (0x1000 - 1)
const u32 SG_DMA_ADVANCED_NR_DESC_IN_BATCH = 3;
const u32 SG_DMA_ADVANCED_TOTAL_DESC_SIZE  = 4 * sizeof(plda_dma_descriptor_t);

#endif

int kl2_dma_init(struct kl2_device *kl2_dev, int ch_num, int ch_bits, struct dma_ops *ops)
{
    struct dma_engine *dma  = &kl2_dev->dma_engine;
    struct pci_dev    *pdev = kl2_dev->kdev->pdev;
    int                i, err;

    dma->pdev = pdev;
    dma->ops  = ops;
    dma->data = kl2_dev;

    dma->ch_num = ch_num;
    sema_init(&dma->sema, bitcount(ch_bits));
    spin_lock_init(&dma->bitmap_lock);
    bitmap_clear(dma->bitmap, 0, dma->ch_num);

    for (i = 0; i < ch_num; ++i) {
        dma->ch[i].idx = i;
        dma->ch[i].ch  = i;

        if (!(ch_bits & BIT(i))) {
            bitmap_set(dma->bitmap, i, 1);
            continue;
        }

        // malloc CPU memory in kernel space for dma use
        dma->ch[i].buffer = kmalloc(KL2_DMA_KBUF_SIZE, GFP_KERNEL);
        if (!dma->ch[i].buffer) {
            err = -ENOMEM;
            goto err_undo_mapping;
        }

        // map kbuf into dma address space
        dma->ch[i].dma_addr = dma_map_single(&dma->pdev->dev, dma->ch[i].buffer, KL2_DMA_KBUF_SIZE,
                                             DMA_BIDIRECTIONAL);
        err                 = dma_mapping_error(&dma->pdev->dev, dma->ch[i].dma_addr);
        if (err) {
            KL2_LOGW("dma_mapping_error= %d, ch[%d].buffer= %px, ch[%d].dma_addr= %llx\n", err, i,
                     dma->ch[i].buffer, i, dma->ch[i].dma_addr);
            err = -ENOMEM;
            goto err_undo_mapping;
        }

        KL2_LOGI("dma_map_single, ch[%d].buffer= %px, ch[%d].dma_addr= %llx\n", i,
                 dma->ch[i].buffer, i, dma->ch[i].dma_addr);
    }

    // Allow large DMA segments, up to 16MB
    dma_set_max_seg_size(&pdev->dev, KL2_MAX_DESC_PAGE_SIZE);

    return 0;

err_undo_mapping:
    while (i >= 0) {
        if (dma->ch[i].dma_addr && !dma_mapping_error(&dma->pdev->dev, dma->ch[i].dma_addr)) {
            dma_unmap_single(&dma->pdev->dev, dma->ch[i].dma_addr, KL2_DMA_KBUF_SIZE,
                             DMA_BIDIRECTIONAL);
        }
        if (dma->ch[i].buffer) {
            kfree(dma->ch[i].buffer);
        }

        --i;
    }

    return err;
}

void kl2_dma_destroy(struct kl2_device *kl2_dev)
{
    struct dma_engine *dma = &kl2_dev->dma_engine;
    int                i;

    for (i = 0; i < dma->ch_num; ++i) {
        if (dma->ch[i].dma_addr && !dma_mapping_error(&dma->pdev->dev, dma->ch[i].dma_addr)) {
            dma_unmap_single(&dma->pdev->dev, dma->ch[i].dma_addr, KL2_DMA_KBUF_SIZE,
                             DMA_BIDIRECTIONAL);
        }
        if (dma->ch[i].buffer) {
            kfree(dma->ch[i].buffer);
        }
    }
}

// XXX(miaotianxiang):
// 某些旧版本libxpurt.so未填充XPUMemcpyExIoctlArgs->kind，需要h2d参数区分memcpy方向
int kl2_dma_valid_check(struct kl2_device *kl2_dev, struct XPUMemcpyExIoctlArgs *args, int h2d)
{
    struct kl_mm_info *mm_info     = kl2_dev->mm_info;
    u64                dma_start   = h2d ? args->dest : args->src;
    u64                dma_size    = args->size;
    u64                dma_end     = dma_start + dma_size;
    int                range_valid = 0;
    int                i;

    if (!dma_size || (dma_end < dma_start))
        goto range_invalid;

    // XXX(miaotianxiang): 普通用户发起dma时，设备侧仅允许传入合法Global Memory或L3或部分寄存器地址，虽然硬件可能具备从其他地址dma的能力
    for (i = 0; i < mm_info->user_dma_rr_rw_range_count; ++i) {
        struct kl_memory_range *range = &mm_info->user_dma_rr_rw_range[i];
        if ((dma_start >= range->base) && (dma_start < range->base + range->size) &&
            (dma_end > range->base) && (dma_end <= range->base + range->size)) {
            range_valid = 1;
            break;
        }
    }
    if (!range_valid)
        goto range_invalid;

    return 0;

range_invalid:
    KL2_LOGW("xpu_memcpy/dma/rr/rw range check failed, h2d= %d, sz= %llx, %llx -> %llx\n", h2d,
             args->size, args->src, args->dest);
    return 1;
}

//void kldma_lock_ch(struct dma_engine *dma, int lock_bits)
//{
//    int i = 0, chnum;
//
//    chnum = dma->ch_num - bitcount(lock_bits);
//    while (i < dma->ch_num) {
//        if (lock_bits & (1 << i))
//            bitmap_set(dma->bitmap, i, 1);
//        ++i;
//    }
//
//    dma->sema.count = chnum;
//}
//
//void kldma_unlock_allch(struct dma_engine *dma)
//{
//    bitmap_clear(dma->bitmap, 0, dma->ch_num);
//    dma->sema.count = dma->ch_num;
//}

static inline struct dma_channel *__get_channel(struct dma_engine *dma)
{
    struct kl2_device *kl2_dev __maybe_unused = dma->data;
    unsigned int               idx;
    struct dma_channel        *ret = NULL;

    KL2_LOGD("dma bitmap=%lx\n", dma->bitmap[0]);

    spin_lock(&dma->bitmap_lock);
    idx = find_first_zero_bit(dma->bitmap, dma->ch_num);
    if (idx < dma->ch_num) {
        bitmap_set(dma->bitmap, idx, 1);
        ret = &dma->ch[idx];
    }
    spin_unlock(&dma->bitmap_lock);

    return ret;
}

static inline void __put_channel(struct dma_engine *dma, struct dma_channel *ch)
{
    spin_lock(&dma->bitmap_lock);
    bitmap_clear(dma->bitmap, ch->idx, 1);
    spin_unlock(&dma->bitmap_lock);
}

static inline int __kl2_dma_ddma_to_host(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz,
                                         int is_userspace, u64 *time_ns)
{
    struct kl2_device *kl2_dev __maybe_unused = dma->data;
    struct dma_channel        *ch;
    u64                        dmasz;
    int                        ret = 0;
    int                        i   = 0;
    ktime_t                    t0, t1;
    u64                        time_acc = 0;
    u64                        szsv     = cpsz;

    if (!cpsz) {
        return -EINVAL;
    }

    /* 1. now we hold kbuf_sem first
     *  1.1 hold dma operation mtx
     *  1.2 DMA
     *  1.3 release dma operation mtx
     *  1.4 copy data
     * 2. loop end
     */
    down(&dma->sema);
    if (kl2_get_in_reset_state(kl2_dev) == KL2_IN_RESET) {
        ret = -XPUERR_DEVRESET;
        goto err_out;
    }
    ch = __get_channel(dma);
    if (!ch) {
        ret = -EINVAL;
        goto err_out;
    }
    KL2_LOGD("get dma ch_%d\n", ch->ch);

    while (cpsz) {
        // DMA size is $KL2_DMA_KBUF_SIZE at most
        dmasz = (cpsz < KL2_DMA_KBUF_SIZE) ? cpsz : KL2_DMA_KBUF_SIZE;

        dma_sync_single_for_device(&dma->pdev->dev, ch->dma_addr, dmasz, DMA_FROM_DEVICE);

        t0 = ktime_get();
        dma->ops->ddma_to_host(dma->data, (u64)ch->dma_addr, src + (u64)KL2_DMA_KBUF_SIZE * i,
                               dmasz, ch->ch);
        ret = dma->ops->wait_dma_finished(dma->data, ch->ch);
        t1  = ktime_get();
        if (ret)
            break;

        time_acc += (u64)ktime_to_ns(ktime_sub(t1, t0));

        // 海光x86/飞腾aarch64等DMA一致性较弱平台上，读channel status判断完成不意味着Host
        // 能立即看到最新的d2h数据（后续copy_to_user可能拷贝旧数据），故此处用额外MemRd来
        // 保证Post MemWr完成。
        if (g_kl_wars.add_tiny_mem_read_after_d2h_war) {
            // TODO(miaotianxiang): 搬到预留地址/尝试zero byte read
            dma->ops->ddma_from_host(dma->data, src + (u64)KL2_DMA_KBUF_SIZE * i, (u64)ch->dma_addr,
                                     dmasz > 4 ? 4 : dmasz, ch->ch);
            ret = dma->ops->wait_dma_finished(dma->data, ch->ch);
            if (ret)
                break;
        }

        dma_sync_single_for_cpu(&dma->pdev->dev, ch->dma_addr, dmasz, DMA_FROM_DEVICE);

        if (is_userspace) {
            ret = copy_to_user((void *)dest, ch->buffer, dmasz);
        } else {
            memcpy((void *)dest, ch->buffer, dmasz);
        }
        if (ret) {
            KL2_LOGW("copy_to_user= %d, dest= %px, ch->buffer= %px, sz= %llx\n", ret, (void *)dest,
                     ch->buffer, dmasz);
            ret = -EFAULT;
            break;
        }

        cpsz -= dmasz;
        dest += dmasz;
        ++i;
    }

    if (time_ns)
        *time_ns = time_acc;

    // register read?
    if (szsv == 4) {
        KL2_LOGD("RR %016llx = %08x\n", src, *((u32 *)ch->buffer));
    }

    __put_channel(dma, ch);

err_out:
    up(&dma->sema);
    return ret;
}

static inline int __kl2_dma_ddma_from_host(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz,
                                           int is_userspace, u64 *time_ns)
{
    struct kl2_device *kl2_dev __maybe_unused = dma->data;
    struct dma_channel        *ch;
    u64                        dmasz;
    int                        ret = 0;
    int                        i   = 0;
    ktime_t                    t0, t1;
    u64                        time_acc = 0;
    u64                        szsv     = cpsz;

    if (!cpsz) {
        return -EINVAL;
    }

    /* 1. now we hold kbuf_sem first
     *  1.1 copy data
     *  1.2 hold dma operation mtx
     *  1.3 DMA
     *  1.4 release dma operation mtx
     * 2. loop end
     */
    down(&dma->sema);
    if (kl2_get_in_reset_state(kl2_dev) == KL2_IN_RESET) {
        ret = -XPUERR_DEVRESET;
        goto err_out;
    }
    ch = __get_channel(dma);
    if (!ch) {
        ret = -EINVAL;
        goto err_out;
    }
    KL2_LOGD("get dma ch_%d\n", ch->ch);

    while (cpsz) {
        // DMA length is $KL2_DMA_KBUF_SIZE at most
        dmasz = (cpsz < KL2_DMA_KBUF_SIZE) ? cpsz : KL2_DMA_KBUF_SIZE;

        dma_sync_single_for_cpu(&dma->pdev->dev, ch->dma_addr, dmasz, DMA_TO_DEVICE);

        if (is_userspace) {
            ret = copy_from_user(ch->buffer, (void *)src, dmasz);
        } else {
            memcpy(ch->buffer, (void *)src, dmasz);
        }
        if (ret) {
            KL2_LOGW("copy_from_user= %d, ch->buffer= %px, src= %px, sz= %llx\n", ret, ch->buffer,
                     (void *)src, dmasz);
            ret = -EFAULT;
            break;
        }

        dma_sync_single_for_device(&dma->pdev->dev, ch->dma_addr, dmasz, DMA_TO_DEVICE);

        t0 = ktime_get();
        dma->ops->ddma_from_host(dma->data, dest + (u64)KL2_DMA_KBUF_SIZE * i, (u64)ch->dma_addr,
                                 dmasz, ch->ch);
        ret = dma->ops->wait_dma_finished(dma->data, ch->ch);
        t1  = ktime_get();
        if (ret)
            break;

        time_acc += (u64)ktime_to_ns(ktime_sub(t1, t0));

        cpsz -= dmasz;
        src += dmasz;
        ++i;
    }

    if (time_ns)
        *time_ns = time_acc;

    // register write?
    if (szsv == 4) {
        KL2_LOGD("RW %016llx = %08x\n", dest, *((u32 *)ch->buffer));
    }

    __put_channel(dma, ch);

err_out:
    up(&dma->sema);
    return ret;
}

int kl2_dma_device_to_device(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz, u64 *time_ns)
{
    struct kl2_device *kl2_dev __maybe_unused = dma->data;
    struct dma_channel        *ch;
    u64                        dmasz;
    int                        ret = 0;
    int                        i   = 0;
    ktime_t                    t0, t1;
    u64                        time_acc = 0;

    if (!cpsz) {
        return -EINVAL;
    }

    down(&dma->sema);
    if (kl2_get_in_reset_state(kl2_dev) == KL2_IN_RESET) {
        ret = -XPUERR_DEVRESET;
        goto err_out;
    }
    ch = __get_channel(dma);
    if (!ch) {
        ret = -EINVAL;
        goto err_out;
    }
    KL2_LOGD("get dma ch_%d\n", ch->ch);

    while (cpsz) {
        // DMA length is $KL2_DMA_KBUF_SIZE at most
        dmasz = (cpsz < KL2_DMA_KBUF_SIZE) ? cpsz : KL2_DMA_KBUF_SIZE;

        t0 = ktime_get();
        dma->ops->ddma_device_to_device(dma->data, dest + (u64)KL2_DMA_KBUF_SIZE * i,
                                        src + (u64)KL2_DMA_KBUF_SIZE * i, dmasz, ch->ch);
        ret = dma->ops->wait_dma_finished(dma->data, ch->ch);
        t1  = ktime_get();
        if (ret)
            break;

        time_acc += (u64)ktime_to_ns(ktime_sub(t1, t0));

        cpsz -= dmasz;
        ++i;
    }

    if (time_ns)
        *time_ns = time_acc;

    __put_channel(dma, ch);

err_out:
    up(&dma->sema);
    return ret;
}

int kl2_dma_peer_to_peer(struct dma_engine *dst_dma, struct dma_engine *src_dma, u64 dst, u64 src,
                         u64 cpsz, u64 *time_ns)
{
    struct kl2_device         *dst_dev        = dst_dma->data;
    struct kl2_device         *src_dev        = src_dma->data;
    struct kl2_device *kl2_dev __maybe_unused = src_dev;
    struct semaphore          *sema_1st;
    struct semaphore          *sema_2nd;
    struct dma_channel        *src_ch;
    struct dma_channel        *dst_ch;
    u64                        dmasz;
    ktime_t                    t0, t1;
    int                        i, loop, pingpongidx;
    dma_addr_t                 src_pp_dma_addr[2];
    dma_addr_t                 dst_pp_dma_addr[2];
    int                        ret      = 0;
    u64                        time_acc = 0;

    if (!cpsz) {
        return -EINVAL;
    }

    // keep the order of locking with devfile_id to avoid deadlock
    if (dst_dev->kinode->devfile_id == src_dev->kinode->devfile_id) {
        return -EINVAL;
    } else if (dst_dev->kinode->devfile_id < src_dev->kinode->devfile_id) {
        sema_1st = &dst_dma->sema;
        sema_2nd = &src_dma->sema;
    } else {
        sema_1st = &src_dma->sema;
        sema_2nd = &dst_dma->sema;
    }

    down(sema_1st);
    down(sema_2nd);
    if (kl2_get_in_reset_state(dst_dev) == KL2_IN_RESET) {
        ret = -XPUERR_DEVRESET;
        goto err_src_channel;
    }
    if (kl2_get_in_reset_state(src_dev) == KL2_IN_RESET) {
        ret = -XPUERR_DEVRESET;
        goto err_src_channel;
    }

    src_ch = __get_channel(src_dma);
    if (!src_ch) {
        ret = -EINVAL;
        goto err_src_channel;
    }
    dst_ch = __get_channel(dst_dma);
    if (!dst_ch) {
        ret = -EINVAL;
        goto err_dst_channel;
    }

    KL2_LOGD("get src_dma ch_%d\n", src_ch->ch);
    KL2_LOGD("get dst_dma ch_%d\n", dst_ch->ch);

    /*
     * 采用 ping-pong 的方法来提高 dma 的速率
     * -----------------------------------------------------------------------------------------------------------------------------------
     *  loop     | 0                            |  1   |  2   |  3   | ... |  n                           |                              |
     *
     *  src_dma  | buf0                         | buf1 | buf0 | buf1 | ... | buf0                         |                              |
     *  src_size | kl2_DMA_KBUF_SIZE or cpsz    |      KL2_DMA_KBUF_SIZE   | cpsz - n * KL2_DMA_KBUF_SIZE |                              |
     *
     *  dst_dma  |                              | buf0 | buf1 | buf0 | ... | buf1                         | buf0                         |
     *  dst_size |                              |                     KL2_DMA_KBUF_SIZE                   | cpsz - n * KL2_DMA_KBUF_SIZE |
     *  ----------------------------------------------------------------------------------------------------------------------------------
     */

    pingpongidx = 0;
    loop        = (cpsz + KL2_DMA_KBUF_SIZE - 1) / KL2_DMA_KBUF_SIZE;

    // XXX(liyunzheng): 解决在开启IOMMU机器上的memcpy peer to peer问题，对跨设备访问的buffer临时进行dma_map。
    src_pp_dma_addr[0] = (u64)src_ch->dma_addr;
    src_pp_dma_addr[1] = (u64)dma_map_single(&src_dma->pdev->dev, dst_ch->buffer, KL2_DMA_KBUF_SIZE,
                                             DMA_BIDIRECTIONAL);
    ret                = dma_mapping_error(&src_dma->pdev->dev, src_pp_dma_addr[1]);
    if (ret) {
        struct kl2_device *kl2_dev = src_dma->data;
        KL2_LOGW("dma_mapping_error= %d, dst_ch->buffer= %px, src_pp_dma_addr[1]= %llx\n", ret,
                 dst_ch->buffer, src_pp_dma_addr[1]);
        ret = -ENOMEM;
        goto err_src_mapping;
    }
    dst_pp_dma_addr[0] = (u64)dma_map_single(&dst_dma->pdev->dev, src_ch->buffer, KL2_DMA_KBUF_SIZE,
                                             DMA_BIDIRECTIONAL);
    ret                = dma_mapping_error(&dst_dma->pdev->dev, dst_pp_dma_addr[0]);
    if (ret) {
        struct kl2_device *kl2_dev = dst_dma->data;
        KL2_LOGW("dma_mapping_error= %d, dst_ch.buffer= %px, dst_pp_dma_addr[0]= %llx\n", ret,
                 dst_ch->buffer, dst_pp_dma_addr[0]);
        ret = -ENOMEM;
        goto err_dst_mapping;
    }
    dst_pp_dma_addr[1] = (u64)dst_ch->dma_addr;

    t0 = ktime_get();
    /* loop 0 */
    dmasz = (cpsz < KL2_DMA_KBUF_SIZE) ? cpsz : KL2_DMA_KBUF_SIZE;
    src_dma->ops->ddma_to_host(src_dma->data,                /* kl2_device */
                               src_pp_dma_addr[pingpongidx], /* dest       */
                               src,                          /* src        */
                               dmasz,                        /* dma size   */
                               src_ch->ch);                  /* channel    */
    src += dmasz;
    /* loop 1 -> n */
    for (i = 1; i < loop; i++) {
        ret = src_dma->ops->wait_dma_finished(src_dma->data, src_ch->ch);
        if (ret)
            goto err_dma;
        dma_sync_single_for_cpu(&src_dma->pdev->dev, src_pp_dma_addr[pingpongidx], dmasz,
                                DMA_FROM_DEVICE);
        dma_sync_single_for_device(&dst_dma->pdev->dev, dst_pp_dma_addr[pingpongidx], dmasz,
                                   DMA_TO_DEVICE);
        dst_dma->ops->ddma_from_host(dst_dma->data,                /* kl2_device  */
                                     dst,                          /* dst         */
                                     dst_pp_dma_addr[pingpongidx], /* src         */
                                     dmasz,                        /* dma size    */
                                     dst_ch->ch);                  /* channel     */
        dst += dmasz;
        pingpongidx ^= 0x1;
        dmasz = (i == loop - 1) ? (cpsz - (u64)KL2_DMA_KBUF_SIZE * i) : KL2_DMA_KBUF_SIZE;
        src_dma->ops->ddma_to_host(src_dma->data,                /* kl2_device */
                                   src_pp_dma_addr[pingpongidx], /* dest       */
                                   src,                          /* src        */
                                   dmasz,                        /* dma size   */
                                   src_ch->ch);                  /* channel    */
        src += dmasz;
        ret = dst_dma->ops->wait_dma_finished(dst_dma->data, dst_ch->ch);
        if (ret)
            goto err_dma;
    }
    /* loop n + 1 */
    ret = src_dma->ops->wait_dma_finished(src_dma->data, src_ch->ch);
    if (ret)
        goto err_dma;
    dma_sync_single_for_cpu(&src_dma->pdev->dev, src_pp_dma_addr[pingpongidx], dmasz,
                            DMA_FROM_DEVICE);
    dma_sync_single_for_device(&dst_dma->pdev->dev, dst_pp_dma_addr[pingpongidx], dmasz,
                               DMA_TO_DEVICE);
    dst_dma->ops->ddma_from_host(dst_dma->data,                /* kl2_device  */
                                 dst,                          /* dst         */
                                 dst_pp_dma_addr[pingpongidx], /* src         */
                                 dmasz,                        /* dma size    */
                                 dst_ch->ch);                  /* channel     */
    ret = dst_dma->ops->wait_dma_finished(dst_dma->data, dst_ch->ch);
    if (ret)
        goto err_dma;

    t1 = ktime_get();
    time_acc += (u64)ktime_to_ns(ktime_sub(t1, t0));

    if (time_ns)
        *time_ns = time_acc;

err_dma:
    dma_unmap_single(&dst_dma->pdev->dev, dst_pp_dma_addr[0], KL2_DMA_KBUF_SIZE, DMA_BIDIRECTIONAL);
err_dst_mapping:
    dma_unmap_single(&src_dma->pdev->dev, src_pp_dma_addr[1], KL2_DMA_KBUF_SIZE, DMA_BIDIRECTIONAL);
err_src_mapping:
    __put_channel(dst_dma, dst_ch);
err_dst_channel:
    __put_channel(src_dma, src_ch);
err_src_channel:
    up(sema_2nd);
    up(sema_1st);
    return ret;
}

int kl2_dma_ddma_to_peer(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz, u64 *time_ns)
{
    struct kl2_device *kl2_dev __maybe_unused = dma->data;
    struct dma_channel        *ch;
    u64                        dmasz;
    int                        ret = 0;
    int                        i   = 0;
    ktime_t                    t0, t1;
    u64                        time_acc  = 0;
    const u64                  KBUF_SIZE = 0x40000000; // 1GB

    if (!cpsz) {
        return -EINVAL;
    }

    /* 1. now we hold kbuf_sem first
     *  1.1 hold dma operation mtx
     *  1.2 DMA
     *  1.3 release dma operation mtx
     *  1.4 copy data
     * 2. loop end
     */
    down(&dma->sema);
    if (kl2_get_in_reset_state(kl2_dev) == KL2_IN_RESET) {
        ret = -XPUERR_DEVRESET;
        goto err_out;
    }
    ch = __get_channel(dma);
    if (!ch) {
        ret = -EINVAL;
        goto err_out;
    }
    KL2_LOGD("get dma ch_%d\n", ch->ch);

    while (cpsz) {
        // DMA size is $KBUF_SIZE at most
        dmasz = (cpsz < KBUF_SIZE) ? cpsz : KBUF_SIZE;

        // XXX(miaotianxiang): 不需要sync，但需要确保MemWr commit即到对端生效
        //dma_sync_single_for_device(&dma->pdev->dev, ch->dma_addr, dmasz, DMA_FROM_DEVICE);

        t0 = ktime_get();
        dma->ops->ddma_to_host(dma->data, dest + (u64)KBUF_SIZE * i, src + (u64)KBUF_SIZE * i,
                               dmasz, ch->ch);
        ret = dma->ops->wait_dma_finished(dma->data, ch->ch);
        t1  = ktime_get();
        if (ret)
            break;

        time_acc += (u64)ktime_to_ns(ktime_sub(t1, t0));

        //dma_sync_single_for_cpu(&dma->pdev->dev, ch->dma_addr, dmasz, DMA_FROM_DEVICE);

        cpsz -= dmasz;
        ++i;
    }

    if (time_ns)
        *time_ns = time_acc;

    __put_channel(dma, ch);

err_out:
    up(&dma->sema);
    return ret;
}

int kl2_dma_ddma_to_host(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz, u64 *time_ns)
{
    return __kl2_dma_ddma_to_host(dma, dest, src, cpsz, 1, time_ns);
}

int kl2_dma_ddma_to_host_kernel(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz)
{
    return __kl2_dma_ddma_to_host(dma, dest, src, cpsz, 0, NULL);
}

int kl2_dma_ddma_from_host(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz, u64 *time_ns)
{
    return __kl2_dma_ddma_from_host(dma, dest, src, cpsz, 1, time_ns);
}

int kl2_dma_ddma_from_host_kernel(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz)
{
    return __kl2_dma_ddma_from_host(dma, dest, src, cpsz, 0, NULL);
}

int kl2_dma_ddma_zero_gm(struct dma_engine *dma, u64 dest, u64 cpsz)
{
    struct kl2_device *kl2_dev __maybe_unused = dma->data;
    struct dma_channel        *ch;
    u64                        dmasz;
    int                        ret = 0;
    int                        i   = 0;

    if (!cpsz) {
        return -EINVAL;
    }

    down(&dma->sema);
    ch = __get_channel(dma);
    if (!ch) {
        ret = -EINVAL;
        goto err_out;
    }

    dma_sync_single_for_cpu(&dma->pdev->dev, ch->dma_addr, KL2_DMA_KBUF_SIZE, DMA_TO_DEVICE);
    memset(ch->buffer, 0, KL2_DMA_KBUF_SIZE);
    while (cpsz) {
        // DMA length is $KL2_DMA_KBUF_SIZE at most
        dmasz = (cpsz < KL2_DMA_KBUF_SIZE) ? cpsz : KL2_DMA_KBUF_SIZE;

        dma_sync_single_for_device(&dma->pdev->dev, ch->dma_addr, dmasz, DMA_TO_DEVICE);

        dma->ops->ddma_from_host(dma->data, dest + (u64)KL2_DMA_KBUF_SIZE * i, (u64)ch->dma_addr,
                                 dmasz, ch->ch);
        ret = dma->ops->wait_dma_finished(dma->data, ch->ch);
        if (ret)
            break;

        cpsz -= dmasz;
        ++i;
    }

    __put_channel(dma, ch);

err_out:
    up(&dma->sema);
    return ret;
}

static inline unsigned long count_pages(unsigned long addr, unsigned long size)
{
    return PAGE_ALIGN(offset_in_page(addr) + size) >> PAGE_SHIFT;
}

/*
 * XXX(weihaoji): 计算当前 sg 描述的连续物理地址空间可被切分为多少个 dma desc
 *                存在 begin_strip 和 end_strip 的原因是用户可能只对部分子空间做 dma
 */
static inline u32 count_descs_for_sg(struct scatterlist *sg, u32 begin_strip, u32 end_strip)
{
    return DIV_ROUND_UP(sg_dma_len(sg) - begin_strip - end_strip, KL2_DESC_PAGE_SIZE);
}

static void __maybe_unused dump_sgdma_descriptor(struct kl2_device           *kl2_dev,
                                                 const plda_dma_descriptor_t *first_desc,
                                                 const plda_dma_descriptor_t *desc, int desc_count)
{
    int i      = 0;
    int offset = desc - first_desc;

    for (; i < desc_count; i++) {
        KL2_LOGI("===================================================\n");
        KL2_LOGI(
                "desc[%d].desc_status = 0x%x, .status_num=%d, .prc_status=0x%x, .prc_page_size=%d\n",
                i + offset, desc[i].desc_status, desc[i].desc_status_field.desc_status_num,
                desc[i].desc_status_field.desc_prc_status,
                desc[i].desc_status_field.desc_prc_page_size);
        KL2_LOGI("desc[%d].desc_control = 0x%x, .status_req=0x%x, .type=0x%x, .page_size=%d\n",
                 i + offset, desc[i].desc_control, desc[i].desc_control_field.desc_status_req,
                 desc[i].desc_control_field.desc_type, desc[i].desc_control_field.desc_page_size);
        KL2_LOGI("desc[%d].desc_se_cond=0x%x\n", i + offset, desc[i].desc_se_cond);
        KL2_LOGI("desc[%d].desc_next_ready=0x%x\n", i + offset, desc[i].desc_next_ready);
        KL2_LOGI("desc[%d].desc_next_address_lo=0x%x\n", i + offset,
                 desc[i].desc_next_address_lo << 5);
        KL2_LOGI("desc[%d].desc_next_address_hi=0x%x\n", i + offset, desc[i].desc_next_address_hi);
        KL2_LOGI("desc[%d].desc_src_address_lo=0x%x\n", i + offset, desc[i].desc_src_address_lo);
        KL2_LOGI("desc[%d].desc_src_address_hi=0x%x\n", i + offset, desc[i].desc_src_address_hi);
        KL2_LOGI("desc[%d].desc_dst_address_lo=0x%x\n", i + offset, desc[i].desc_dst_address_lo);
        KL2_LOGI("desc[%d].desc_dst_address_hi=0x%x\n", i + offset, desc[i].desc_dst_address_hi);
    }
}

// 最终得到的是与目标范围有交集的 minfo
struct kl2_sg_minfo *kl2_minfo_rb_search(struct rb_root *root, u64 addr, u64 size)
{
    struct rb_node      *node  = root->rb_node;
    struct kl2_sg_minfo *minfo = NULL;
    u64                  left_closed, right_open;

    while (node) {
        minfo       = container_of(node, struct kl2_sg_minfo, uproc_node);
        left_closed = minfo->addr;
        right_open  = minfo->addr + minfo->size;

        if (addr + size <= left_closed) {
            node = node->rb_left;
        } else if (addr >= right_open) {
            node = node->rb_right;
        } else {
            return minfo;
        }
    }
    return NULL;
}

int kl2_minfo_rb_insert(struct rb_root *root, struct kl2_sg_minfo *minfo)
{
    struct rb_node **new        = &(root->rb_node);
    struct rb_node      *parent = NULL;
    struct kl2_sg_minfo *pminfo = NULL;
    u64                  left_closed, right_open;

    while (*new) {
        pminfo      = container_of(*new, struct kl2_sg_minfo, uproc_node);
        left_closed = pminfo->addr;
        right_open  = pminfo->addr + pminfo->size;

        parent = *new;
        if (minfo->addr + minfo->size <= left_closed) {
            new = &((*new)->rb_left);
        } else if (minfo->addr >= right_open) {
            new = &((*new)->rb_right);
        } else {
            return -XPUERR_HOSTMEM_ALREADY_REGISTERED;
        }
    }

    rb_link_node(&minfo->uproc_node, parent, new);
    rb_insert_color(&minfo->uproc_node, root);

    return 0;
}

void kl2_minfo_rb_erase(struct rb_root *root, struct kl2_sg_minfo *minfo)
{
    rb_erase(&minfo->uproc_node, root);
}

// refer to: https://elixir.bootlin.com/linux/v5.11.8/source/drivers/media/common/videobuf2/
// videobuf2-dma-sg.c#L58
int kl2_host_alloc_hugepages(struct kl2_userprocess *uproc, u64 addr, u64 size,
                             struct kl2_sg_minfo *minfo)
{
    struct kl2_device *kl2_dev = uproc->kl2_dev;
    struct device     *dev     = &kl2_dev->kdev->pdev->dev;
    int                i, j, hugepage_cnt;
    struct page       *page;
    int                last_page = 0;
    int                ret       = 0;

    if (!addr) {
        ret = -EINVAL;
        goto err_out;
    }
    if (!size) {
        ret = -EINVAL;
        goto err_out;
    }
    if (PAGE_ALIGN(addr) != addr || PAGE_ALIGN(size) != size) {
        ret = -EINVAL;
        goto err_out;
    }
    if (((addr + size) < addr) || PAGE_ALIGN(addr + size) < (addr + size)) {
        ret = -EINVAL;
        goto err_out;
    }

    minfo->kl2_dev = kl2_dev;
    minfo->addr    = addr;
    minfo->size    = size;
    minfo->mmaped  = true;
    /* size is already page aligned */
    minfo->page_count = size >> PAGE_SHIFT;
    hugepage_cnt      = minfo->page_count >> KL2_HOSTMEM_HUGEPAGE_ORDER;

    minfo->user_pages = vzalloc(minfo->page_count * sizeof(*minfo->user_pages));
    if (!minfo->user_pages) {
        ret = -ENOMEM;
        goto err_out;
    }

    // 首先看 va 对应了多少个 hugepage，用这些 hugepage 填充 sgt；
    // 对于小于一个 hugepage 的部分，用 alloc_pages 逐个 order 分配 pages 填充 sgt
    for (i = 0; i < hugepage_cnt; i++) {
        page = alloc_pages(GFP_KERNEL, KL2_HOSTMEM_HUGEPAGE_ORDER);
        if (!page) {
            ret = -ENOMEM;
            goto err_alloc_pages;
        }
        split_page(page, KL2_HOSTMEM_HUGEPAGE_ORDER);
        for (j = 0; j < (1 << KL2_HOSTMEM_HUGEPAGE_ORDER); j++) {
            minfo->user_pages[last_page++] = &page[j];
            SetPageReserved(&page[j]);
        }
    }
    for (i = KL2_HOSTMEM_HUGEPAGE_ORDER - 1; i >= 0; i--) {
        if (minfo->page_count & BIT(i)) {
            page = alloc_pages(GFP_KERNEL, i);
            if (!page) {
                ret = -ENOMEM;
                goto err_alloc_pages;
            }
            split_page(page, i);
            for (j = 0; j < (1 << i); j++) {
                minfo->user_pages[last_page++] = &page[j];
                SetPageReserved(&page[j]);
            }
        }
    }

    if (last_page != minfo->page_count) {
        ret = -EFAULT;
        goto err_alloc_pages;
    }

    ret = sg_alloc_table_from_pages(&minfo->sgt, minfo->user_pages, minfo->page_count, 0, size,
                                    GFP_KERNEL);
    if (ret) {
        goto err_alloc_pages;
    }

    minfo->mapped_nents = dma_map_sg(dev, minfo->sgt.sgl, minfo->sgt.orig_nents, DMA_BIDIRECTIONAL);
    if (!minfo->mapped_nents) {
        ret = -EFAULT;
        goto err_map_sg;
    }
    minfo->sgt.nents = minfo->mapped_nents;
    xref_init(&minfo->xref);

    return 0;

err_map_sg:
    sg_free_table(&minfo->sgt);

err_alloc_pages:
    for (i = 0; i < last_page; i++) {
        ClearPageReserved(minfo->user_pages[i]);
        __free_page(minfo->user_pages[i]);
    }
    vfree(minfo->user_pages);

err_out:
    return ret;
}

void kl2_host_free_hugepages(struct kl2_sg_minfo *minfo)
{
    struct kl2_device *kl2_dev = minfo->kl2_dev;
    struct device     *dev     = &kl2_dev->kdev->pdev->dev;
    int                i;

    dma_unmap_sg(dev, minfo->sgt.sgl, minfo->sgt.orig_nents, DMA_BIDIRECTIONAL);

    sg_free_table(&minfo->sgt);

    for (i = 0; i < minfo->page_count; i++) {
        ClearPageReserved(minfo->user_pages[i]);
        __free_page(minfo->user_pages[i]);
    }

    vfree(minfo->user_pages);
}

int kl2_pin_host_memory(struct kl2_userprocess *uproc, u64 addr, u64 size,
                        struct kl2_sg_minfo *minfo)
{
    struct kl2_device *kl2_dev = uproc->kl2_dev;
    struct device     *dev     = &kl2_dev->kdev->pdev->dev;
    int                ret, i;
    unsigned int       flags = DRF_DEF(_LOCK_USER_PAGES, _FLAGS, _WRITE, _YES);

    if (!addr) {
        ret = -EINVAL;
        goto err_out;
    }
    if (!size) {
        ret = -EINVAL;
        goto err_out;
    }
    /*
     * If the combination of the address and size requested for this memory
     * region causes an integer overflow, return error.
     */
    if (((addr + size) < addr) || PAGE_ALIGN(addr + size) < (addr + size)) {
        ret = -EINVAL;
        goto err_out;
    }

    minfo->kl2_dev    = kl2_dev;
    minfo->addr       = addr;
    minfo->size       = size;
    minfo->mmaped     = false;
    minfo->page_count = count_pages(addr, size);

    ret = os_lock_user_pages(addr, minfo->page_count, (void **)&minfo->user_pages, flags);
    if (ret != 0) {
        goto err_get_user_pages;
    }

    ret = sg_alloc_table_from_pages(&minfo->sgt, minfo->user_pages, minfo->page_count,
                                    offset_in_page(addr), size, GFP_KERNEL);
    if (ret) {
        goto err_alloc_sg_table;
    }

    minfo->mapped_nents = dma_map_sg(dev, minfo->sgt.sgl, minfo->sgt.orig_nents, DMA_BIDIRECTIONAL);
    if (!minfo->mapped_nents) {
        ret = -EFAULT;
        goto err_map_sg;
    }
    // sgt.nents      - number of mapped entries
    // sgt.orig_nents - original size of list
    minfo->sgt.nents = minfo->mapped_nents;
    xref_init(&minfo->xref);

    return 0;

err_map_sg:
    sg_free_table(&minfo->sgt);

err_alloc_sg_table:
    os_unlock_user_pages(minfo->page_count, minfo->user_pages);

err_get_user_pages:
err_out:
    return ret;
}

void kl2_unpin_host_memory(struct kl2_sg_minfo *minfo)
{
    struct kl2_device *kl2_dev = minfo->kl2_dev;
    struct device     *dev     = &kl2_dev->kdev->pdev->dev;
    int                i;

    dma_unmap_sg(dev, minfo->sgt.sgl, minfo->sgt.orig_nents, DMA_BIDIRECTIONAL);

    sg_free_table(&minfo->sgt);

    os_unlock_user_pages(minfo->page_count, minfo->user_pages);
}

int kl2_host_memory_is_pinned(struct kl2_userprocess *uproc, u64 addr, u64 size,
                              struct kl2_sg_minfo **minfo)
{
    if (!addr) {
        return -EINVAL;
    }
    if (!size) {
        return -EINVAL;
    }
    if (((addr + size) < addr) || PAGE_ALIGN(addr + size) < (addr + size)) {
        return -EINVAL;
    }

    *minfo = kl2_minfo_rb_search(&uproc->sg_minfo_rb, addr, size);

    if (!(*minfo)) {
        return KL2_HOSTMEM_UNPINNED;
    } else if ((addr >= (*minfo)->addr) && (addr + size <= (*minfo)->addr + (*minfo)->size)) {
        return KL2_HOSTMEM_PINNED;
    } else {
        return KL2_HOSTMEM_PARTLY_PINNED;
    }
}

void kl2_destroy_minfo_ref(struct kref *kref)
{
    struct kl2_sg_minfo *minfo;

    minfo = container_of((struct xref *)kref, struct kl2_sg_minfo, xref);

    if (minfo->mmaped) {
        kl2_host_free_hugepages(minfo);
    } else {
        kl2_unpin_host_memory(minfo);
    }

    kfree(minfo);
}

static inline int __wait_desc_done(struct kl2_device *kl2_dev, const plda_dma_descriptor_t *desc,
                                   int ch)
{
    void __iomem *base            = kl2_dev->iomem_base.dma_base + ch * 0x40;
    static u32    DONE            = BIT(4);
    static u32    ERROR           = BIT(5);
    u32           val             = 0;
    int           ite             = 0;
    u64           start_jiffies   = jiffies;
    u64           elapsed_jiffies = 0;
    int           in_reset_state  = 0;

    do {
        // READ_ONCE()不适用于bit-field
        val = COMPAT_READ_ONCE(desc->desc_status);
        if (val & (DONE | ERROR)) {
            break;
        }
        in_reset_state = kl2_get_in_reset_state(kl2_dev);
        if (in_reset_state) {
            break;
        }

        ++ite;
        cpu_relax();

        if (elapsed_jiffies > HZ /* 1s */)
            usleep_range(10, 20);
    } while ((elapsed_jiffies = (jiffies - start_jiffies)) < (20UL * HZ) /* 20s */);

    if (val & DONE)
        return 0;

    // Abort anyway
    kl2_writel(kl2_dev, 0, base + RDMA_CONTROL);
    if (val & ERROR) {
        KL2_LOGW("DMA error, desc->desc_status= %08x, ite= %d\n", val, ite);
        return -XPUERR_DMAABORT;
    } else if (in_reset_state) {
        KL2_LOGW("DMA abort due to reset, desc->desc_status= %08x, ite= %d\n", val, ite);
        return -XPUERR_DEVRESET;
    } else {
        KL2_LOGW("DMA timeout, desc->desc_status= %08x, ite= %d\n", val, ite);
        return -XPUERR_DMATIMEOUT;
    }
}

// advanced mode will start the sg dma after the first descriptor is ready;
// HW will monitor the desc_next_ready bit, once set to 1, HW will start the DMA
// which is described by the next descriptor;
// Up to 4GB once time;
//static int __maybe_unused kl2_dma_sgdma_advanced(struct dma_engine *dma, u64 dst, u64 src, u64 size,
//                                                 struct kl2_sg_minfo *minfo,
//                                                 int is_from_host, u64 *cost)
//{
//    struct kl2_device *kl2_dev __maybe_unused = dma->data;
//    struct dma_channel        *ch;
//    int                        i, p, ret;
//    struct scatterlist        *sg;
//    sg_dma_data_t              sgdma_data;
//    plda_dma_descriptor_t     *last_batch_tail_desc = NULL;
//    plda_dma_descriptor_t     *desc;
//    u32                        desc_constructed = 0;
//    u32                        pcnt;
//    u64                        pcie_addr;
//    u64                        axi_addr = is_from_host ? dst : src;
//    u64                        next_desc_dma_addr;
//    ktime_t                    t0, t1;
//    // 当前积攒的desc数量
//    u32 nr_desc = 0;
//
//    BUILD_BUG_ON(sizeof(plda_dma_descriptor_t) != 32);
//    BUILD_BUG_ON((SG_DMA_ADVANCED_TOTAL_DESC_SIZE / sizeof(plda_dma_descriptor_t)) <
//                 (SG_DMA_ADVANCED_NR_DESC_IN_BATCH + 1));
//
//    if (!size) {
//        return -EINVAL;
//    }
//
//    ret = map_user_va(dma, is_from_host ? src : dst, size, is_from_host, &sgdma_data);
//    if (ret) {
//        goto err_map_user_va;
//    }
//
//    down(&dma->sema);
//    if (kl2_get_in_reset_state(kl2_dev) == KL2_IN_RESET) {
//        ret = -XPUERR_DEVRESET;
//        goto err_get_dma_channel;
//    }
//    ch = __get_channel(dma);
//    if (!ch) {
//        ret = -EINVAL;
//        goto err_get_dma_channel;
//    }
//
//    t0   = ktime_get();
//    desc = ch->buffer;
//    for_each_sg(sgdma_data.sgt.sgl, sg, sgdma_data.sgt.nents, i) {
//        pcnt = count_descs_for_sg(sg);
//        for (p = 0; p < pcnt; p++) {
//            bool last_desc = (i == sgdma_data.sgt.nents - 1) && (p == pcnt - 1);
//            bool tail_desc = (desc_constructed + 1) ==
//                             (SG_DMA_ADVANCED_TOTAL_DESC_SIZE / sizeof(plda_dma_descriptor_t));
//
//            desc->desc_status                        = 0;
//            desc->desc_control_field.desc_status_req = 1;
//            desc->desc_control_field.desc_type       = 0x0;
//            desc->desc_control_field.desc_irq        = 0x6;
//            if (p != pcnt - 1) {
//                desc->desc_control_field.desc_page_size =
//                        (KL2_DESC_PAGE_SIZE == KL2_MAX_DESC_PAGE_SIZE) ?
//                                0 :
//                                KL2_DESC_PAGE_SIZE; // 0 means KL2_MAX_DESC_PAGE_SIZE
//            } else {
//                desc->desc_control_field.desc_page_size = sg_dma_len(sg) - (p * KL2_DESC_PAGE_SIZE);
//            }
//
//            if (last_desc || tail_desc)
//                desc->desc_se_cond = 0x3;
//            else
//                desc->desc_se_cond = 0x2;
//            desc->desc_next_ready = 1;
//            if (!tail_desc) {
//                next_desc_dma_addr =
//                        ch->dma_addr + (desc_constructed + 1) * sizeof(plda_dma_descriptor_t);
//                desc->desc_next_address_lo = low32(next_desc_dma_addr) >> 5;
//                desc->desc_next_address_hi = high32(next_desc_dma_addr);
//            } else {
//                desc->desc_next_address_lo = 0;
//                desc->desc_next_address_hi = 0;
//            }
//
//            pcie_addr                 = sg_dma_address(sg) + (p * KL2_DESC_PAGE_SIZE);
//            desc->desc_src_address_lo = is_from_host ? low32(pcie_addr) : low32(axi_addr);
//            desc->desc_src_address_hi = is_from_host ? high32(pcie_addr) : high32(axi_addr);
//            desc->desc_dst_address_lo = is_from_host ? low32(axi_addr) : low32(pcie_addr);
//            desc->desc_dst_address_hi = is_from_host ? high32(axi_addr) : high32(pcie_addr);
//
//            axi_addr += (desc->desc_control_field.desc_page_size == 0) ?
//                                KL2_MAX_DESC_PAGE_SIZE :
//                                desc->desc_control_field.desc_page_size;
//
//            ++nr_desc;
//            if (nr_desc == SG_DMA_ADVANCED_NR_DESC_IN_BATCH || tail_desc || last_desc) {
//                desc->desc_next_ready = 0;
//                //dma_sync_single_for_device(&dma->pdev->dev, ch->dma_addr, KL2_DMA_KBUF_SIZE, DMA_TO_DEVICE);
//
//                if (!last_batch_tail_desc) {
//                    // 首次开始sgdma
//                    dma->ops->sg_dma(dma->data, ch->dma_addr, 0, ch->ch, is_from_host, 1,
//                                     SG_DMA_ADVANCED_CTRL_REG_VAL);
//                } else {
//                    // 通知dma engine开始处理当前batch
//                    last_batch_tail_desc->desc_next_ready = 1;
//                    //dma->ops->sg_dma_update_desc(dma->data, ch->ch);
//                }
//                last_batch_tail_desc = desc;
//                nr_desc              = 0;
//            }
//
//            ++desc_constructed;
//            if (tail_desc || last_desc) {
//                ret = __wait_desc_done(kl2_dev, desc);
//                if (ret) {
//                    KL2_LOGW("sg_dma_advanced failed, ret = %d\n", ret);
//                    goto err_wait_dma;
//                }
//
//                desc_constructed     = 0;
//                desc                 = ch->buffer;
//                last_batch_tail_desc = NULL;
//            } else {
//                ++desc;
//            }
//
//            //{
//            //    void __iomem *base    = kl2_dev->iomem_base.dma_base + ch->ch * 0x40;
//            //    void __iomem *rstatus = base + RDMA_STATUS;
//            //    u32           val     = kl2_readl(kl2_dev, rstatus);
//            //    if (val & BIT(0)) {
//            //        KL2_LOGW("dma done, weird!!!\n");
//            //    }
//            //}
//        }
//    }
//    t1 = ktime_get();
//
//    if (cost)
//        *cost = (u64)ktime_to_ns(ktime_sub(t1, t0));
//
//err_wait_dma:
//    __put_channel(dma, ch);
//
//err_get_dma_channel:
//    up(&dma->sema);
//    unmap_user_va(dma, is_from_host, &sgdma_data);
//
//err_map_user_va:
//    return ret;
//}

static int __maybe_unused kl2_dma_sgdma_basic(struct dma_engine *dma, u64 dst, u64 src, u64 size,
                                              struct kl2_sg_minfo *minfo, int is_from_host,
                                              u64 *cost)
{
    struct kl2_device *kl2_dev __maybe_unused = dma->data;
    struct dma_channel        *ch;
    int                        i, p, ret = 0;
    struct scatterlist        *sg;
    plda_dma_descriptor_t     *desc;
    u32                        desc_constructed = 0;
    u32                        pcnt;
    u64                        pcie_addr;
    u64                        axi_addr = is_from_host ? dst : src;
    u64                        next_desc_dma_addr;
    u64                        dma_len = 0;
    u64                        sg_offset, minfo_begin_offset, minfo_end_offset;
    u32                        sg_begin_strip, sg_end_strip;
    //u64                        first_desc_pcie_addr = 0, first_desc_size = 0;
    ktime_t t0, t1;

    BUILD_BUG_ON(sizeof(plda_dma_descriptor_t) != 32);

    if (!size) {
        return -EINVAL;
    }

    dma_sync_sg_for_device(&dma->pdev->dev, minfo->sgt.sgl, minfo->sgt.nents,
                           is_from_host ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

    down(&dma->sema);
    if (kl2_get_in_reset_state(kl2_dev) == KL2_IN_RESET) {
        ret = -XPUERR_DEVRESET;
        goto err_get_dma_channel;
    }
    ch = __get_channel(dma);
    if (!ch) {
        ret = -EINVAL;
        goto err_get_dma_channel;
    }

    /*
     * XXX(weihaoji): 需要被拷贝的 host 内存块可能是被 registered 内存块的一部分
     *
     * registered 内存块:         |*************************************************|
     * 用户态虚拟地址：        minfo->addr                              minfo->addr + minfo->size
     * 需要被拷贝的内存块：                                |**************|
     * 用户态虚拟地址：                                   src         src + size
     *                            |<--minfo_begin_offset-->|
     *                            |<------------minfo_end_offset--------->|
     *
     * 以下循环遍历所有 sg 描述的连续物理空间，判断是否落在 [src, src + size) 区间内
     */
    minfo_begin_offset = (is_from_host ? src : dst) - minfo->addr;
    minfo_end_offset   = minfo_begin_offset + size;
    sg_offset          = 0; /* 当前sg距离minfo->addr的偏移 */

    desc = ch->buffer;
    for_each_sg(minfo->sgt.sgl, sg, minfo->sgt.nents, i) {
        if (sg_offset + sg_dma_len(sg) <= minfo_begin_offset) {
            sg_offset += sg_dma_len(sg);
            continue;
        } else if ((sg_offset + sg_dma_len(sg)) > minfo_begin_offset &&
                   sg_offset <= minfo_begin_offset) {
            sg_begin_strip = minfo_begin_offset - sg_offset;
        } else { /* sg_offset > minfo_begin_offset */
            sg_begin_strip = 0;
        }
        if (sg_offset >= minfo_end_offset) {
            break;
        } else if (sg_offset < minfo_end_offset &&
                   (sg_offset + sg_dma_len(sg)) >= minfo_end_offset) {
            sg_end_strip = sg_offset + sg_dma_len(sg) - minfo_end_offset;
        } else { /* (sg_offset + sg_dma_len(sg)) < minfo_end_offset */
            sg_end_strip = 0;
        }
        sg_offset += sg_dma_len(sg);

        pcnt = count_descs_for_sg(sg, sg_begin_strip, sg_end_strip);
        for (p = 0; p < pcnt; ++p) {
            desc->desc_status                        = 0;
            desc->desc_control_field.desc_status_req = 1;
            desc->desc_control_field.desc_type       = 0x0;
            desc->desc_control_field.desc_irq        = 0x6;
            if (p != pcnt - 1) {
                desc->desc_control_field.desc_page_size =
                        (KL2_DESC_PAGE_SIZE == KL2_MAX_DESC_PAGE_SIZE) ?
                                0 :
                                KL2_DESC_PAGE_SIZE; // 0 means KL2_MAX_DESC_PAGE_SIZE
            } else {
                desc->desc_control_field.desc_page_size =
                        sg_dma_len(sg) - sg_begin_strip - sg_end_strip - (p * KL2_DESC_PAGE_SIZE);
            }

            desc->desc_se_cond    = 0x2;
            desc->desc_next_ready = 1;
            next_desc_dma_addr =
                    ch->dma_addr + (desc_constructed + 1) * sizeof(plda_dma_descriptor_t);
            desc->desc_next_address_lo = low32(next_desc_dma_addr) >> 5;
            desc->desc_next_address_hi = high32(next_desc_dma_addr);

            pcie_addr = sg_dma_address(sg) + sg_begin_strip + (p * KL2_DESC_PAGE_SIZE);
            desc->desc_src_address_lo = is_from_host ? low32(pcie_addr) : low32(axi_addr);
            desc->desc_src_address_hi = is_from_host ? high32(pcie_addr) : high32(axi_addr);
            desc->desc_dst_address_lo = is_from_host ? low32(axi_addr) : low32(pcie_addr);
            desc->desc_dst_address_hi = is_from_host ? high32(axi_addr) : high32(pcie_addr);

            axi_addr += (desc->desc_control_field.desc_page_size == 0) ?
                                KL2_MAX_DESC_PAGE_SIZE :
                                desc->desc_control_field.desc_page_size;
            dma_len += (desc->desc_control_field.desc_page_size == 0) ?
                               KL2_MAX_DESC_PAGE_SIZE :
                               desc->desc_control_field.desc_page_size;

            //if (desc_constructed == 0) {
            //    // first desc
            //    first_desc_pcie_addr = pcie_addr;
            //    first_desc_size      = (desc->desc_control_field.desc_page_size == 0) ?
            //                                   KL2_MAX_DESC_PAGE_SIZE :
            //                                   desc->desc_control_field.desc_page_size;
            //}
            ++desc_constructed;
            if ((desc_constructed == (KL2_DMA_KBUF_SIZE / sizeof(plda_dma_descriptor_t))) ||
                (dma_len >= (Kl2_MAX_DMA_DATA_LENGTH - KL2_DESC_PAGE_SIZE))) {
                // the preallocated descriptor buffer is full,
                // or the transfer size is greater than Kl2_MAX_DMA_DATA_LENGTH,
                // split the dma transfer to mutiple times.
                desc->desc_se_cond         = 0x3;
                desc->desc_next_ready      = 0;
                desc->desc_next_address_lo = 0;
                desc->desc_next_address_hi = 0;

                KL2_LOGD("split SG DMA, desc_constructed = %d, dma_len = %lld\n", desc_constructed,
                         dma_len);

                dma_sync_single_for_device(&dma->pdev->dev, ch->dma_addr,
                                           desc_constructed * sizeof(plda_dma_descriptor_t),
                                           DMA_TO_DEVICE);

                t0  = ktime_get();
                ret = dma->ops->sg_dma(dma->data, ch->dma_addr, 0, ch->ch, is_from_host, 0,
                                       SG_DMA_BASIC_CTRL_REG_VAL);
                ret = __wait_desc_done(kl2_dev, desc, ch->ch);
                t1  = ktime_get();
                if (ret) {
                    KL2_LOGW("sg_dma failed, ret = %d\n", ret);
                    goto err_sg_dma;
                }

                if (cost)
                    *cost += (u64)ktime_to_ns(ktime_sub(t1, t0));

                // reset the desc pointer
                desc             = ch->buffer;
                desc_constructed = 0;
                dma_len          = 0;
            } else {
                ++desc;
            }
        }
    }

    if ((ret == 0) && (desc_constructed > 0)) {
        --desc;
        desc->desc_se_cond         = 0x3;
        desc->desc_next_ready      = 0;
        desc->desc_next_address_lo = 0;
        desc->desc_next_address_hi = 0;

        dma_sync_single_for_device(&dma->pdev->dev, ch->dma_addr,
                                   desc_constructed * sizeof(plda_dma_descriptor_t), DMA_TO_DEVICE);

        t0  = ktime_get();
        ret = dma->ops->sg_dma(dma->data, ch->dma_addr, 0, ch->ch, is_from_host, 0,
                               SG_DMA_BASIC_CTRL_REG_VAL);
        ret = __wait_desc_done(kl2_dev, desc, ch->ch);
        t1  = ktime_get();
        if (ret) {
            KL2_LOGW("sg_dma failed, ret = %d\n", ret);
            goto err_sg_dma;
        }

        if (cost)
            *cost += (u64)ktime_to_ns(ktime_sub(t1, t0));
    }

    // sgdma通过写desc.desc_status通知DMA完成，MemWr不会超越前序MemWr，Host看到desc.desc_status
    // 置位则意味着d2h一定已完成（Host能立即看到最新的d2h数据），故此处无需加上war。
    //
    // 海光x86/飞腾aarch64等DMA一致性较弱平台上，读channel status判断完成不意味着Host
    // 能立即看到最新的d2h数据（后续copy_to_user可能拷贝旧数据），故此处用额外MemRd来
    // 保证Post MemWr完成。
    //if (is_from_host == 0 && g_kl_wars.add_tiny_mem_read_after_d2h_war &&
    //    first_desc_pcie_addr != 0 && first_desc_size != 0) {
    //    // TODO(miaotianxiang): 搬到预留地址/尝试zero byte read
    //    dma->ops->ddma_from_host(dma->data, src, first_desc_pcie_addr,
    //                             first_desc_size > 4 ? 4 : first_desc_size, ch->ch);
    //    ret = dma->ops->wait_dma_finished(dma->data, ch->ch);
    //}

    dma_sync_sg_for_cpu(&dma->pdev->dev, minfo->sgt.sgl, minfo->sgt.nents,
                        is_from_host ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

err_sg_dma:
    __put_channel(dma, ch);

err_get_dma_channel:
    up(&dma->sema);

    return ret;
}

int kl2_dma_sgdma(struct dma_engine *dma, u64 dst, u64 src, u64 size, struct kl2_sg_minfo *minfo,
                  int is_from_host, u64 *cost)
{
#ifdef SG_DMA_USE_BASIC_MODE
    return kl2_dma_sgdma_basic(dma, dst, src, size, minfo, is_from_host, cost);
#else
    return -EINVAL;
    //int i        = 0;
    //int ret      = 0;
    //u64 dst_tmp  = dst;
    //u64 src_tmp  = src;
    //u64 size_tmp = 0;

    //if (!size) {
    //    return -EINVAL;
    //}

    //for (i = 0; i < size / Kl2_MAX_DMA_DATA_LENGTH; i++) {
    //    ret = kl2_dma_sgdma_advanced(dma, dst_tmp, src_tmp, Kl2_MAX_DMA_DATA_LENGTH, is_from_host,
    //                                  cost);
    //    if (ret != 0) {
    //        return ret;
    //    } else {
    //        dst_tmp += Kl2_MAX_DMA_DATA_LENGTH;
    //        src_tmp += Kl2_MAX_DMA_DATA_LENGTH;
    //    }
    //}

    //size_tmp = size - i * Kl2_MAX_DMA_DATA_LENGTH;
    //if (size_tmp > 0) {
    //    return kl2_dma_sgdma_advanced(dma, dst_tmp, src_tmp, size_tmp, is_from_host, cost);
    //} else {
    //    return ret;
    //}
#endif
}
