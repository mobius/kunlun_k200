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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL_MM_H
#define BAIDU_XPU_RUNTIME_MODULE_KL_MM_H

#include <linux/types.h>
#include <linux/spinlock.h>

struct kl_device;

struct kl_memory_info {
    u64 base;
    u64 size;
    u32 page_bits;
    int kind;
};

struct kl_memory_range {
    u64 base;
    u64 size;
};

struct kl_mm_info {
    int                     count;
    struct kl_memory_info  *mem;
    int                     user_dma_rr_rw_range_count;
    struct kl_memory_range *user_dma_rr_rw_range;
};

// kunlun memory info
struct kl_memory {
    // memory index
    int idx;
    // memory kind
    int kind;

    spinlock_t lock;
    // base address of this memory
    u64 base;
    // size in bytes of this memory
    u64 size;
    // base + size
    u64 limit;
    // 1 << page_bits == page_size
    u32 page_bits;
    // page size used to manage this memory
    u32 page_size;
    // total number of pages
    u64 page_count;
    // page bitmap table
    unsigned long *page_bitmap;
    // current free page index
    u64 page_current;
    // number of used pages
    u64 page_used;
    // length of each malloc
    u64 *free_table;
    // owner of each malloc
    const void **owner_table;
};

// general kunlun memory management
struct kl_mm {
    struct kl_device *kdev;

    // number of memories this mm manages
    int mem_count;
    // info of each memory
    struct kl_memory *mem;
    // info of huge memory
    struct kl_memory *huge_mem;
};

typedef void (*kl_mm_cb_t)(u64 addr, u64 size, int kind, void *owner, struct kl_memory *mem);

static inline u64 pageid_to_addr(struct kl_memory *mem, u64 page_id)
{
    return mem->base + (page_id << mem->page_bits);
}

static inline u64 addr_to_pageid(struct kl_memory *mem, u64 addr)
{
    return (addr - mem->base) >> mem->page_bits;
}

// initialize a kunlun memory manager
int kl_mm_init(struct kl_device *kdev, struct kl_mm *mm, struct kl_mm_info *mminfo);

// uninitialize a kunlun memory manager
void kl_mm_uninit(struct kl_mm *mm);

// malloc size bytes from memory_idx
int kl_mm_malloc(struct kl_mm *mm, u64 size, int kind, void *owner, u64 *addr,
                 struct kl_memory **mem, kl_mm_cb_t cb);

struct kl_memory *find_memory_by_addr(struct kl_mm *mm, u64 addr);

int kl_mm_free(struct kl_mm *mm, u64 addr, void *owner, kl_mm_cb_t cb);

// free all memories owned by owner
void kl_mm_free_by_owner(struct kl_mm *mm, void *owner, kl_mm_cb_t cb);

u32 kl_mm_get_pgsz(struct kl_mm *mm, int kind);

u64 kl_mm_get_pg_all(struct kl_mm *mm, int kind);

u64 kl_mm_get_bytes_used(struct kl_mm *mm, int kind);

u64 kl_mm_get_bytes_all(struct kl_mm *mm, int kind);

#endif
