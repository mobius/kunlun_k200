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

#include "kl_mm.h"
#include "kl_drv.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

// lock mem block and sync bitmap, iff kind == XPU_MEM_MAIN
#define lock_and_sync_mem_bitmap                                                                   \
    page_id_begin = 0;                                                                             \
    for (idx = 0; idx < mm->mem_count; ++idx) {                                                    \
        if (mm->mem[idx].kind != XPU_MEM_MAIN)                                                     \
            continue;                                                                              \
        spin_lock(&mm->mem[idx].lock);                                                             \
        memcpy(mm->huge_mem->page_bitmap + page_id_begin / BITS_PER_LONG,                          \
               mm->mem[idx].page_bitmap, mm->mem[idx].page_count >> 3);                            \
        page_id_begin += mm->mem[idx].page_count;                                                  \
    }

// unlock mem block and sync bitmap, iff kind == XPU_MEM_MAIN
#define sync_mem_bitmap_and_unlock                                                                 \
    page_id_begin = 0;                                                                             \
    for (idx = 0; idx < mm->mem_count; ++idx) {                                                    \
        if (mm->mem[idx].kind != XPU_MEM_MAIN)                                                     \
            continue;                                                                              \
        memcpy(mm->mem[idx].page_bitmap,                                                           \
               mm->huge_mem->page_bitmap + page_id_begin / BITS_PER_LONG,                          \
               mm->mem[idx].page_count >> 3);                                                      \
        spin_unlock(&mm->mem[idx].lock);                                                           \
        page_id_begin += mm->mem[idx].page_count;                                                  \
    }

static int __setup_kl_memory(struct kl_memory *mem, struct kl_memory_info *info)
{
    if (mem == NULL || info == NULL)
        return -XPUERR_INVALID_PARAM;

    mem->kind         = info->kind;
    mem->base         = info->base;
    mem->size         = info->size;
    mem->limit        = info->base + info->size;
    mem->page_bits    = info->page_bits;
    mem->page_size    = 1 << info->page_bits;
    mem->page_count   = info->size >> info->page_bits;
    mem->page_current = 0;
    mem->page_used    = 0;

    LOGI("setup mem[%d] kind=%d base=%llx limit=%llx pgsz=%u pgcnt=%llu\n", mem->idx, mem->kind,
         mem->base, mem->limit, mem->page_size, mem->page_count);

    if (mem->page_count) {
        mem->page_bitmap = vzalloc((mem->page_count + 0x7) >> 3);
        mem->free_table  = vzalloc(mem->page_count * sizeof(u64));
        mem->owner_table = vzalloc(mem->page_count * sizeof(void *));
        if (mem->page_bitmap == NULL || mem->free_table == NULL || mem->owner_table == NULL)
            goto err_out;
    }

    spin_lock_init(&mem->lock);

    return 0;

err_out:
    if (mem->owner_table)
        vfree(mem->owner_table);

    if (mem->free_table)
        vfree(mem->free_table);

    if (mem->page_bitmap)
        vfree(mem->page_bitmap);

    return -XPUERR_NOCPUMEM;
}

static void __unsetup_kl_memory(struct kl_memory *mem)
{
    if (mem == NULL)
        return;

    mem->page_size    = 0;
    mem->page_count   = 0;
    mem->page_current = 0;
    mem->page_used    = 0;

    if (mem->owner_table)
        vfree(mem->owner_table);

    if (mem->free_table)
        vfree(mem->free_table);

    if (mem->page_bitmap)
        vfree(mem->page_bitmap);
}

