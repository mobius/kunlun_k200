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

#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/sched/mm.h>
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

/* S4: EDMA directly to/from dma_map_page(host_alloc hugepage), skip bounce copy. */
static int kl1_edma_h2d_page(struct xpu_edma *edma, u64 dst_dev, struct page *page,
                             unsigned int offset, size_t len, u64 *cycles)
{
    struct device *dev = &edma->xpd->xdev->pdev->dev;
    dma_addr_t     dma_addr;
    int            ret;

    dma_addr = dma_map_page(dev, page, offset, len, DMA_TO_DEVICE);
    if (dma_mapping_error(dev, dma_addr))
        return -EIO;

    mutex_lock(&edma->lock);
    ret = xpuhw_edma_read_locked(edma, dst_dev, (u64)dma_addr, len, cycles);
    mutex_unlock(&edma->lock);

    dma_unmap_page(dev, dma_addr, len, DMA_TO_DEVICE);
    return ret;
}

static int kl1_edma_d2h_page(struct xpu_edma *edma, u64 src_dev, struct page *page,
                             unsigned int offset, size_t len, u64 *cycles)
{
    struct device *dev = &edma->xpd->xdev->pdev->dev;
    dma_addr_t     dma_addr;
    int            ret;

    dma_addr = dma_map_page(dev, page, offset, len, DMA_FROM_DEVICE);
    if (dma_mapping_error(dev, dma_addr))
        return -EIO;

    mutex_lock(&edma->lock);
    ret = xpuhw_edma_write_locked(edma, (u64)dma_addr, src_dev, len, cycles);
    mutex_unlock(&edma->lock);

    dma_unmap_page(dev, dma_addr, len, DMA_FROM_DEVICE);
    return ret;
}

static int dma_host_to_device_direct(struct xpu_pd *xpd, u64 dst, unsigned long src_u, u64 cpsz,
                                     u64 *total_cycles)
{
    struct mm_struct *mm = current->mm;
    int               ch = -1, ret = 0;
    u64               cycles = 0, done = 0;
    struct xpu_edma  *edma;

    if (!mm || !total_cycles)
        return -XPUERR_INVALID_PARAM;

    if (down_timeout(&xpd->rdch_sema, 60 * HZ)) {
        LOGW("[xpu_%d] S4 H2D rdch_sema timeout\n", xpd->devfile_id);
        return -XPUERR_TIMEOUT;
    }

    ch = get_rdch(xpd);
    if (ch < 0) {
        ret = -EFAULT;
        goto out_up;
    }

    edma          = &xpd->rdch_edma[ch];
    *total_cycles = 0;
    LOGL2("[xpu_%d] S4 H2D direct src=0x%lx sz=0x%llx\n", xpd->devfile_id, src_u, cpsz);

    while (done < cpsz) {
        struct page   *page;
        unsigned int   pgoff;
        /* S5: direct path may span full hugepage (≤2MB); no bounce kbuf cap. */
        size_t         span, run = min_t(size_t, cpsz - done, 2 * 1024 * 1024UL);
        size_t         seg;

        ret = kl1_host_alloc_get_span(mm, src_u + done, &page, &pgoff, &span);
        if (ret)
            break;

        seg = min(run, span);
        ret = kl1_edma_h2d_page(edma, dst + done, page, pgoff, seg, &cycles);
        put_page(page);
        if (ret)
            break;

        *total_cycles += cycles;
        done += seg;
    }

    put_rdch(xpd, ch);
out_up:
    up(&xpd->rdch_sema);
    return ret;
}

