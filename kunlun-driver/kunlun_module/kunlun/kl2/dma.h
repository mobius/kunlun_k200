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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_DMA_H
#define BAIDU_XPU_RUNTIME_MODULE_KL2_DMA_H

#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/bitops.h>
#include <linux/mm.h>
#include <linux/rbtree.h>

#include "kl_util.h"

#define SG_DMA_USE_BASIC_MODE

struct kl2_device;
struct kl2_userprocess;
struct XPUMemcpyExIoctlArgs;

enum {
    KL2_DMA_MAX_CHANNEL      = 8,
    KL2_DMA_BITMAP_ULONG_NUM = BITS_TO_LONGS(KL2_DMA_MAX_CHANNEL),
};

struct dma_channel {
    int        idx;
    int        ch;
    void      *buffer;
    dma_addr_t dma_addr;
};

enum {
    KL2_HOSTMEM_UNPINNED      = 0,
    KL2_HOSTMEM_PINNED        = 1,
    KL2_HOSTMEM_PARTLY_PINNED = 2,
};

enum {
    KL2_HOSTMEM_HUGEPAGE_ORDER = 3,
    KL2_HOSTMEM_HUGEPAGE_SIZE  = PAGE_SIZE << KL2_HOSTMEM_HUGEPAGE_ORDER,
};

struct kl2_sg_minfo {
    u64                addr;
    u64                size;
    u64                page_count;
    struct page      **user_pages;
    struct sg_table    sgt;
    u32                mapped_nents;
    struct rb_node     uproc_node;
    struct kl2_device *kl2_dev;
    bool               mmaped;
    struct xref        xref;
};

struct dma_engine {
    struct pci_dev    *pdev;
    struct dma_ops    *ops;
    void              *data;
    int                ch_num;
    struct semaphore   sema;
    spinlock_t         bitmap_lock;
    unsigned long      bitmap[KL2_DMA_BITMAP_ULONG_NUM];
    struct dma_channel ch[KL2_DMA_MAX_CHANNEL];
};

struct dma_ops {
    void (*ddma_to_host)(void *data, u64 dest, u64 src, u64 sz, int ch);
    void (*ddma_from_host)(void *data, u64 dest, u64 src, u64 sz, int ch);
    void (*ddma_device_to_device)(void *data, u64 dest, u64 src, u64 sz, int ch);
    int (*sg_dma)(void *data, u64 descriptor, u64 descsz, int ch, int is_from_host, int nowait,
                  u32 ctrl);
    //int (*sg_dma_update_desc)(void *data, int ch);
    int (*wait_dma_finished)(void *data, int ch);
};

struct kl2_sg_minfo *kl2_minfo_rb_search(struct rb_root *root, u64 addr, u64 size);
int                  kl2_minfo_rb_insert(struct rb_root *root, struct kl2_sg_minfo *minfo);
void                 kl2_minfo_rb_erase(struct rb_root *root, struct kl2_sg_minfo *minfo);

int  kl2_pin_host_memory(struct kl2_userprocess *uproc, u64 addr, u64 size,
                         struct kl2_sg_minfo *minfo);
void kl2_unpin_host_memory(struct kl2_sg_minfo *minfo);
int  kl2_host_memory_is_pinned(struct kl2_userprocess *uproc, u64 addr, u64 size,
                               struct kl2_sg_minfo **minfo);

int  kl2_host_alloc_hugepages(struct kl2_userprocess *uproc, u64 addr, u64 size,
                              struct kl2_sg_minfo *minfo);
void kl2_host_free_hugepages(struct kl2_sg_minfo *minfo);
void kl2_destroy_minfo_ref(struct kref *kref);

extern struct dma_ops kl2_dma_ops;

int  kl2_dma_init(struct kl2_device *kl2_dev, int ch_num, int ch_bits, struct dma_ops *ops);
void kl2_dma_destroy(struct kl2_device *kl2_dev);
int  kl2_dma_valid_check(struct kl2_device *kl2_dev, struct XPUMemcpyExIoctlArgs *args, int h2d);
int  kl2_dma_device_to_device(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz, u64 *time_ns);
int  kl2_dma_peer_to_peer(struct dma_engine *dst_dma, struct dma_engine *src_dma, u64 dst, u64 src,
                          u64 cpsz, u64 *time_ns);
int  kl2_dma_ddma_to_peer(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz, u64 *time_ns);
int  kl2_dma_ddma_to_host(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz, u64 *cost);
int  kl2_dma_ddma_to_host_kernel(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz);
int  kl2_dma_ddma_from_host(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz, u64 *cost);
int  kl2_dma_ddma_from_host_kernel(struct dma_engine *dma, u64 dest, u64 src, u64 cpsz);
int  kl2_dma_ddma_zero_gm(struct dma_engine *dma, u64 dest, u64 cpsz);
int  kl2_dma_sgdma(struct dma_engine *dma, u64 dst, u64 src, u64 size, struct kl2_sg_minfo *minfo,
                   int is_from_host, u64 *cost);

#endif