int kl_mm_init(struct kl_device *kdev, struct kl_mm *mm, struct kl_mm_info *mminfo)
{
    // base, size, page_bits, kind
    struct kl_memory_info huge_mem_info   = { ~0ull, 0ull, 0, XPU_MEM_MAIN };
    u64                   last_info_limit = 0;
    int                   err;
    int                   idx;

    if (mm == NULL || mminfo == NULL)
        return -XPUERR_INVALID_PARAM;

    LOGD("mem_count=%d\n", mminfo->count);

    for (idx = 0; idx < mminfo->count; ++idx) {
        struct kl_memory_info *info = &mminfo->mem[idx];
        if (info->kind != XPU_MEM_MAIN)
            continue;

        if (info->base & ((1ull << info->page_bits) - 1)) {
            LOGW("info->base not aligned, idx= %d, info->base= %llx, info->page_bits= %d", idx,
                 info->base, info->page_bits);
            return -XPUERR_INVALID_PARAM;
        }
        if (info->size & ((1ull << info->page_bits) - 1)) {
            LOGW("info->size not aligned, idx= %d, info->size= %llx, info->page_bits= %d", idx,
                 info->size, info->page_bits);
            return -XPUERR_INVALID_PARAM;
        }
        if ((info->size >> info->page_bits) % BITS_PER_LONG) {
            LOGW("info->page_count not aligned to BITS_PER_LONG, idx= %d, info->size= %llx, info->page_bits= %d\n",
                 idx, info->size, info->page_bits);
            return -XPUERR_INVALID_PARAM;
        }

        if (huge_mem_info.size) {
            // 非首个kl_memory_info
            if (huge_mem_info.page_bits != info->page_bits) {
                LOGW("diff in info->page_bits, page_bits0= %d, page_bits1= %d\n",
                     huge_mem_info.page_bits, info->page_bits);
                return -XPUERR_INVALID_PARAM;
            }
            if (last_info_limit != info->base) {
                LOGW("info not contiguous, last_info_limit= %llx, info->base= %llx\n",
                     last_info_limit, info->base);
                return -XPUERR_INVALID_PARAM;
            }
        } else {
            // 首个kl_memory_info
            huge_mem_info.base      = info->base;
            huge_mem_info.page_bits = info->page_bits;
        }
        huge_mem_info.size += info->size;
        last_info_limit = info->base + info->size;
    }

    mm->mem = kcalloc(mminfo->count, sizeof(struct kl_memory), GFP_KERNEL);
    if (mm->mem == NULL)
        return -XPUERR_NOCPUMEM;

    for (idx = 0; idx < mminfo->count; ++idx) {
        mm->mem[idx].idx = idx;
        err              = __setup_kl_memory(&mm->mem[idx], &mminfo->mem[idx]);
        if (err)
            goto err_unsetup_kl_memory;
    }

    // 初始化huge_mem
    mm->huge_mem = kzalloc(sizeof(struct kl_memory), GFP_KERNEL);
    if (mm->huge_mem == NULL) {
        err = -XPUERR_NOCPUMEM;
        goto err_unsetup_kl_memory;
    }
    err = __setup_kl_memory(mm->huge_mem, &huge_mem_info);
    if (err)
        goto err_unsetup_huge_mem;

    mm->kdev      = kdev;
    mm->mem_count = mminfo->count;

    return 0;

err_unsetup_huge_mem:
    kfree(mm->huge_mem);

err_unsetup_kl_memory:
    for (--idx; idx >= 0; --idx) {
        __unsetup_kl_memory(&mm->mem[idx]);
    }
    kfree(mm->mem);

    return err;
}

// uninitialize a kunlun memory manager
void kl_mm_uninit(struct kl_mm *mm)
{
    int idx = 0;

    if (mm == NULL)
        return;

    __unsetup_kl_memory(mm->huge_mem);
    kfree(mm->huge_mem);

    for (idx = 0; idx < mm->mem_count; ++idx) {
        __unsetup_kl_memory(&mm->mem[idx]);
    }
    kfree(mm->mem);

    mm->mem_count = 0;
}

static int __find_free_pages(struct kl_memory *mem, u64 page_cnt, u64 *page_id)
{
    u64 pos = 0;

    pos = bitmap_find_next_zero_area(mem->page_bitmap, mem->page_count, 0, page_cnt, 0);
    if (pos >= mem->page_count)
        return -XPUERR_NOMEM;

    *page_id = pos;

    return 0;
}

static void __take_pages(struct kl_memory *mem, u64 page_id, u64 page_cnt, void *owner)
{
    bitmap_set(mem->page_bitmap, page_id, page_cnt);
    mem->page_used += page_cnt;
    mem->free_table[page_id]  = page_cnt;
    mem->owner_table[page_id] = owner;

    mem->page_current = (page_id + page_cnt);
    if (mem->page_current >= mem->page_count) {
        mem->page_current = 0;
    }
}