static int dma_device_to_host_direct(struct xpu_pd *xpd, unsigned long dst_u, u64 src, u64 cpsz,
                                     u64 *total_cycles)
{
    struct mm_struct *mm = current->mm;
    int               ch = -1, ret = 0;
    u64               cycles = 0, done = 0;
    struct xpu_edma  *edma;

    if (!mm || !total_cycles)
        return -XPUERR_INVALID_PARAM;

    if (down_timeout(&xpd->wrch_sema, 60 * HZ)) {
        LOGW("[xpu_%d] S4 D2H wrch_sema timeout\n", xpd->devfile_id);
        return -XPUERR_TIMEOUT;
    }

    ch = get_wrch(xpd);
    if (ch < 0) {
        ret = -EFAULT;
        goto out_up;
    }

    edma          = &xpd->wrch_edma[ch];
    *total_cycles = 0;
    LOGL2("[xpu_%d] S4 D2H direct dst=0x%lx sz=0x%llx\n", xpd->devfile_id, dst_u, cpsz);

    while (done < cpsz) {
        struct page   *page;
        unsigned int   pgoff;
        size_t         span, run = min_t(size_t, cpsz - done, 2 * 1024 * 1024UL);
        size_t         seg;

        ret = kl1_host_alloc_get_span(mm, dst_u + done, &page, &pgoff, &span);
        if (ret)
            break;

        seg = min(run, span);
        ret = kl1_edma_d2h_page(edma, src + done, page, pgoff, seg, &cycles);
        put_page(page);
        if (ret)
            break;

        *total_cycles += cycles;
        done += seg;
    }

    put_wrch(xpd, ch);
out_up:
    up(&xpd->wrch_sema);
    return ret;
}

/*
 * Legacy serial bounce (pre-S6). Kept intact as the crash-safe baseline and as
 * fallback when kl1_bounce_pipe=0, only one channel is free, or transfer fits
 * in a single kbuf chunk.
 */
