/*
 * SPDX-FileCopyrightText: Copyright (c) 2021-2022 KUNLUNXIN CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#define __FILENAME__ "kl1_host_mem.c"

#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/sched/mm.h>
#include "xpu_drv.h"

struct kl1_host_minfo {
    struct page **pages;
    int           page_count;
    unsigned long addr;
    unsigned long size;
};

static void kl1_host_alloc_vm_open(struct vm_area_struct *vma)
{
}

static void kl1_host_alloc_vm_close(struct vm_area_struct *vma)
{
    struct kl1_host_minfo *minfo = vma->vm_private_data;
    int                    i;

    if (!minfo)
        return;

    for (i = 0; i < minfo->page_count; i++) {
        if (minfo->pages[i]) {
            ClearPageReserved(minfo->pages[i]);
            __free_page(minfo->pages[i]);
        }
    }

    vfree(minfo->pages);
    kfree(minfo);
    vma->vm_private_data = NULL;
}

static struct vm_operations_struct kl1_host_alloc_vm_ops = {
    .open  = kl1_host_alloc_vm_open,
    .close = kl1_host_alloc_vm_close,
};

static void kl1_host_free_partial(struct kl1_host_minfo *minfo, int allocated)
{
    int i;

    for (i = 0; i < allocated; i++) {
        ClearPageReserved(minfo->pages[i]);
        __free_page(minfo->pages[i]);
    }
    vfree(minfo->pages);
    kfree(minfo);
}

int kl1_mmap_host_alloc(struct xpu_pd *xpd, struct vm_area_struct *vma)
{
    struct kl1_host_minfo *minfo;
    unsigned long          map_addr;
    unsigned long          size = vma->vm_end - vma->vm_start;
    int                    page_count;
    int                    i, ret = 0;

    if (vma->vm_pgoff != 0)
        return -EINVAL;
    if (!size || PAGE_ALIGN(size) != size)
        return -EINVAL;

    minfo = kzalloc(sizeof(*minfo), GFP_KERNEL);
    if (!minfo)
        return -ENOMEM;

    page_count = size >> PAGE_SHIFT;
    minfo->pages = vzalloc(page_count * sizeof(*minfo->pages));
    if (!minfo->pages) {
        kfree(minfo);
        return -ENOMEM;
    }

    minfo->page_count = page_count;
    minfo->addr       = vma->vm_start;
    minfo->size       = size;
    map_addr          = vma->vm_start;

    for (i = 0; i < page_count; i++) {
        struct page *page = alloc_page(GFP_KERNEL | __GFP_ZERO);

        if (!page) {
            ret = -ENOMEM;
            goto err_free;
        }

        SetPageReserved(page);
        minfo->pages[i] = page;

        ret = vm_insert_page(vma, map_addr, page);
        if (ret) {
            LOGW("[xpu_%d] vm_insert_page failed at 0x%lx ret=%d\n", xpd->devfile_id, map_addr,
                 ret);
            goto err_free;
        }
        map_addr += PAGE_SIZE;
    }

    vm_flags_set(vma, VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP | VM_LOCKED);
    vma->vm_private_data = minfo;
    vma->vm_ops          = &kl1_host_alloc_vm_ops;
    vma->vm_ops->open(vma);

    LOGI("[xpu_%d] host_alloc mmap addr=0x%lx size=0x%lx pages=%d\n", xpd->devfile_id,
         vma->vm_start, size, page_count);
    return 0;

err_free:
    kl1_host_free_partial(minfo, i);
    return ret;
}

bool kl1_user_range_is_host_alloc(struct mm_struct *mm, unsigned long uaddr, unsigned long len)
{
    struct vm_area_struct *vma;
    bool                   ok = false;

    if (!mm || !len)
        return false;

    mmap_read_lock(mm);
    vma = find_vma(mm, uaddr);
    if (vma && vma->vm_ops == &kl1_host_alloc_vm_ops && uaddr >= vma->vm_start &&
        uaddr + len <= vma->vm_end)
        ok = true;
    mmap_read_unlock(mm);
    return ok;
}

/*
 * Resolve a user VA in a host_alloc mapping to struct page + in-page offset.
 * Caller must put_page() when done.
 */
int kl1_host_alloc_get_page(struct mm_struct *mm, unsigned long uaddr, struct page **page_out,
                            unsigned int *offset_out)
{
    struct vm_area_struct *vma;
    struct kl1_host_minfo *minfo;
    unsigned long          pgidx;
    struct page           *page;
    int                    ret = -EINVAL;

    if (!mm || !page_out || !offset_out)
        return -EINVAL;

    *page_out   = NULL;
    *offset_out = 0;

    mmap_read_lock(mm);
    vma = find_vma(mm, uaddr);
    if (!vma || vma->vm_ops != &kl1_host_alloc_vm_ops || uaddr < vma->vm_start ||
        uaddr >= vma->vm_end) {
        ret = -EINVAL;
        goto out_unlock;
    }

    minfo = vma->vm_private_data;
    if (!minfo || !minfo->pages) {
        ret = -EINVAL;
        goto out_unlock;
    }

    pgidx = (uaddr - minfo->addr) >> PAGE_SHIFT;
    if (pgidx >= (unsigned long)minfo->page_count) {
        ret = -EINVAL;
        goto out_unlock;
    }

    page = minfo->pages[pgidx];
    if (!page) {
        ret = -EINVAL;
        goto out_unlock;
    }

    get_page(page);
    *page_out   = page;
    *offset_out = offset_in_page(uaddr);
    ret         = 0;

out_unlock:
    mmap_read_unlock(mm);
    return ret;
}