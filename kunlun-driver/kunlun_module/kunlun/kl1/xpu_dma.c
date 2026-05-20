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

#define __FILENAME__ "xpu_dma.c"

#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include "xpu_drv.h"
#include "xpu_hw.h"

////////////////////////////
// Driver DMA implementation
////////////////////////////
static inline int xpu_edma_setup(struct xpu_edma *edma, struct xpu_pd *xpd, int ch)
{
    int err;

    edma->enable  = 0;
    edma->xpd     = xpd;
    edma->channel = xpd->id * KL1_EDMA_CHANNEL_NUM_ONE_PD + ch;
    mutex_init(&edma->lock);

    edma->kbuf = dma_alloc_coherent(&edma->xpd->xdev->pdev->dev, KL1_DMA_KBUF_SIZE, &edma->dmabuf,
                                    GFP_KERNEL | __GFP_ZERO);
    if (!edma->kbuf) {
        LOGW("error dma_alloc_coherent for edma_ch=%d.\n", edma->channel);
        err = -XPUERR_NOCPUMEM;
        goto err_out;
    }

    edma->enable = 1;

    LOGL4("DMA ch_%d mapping k= %px pcie= %llx\n", edma->channel, edma->kbuf, edma->dmabuf);
    return 0;

err_out:
    return err;
}

static void xpu_edma_unsetup(struct xpu_edma *edma)
{
    if (!edma->enable)
        return;

    if (edma->kbuf) {
        dma_free_coherent(&edma->xpd->xdev->pdev->dev, KL1_DMA_KBUF_SIZE, edma->kbuf, edma->dmabuf);
        edma->kbuf   = NULL;
        edma->dmabuf = 0;
    }

    edma->enable = 0;
}

int dma_setup(struct xpu_pd *xpd)
{
    int i;
    int err;

    spin_lock_init(&xpd->rdch_bitmap_lock);
    xpd->rdch_bitmap      = 0UL;
    xpd->rdch_enabled_cnt = 0;

    spin_lock_init(&xpd->wrch_bitmap_lock);
    xpd->wrch_bitmap      = 0UL;
    xpd->wrch_enabled_cnt = 0;

    // initialize xpu_edma
    for (i = 0; i < KL1_EDMA_CHANNEL_NUM_ONE_PD; ++i) {
        err = xpu_edma_setup(&xpd->rdch_edma[i], xpd, i);
        if (err == 0)
            ++xpd->rdch_enabled_cnt;
        else
            bitmap_set(&xpd->rdch_bitmap, i, 1);

        err = xpu_edma_setup(&xpd->wrch_edma[i], xpd, i);
        if (err == 0)
            ++xpd->wrch_enabled_cnt;
        else
            bitmap_set(&xpd->wrch_bitmap, i, 1);
    }

    if (xpd->rdch_enabled_cnt == 0 || xpd->wrch_enabled_cnt == 0) {
        LOGW("No DMA buffer available, rdch=%d wrch=%d\n", xpd->rdch_enabled_cnt,
             xpd->wrch_enabled_cnt);
        err = -XPUERR_BUSY;
        goto err_out;
    } else {
        LOGL4("DMA Channel enabled, rdch=%d wrch=%d\n", xpd->rdch_enabled_cnt,
              xpd->wrch_enabled_cnt);
    }

    sema_init(&xpd->rdch_sema, xpd->rdch_enabled_cnt);
    sema_init(&xpd->wrch_sema, xpd->wrch_enabled_cnt);

    return 0;

err_out:
    while (i >= 0) {
        xpu_edma_unsetup(&xpd->wrch_edma[i]);
        xpu_edma_unsetup(&xpd->rdch_edma[i]);
        --i;
    }

    return err;
}

void dma_unsetup(struct xpu_pd *xpd)
{
    int i = 0;
    for (i = 0; i < KL1_EDMA_CHANNEL_NUM_ONE_PD; ++i) {
        xpu_edma_unsetup(&xpd->wrch_edma[i]);
        xpu_edma_unsetup(&xpd->rdch_edma[i]);
    }
}

static inline int get_channel(spinlock_t *lock, unsigned long *bitmap)
{
    unsigned int ch  = 0;
    int          ret = -1;

    spin_lock(lock);
    ch = find_first_zero_bit(bitmap, KL1_EDMA_CHANNEL_NUM_ONE_PD);
    if (ch < KL1_EDMA_CHANNEL_NUM_ONE_PD) {
        bitmap_set(bitmap, ch, 1);
        ret = ch;
    }
    spin_unlock(lock);

    return ret;
}
#define get_rdch(xpd) get_channel(&((xpd)->rdch_bitmap_lock), &((xpd)->rdch_bitmap))
#define get_wrch(xpd) get_channel(&((xpd)->wrch_bitmap_lock), &((xpd)->wrch_bitmap))

static inline void put_channel(spinlock_t *lock, unsigned long *bitmap, int ch)
{
    spin_lock(lock);
    bitmap_clear(bitmap, ch, 1);
    spin_unlock(lock);
}
#define put_rdch(xpd, ch) put_channel(&((xpd)->rdch_bitmap_lock), &((xpd)->rdch_bitmap), (ch))
#define put_wrch(xpd, ch) put_channel(&((xpd)->wrch_bitmap_lock), &((xpd)->wrch_bitmap), (ch))