static int dma_device_to_host_serial(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz,
                                     u64 *total_cycles)
{
    int              i = 0, ret = 0, ch = -1;
    size_t           dmasz;
    u64              cycles = 0;
    struct xpu_edma *edma;

    if (down_timeout(&xpd->wrch_sema, 60 * HZ)) {
        LOGW("[xpu_%d] D2H serial wrch_sema timeout\n", xpd->devfile_id);
        return -XPUERR_TIMEOUT;
    }

    ch = get_wrch(xpd);
    if (ch < 0) {
        LOGW("Cannot get write channel\n");
        ret = -EFAULT;
        goto err_out;
    }

    edma          = &xpd->wrch_edma[ch];
    *total_cycles = 0;

    LOGL2("[xpu_%d] D2H serial dev=0x%llx host=u0x%llx sz=0x%llx\n", xpd->devfile_id, src, dst,
          cpsz);

    while (cpsz) {
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

static int dma_host_to_device_serial(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz,
                                     u64 *total_cycles)
{
    int              i = 0, ret = 0, ch = -1;
    size_t           dmasz;
    u64              cycles = 0;
    struct xpu_edma *edma;

    if (down_timeout(&xpd->rdch_sema, 60 * HZ)) {
        LOGW("[xpu_%d] H2D serial rdch_sema timeout\n", xpd->devfile_id);
        return -XPUERR_TIMEOUT;
    }

    ch = get_rdch(xpd);
    if (ch < 0) {
        LOGW("Cannot get read channel\n");
        ret = -EFAULT;
        goto err_out;
    }

    edma          = &xpd->rdch_edma[ch];
    *total_cycles = 0;

    LOGL2("[xpu_%d] H2D serial host=u0x%llx dev=0x%llx sz=0x%llx\n", xpd->devfile_id, src, dst,
          cpsz);

    while (cpsz) {
        dmasz = (cpsz < KL1_DMA_KBUF_SIZE) ? cpsz : KL1_DMA_KBUF_SIZE;
        ret   = copy_from_user(edma->kbuf, (void *)src, dmasz);
        if (ret != 0) {
            LOGW("Copy from host to device error, ret=%d, sz=0x%zx\n", ret, dmasz);
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

/*
 * S6 D2H pipeline: EDMA(chunk N+1) overlaps copy_to_user(chunk N).
 * Uses two write channels' coherent kbufs. On any failure drains in-flight
 * DMA then falls through with error (never leaves channel stuck if wait ok).
 */
static int dma_device_to_host_pipe(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz,
                                   u64 *total_cycles)
{
    struct xpu_edma *edma[2];
    int              ch[2] = { -1, -1 };
    int              ret = 0, ping = 0, got_second = 0;
    u64              cycles = 0, remain, dev_off, host_off;
    size_t           dmasz, prev_dmasz = 0;
    bool             inflight = false;

    if (down_timeout(&xpd->wrch_sema, 60 * HZ)) {
        LOGW("[xpu_%d] D2H pipe wrch_sema timeout\n", xpd->devfile_id);
        return -XPUERR_TIMEOUT;
    }
    if (down_trylock(&xpd->wrch_sema) == 0)
        got_second = 1;
    else {
        /* Only one channel — use serial (already hold one wrch_sema). */
        up(&xpd->wrch_sema);
        return dma_device_to_host_serial(xpd, dst, src, cpsz, total_cycles);
    }

    ch[0] = get_wrch(xpd);
    ch[1] = get_wrch(xpd);
    if (ch[0] < 0 || ch[1] < 0 || !xpd->wrch_edma[ch[0]].enable ||
        !xpd->wrch_edma[ch[1]].enable || !xpd->wrch_edma[ch[0]].kbuf ||
        !xpd->wrch_edma[ch[1]].kbuf) {
        if (ch[1] >= 0)
            put_wrch(xpd, ch[1]);
        if (ch[0] >= 0)
            put_wrch(xpd, ch[0]);
        up(&xpd->wrch_sema);
        up(&xpd->wrch_sema);
        return dma_device_to_host_serial(xpd, dst, src, cpsz, total_cycles);
    }

    edma[0]       = &xpd->wrch_edma[ch[0]];
    edma[1]       = &xpd->wrch_edma[ch[1]];
    *total_cycles = 0;
    remain        = cpsz;
    dev_off       = src;
    host_off      = dst;

    LOGL2("[xpu_%d] D2H pipe sz=0x%llx ch=%d,%d\n", xpd->devfile_id, cpsz, ch[0], ch[1]);

    /* Lock by ascending channel index to avoid AB-BA deadlock across threads. */
    if (ch[0] < ch[1]) {
        mutex_lock(&edma[0]->lock);
        mutex_lock(&edma[1]->lock);
    } else {
        mutex_lock(&edma[1]->lock);
        mutex_lock(&edma[0]->lock);
    }

    dmasz = (remain < KL1_DMA_KBUF_SIZE) ? remain : KL1_DMA_KBUF_SIZE;
    ret   = xpuhw_edma_write_start(edma[0], (u64)edma[0]->dmabuf, dev_off, dmasz);
    if (ret)
        goto out_unlock;
    inflight   = true;
    prev_dmasz = dmasz;
    dev_off += dmasz;
    remain -= dmasz;
    ping = 0;

    while (remain) {
        size_t next_sz = (remain < KL1_DMA_KBUF_SIZE) ? remain : KL1_DMA_KBUF_SIZE;
        int    next    = ping ^ 1;

        /* Kick next DMA, then wait+copy previous (CPU overlaps with next DMA). */
        ret = xpuhw_edma_write_start(edma[next], (u64)edma[next]->dmabuf, dev_off, next_sz);
        if (ret)
            goto out_unlock;

        ret = xpuhw_edma_write_wait(edma[ping], &cycles);
        if (ret) {
            /* next is in flight — drain it before exit */
            u64 discard = 0;
            (void)xpuhw_edma_write_wait(edma[next], &discard);
            inflight = false;
            goto out_unlock;
        }
        *total_cycles += cycles;

        ret = copy_to_user((void *)host_off, edma[ping]->kbuf, prev_dmasz);
        if (ret != 0) {
            u64 discard = 0;
            (void)xpuhw_edma_write_wait(edma[next], &discard);
            inflight = false;
            LOGW("Copy to user error ret=%d\n", ret);
            ret = -EFAULT;
            goto out_unlock;
        }

        host_off += prev_dmasz;
        prev_dmasz = next_sz;
        dev_off += next_sz;
        remain -= next_sz;
        ping = next;
        inflight = true;
    }

    ret = xpuhw_edma_write_wait(edma[ping], &cycles);
    inflight = false;
    if (ret)
        goto out_unlock;
    *total_cycles += cycles;

    ret = copy_to_user((void *)host_off, edma[ping]->kbuf, prev_dmasz);
    if (ret != 0) {
        LOGW("Copy to user error ret=%d\n", ret);
        ret = -EFAULT;
    }

out_unlock:
    if (ret && inflight) {
        u64 discard = 0;
        (void)xpuhw_edma_write_wait(edma[ping], &discard);
    }
    if (ch[0] < ch[1]) {
        mutex_unlock(&edma[1]->lock);
        mutex_unlock(&edma[0]->lock);
    } else {
        mutex_unlock(&edma[0]->lock);
        mutex_unlock(&edma[1]->lock);
    }

    put_wrch(xpd, ch[1]);
    put_wrch(xpd, ch[0]);
    up(&xpd->wrch_sema);
    up(&xpd->wrch_sema);
    return ret;
}

/*
 * S6 H2D pipeline: copy_from_user(chunk N+1) overlaps EDMA(chunk N).
 */
static int dma_host_to_device_pipe(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz,
                                   u64 *total_cycles)
{
    struct xpu_edma *edma[2];
    int              ch[2] = { -1, -1 };
    int              ret = 0, ping = 0;
    u64              cycles = 0, remain, dev_off, host_off;
    size_t           dmasz, prev_dmasz = 0;
    bool             inflight = false;

    if (down_timeout(&xpd->rdch_sema, 60 * HZ)) {
        LOGW("[xpu_%d] H2D pipe rdch_sema timeout\n", xpd->devfile_id);
        return -XPUERR_TIMEOUT;
    }
    if (down_trylock(&xpd->rdch_sema) != 0) {
        up(&xpd->rdch_sema);
        return dma_host_to_device_serial(xpd, dst, src, cpsz, total_cycles);
    }

    ch[0] = get_rdch(xpd);
    ch[1] = get_rdch(xpd);
    if (ch[0] < 0 || ch[1] < 0 || !xpd->rdch_edma[ch[0]].enable ||
        !xpd->rdch_edma[ch[1]].enable || !xpd->rdch_edma[ch[0]].kbuf ||
        !xpd->rdch_edma[ch[1]].kbuf) {
        if (ch[1] >= 0)
            put_rdch(xpd, ch[1]);
        if (ch[0] >= 0)
            put_rdch(xpd, ch[0]);
        up(&xpd->rdch_sema);
        up(&xpd->rdch_sema);
        return dma_host_to_device_serial(xpd, dst, src, cpsz, total_cycles);
    }

    edma[0]       = &xpd->rdch_edma[ch[0]];
    edma[1]       = &xpd->rdch_edma[ch[1]];
    *total_cycles = 0;
    remain        = cpsz;
    dev_off       = dst;
    host_off      = src;

    LOGL2("[xpu_%d] H2D pipe sz=0x%llx ch=%d,%d\n", xpd->devfile_id, cpsz, ch[0], ch[1]);

    if (ch[0] < ch[1]) {
        mutex_lock(&edma[0]->lock);
        mutex_lock(&edma[1]->lock);
    } else {
        mutex_lock(&edma[1]->lock);
        mutex_lock(&edma[0]->lock);
    }

    dmasz = (remain < KL1_DMA_KBUF_SIZE) ? remain : KL1_DMA_KBUF_SIZE;
    ret   = copy_from_user(edma[0]->kbuf, (void *)host_off, dmasz);
    if (ret != 0) {
        LOGW("Copy from host to device error, ret=%d, sz=0x%zx\n", ret, dmasz);
        ret = -EFAULT;
        goto out_unlock;
    }
    ret = xpuhw_edma_read_start(edma[0], dev_off, (u64)edma[0]->dmabuf, dmasz);
    if (ret)
        goto out_unlock;
    inflight   = true;
    prev_dmasz = dmasz;
    host_off += dmasz;
    dev_off += dmasz;
    remain -= dmasz;
    ping = 0;

    while (remain) {
        size_t next_sz = (remain < KL1_DMA_KBUF_SIZE) ? remain : KL1_DMA_KBUF_SIZE;
        int    next    = ping ^ 1;

        /* CPU fill next buffer while previous EDMA runs. */
        ret = copy_from_user(edma[next]->kbuf, (void *)host_off, next_sz);
        if (ret != 0) {
            u64 discard = 0;
            (void)xpuhw_edma_read_wait(edma[ping], &discard);
            inflight = false;
            LOGW("Copy from host to device error, ret=%d, sz=0x%zx\n", ret, next_sz);
            ret = -EFAULT;
            goto out_unlock;
        }

        ret = xpuhw_edma_read_wait(edma[ping], &cycles);
        inflight = false;
        if (ret)
            goto out_unlock;
        *total_cycles += cycles;

        ret = xpuhw_edma_read_start(edma[next], dev_off, (u64)edma[next]->dmabuf, next_sz);
        if (ret)
            goto out_unlock;
        inflight = true;

        host_off += next_sz;
        dev_off += next_sz;
        remain -= next_sz;
        prev_dmasz = next_sz;
        ping = next;
    }

    ret = xpuhw_edma_read_wait(edma[ping], &cycles);
    inflight = false;
    if (ret)
        goto out_unlock;
    *total_cycles += cycles;
    (void)prev_dmasz;

out_unlock:
    if (ret && inflight) {
        u64 discard = 0;
        (void)xpuhw_edma_read_wait(edma[ping], &discard);
    }
    if (ch[0] < ch[1]) {
        mutex_unlock(&edma[1]->lock);
        mutex_unlock(&edma[0]->lock);
    } else {
        mutex_unlock(&edma[0]->lock);
        mutex_unlock(&edma[1]->lock);
    }

    put_rdch(xpd, ch[1]);
    put_rdch(xpd, ch[0]);
    up(&xpd->rdch_sema);
    up(&xpd->rdch_sema);
    return ret;
}

/* Memcpy between device memory and host cpu memory (pageable bounce or S4 direct). */
int dma_device_to_host(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz, u64 *total_cycles)
{
    struct mm_struct *mm = current->mm;

    if (kl1_dma_direct && mm && kl1_user_range_is_host_alloc(mm, (unsigned long)dst, cpsz))
        return dma_device_to_host_direct(xpd, (unsigned long)dst, src, cpsz, total_cycles);

    if (total_cycles == NULL)
        return -XPUERR_INVALID_PARAM;

    /* Single chunk or pipe disabled → proven serial path only. */
    if (!kl1_bounce_pipe || cpsz <= KL1_DMA_KBUF_SIZE)
        return dma_device_to_host_serial(xpd, dst, src, cpsz, total_cycles);

    return dma_device_to_host_pipe(xpd, dst, src, cpsz, total_cycles);
}

int dma_host_to_device(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz, u64 *total_cycles)
{
    struct mm_struct *mm = current->mm;

    if (kl1_dma_direct && mm && kl1_user_range_is_host_alloc(mm, (unsigned long)src, cpsz))
        return dma_host_to_device_direct(xpd, dst, (unsigned long)src, cpsz, total_cycles);

    if (total_cycles == NULL)
        return -XPUERR_INVALID_PARAM;

    if (!kl1_bounce_pipe || cpsz <= KL1_DMA_KBUF_SIZE)
        return dma_host_to_device_serial(xpd, dst, src, cpsz, total_cycles);

    return dma_host_to_device_pipe(xpd, dst, src, cpsz, total_cycles);
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

/*!
 * kl1_dma_peer_to_peer - S5 same-card P2P via coherent EDMA staging + ping-pong
 *
 * Previous path: full-size kvmalloc + D2H(edma.kbuf)+memcpy + memcpy+H2D (sequential).
 * New path (same physical card / shared PCIe DMA domain):
 *   - Stage in existing dma_alloc_coherent kbufs (no full-size kvmalloc, no CPU memcpy)
 *   - Dual-buffer ping-pong: overlap D2H of chunk N+1 with H2D of chunk N
 *   - Different EDMA channels on PD0/PD1 run concurrently
 */
int kl1_dma_peer_to_peer(struct xpu_pd *src_xpd, struct xpu_pd *dst_xpd, u64 dst_addr, u64 src_addr,
                         u64 sz, u64 *cycles)
{
    struct xpu_edma *src_edma, *dst_edma;
    struct xpu_edma *pp_edma[2];
    dma_addr_t       pp_dma[2];
    u64              total_cycles = 0, cyc = 0;
    u64              remain, dmasz, prev_dmasz, src_off, dst_off;
    int              src_ch = -1, dst_ch = -1, pp_ch = -1;
    int              ret = 0, ping = 0, loop, i;
    bool             got_src = false, got_dst = false, got_pp = false;
    bool             d2h_inflight = false, h2d_inflight = false;
    bool             src_first;

    if (!cycles || !src_xpd || !dst_xpd || !sz)
        return -XPUERR_INVALID_PARAM;

    if (src_xpd->devfile_id == dst_xpd->devfile_id)
        return -EINVAL;

    if (kl1_p2p_stub) {
        LOGL2("KL1_P2P_V6 stub success\n");
        *cycles = 0;
        return 0;
    }

    /* Same card only — cross-card has no BAR/IOVA mapping on KL1. */
    if (src_xpd->xdev != dst_xpd->xdev) {
        LOGW("KL1_P2P_V6 cross-device not supported src=%d dst=%d\n", src_xpd->devfile_id,
             dst_xpd->devfile_id);
        return -XPUERR_NOIOC;
    }

    LOGL2("KL1_P2P_V6 pingpong src_pd=%d dst_pd=%d sz=0x%llx\n", src_xpd->devfile_id,
          dst_xpd->devfile_id, sz);

    /*
     * Resources: src.wrch (D2H, ×1 or ×2 for dual staging) + dst.rdch (H2D).
     * Semaphore order follows ascending devfile_id so reverse concurrent P2P
     * cannot deadlock.
     */
    src_first = src_xpd->devfile_id < dst_xpd->devfile_id;

    if (src_first) {
        if (down_timeout(&src_xpd->wrch_sema, 10 * HZ)) {
            LOGW("KL1_P2P_V6 wrch_sema timeout pd=%d\n", src_xpd->devfile_id);
            return -EBUSY;
        }
        got_src = true;
        /* Try second staging buffer (optional). */
        if (down_trylock(&src_xpd->wrch_sema) == 0)
            got_pp = true;
        if (down_timeout(&dst_xpd->rdch_sema, 10 * HZ)) {
            if (got_pp)
                up(&src_xpd->wrch_sema);
            up(&src_xpd->wrch_sema);
            got_src = false;
            got_pp  = false;
            LOGW("KL1_P2P_V6 rdch_sema timeout pd=%d\n", dst_xpd->devfile_id);
            return -EBUSY;
        }
        got_dst = true;
    } else {
        /* first is dst: take dst.rdch first, then src.wrch */
        if (down_timeout(&dst_xpd->rdch_sema, 10 * HZ)) {
            LOGW("KL1_P2P_V6 rdch_sema timeout pd=%d\n", dst_xpd->devfile_id);
            return -EBUSY;
        }
        got_dst = true;
        if (down_timeout(&src_xpd->wrch_sema, 10 * HZ)) {
            up(&dst_xpd->rdch_sema);
            got_dst = false;
            LOGW("KL1_P2P_V6 wrch_sema timeout pd=%d\n", src_xpd->devfile_id);
            return -EBUSY;
        }
        got_src = true;
        if (down_trylock(&src_xpd->wrch_sema) == 0)
            got_pp = true;
    }

    src_ch = get_wrch(src_xpd);
    if (src_ch < 0) {
        ret = -EFAULT;
        goto out_semas;
    }
    src_edma = &src_xpd->wrch_edma[src_ch];

    dst_ch = get_rdch(dst_xpd);
    if (dst_ch < 0) {
        ret = -EFAULT;
        goto out_put;
    }
    dst_edma = &dst_xpd->rdch_edma[dst_ch];

    /* Second staging buffer: another write channel on src (same DMA domain). */
    if (got_pp) {
        pp_ch = get_wrch(src_xpd);
        if (pp_ch < 0) {
            up(&src_xpd->wrch_sema);
            got_pp = false;
            pp_edma[0] = src_edma;
            pp_edma[1] = src_edma;
            pp_dma[0]  = src_edma->dmabuf;
            pp_dma[1]  = src_edma->dmabuf;
        } else {
            pp_edma[0] = src_edma;
            pp_edma[1] = &src_xpd->wrch_edma[pp_ch];
            pp_dma[0]  = src_edma->dmabuf;
            pp_dma[1]  = pp_edma[1]->dmabuf;
        }
    } else {
        /* Single-buffer fallback (still zero-copy, no CPU memcpy). */
        pp_edma[0] = src_edma;
        pp_edma[1] = src_edma;
        pp_dma[0]  = src_edma->dmabuf;
        pp_dma[1]  = src_edma->dmabuf;
    }

    if (!src_edma->enable || !dst_edma->enable || !pp_dma[0] || !pp_dma[1]) {
        ret = -XPUERR_PEERRESET;
        goto out_put;
    }

    loop    = (int)((sz + KL1_DMA_KBUF_SIZE - 1) / KL1_DMA_KBUF_SIZE);
    remain  = sz;
    src_off = src_addr;
    dst_off = dst_addr;
    ping    = 0;

    mutex_lock(&src_edma->lock);
    if (got_pp && pp_edma[1] != src_edma)
        mutex_lock(&pp_edma[1]->lock);
    mutex_lock(&dst_edma->lock);

    /* Kick first D2H into buf0 */
    dmasz = (remain < KL1_DMA_KBUF_SIZE) ? remain : KL1_DMA_KBUF_SIZE;
    ret   = xpuhw_edma_write_start(src_edma, (u64)pp_dma[0], src_off, dmasz);
    if (ret)
        goto out_unlock;
    d2h_inflight = true;
    prev_dmasz   = dmasz;
    src_off += dmasz;
    remain -= dmasz;

    for (i = 1; i < loop; i++) {
        /* Wait previous D2H, then overlap H2D(prev) with D2H(next). */
        ret = xpuhw_edma_write_wait(src_edma, &cyc);
        d2h_inflight = false;
        if (ret)
            goto out_unlock;
        total_cycles += cyc;

        ret = xpuhw_edma_read_start(dst_edma, dst_off, (u64)pp_dma[ping], prev_dmasz);
        if (ret)
            goto out_unlock;
        h2d_inflight = true;
        dst_off += prev_dmasz;

        if (got_pp) {
            ping ^= 1;
            dmasz = (remain < KL1_DMA_KBUF_SIZE) ? remain : KL1_DMA_KBUF_SIZE;
            ret   = xpuhw_edma_write_start(src_edma, (u64)pp_dma[ping], src_off, dmasz);
            if (ret)
                goto out_unlock;
            d2h_inflight = true;
            prev_dmasz   = dmasz;
            src_off += dmasz;
            remain -= dmasz;

            ret = xpuhw_edma_read_wait(dst_edma, &cyc);
            h2d_inflight = false;
            if (ret)
                goto out_unlock;
            total_cycles += cyc;
        } else {
            /* Single buffer: must finish H2D before next D2H. */
            ret = xpuhw_edma_read_wait(dst_edma, &cyc);
            h2d_inflight = false;
            if (ret)
                goto out_unlock;
            total_cycles += cyc;

            dmasz = (remain < KL1_DMA_KBUF_SIZE) ? remain : KL1_DMA_KBUF_SIZE;
            ret   = xpuhw_edma_write_start(src_edma, (u64)pp_dma[0], src_off, dmasz);
            if (ret)
                goto out_unlock;
            d2h_inflight = true;
            prev_dmasz   = dmasz;
            src_off += dmasz;
            remain -= dmasz;
        }

        if ((i & 0x3f) == 0)
            cond_resched();
    }

    /* Drain last D2H then final H2D */
    if (d2h_inflight) {
        ret = xpuhw_edma_write_wait(src_edma, &cyc);
        d2h_inflight = false;
        if (ret)
            goto out_unlock;
        total_cycles += cyc;
    }

    ret = xpuhw_edma_read_start(dst_edma, dst_off, (u64)pp_dma[ping], prev_dmasz);
    if (ret)
        goto out_unlock;
    ret = xpuhw_edma_read_wait(dst_edma, &cyc);
    if (ret)
        goto out_unlock;
    total_cycles += cyc;

    *cycles = total_cycles;
    LOGL2("KL1_P2P_V6 done src=%d dst=%d sz=0x%llx cycles=%llu dual=%d\n", src_xpd->devfile_id,
          dst_xpd->devfile_id, sz, total_cycles, got_pp ? 1 : 0);

out_unlock:
    if (ret) {
        u64 discard = 0;

        /* Best-effort drain so channel state is clean for later transfers. */
        if (d2h_inflight)
            (void)xpuhw_edma_write_wait(src_edma, &discard);
        if (h2d_inflight)
            (void)xpuhw_edma_read_wait(dst_edma, &discard);
        LOGW("KL1_P2P_V6 fail ret=%d src=%d dst=%d\n", ret, src_xpd->devfile_id,
             dst_xpd->devfile_id);
    }
    mutex_unlock(&dst_edma->lock);
    if (got_pp && pp_edma[1] != src_edma)
        mutex_unlock(&pp_edma[1]->lock);
    mutex_unlock(&src_edma->lock);

out_put:
    if (got_pp && pp_ch >= 0)
        put_wrch(src_xpd, pp_ch);
    if (dst_ch >= 0)
        put_rdch(dst_xpd, dst_ch);
    if (src_ch >= 0)
        put_wrch(src_xpd, src_ch);

out_semas:
    if (got_pp)
        up(&src_xpd->wrch_sema);
    if (got_src)
        up(&src_xpd->wrch_sema);
    if (got_dst)
        up(&dst_xpd->rdch_sema);
    return ret;
}