static void __release_pages(struct kl_memory *mem, u64 page_id, u64 page_cnt)
{
    bitmap_clear(mem->page_bitmap, page_id, page_cnt);
    mem->page_used -= page_cnt;
    mem->free_table[page_id]  = 0;
    mem->owner_table[page_id] = NULL;
}

static int __malloc_locked(struct kl_memory *mem, u64 sz, void *owner, u64 *addr)
{
    u64 page_id     = 0ULL;
    u64 page_needed = 0;
    int err         = 0;

    page_needed = (u64)((sz + mem->page_size - 1) >> mem->page_bits);

    if ((page_needed + mem->page_used) > mem->page_count)
        return -XPUERR_NOMEM;

    err = __find_free_pages(mem, page_needed, &page_id);
    if (err)
        return -XPUERR_NOMEM;

    __take_pages(mem, page_id, page_needed, owner);

    *addr = pageid_to_addr(mem, page_id);

    return 0;
}

// XXX (liyunzheng): __malloc_huge_mem __free_huge_mem用于分配跨4G内存块，不限于大于4G内存。
//  mem_block0   mem_block1   mem_block2 ... mem_blockn
//  [....XXXX]   [XXXXXXXX]   [XXX....]      [.......]
//  如上所示一个huge_mem占用了mem_block0 mem_block1 mem_block2的部分区域。其中mem_block0的第一个X对应位置的owner_table,
//  free_table中储存huge_mem的owner和完整size。memblock1和memblock2的第一个X中存储huge_mem在该区域中的空间占用，
//  owner_table留空。
static int __malloc_huge_mem(struct kl_mm *mm, u64 size, int kind, void *owner, u64 *addr,
                             kl_mm_cb_t cb)
{
    int idx;
    u64 page_id_begin, page_id_end, page_id, page_needed, page_id_in_mem, page_needed_in_mem;
    int first_mem = 1;
    int err       = -XPUERR_NOMEM;

    page_needed = (u64)((size + mm->huge_mem->page_size - 1) >> mm->huge_mem->page_bits);

    lock_and_sync_mem_bitmap;

    err = __find_free_pages(mm->huge_mem, page_needed, &page_id);
    if (!err) {
        __take_pages(mm->huge_mem, page_id, page_needed, owner);
        *addr = pageid_to_addr(mm->huge_mem, page_id);
        if (cb)
            cb(*addr, size, XPU_MEM_MAIN, owner, mm->huge_mem);

        page_id_begin = 0;
        for (idx = 0; idx < mm->mem_count; ++idx) {
            if (mm->mem[idx].kind != XPU_MEM_MAIN)
                continue;
            page_id_end = page_id_begin + mm->mem[idx].page_count;
            if (page_id >= page_id_begin && page_id < page_id_end && page_needed > 0) {
                page_id_in_mem     = page_id - page_id_begin;
                page_needed_in_mem = MIN(page_id_end - page_id, page_needed);
                if (first_mem) {
                    first_mem                                = 0;
                    mm->mem[idx].free_table[page_id_in_mem]  = page_needed;
                    mm->mem[idx].owner_table[page_id_in_mem] = owner;
                } else {
                    mm->mem[idx].free_table[page_id_in_mem] = page_needed_in_mem;
                }
                mm->mem[idx].page_used += page_needed_in_mem;

                page_id = page_id_end;
                page_needed -= page_needed_in_mem;
            }
            page_id_begin = page_id_end;
        }
    }

    sync_mem_bitmap_and_unlock;

    return err;
}

