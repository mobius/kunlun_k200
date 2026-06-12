/*
 * SPDX-FileCopyrightText: Copyright (c) 2021-2022 KUNLUNXIN CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#define __FILENAME__ "kl1_host_mem.c"

#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/sched/mm.h>
#include "xpu_drv.h"

#ifndef HPAGE_PMD_ORDER
#define KL1_HOST_HPAGE_ORDER_MAX 9 /* 2MB on x86_64 */
#else
#define KL1_HOST_HPAGE_ORDER_MAX HPAGE_PMD_ORDER
#endif

struct kl1_host_chunk {
    struct page   *head;
    unsigned int   order;
    unsigned long  vm_offset;
};

struct kl1_host_minfo {
    struct kl1_host_chunk *chunks;
    int                    chunk_count;
    int                    chunk_capacity;
    unsigned long          addr;
    unsigned long          size;
};

static unsigned long kl1_host_chunk_bytes(unsigned int order)
{
    return PAGE_SIZE << order;
}

static void kl1_host_free_chunks(struct kl1_host_minfo *minfo, int allocated)
{
    int i;

    for (i = 0; i < allocated; i++) {
        if (minfo->chunks[i].head) {
            ClearPageReserved(minfo->chunks[i].head);
            __free_pages(minfo->chunks[i].head, minfo->chunks[i].order);
        }
    }
}

static void kl1_host_alloc_vm_open(struct vm_area_struct *vma)
{
}

static void kl1_host_alloc_vm_close(struct vm_area_struct *vma)
{
    struct kl1_host_minfo *minfo = vma->vm_private_data;

    if (!minfo)
        return;

    kl1_host_free_chunks(minfo, minfo->chunk_count);
    vfree(minfo->chunks);
    kfree(minfo);
    vma->vm_private_data = NULL;
}

static struct vm_operations_struct kl1_host_alloc_vm_ops = {
    .open  = kl1_host_alloc_vm_open,
    .close = kl1_host_alloc_vm_close,
};

/*
 * Map one physically contiguous run (PAGE_SIZE << order). On alloc failure,
 * split into two (order-1) runs so 1GB mappings survive hugepage exhaustion.
 */
static int kl1_map_chunk(struct xpu_pd *xpd, struct vm_area_struct *vma,
                         struct kl1_host_minfo *minfo, int *chunk_idx, unsigned long *map_addr,
                         unsigned int order)
{
    struct page *page;
    unsigned int sub_pages;
    int          j, ret;

    if (*chunk_idx >= minfo->chunk_capacity)
        return -ENOMEM;

    if (order > 0)
        page = alloc_pages(GFP_KERNEL | __GFP_ZERO | __GFP_COMP, order);
    else
        page = alloc_page(GFP_KERNEL | __GFP_ZERO);

    if (!page) {
        if (order == 0)
            return -ENOMEM;
        ret = kl1_map_chunk(xpd, vma, minfo, chunk_idx, map_addr, order - 1);
        if (ret)
            return ret;
        return kl1_map_chunk(xpd, vma, minfo, chunk_idx, map_addr, order - 1);
    }

    sub_pages = 1U << order;
    SetPageReserved(page);
    minfo->chunks[*chunk_idx].head      = page;
    minfo->chunks[*chunk_idx].order     = order;
    minfo->chunks[*chunk_idx].vm_offset = *map_addr - vma->vm_start;
    (*chunk_idx)++;

    for (j = 0; j < sub_pages; j++) {
        ret = vm_insert_page(vma, *map_addr, nth_page(page, j));
        if (ret) {
            LOGW("[xpu_%d] vm_insert_page failed at 0x%lx order=%u ret=%d\n", xpd->devfile_id,
                 *map_addr, order, ret);
            return ret;
        }
        *map_addr += PAGE_SIZE;
    }

    return 0;
}

