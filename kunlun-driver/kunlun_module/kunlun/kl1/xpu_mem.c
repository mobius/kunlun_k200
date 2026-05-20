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

// Copyright 2018 Baidu Inc. All Rights Reserved.
// authors: Han Jinchen hanjinche@baidu.com
//
// xpu_mem.c - XPU memory management
//
#define __FILENAME__ "xpu_mem.c"

#include <linux/bitmap.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include "xpu_drv.h"

int __setup_mem_data(struct xpu_mem_data *mem);
int __unsetup_mem_data(struct xpu_mem_data *);

inline u64 pageid_to_addr(struct xpu_mem_data *mem, u64 page_id)
{
    return mem->base + (page_id << mem->page_bits);
}

inline u64 addr_to_pageid(struct xpu_mem_data *mem, u64 addr)
{
    return (addr - mem->base) >> mem->page_bits;
}

static struct xpu_mem_data xpu_mems[XPU_PD_NUM][MMRGN_CNT] = {
    {
            {
                    .kind      = XPU_MEM_HBM,
                    .region    = MMRGN_HBM_LO,
                    .base      = 0x208000000ULL,
                    .size      = 0x0f8000000ULL,
                    .page_size = KL1_HBM_PAGE_SIZE,
                    .page_bits = KL1_HBM_PAGE_BITS,
            },
            {
                    .kind      = XPU_MEM_HBM,
                    .region    = MMRGN_HBM_HI,
                    .base      = 0x300000000ULL,
                    .size      = 0x100000000ULL,
                    .page_size = KL1_HBM_PAGE_SIZE,
                    .page_bits = KL1_HBM_PAGE_BITS,
            },
            {
                    .kind      = XPU_MEM_L3,
                    .region    = MMRGN_L3,
                    .base      = 0x0c0000000ULL,
                    .size      = 16 * 1024 * 1024ULL,
                    .page_size = KL1_L3_PAGE_SIZE,
                    .page_bits = KL1_L3_PAGE_BITS,
            },
            {
                    .kind      = XPU_MEM_HBM,
                    .region    = MMRGN_CODE,
                    .base      = 0x200000000ULL,
                    .size      = 0x008000000ULL,
                    .page_size = 1 << 12,
                    .page_bits = 12,
            },
    },
    {
            // pd1
            {
                    .kind      = XPU_MEM_HBM,
                    .region    = MMRGN_HBM_LO,
                    .base      = 0x608000000ULL,
                    .size      = 0x0f8000000ULL,
                    .page_size = KL1_HBM_PAGE_SIZE,
                    .page_bits = KL1_HBM_PAGE_BITS,
            },
            {
                    .kind      = XPU_MEM_HBM,
                    .region    = MMRGN_HBM_HI,
                    .base      = 0x700000000ULL,
                    .size      = 0x100000000ULL,
                    .page_size = KL1_HBM_PAGE_SIZE,
                    .page_bits = KL1_HBM_PAGE_BITS,
            },
            {
                    .kind      = XPU_MEM_L3,
                    .region    = MMRGN_L3,
                    .base      = 0x4c0000000ULL,
                    .size      = 16 * 1024 * 1024ULL,
                    .page_size = KL1_L3_PAGE_SIZE,
                    .page_bits = KL1_L3_PAGE_BITS,
            },
            {
                    .kind      = XPU_MEM_HBM,
                    .region    = MMRGN_CODE,
                    .base      = 0x600000000ULL,
                    .size      = 0x008000000ULL,
                    .page_size = 1 << 12,
                    .page_bits = 12,
            },
    }
};

int __setup_mem_data(struct xpu_mem_data *mem)
{
    u64 page_count = mem->size >> mem->page_bits;

    LOGL4("MEMORY_INFO "
          "kind= %d region= %d sz= 0x%llx pg_bits= %d pg_cnt= %llu bitmap_sz= %llu\n",
          mem->kind, mem->region, mem->size, mem->page_bits, page_count, page_count >> 3);

    // Alloc bitmap, each byte is 8 bits, which is 8 pages, so we need $page_count/8 bytes
    mem->page_bitmap = vmalloc(page_count >> 3);
    if (!mem->page_bitmap) {
        LOGE("vmalloc page_bitmap failed, region= %d\n", mem->region);
        goto err_nomem;
    }

    mem->page_count = page_count;
    mem->first_free = 0;
    mem->page_used  = 0;

    // The table record number of pages alloced at each position which used for free
    mem->free_table = vmalloc((page_count * sizeof(u64)));

    // score mem owner
    mem->owner_table = vmalloc((page_count * sizeof(struct xpu_session *)));
    if ((!mem->free_table) || (!mem->owner_table)) {
        LOGE("[xpu_%d] vmalloc owner_table failed, region= %d\n", mem->xpd->devfile_id,
             mem->region);
        goto err_nomem;
    }

    // Init all tables and lock
    spin_lock_init(&mem->lock);

    bitmap_clear(mem->page_bitmap, 0, mem->page_count);
    memset(mem->free_table, 0, page_count * sizeof(u64));
    memset(mem->owner_table, 0, page_count * sizeof(struct xpu_session *));

    return 0;

err_nomem:
    __unsetup_mem_data(mem);

    return -XPUERR_NOCPUMEM;
}