static int __free_huge_mem(struct kl_mm *mm, struct kl_memory *mem, void *owner, u64 addr,
                           kl_mm_cb_t cb)
{
    int idx;
    u64 page_id_begin, page_id_end, page_id, page_freed, page_id_in_mem, page_freed_in_mem;
    int err = 0;

    lock_and_sync_mem_bitmap;

    page_id        = addr_to_pageid(mm->huge_mem, addr);
    page_id_in_mem = addr_to_pageid(mem, addr);
    page_freed     = mm->huge_mem->free_table[page_id];

    if (0 == page_freed) {
        //LOGW("free invalid addr= %px mem= %d page= %llu + %llu pgcnt= %llu\n",
        //     (void *)addr, mem->idx, page_id, page_freed, mem->page_count);
        return -XPUERR_INVALID_PARAM;
    }

    if (owner != mem->owner_table[page_id_in_mem]) {
        LOGW("free addr= %px but owner does not match\n", (void *)addr);
        return -XPUERR_INVALID_PARAM;
    }

    __release_pages(mm->huge_mem, page_id, page_freed);
    if (cb)
        cb(addr, page_freed * mm->huge_mem->page_size, XPU_MEM_MAIN, owner, mm->huge_mem);

    page_id_begin = 0;
    for (idx = 0; idx < mm->mem_count; ++idx) {
        if (mm->mem[idx].kind != XPU_MEM_MAIN)
            continue;
        page_id_end = page_id_begin + mm->mem[idx].page_count;
        if (page_id >= page_id_begin && page_id < page_id_end && page_freed > 0) {
            page_id_in_mem                           = page_id - page_id_begin;
            page_freed_in_mem                        = MIN(page_id_end - page_id, page_freed);
            mm->mem[idx].free_table[page_id_in_mem]  = 0;
            mm->mem[idx].owner_table[page_id_in_mem] = NULL;
            mm->mem[idx].page_used -= page_freed_in_mem;

            page_id = page_id_end;
            page_freed -= page_freed_in_mem;
        }
        page_id_begin = page_id_end;
    }

    sync_mem_bitmap_and_unlock;

    return err;
}

// malloc size bytes from memory_idx
int kl_mm_malloc(struct kl_mm *mm, u64 size, int kind, void *owner, u64 *addr,
                 struct kl_memory **mem, kl_mm_cb_t cb)
{
    int idx;
    int err = -XPUERR_NOMEM;

#ifdef BRINGUP_L3_ONLY
    // try use L3 only, for bringup
    if (kind != XPU_MEM_L3) {
        LOGD("only support L3: try L3 for size: 0x%llx, kind:%d\n", size, kind);
        kind = XPU_MEM_L3;
    }
#endif

    if (mm == NULL || size == 0 || owner == NULL || addr == NULL)
        return -XPUERR_INVALID_PARAM;

    if (kind == XPU_MEM_MAIN_KL2_HUGE) {
        err = __malloc_huge_mem(mm, size, kind, owner, addr, cb);
        return err;
    }

    for (idx = 0; idx < mm->mem_count; ++idx) {
        if (mm->mem[idx].kind != kind)
            continue;

        spin_lock(&mm->mem[idx].lock);
        err = __malloc_locked(&mm->mem[idx], size, owner, addr);
        spin_unlock(&mm->mem[idx].lock);
#ifdef BRINGUP_L3_ONLY
        if (*addr < 0x0C0000000ull || *addr >= 0xC4000000ull) {
            // malloc return beyond L3 space, this should not happen
            LOGE("malloc address not valid L3 :0x%llx\n", *addr);
            err = -XPUERR_NOMEM;
        }
#endif
        if (!err) {
            if (mem)
                *mem = &mm->mem[idx];
            if (cb)
                cb(*addr, size, kind, owner, &mm->mem[idx]);
            LOGD("%s: malloc %016llx\n", mm->kdev->name, *addr);
            return 0;
        }

        LOGD("fail to malloc sz=%llu on idx=%d kind=%d\n", size, idx, kind);
    }

    LOGD("malloc fail. kind=%d sz=0x%llx\n", kind, size);

    return err;
}

struct kl_memory *find_memory_by_addr(struct kl_mm *mm, u64 addr)
{
    int idx = 0;

    for (; idx < mm->mem_count; ++idx) {
        if ((mm->mem[idx].base <= addr) && (addr < mm->mem[idx].limit))
            return &mm->mem[idx];
    }

    return NULL;
}

