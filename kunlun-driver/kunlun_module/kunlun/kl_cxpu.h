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

#ifndef BAIDU_XPU_RUNTIME_MODULE_CXPU_H
#define BAIDU_XPU_RUNTIME_MODULE_CXPU_H

#include "kl_util.h"
#include "xpu/defs.h"
#include "xpurt_priv/defs_private.h"

#define CXPU_INSTANCE_ID_LEN (12)
#define CXPU_INSTANCE_MEM_UNLIMIT (~0x0ull)
#define CXPU_INSTANCE_CORE_UNLIMIT (~0x0u)
#define CXPU_INSTANCE_MAX_CORE_NUM (100)

typedef struct kl_cxpu_mem_info {
    u32 page_size;
    u64 page_cnt;
    u64 page_used;
} kl_cxpu_mem_info_t;

typedef struct kl_cxpu_core_info {
    u64 total_core;
    u64 used_core;
} kl_cxpu_core_info_t;

typedef struct kl_cxpu {
    struct mutex     instance_list_lock;
    struct list_head instance_list;
    int              instance_cnt;
    int              instance_max_cnt;

    bool enabled;

    //cxpu memory info
    struct kl_mm *mm;
    int           ecc_on;

    //cxpu memory info
    kl_cxpu_mem_info_t main_mem;
    kl_cxpu_mem_info_t l3_mem;

    //cxpu core info
    u32 assigned_core;
    u32 total_core;
} kl_cxpu_t;

typedef struct kl_cxpu_instance {
    struct list_head ent;
    struct xref      xref;
    struct mutex     lock;

    kl_cxpu_t *cxpu;
    char       instance_id[CXPU_INSTANCE_ID_LEN + 1];
    u64        token;

    //cxpu instance memory info
    atomic64_t mem_used_pgcnt[XPU_MEM_COUNT];
    u64        mem_limit_pgcnt[XPU_MEM_COUNT];

    //cxpu instance core info
    u32 core_limit;
} kl_cxpu_instance_t;

// cxpu interface
int                 kl_cxpu_proc_show(kl_cxpu_t *cxpu, struct seq_file *m);
int                 kl_cxpu_init(kl_cxpu_t *cxpu);
int                 kl_cxpu_uninit(kl_cxpu_t *cxpu);
int                 kl_cxpu_get_instance_count(kl_cxpu_t *cxpu);
int                 kl_cxpu_get_max_instance_count(kl_cxpu_t *cxpu);
int                 kl_cxpu_get_instance_id(kl_cxpu_t *cxpu, int index, char *instance_id);
kl_cxpu_instance_t *kl_cxpu_get_instance_by_id_locked(kl_cxpu_t *cxpu, char *instance_id);
kl_cxpu_instance_t *kl_cxpu_get_instance_by_token_locked(kl_cxpu_t *cxpu, u64 cgtoken);
void                kl_cxpu_put_instance_locked(kl_cxpu_t *cxpu, kl_cxpu_instance_t *instance);
int                 kl_cxpu_create_instance(kl_cxpu_t *cxpu, char *instance_id);
int                 kl_cxpu_destroy_instance(kl_cxpu_t *cxpu, char *instance_id);
int kl_cxpu_set_instance_mem_limit(kl_cxpu_t *cxpu, char *instance_id, XPUMemoryKind kind,
                                   u64 limit_bytes);
int kl_cxpu_get_memory_info(kl_cxpu_t *cxpu, XPUMemoryKind mem_kind, kl_cxpu_mem_info_t *mem_info);
int kl_cxpu_get_instance_memory_info(kl_cxpu_t *cxpu, char *instance_id, XPUMemoryKind mem_kind,
                                     kl_cxpu_mem_info_t *mem_info);

// cxpu instance interface
u64  kl_cxpu_instance_get_token(kl_cxpu_instance_t *instance);
int  kl_cxpu_instance_get_refcount(kl_cxpu_instance_t *instance);
bool kl_cxpu_instance_is_mem_limit_on(kl_cxpu_instance_t *instance, XPUMemoryKind kind);
u64  kl_cxpu_instance_get_mem_used(kl_cxpu_instance_t *instance, XPUMemoryKind kind);
u64  kl_cxpu_instance_get_mem_limit(kl_cxpu_instance_t *instance, XPUMemoryKind kind);
void kl_cxpu_instance_add_mem_used(kl_cxpu_instance_t *instance, XPUMemoryKind kind, u64 page_cnt);
void kl_cxpu_instance_sub_mem_used(kl_cxpu_instance_t *instance, XPUMemoryKind kind, u64 page_cnt);
int  kl_cxpu_instance_set_mem_limit_locked(kl_cxpu_instance_t *instance, XPUMemoryKind kind,
                                           u64 limit_bytes);
int  kl_cxpu_instance_check_mem_limit(kl_cxpu_instance_t *instance, XPUMemoryKind kind,
                                      u64 page_needed);
u32  kl_cxpu_instance_get_core_limit(kl_cxpu_instance_t *instance);

#endif