// Allocate and init memory management bitmap
int xpu_mem_setup(struct xpu_pd *xpd)
{
    int ret = 0;

    memcpy(xpd->mem, xpu_mems[xpd->id], MMRGN_CNT * sizeof(struct xpu_mem_data));

    xpd->mem[MMRGN_HBM_LO].xpd = xpd;
    ret                        = __setup_mem_data(&xpd->mem[MMRGN_HBM_LO]);
    if (ret != 0)
        goto err_setup_out;

    xpd->mem[MMRGN_HBM_HI].xpd = xpd;
    ret                        = __setup_mem_data(&xpd->mem[MMRGN_HBM_HI]);
    if (ret != 0)
        goto err_unsetup_hbm_lo;

    xpd->mem[MMRGN_L3].xpd = xpd;
    ret                    = __setup_mem_data(&xpd->mem[MMRGN_L3]);
    if (ret != 0)
        goto err_unsetup_hbm_hi;

    xpd->mem[MMRGN_CODE].xpd = xpd;
    ret                      = __setup_mem_data(&xpd->mem[MMRGN_CODE]);
    if (ret != 0)
        goto err_unsetup_l3;

    return 0;

err_unsetup_l3:
    __unsetup_mem_data(&xpd->mem[MMRGN_L3]);
err_unsetup_hbm_hi:
    __unsetup_mem_data(&xpd->mem[MMRGN_HBM_HI]);
err_unsetup_hbm_lo:
    __unsetup_mem_data(&xpd->mem[MMRGN_HBM_LO]);
err_setup_out:
    return ret;
}

// Free vmalloced space inside a struct xpu_mem_data
int __unsetup_mem_data(struct xpu_mem_data *mem)
{
    if (mem->page_bitmap) {
        vfree(mem->page_bitmap);
        mem->page_bitmap = NULL;
    }

    if (mem->free_table) {
        vfree(mem->free_table);
        mem->free_table = NULL;
    }

    if (mem->owner_table) {
        vfree(mem->owner_table);
        mem->owner_table = NULL;
    }

    return 0;
}

// Free XPU memory management metadata
void xpu_mem_unsetup(struct xpu_pd *xpd)
{
    __unsetup_mem_data(&xpd->mem[MMRGN_CODE]);
    __unsetup_mem_data(&xpd->mem[MMRGN_L3]);
    __unsetup_mem_data(&xpd->mem[MMRGN_HBM_HI]);
    __unsetup_mem_data(&xpd->mem[MMRGN_HBM_LO]);
}

u64 __malloc_locked(struct xpu_session *xsess, struct xpu_mem_data *mem, u64 sz)
{
    u64 pos         = 0ULL;
    u64 addr        = 0ULL;
    u64 page_needed = 0;

    page_needed = (u64)((sz + mem->page_size - 1) >> mem->page_bits);

    if ((page_needed + mem->page_used) > mem->page_count)
        return 0;

    //pos = bitmap_find_next_zero_area(mem->page_bitmap,
    //                                 mem->page_count,
    //                                 mem->first_free,
    //                                 page_needed, 0);
    pos = bitmap_find_next_zero_area(mem->page_bitmap, mem->page_count, 0, page_needed, 0);
    if (pos >= mem->page_count)
        return 0;

    bitmap_set(mem->page_bitmap, pos, page_needed);
    mem->page_used += page_needed;
    if (pos == mem->first_free)
        mem->first_free = (pos + page_needed);
    mem->free_table[pos]  = page_needed;
    mem->owner_table[pos] = xsess;
    addr                  = pageid_to_addr(mem, pos);
    LOGL3("[xpu_%d xsess_%u] Alloc "
          "region= %d sz= %llu pgcnt= %llu addr= %px pos= %llu ff= %llu\n",
          xsess->xpd->devfile_id, xsess->id, mem->region, sz, page_needed, (void *)(addr), pos,
          mem->first_free);

    if (mem->kind == XPU_MEM_HBM) {
        atomic_add(page_needed, &xsess->xctx->main_page_used);
    } else if (mem->kind == XPU_MEM_L3) {
        atomic_add(page_needed, &xsess->xctx->cache_mem_page_used);
    }

    return addr;
}

