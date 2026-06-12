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

/* S4 spike: EDMA directly to/from dma_map_page(host_alloc page), skip bounce copy. */
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

    if (down_timeout(&xpd->rdch_sema, 5 * HZ)) {
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
    LOGI("[xpu_%d] S4 H2D direct src=0x%lx sz=0x%llx\n", xpd->devfile_id, src_u, cpsz);

    while (done < cpsz) {
        struct page   *page;
        unsigned int   pgoff;
        size_t         run = min_t(size_t, cpsz - done, KL1_DMA_KBUF_SIZE);
        size_t         seg;

        ret = kl1_host_alloc_get_page(mm, src_u + done, &page, &pgoff);
        if (ret)
            break;

        seg = min(run, (size_t)PAGE_SIZE - pgoff);
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

    if (down_timeout(&xpd->wrch_sema, 5 * HZ)) {
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
    LOGI("[xpu_%d] S4 D2H direct dst=0x%lx sz=0x%llx\n", xpd->devfile_id, dst_u, cpsz);

    while (done < cpsz) {
        struct page   *page;
        unsigned int   pgoff;
        size_t         run = min_t(size_t, cpsz - done, KL1_DMA_KBUF_SIZE);
        size_t         seg;

        ret = kl1_host_alloc_get_page(mm, dst_u + done, &page, &pgoff);
        if (ret)
            break;

        seg = min(run, (size_t)PAGE_SIZE - pgoff);
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

/* Memcpy between device memory and host cpu memory
 * Memory will be split into multiple $KL1_DMA_KBUF_SIZE parts, and issue one DMA request for each
 * part.
 */
int dma_device_to_host(struct xpu_pd *xpd, u64 dst, u64 src, u64 cpsz, u64 *total_cycles)
{
    struct mm_struct *mm = current->mm;
    int              i      = 0;
    int              ret    = 0;
    size_t           dmasz  = 0;
    int              ch     = -1;
    u64              cycles = 0;
    struct xpu_edma *edma;

    if (kl1_dma_direct && mm && kl1_user_range_is_host_alloc(mm, (unsigned long)dst, cpsz))
        return dma_device_to_host_direct(xpd, (unsigned long)dst, src, cpsz, total_cycles);

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
    struct mm_struct *mm = current->mm;
    int              i      = 0;
    int              ret    = 0;
    size_t           dmasz  = 0;
    int              ch     = -1;
    u64              cycles = 0;
    struct xpu_edma *edma;

    if (kl1_dma_direct && mm && kl1_user_range_is_host_alloc(mm, (unsigned long)src, cpsz))
        return dma_host_to_device_direct(xpd, dst, (unsigned long)src, cpsz, total_cycles);

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

/* Kernel-buffer variants (no copy_from/to_user) for P2P staging. */
static int dma_device_to_kbuf(struct xpu_pd *xpd, void *kbuf, u64 src, u64 cpsz, u64 *total_cycles)
{
    int              i = 0, ret = 0, ch = -1;
    size_t           dmasz;
    u64              cycles = 0;
    struct xpu_edma *edma;

    if (!total_cycles || !kbuf)
        return -XPUERR_INVALID_PARAM;

    LOGI("KL1_P2P_V5 D2H kbuf dev=%d src=0x%llx sz=0x%llx\n", xpd->devfile_id, src, cpsz);

    if (down_timeout(&xpd->wrch_sema, 5 * HZ)) {
        LOGW("KL1_P2P_V5 wrch_sema timeout dev=%d\n", xpd->devfile_id);
        return -EBUSY;
    }

    ch = get_wrch(xpd);
    if (ch < 0) {
        ret = -EFAULT;
        goto err_out;
    }

    edma          = &xpd->wrch_edma[ch];
    *total_cycles = 0;

    while (cpsz) {
        dmasz = (cpsz < KL1_DMA_KBUF_SIZE) ? cpsz : KL1_DMA_KBUF_SIZE;

        mutex_lock(&edma->lock);
        ret = xpuhw_edma_write_locked(edma, (u64)edma->dmabuf, src + (u64)KL1_DMA_KBUF_SIZE * i,
                                      dmasz, &cycles);
        mutex_unlock(&edma->lock);
        if (ret) {
            LOGW("KL1_P2P_V5 D2H edma ret=%d dev=%d\n", ret, xpd->devfile_id);
            break;
        }

        *total_cycles += cycles;
        memcpy((char *)kbuf + (u64)KL1_DMA_KBUF_SIZE * i, edma->kbuf, dmasz);
        cpsz -= dmasz;
        ++i;
    }

    put_wrch(xpd, ch);
err_out:
    up(&xpd->wrch_sema);
    return ret;
}

static int dma_kbuf_to_device(struct xpu_pd *xpd, u64 dst, void *kbuf, u64 cpsz, u64 *total_cycles)
{
    int              i = 0, ret = 0, ch = -1;
    size_t           dmasz;
    u64              cycles = 0;
    struct xpu_edma *edma;

    if (!total_cycles || !kbuf)
        return -XPUERR_INVALID_PARAM;

    LOGI("KL1_P2P_V5 H2D kbuf dev=%d dst=0x%llx sz=0x%llx\n", xpd->devfile_id, dst, cpsz);

    if (down_timeout(&xpd->rdch_sema, 5 * HZ)) {
        LOGW("KL1_P2P_V5 rdch_sema timeout dev=%d\n", xpd->devfile_id);
        return -EBUSY;
    }

    ch = get_rdch(xpd);
    if (ch < 0) {
        ret = -EFAULT;
        goto err_out;
    }

    edma          = &xpd->rdch_edma[ch];
    *total_cycles = 0;

    while (cpsz) {
        dmasz = (cpsz < KL1_DMA_KBUF_SIZE) ? cpsz : KL1_DMA_KBUF_SIZE;

        memcpy(edma->kbuf, (char *)kbuf + (u64)KL1_DMA_KBUF_SIZE * i, dmasz);

        mutex_lock(&edma->lock);
        ret = xpuhw_edma_read_locked(edma, dst + (u64)KL1_DMA_KBUF_SIZE * i, (u64)edma->dmabuf,
                                     dmasz, &cycles);
        mutex_unlock(&edma->lock);
        if (ret) {
            LOGW("KL1_P2P_V5 H2D edma ret=%d dev=%d\n", ret, xpd->devfile_id);
            break;
        }

        *total_cycles += cycles;
        cpsz -= dmasz;
        ++i;
    }

    put_rdch(xpd, ch);
err_out:
    up(&xpd->rdch_sema);
    return ret;
}

/*!
 * kl1_dma_peer_to_peer - P2P via host-staging (sequential D2H then H2D)
 *
 * Uses a kvmalloc bounce buffer; avoids vm_mmap (hangs when called from ioctl).
 */
int kl1_dma_peer_to_peer(struct xpu_pd *src_xpd, struct xpu_pd *dst_xpd, u64 dst_addr, u64 src_addr,
                         u64 sz, u64 *cycles)
{
    void  *staging = NULL;
    u64    cpsz = sz, total_cycles = 0, chunk_cycles = 0;
    size_t dmasz;
    int    ret = 0;

    if (!cycles || !src_xpd || !dst_xpd || !sz)
        return -XPUERR_INVALID_PARAM;

    if (src_xpd->devfile_id == dst_xpd->devfile_id)
        return -EINVAL;

    if (kl1_p2p_stub) {
        LOGI("KL1_P2P_V5 stub success\n");
        *cycles = 0;
        return 0;
    }

    LOGI("KL1_P2P_V5 kbuf enter src_pd=%d dst_pd=%d sz=0x%llx\n", src_xpd->devfile_id,
         dst_xpd->devfile_id, sz);

    staging = kvmalloc(sz, GFP_KERNEL);
    if (!staging) {
        LOGW("KL1_P2P_V5 kvmalloc failed sz=0x%llx\n", sz);
        return -ENOMEM;
    }

    LOGI("KL1_P2P_V5 staging=%px sz=0x%llx\n", staging, sz);

    while (cpsz) {
        dmasz = (cpsz < KL1_DMA_KBUF_SIZE) ? cpsz : KL1_DMA_KBUF_SIZE;

        ret = dma_device_to_kbuf(src_xpd, staging, src_addr, dmasz, &chunk_cycles);
        LOGI("[xpu_%d] p2p D2H chunk ret=%d cycles=%llu\n", src_xpd->devfile_id, ret,
             chunk_cycles);
        if (ret)
            break;
        total_cycles += chunk_cycles;

        cond_resched();

        ret = dma_kbuf_to_device(dst_xpd, dst_addr, staging, dmasz, &chunk_cycles);
        LOGI("[xpu_%d] p2p H2D chunk ret=%d cycles=%llu\n", dst_xpd->devfile_id, ret,
             chunk_cycles);
        if (ret)
            break;
        total_cycles += chunk_cycles;

        src_addr += dmasz;
        dst_addr += dmasz;
        cpsz -= dmasz;
    }

    *cycles = total_cycles;
    kvfree(staging);
    return ret;
}