/* Memcpy between device memory and host cpu memory
 * Memory will be split into multiple $KL1_DMA_KBUF_SIZE parts, and issue one DMA request for each
 * part.
 */
int dma_device_to_host(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz, u64 *total_cycles)
{
    int              i      = 0;
    int              ret    = 0;
    size_t           dmasz  = 0;
    int              ch     = -1;
    u64              cycles = 0;
    struct xpu_edma *edma;

    if (total_cycles == NULL)
        return -XPUERR_INVALID_PARAM;

    down(&xpd->wrch_sema);

    ch = get_wrch(xpd);
    if (ch < 0) {
        LOGW("Cannot get write channel\n");
        ret = -EFAULT;
        goto err_out;
    }

    edma          = &xpd->wrch_edma[ch];
    *total_cycles = 0;

    LOGL2("[xpu_%d] dev= 0x%llx host= u0x%llx 0x%llx sz= 0x%llx\n", xpd->devfile_id, src, dst,
          edma->dmabuf, cpsz);

    while (cpsz) {
        // DMA size is $KL1_DMA_KBUF_SIZE at most
        dmasz = (cpsz < KL1_DMA_KBUF_SIZE) ? cpsz : KL1_DMA_KBUF_SIZE;

        mutex_lock(&edma->lock);
        ret = xpuhw_edma_write_locked(edma, (u64)edma->dmabuf, src + (u64)KL1_DMA_KBUF_SIZE * i,
                                      dmasz, &cycles);
        mutex_unlock(&edma->lock);
        if (ret != 0)
            break;

        *total_cycles += cycles;

        ret = copy_to_user((void *)dst, edma->kbuf, dmasz);
        if (ret != 0) {
            LOGW("Copy to user error ret=%d\n", ret);
            ret = -EFAULT;
            break;
        }

        cpsz -= dmasz;
        dst += dmasz;
        ++i;
    }

    put_wrch(xpd, ch);

err_out:
    up(&xpd->wrch_sema);

    return ret;
}

int dma_host_to_device(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz, u64 *total_cycles)
{
    int              i      = 0;
    int              ret    = 0;
    size_t           dmasz  = 0;
    int              ch     = -1;
    u64              cycles = 0;
    struct xpu_edma *edma;

    if (total_cycles == NULL)
        return -XPUERR_INVALID_PARAM;

    down(&xpd->rdch_sema);

    ch = get_rdch(xpd);
    if (ch < 0) {
        LOGW("Cannot get read channel\n");
        ret = -EFAULT;
        goto err_out;
    }

    edma          = &xpd->rdch_edma[ch];
    *total_cycles = 0;

    LOGL2("[xpu_%d] host= u0x%llx 0x%llx dev= 0x%llx sz= 0x%llx\n", xpd->devfile_id, src,
          edma->dmabuf, dst, cpsz);

    while (cpsz) {
        // DMA length is $KL1_DMA_KBUF_SIZE at most
        dmasz = (cpsz < KL1_DMA_KBUF_SIZE) ? cpsz : KL1_DMA_KBUF_SIZE;
        ret   = copy_from_user(edma->kbuf, (void *)src, dmasz);
        if (ret != 0) {
            LOGW("Copy from host to devie error, ret=%d, sz=0x%zx\n", ret, dmasz);
            ret = -EFAULT;
            break;
        }

        mutex_lock(&edma->lock);
        ret = xpuhw_edma_read_locked(edma, dst + (u64)KL1_DMA_KBUF_SIZE * i, (u64)edma->dmabuf,
                                     dmasz, &cycles);
        mutex_unlock(&edma->lock);

        if (ret != 0)
            break;

        *total_cycles += cycles;

        cpsz -= dmasz;
        src += dmasz;
        ++i;
    }
    put_rdch(xpd, ch);

err_out:
    up(&xpd->rdch_sema);
    return ret;
}

int dma_device_to_device(struct xpu_pd *xpd, u64 dst, u64 src, u64 sz, u64 *cycles)
{
    int ret = 0;

    if (cycles == NULL)
        return -XPUERR_INVALID_PARAM;

    mutex_lock(&xpd->ssedma.lock);
    {
        ret = xpuhw_ssedma_locked(&xpd->ssedma, dst, src, sz, cycles);
        if ((ret == 0) && (*cycles > xpd->ssedma.max_cycles))
            xpd->ssedma.max_cycles = *cycles;
    }
    mutex_unlock(&xpd->ssedma.lock);

    return ret;
}

/*!
 * dma_device_to_device_p2p - Copy between two PDs on the same K200 chip
 *
 * Uses source PD's SSE DMA engine to copy to destination PD's memory.
 * Both PDs share the same BAR space on the same PCIe device.
 * The dst address must include the destination PD's rbase offset.
 */
int dma_device_to_device_p2p(struct xpu_pd *src_xpd, u64 dst_addr, u64 src_addr,
                             u64 sz, u64 *cycles)
{
    int ret;

    if (cycles == NULL)
        return -XPUERR_INVALID_PARAM;

    LOGL2("[xpu_%d->xpu_???] p2p src=0x%llx dst=0x%llx sz=0x%llx\n",
          src_xpd->devfile_id, src_addr, dst_addr, sz);

    mutex_lock(&src_xpd->ssedma.lock);
    ret = xpuhw_ssedma_locked(&src_xpd->ssedma, dst_addr, src_addr, sz, cycles);
    mutex_unlock(&src_xpd->ssedma.lock);

    return ret;
}