// Alloc XPU memory
u64 xpu_mem_alloc(struct xpu_session *xsess, u64 sz, int kind)
{
    struct xpu_mem_data *mem                          = NULL;
    XPUMemoryRegion      candidate_regions[MMRGN_CNT] = { -1 };
    int                  candidate_region_cnt         = 0;
    int                  i                            = 0;
    u64                  addr                         = 0;

    if (xsess == NULL) {
        LOGW("xsess= %px\n", xsess);
        return 0;
    }

    switch (kind) {
    case XPU_MEM_HBM:
        candidate_regions[candidate_region_cnt++] = MMRGN_HBM_LO;
        candidate_regions[candidate_region_cnt++] = MMRGN_HBM_HI;
        break;
    case XPU_MEM_L3:
        candidate_regions[candidate_region_cnt++] = MMRGN_L3;
        break;
    case XPU_MEM_CODE:
        candidate_regions[candidate_region_cnt++] = MMRGN_CODE;
        break;
    default:
        break;
    }

    for (i = 0; i < candidate_region_cnt; ++i) {
        mem = &xsess->xpd->mem[candidate_regions[i]];

        spin_lock_bh(&mem->lock);
        addr = __malloc_locked(xsess, mem, sz);
        spin_unlock_bh(&mem->lock);

        if (addr)
            break;
    }

    if ((addr == 0) && (kind != XPU_MEM_L3))
        LOGW("malloc not success, sz=%llx type=%d\n", sz, kind);

    return addr;
}

u64 __free_locked(struct xpu_session *xsess, struct xpu_mem_data *mem, u64 addr)
{
    struct xpu_session *owner      = NULL;
    u64                 page_freed = 0;
    u64                 start      = 0;

    start      = addr_to_pageid(mem, addr);
    page_freed = mem->free_table[start];

    if ((0 == page_freed) || ((start + page_freed) > mem->page_count)) {
        LOGW("[xpu_%d xsess_%u] "
             "free invalid addr= %px region= %d pos= %llu + %llu pgcnt= %llu\n",
             xsess->xpd->devfile_id, xsess->id, (void *)addr, mem->region, start, page_freed,
             mem->page_count);
        return 0;
    }

    //check permission
    owner = mem->owner_table[start];
    if (xsess != owner) {
        LOGW("[xpu_%d xsess_%u] free addr= %px but owner is xsess_%u\n", xsess->xpd->devfile_id,
             xsess->id, (void *)addr, (owner ? owner->id : 0));
        return 0;
    }

    bitmap_clear(mem->page_bitmap, start, page_freed);
    mem->page_used -= page_freed;
    mem->free_table[start]  = 0;
    mem->owner_table[start] = NULL;
    if (start < mem->first_free)
        mem->first_free = start;

    if (mem->kind == XPU_MEM_HBM) {
        atomic_sub(page_freed, &xsess->xctx->main_page_used);
    } else if (mem->kind == XPU_MEM_L3) {
        atomic_sub(page_freed, &xsess->xctx->cache_mem_page_used);
    }

    return page_freed;
}

void xpu_mem_free(struct xpu_session *xsess, u64 addr)
{
    struct xpu_pd       *xpd = NULL;
    struct xpu_mem_data *mem = NULL;
    int                  i   = 0;

    xpd = xsess->xpd;

    for (i = 0; i < MMRGN_CNT; ++i) {
        if ((addr >= xpd->mem[i].base) && (addr < xpd->mem[i].base + xpd->mem[i].size)) {
            mem = &xpd->mem[i];
            break;
        }
    }

    if (mem == NULL) {
        LOGW("[xpu_%d xsess_%u] invalid memaddr= 0x%llx\n", xpd->devfile_id, xsess->id, addr);
        return;
    }

    LOGL3("[xpu_%d xsess_%u] Free addr= %px region= %d\n", xpd->devfile_id, xsess->id, (void *)addr,
          mem->region);

    spin_lock_bh(&mem->lock);
    __free_locked(xsess, mem, addr);
    spin_unlock_bh(&mem->lock);
}

void __release_mem_data_by_owner(struct xpu_mem_data *mem, struct xpu_session *xsess)
{
    u64 offset           = 0UL;
    u64 start            = 0UL;
    u64 size             = 0;
    u64 freed_page_count = 0;

    spin_lock_bh(&mem->lock);
    while (offset < mem->page_count) {
        //LOGL2("Dev[%d] try to find_next_bit, start=0x%px, page_count=%llu, offset=%llu\n",
        //        dev->dev_num, start, page_count, offset);
        start = find_next_bit(mem->page_bitmap, mem->page_count, offset);

        //If no bits are set, find_next_bit returns $page_count
        if (start >= mem->page_count)
            break;

        size   = mem->free_table[start];
        offset = start + size;

        if (mem->owner_table[start] != xsess)
            continue;

        LOGL3("[xpu_%d xsess_%u] AutoFree addr= %px region= %d ownercheck= %u\n",
              xsess->xpd->devfile_id, xsess->id, (void *)pageid_to_addr(mem, start), mem->region,
              mem->owner_table[start]->id);

        __free_locked(xsess, mem, pageid_to_addr(mem, start));
        freed_page_count += size;
    }
    spin_unlock_bh(&mem->lock);
}

void xpu_release_mem(struct xpu_session *xsess)
{
    int i = 0;
    for (i = 0; i < MMRGN_CNT; ++i)
        __release_mem_data_by_owner(&xsess->xpd->mem[i], xsess);
}