static int __free_locked(struct kl_mm *mm, struct kl_memory *mem, u64 addr, void *owner,
                         kl_mm_cb_t cb)
{
    int err;
    u64 page_id;
    u64 page_freed;

    page_id    = addr_to_pageid(mem, addr);
    page_freed = mem->free_table[page_id];

    if (0 == page_freed) {
        //LOGW("free invalid addr= %px mem= %d page= %llu + %llu pgcnt= %llu\n",
        //     (void *)addr, mem->idx, page_id, page_freed, mem->page_count);
        return -XPUERR_INVALID_PARAM;
    }

    if (owner != mem->owner_table[page_id]) {
        LOGW("free addr= %px but owner does not match\n", (void *)addr);
        return -XPUERR_INVALID_PARAM;
    }

    // 原先从huge_mem分配
    if ((page_id + page_freed) > mem->page_count) {
        spin_unlock(&mem->lock);
        err = __free_huge_mem(mm, mem, owner, addr, cb);
        spin_lock(&mem->lock);
        return err;
    }

    __release_pages(mem, page_id, page_freed);
    if (cb)
        cb(addr, page_freed * mem->page_size, mem->kind, owner, mem);
    return 0;
}

int kl_mm_free(struct kl_mm *mm, u64 addr, void *owner, kl_mm_cb_t cb)
{
    struct kl_memory *mem;
    int               err;

    mem = find_memory_by_addr(mm, addr);
    if (!mem)
        return -XPUERR_INVALID_PARAM;

    spin_lock(&mem->lock);
    err = __free_locked(mm, mem, addr, owner, cb);
    spin_unlock(&mem->lock);

    return err;
}

static void __free_by_owner_locked(struct kl_mm *mm, struct kl_memory *mem, void *owner,
                                   kl_mm_cb_t cb)
{
    u64 freed_page_count;
    u64 p;
    u64 size;

    freed_page_count = 0;
    p                = 0;
    while (p < mem->page_count) {
        u64 blk = find_next_bit(mem->page_bitmap, mem->page_count, p);

        if (blk >= mem->page_count)
            break;

        size = mem->free_table[blk];
        p    = blk + size;

        if (mem->owner_table[blk] != owner)
            continue;

        __free_locked(mm, mem, pageid_to_addr(mem, blk), owner, cb);

        freed_page_count += size;
    }
}

// free all memories owned by owner
void kl_mm_free_by_owner(struct kl_mm *mm, void *owner, kl_mm_cb_t cb)
{
    int idx = 0;

    for (idx = 0; idx < mm->mem_count; ++idx) {
        struct kl_memory *mem = &mm->mem[idx];

        spin_lock(&mem->lock);
        __free_by_owner_locked(mm, mem, owner, cb);
        spin_unlock(&mem->lock);
    }
}

inline u32 kl_mm_get_pgsz(struct kl_mm *mm, int kind)
{
    int idx = 0;
    for (idx = 0; idx < mm->mem_count; ++idx)
        if (mm->mem[idx].kind == kind)
            return mm->mem[idx].page_size;
    return 0;
}

u64 kl_mm_get_pg_used(struct kl_mm *mm, int kind)
{
    int idx = 0;
    u64 ret = 0;
    for (idx = 0; idx < mm->mem_count; ++idx)
        if (mm->mem[idx].kind == kind)
            ret += mm->mem[idx].page_used;
    return ret;
}

u64 kl_mm_get_pg_all(struct kl_mm *mm, int kind)
{
    int idx = 0;
    u64 ret = 0;
    for (idx = 0; idx < mm->mem_count; ++idx)
        if (mm->mem[idx].kind == kind)
            ret += mm->mem[idx].page_count;
    return ret;
}

u64 kl_mm_get_bytes_used(struct kl_mm *mm, int kind)
{
    int idx = 0;
    u64 ret = 0;
    for (idx = 0; idx < mm->mem_count; ++idx)
        if (mm->mem[idx].kind == kind)
            ret += mm->mem[idx].page_used * mm->mem[idx].page_size;
    return ret;
}

u64 kl_mm_get_bytes_all(struct kl_mm *mm, int kind)
{
    int idx = 0;
    u64 ret = 0;
    for (idx = 0; idx < mm->mem_count; ++idx)
        if (mm->mem[idx].kind == kind)
            ret += mm->mem[idx].size;
    return ret;
}