int kl1_mmap_host_alloc(struct xpu_pd *xpd, struct vm_area_struct *vma)
{
    struct kl1_host_minfo *minfo;
    unsigned long          map_addr;
    unsigned long          size = vma->vm_end - vma->vm_start;
    unsigned long          page_count;
    int                    hugepage_cnt;
    int                    i, order, ret = 0;
    int                    chunk_idx = 0;

    if (vma->vm_pgoff != 0)
        return -EINVAL;
    if (!size || PAGE_ALIGN(size) != size)
        return -EINVAL;

    page_count = size >> PAGE_SHIFT;

    minfo = kzalloc(sizeof(*minfo), GFP_KERNEL);
    if (!minfo)
        return -ENOMEM;

    minfo->chunk_capacity = page_count;
    minfo->chunks         = vzalloc(minfo->chunk_capacity * sizeof(*minfo->chunks));
    if (!minfo->chunks) {
        kfree(minfo);
        return -ENOMEM;
    }

    minfo->addr = vma->vm_start;
    minfo->size = size;
    map_addr    = vma->vm_start;

    hugepage_cnt = page_count >> KL1_HOST_HPAGE_ORDER_MAX;
    for (i = 0; i < hugepage_cnt; i++) {
        ret = kl1_map_chunk(xpd, vma, minfo, &chunk_idx, &map_addr, KL1_HOST_HPAGE_ORDER_MAX);
        if (ret)
            goto err_free;
    }

    for (order = KL1_HOST_HPAGE_ORDER_MAX - 1; order >= 0; order--) {
        if (!(page_count & (1U << order)))
            continue;

        ret = kl1_map_chunk(xpd, vma, minfo, &chunk_idx, &map_addr, order);
        if (ret)
            goto err_free;
    }

    if (map_addr != vma->vm_end) {
        ret = -EFAULT;
        goto err_free;
    }

    minfo->chunk_count = chunk_idx;
    vm_flags_set(vma, VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP | VM_LOCKED);
    vma->vm_private_data = minfo;
    vma->vm_ops          = &kl1_host_alloc_vm_ops;
    vma->vm_ops->open(vma);

    LOGI("[xpu_%d] host_alloc mmap addr=0x%lx size=0x%lx chunks=%d (huge max_order=%d)\n",
         xpd->devfile_id, vma->vm_start, size, chunk_idx, KL1_HOST_HPAGE_ORDER_MAX);
    return 0;

err_free:
    kl1_host_free_chunks(minfo, chunk_idx);
    vfree(minfo->chunks);
    kfree(minfo);
    return ret;
}

bool kl1_user_range_is_host_alloc(struct mm_struct *mm, unsigned long uaddr, u64 len)
{
    struct vm_area_struct *vma;
    bool                   ok = false;
    u64                    vma_size;

    if (!mm || !len)
        return false;

    mmap_read_lock(mm);
    vma = find_vma(mm, uaddr);
    if (vma && vma->vm_ops == &kl1_host_alloc_vm_ops && uaddr >= vma->vm_start &&
        uaddr < vma->vm_end) {
        vma_size = (u64)(vma->vm_end - vma->vm_start);
        if ((u64)(uaddr - vma->vm_start) + len <= vma_size)
            ok = true;
    }
    mmap_read_unlock(mm);
    return ok;
}

static struct kl1_host_chunk *kl1_host_find_chunk(struct kl1_host_minfo *minfo, unsigned long uaddr,
                                                  unsigned long *offset_in_chunk)
{
    unsigned long rel = uaddr - minfo->addr;
    int           i;

    for (i = 0; i < minfo->chunk_count; i++) {
        unsigned long csize = kl1_host_chunk_bytes(minfo->chunks[i].order);

        if (rel >= minfo->chunks[i].vm_offset && rel < minfo->chunks[i].vm_offset + csize) {
            *offset_in_chunk = rel - minfo->chunks[i].vm_offset;
            return &minfo->chunks[i];
        }
    }

    return NULL;
}

int kl1_host_alloc_get_span(struct mm_struct *mm, unsigned long uaddr, struct page **page_out,
                            unsigned int *offset_out, size_t *span_out)
{
    struct vm_area_struct *vma;
    struct kl1_host_minfo *minfo;
    struct kl1_host_chunk *chunk;
    unsigned long          offset_in_chunk;
    unsigned long          csize;
    int                    ret = -EINVAL;

    if (!mm || !page_out || !offset_out || !span_out)
        return -EINVAL;

    *page_out   = NULL;
    *offset_out = 0;
    *span_out   = 0;

    mmap_read_lock(mm);
    vma = find_vma(mm, uaddr);
    if (!vma || vma->vm_ops != &kl1_host_alloc_vm_ops || uaddr < vma->vm_start ||
        uaddr >= vma->vm_end) {
        goto out_unlock;
    }

    minfo = vma->vm_private_data;
    if (!minfo || !minfo->chunks) {
        goto out_unlock;
    }

    chunk = kl1_host_find_chunk(minfo, uaddr, &offset_in_chunk);
    if (!chunk || !chunk->head) {
        goto out_unlock;
    }

    csize = kl1_host_chunk_bytes(chunk->order);
    if (offset_in_chunk >= csize) {
        goto out_unlock;
    }

    get_page(chunk->head);
    *page_out   = chunk->head;
    *offset_out = offset_in_chunk;
    *span_out   = min_t(size_t, csize - offset_in_chunk, vma->vm_end - uaddr);
    ret         = 0;

out_unlock:
    mmap_read_unlock(mm);
    return ret;
}

int kl1_host_alloc_get_page(struct mm_struct *mm, unsigned long uaddr, struct page **page_out,
                            unsigned int *offset_out)
{
    size_t span;

    return kl1_host_alloc_get_span(mm, uaddr, page_out, offset_out, &span);
}