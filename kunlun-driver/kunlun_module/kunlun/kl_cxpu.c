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

#include "kl_cxpu.h"
#include "kl_mm.h"
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/cgroup.h>
#include <linux/slab.h>
#include <linux/seq_file.h>

static inline struct kl_memory *__mk2md(struct kl_mm *mm, XPUMemoryKind kind)
{
    int idx;

    if (!mm)
        return NULL;

    for (idx = 0; idx < mm->mem_count; ++idx) {
        if (mm->mem[idx].kind == kind)
            return &(mm->mem[idx]);
    }
    return NULL;
}

static inline u64 __bytes2pgcnt(struct kl_memory *mem, u64 bytes)
{
    // overflow
    if ((bytes - 1 + mem->page_size) < bytes)
        return CXPU_INSTANCE_MEM_UNLIMIT;

    return (u64)((bytes + mem->page_size - 1) >> mem->page_bits);
}

static int __exceed_limit(u64 used, u64 needed, u64 limit)
{
    return ((used + needed) > limit);
}

static u64 __actmem_to_showmem(int ecc_on, XPUMemoryKind kind, u64 act_mem)
{
    if (ecc_on && kind == XPU_MEM_MAIN) {
        return act_mem * 8 / 7;
    } else {
        return act_mem;
    }
}

static u64 __showmem_to_actmem(int ecc_on, XPUMemoryKind kind, u64 show_mem)
{
    if (ecc_on && kind == XPU_MEM_MAIN) {
        return DIV_ROUND_UP(show_mem * 7, 8);
    } else {
        return show_mem;
    }
}

static void kl_cxpu_destroy_instance_ref(struct kref *kref)
{
    kl_cxpu_instance_t *instance = container_of((struct xref *)kref, kl_cxpu_instance_t, xref);
    kl_cxpu_t          *cxpu     = instance->cxpu;

    list_del(&instance->ent);
    cxpu->instance_cnt--;

    if (instance->core_limit != CXPU_INSTANCE_CORE_UNLIMIT)
        cxpu->assigned_core -= instance->core_limit;

    if (instance->mem_limit_pgcnt[XPU_MEM_MAIN] != CXPU_INSTANCE_MEM_UNLIMIT)
        cxpu->main_mem.page_used -= instance->mem_limit_pgcnt[XPU_MEM_MAIN];

    if (instance->mem_limit_pgcnt[XPU_MEM_L3] != CXPU_INSTANCE_MEM_UNLIMIT)
        cxpu->l3_mem.page_used -= instance->mem_limit_pgcnt[XPU_MEM_L3];

    kfree(instance);

    return;
}

static int kl_cxpu_instance_get_cgroup_refcount(kl_cxpu_instance_t *instance)
{
    struct css_set *cgroups;

    if (!instance) {
        return 0;
    }

    cgroups = (struct css_set *)instance->token;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    return refcount_read(&cgroups->refcount);
#else
    return atomic_read(&cgroups->refcount);
#endif
}

static u64 kl_cxpu_get_mainmem_cnt_all(kl_cxpu_t *cxpu)
{
    u64 mm_page_cnt_all;

    u64 page_sz_main = kl_mm_get_pgsz(cxpu->mm, XPU_MEM_MAIN);
    u64 mm_bytes_all = kl_mm_get_bytes_all(cxpu->mm, XPU_MEM_MAIN) +
                       kl_mm_get_bytes_all(cxpu->mm, XPU_MEM_CODE) +
                       kl_mm_get_bytes_all(cxpu->mm, XPU_MEM_PARAM) +
                       kl_mm_get_bytes_all(cxpu->mm, XPU_MEM_PRINTF) +
                       kl_mm_get_bytes_all(cxpu->mm, XPU_MEM_RESERVED);
    mm_page_cnt_all = mm_bytes_all / page_sz_main;

    return mm_page_cnt_all;
}

/*
 * Example:
 *
 * cxpu info:
 * cxpu instance_cnt[2], core_assigned[50/100], l3_page_assigned[512/16383], l3_page_size[4096], mm_page_assigned[512/524288], mm_page_size[32768]
 * current user token=ffff8d28fc66c800
 * cxpu instance id=containerId0, token=ffff8d28ecedcc00, refcnt=1, core_limit=25, l3_page_limit=256, l3_page_used=0, mm_page_limit=256, mm_page_used=0
 * cxpu instance id=containerId1, token=ffff8d28fc66c800, refcnt=1, core_limit=25, l3_page_limit=256, l3_page_used=0, mm_page_limit=256, mm_page_used=0
 */
int kl_cxpu_proc_show(kl_cxpu_t *cxpu, struct seq_file *m)
{
    kl_cxpu_instance_t *instance;

    seq_printf(m, "cxpu info:\n");
    mutex_lock(&cxpu->instance_list_lock);
    seq_printf(
            m,
            "cxpu instance_cnt[%d], core_assigned[%d/%d], l3_page_assigned[%lld/%lld], l3_page_size[%d], mm_page_assigned[%lld/%lld], mm_page_size[%d]\n",
            cxpu->instance_cnt, cxpu->assigned_core, cxpu->total_core,
            __actmem_to_showmem(cxpu->ecc_on, XPU_MEM_L3, cxpu->l3_mem.page_used),
            __actmem_to_showmem(cxpu->ecc_on, XPU_MEM_L3, cxpu->l3_mem.page_cnt),
            kl_mm_get_pgsz(cxpu->mm, XPU_MEM_L3),
            __actmem_to_showmem(cxpu->ecc_on, XPU_MEM_MAIN, cxpu->main_mem.page_used),
            __actmem_to_showmem(cxpu->ecc_on, XPU_MEM_MAIN, cxpu->main_mem.page_cnt),
            kl_mm_get_pgsz(cxpu->mm, XPU_MEM_MAIN));
    if (cxpu->instance_cnt > 0) {
        seq_printf(m, "current user token=%llx\n", (u64)current->cgroups);
        list_for_each_entry(instance, &cxpu->instance_list, ent) {
            seq_printf(
                    m,
                    "cxpu instance id=%s, token=%llx, refcnt=%d, core_limit=%u, l3_page_limit=%llu, l3_page_used=%llu, mm_page_limit=%llu, mm_page_used=%llu\n",
                    instance->instance_id, instance->token, kl_cxpu_instance_get_refcount(instance),
                    kl_cxpu_instance_get_core_limit(instance),
                    kl_cxpu_instance_get_mem_limit(instance, XPU_MEM_L3),
                    kl_cxpu_instance_get_mem_used(instance, XPU_MEM_L3),
                    kl_cxpu_instance_get_mem_limit(instance, XPU_MEM_MAIN),
                    kl_cxpu_instance_get_mem_used(instance, XPU_MEM_MAIN));
        }
    }
    mutex_unlock(&cxpu->instance_list_lock);

    return 0;
}

int kl_cxpu_init(kl_cxpu_t *cxpu)
{
    if (!cxpu)
        return -EINVAL;

    mutex_init(&cxpu->instance_list_lock);
    INIT_LIST_HEAD(&cxpu->instance_list);

    cxpu->instance_cnt     = 0;
    cxpu->instance_max_cnt = 12; //max count is equal to max stream num

    cxpu->total_core    = CXPU_INSTANCE_MAX_CORE_NUM;
    cxpu->assigned_core = 0;

    cxpu->main_mem.page_size = kl_mm_get_pgsz(cxpu->mm, XPU_MEM_MAIN);
    cxpu->main_mem.page_cnt  = kl_cxpu_get_mainmem_cnt_all(cxpu);
    cxpu->main_mem.page_used = 0;

    cxpu->l3_mem.page_size = kl_mm_get_pgsz(cxpu->mm, XPU_MEM_L3);
    cxpu->l3_mem.page_cnt  = kl_mm_get_pg_all(cxpu->mm, XPU_MEM_L3);
    cxpu->l3_mem.page_used = 0;

    cxpu->enabled = true;

    LOGD("CXPU init successfully.\n");

    return 0;
}

int kl_cxpu_uninit(kl_cxpu_t *cxpu)
{
    kl_cxpu_instance_t *instance_safe, *instance;

    if (!cxpu || !cxpu->enabled)
        return -EINVAL;

    cxpu->enabled = false;

    mutex_lock(&cxpu->instance_list_lock);
    list_for_each_entry_safe(instance, instance_safe, &cxpu->instance_list, ent) {
        list_del(&instance->ent);
        kfree(instance);
    }
    mutex_unlock(&cxpu->instance_list_lock);

    return 0;
}

int kl_cxpu_get_instance_count(kl_cxpu_t *cxpu)
{
    return cxpu->instance_cnt;
}

int kl_cxpu_get_max_instance_count(kl_cxpu_t *cxpu)
{
    return cxpu->instance_max_cnt;
}

int kl_cxpu_get_instance_id(kl_cxpu_t *cxpu, int index, char *instance_id)
{
    int                 i = 0;
    kl_cxpu_instance_t *instance;

    if (index >= cxpu->instance_max_cnt)
        return -EINVAL;

    mutex_lock(&cxpu->instance_list_lock);
    list_for_each_entry(instance, &cxpu->instance_list, ent) {
        if (i == index) {
            strncpy(instance_id, instance->instance_id, CXPU_INSTANCE_ID_LEN + 1);
            break;
        }
        i++;
    }
    mutex_unlock(&cxpu->instance_list_lock);

    return 0;
}

kl_cxpu_instance_t *kl_cxpu_get_instance_by_id_locked(kl_cxpu_t *cxpu, char *instance_id)
{
    kl_cxpu_instance_t *instance, *ite;

    if (!cxpu || !cxpu->enabled)
        return NULL;

    instance = NULL;

    mutex_lock(&cxpu->instance_list_lock);
    list_for_each_entry(ite, &cxpu->instance_list, ent) {
        if (!strncmp(ite->instance_id, instance_id, CXPU_INSTANCE_ID_LEN)) {
            instance = ite;
            xref_get(&instance->xref);
            break;
        }
    }
    mutex_unlock(&cxpu->instance_list_lock);

    return instance;
}

kl_cxpu_instance_t *kl_cxpu_get_instance_by_token_locked(kl_cxpu_t *cxpu, u64 token)
{
    kl_cxpu_instance_t *instance, *ite;

    if (!cxpu || !cxpu->enabled)
        return NULL;

    instance = NULL;

    mutex_lock(&cxpu->instance_list_lock);
    list_for_each_entry(ite, &cxpu->instance_list, ent) {
        if (ite->token == token) {
            instance = ite;
            xref_get(&instance->xref);
            break;
        }
    }
    mutex_unlock(&cxpu->instance_list_lock);

    return instance;
}

void kl_cxpu_put_instance_locked(kl_cxpu_t *cxpu, kl_cxpu_instance_t *instance)
{
    if (!cxpu || !cxpu->enabled)
        return;

    mutex_lock(&cxpu->instance_list_lock);
    if (instance)
        xref_put(&instance->xref, kl_cxpu_destroy_instance_ref);
    mutex_unlock(&cxpu->instance_list_lock);
}

int kl_cxpu_create_instance(kl_cxpu_t *cxpu, char *instance_id)
{
    kl_cxpu_instance_t *instance, *ite;
    u64                 token;
    int                 i;

    if (!cxpu || !cxpu->enabled)
        return -EINVAL;

    if (cxpu->instance_cnt >= cxpu->instance_max_cnt) {
        return -ENOMEM;
    }

    token = (u64)(current->cgroups);

    mutex_lock(&cxpu->instance_list_lock);
    list_for_each_entry(ite, &cxpu->instance_list, ent) {
        if (!strncmp(ite->instance_id, instance_id, CXPU_INSTANCE_ID_LEN)) {
            mutex_unlock(&cxpu->instance_list_lock);
            return -EEXIST;
        }
    }

    instance = kzalloc(sizeof(*instance), GFP_KERNEL);
    xref_init(&instance->xref);
    instance->token = token;
    strncpy(instance->instance_id, instance_id, CXPU_INSTANCE_ID_LEN);
    for (i = 0; i < XPU_MEM_COUNT; ++i) {
        atomic64_set(&instance->mem_used_pgcnt[i], 0);
        instance->mem_limit_pgcnt[i] = CXPU_INSTANCE_MEM_UNLIMIT;
    }
    instance->core_limit = CXPU_INSTANCE_CORE_UNLIMIT;
    instance->cxpu       = cxpu;
    mutex_init(&instance->lock);

    list_add_tail(&instance->ent, &cxpu->instance_list);
    cxpu->instance_cnt++;
    mutex_unlock(&cxpu->instance_list_lock);

    LOGD("CXPU create instance, instance_id: %s, token: %llx\n", instance->instance_id, token);

    return 0;
}

int kl_cxpu_destroy_instance(kl_cxpu_t *cxpu, char *instance_id)
{
    kl_cxpu_instance_t *instance_safe, *instance;

    if (!cxpu || !cxpu->enabled)
        return -EINVAL;

    LOGD("CXPU destroy instance, instance id: %s\n", instance_id);

    mutex_lock(&cxpu->instance_list_lock);
    list_for_each_entry_safe(instance, instance_safe, &cxpu->instance_list, ent) {
        if (!strncmp(instance->instance_id, instance_id, CXPU_INSTANCE_ID_LEN)) {
            if (xref_read(&instance->xref) > 1) {
                LOGW("cxpu instance is busy, refcount = %d\n", xref_read(&instance->xref));
                mutex_unlock(&cxpu->instance_list_lock);
                return -EBUSY;
            }
            xref_put(&instance->xref, kl_cxpu_destroy_instance_ref);
            break;
        }
    }
    mutex_unlock(&cxpu->instance_list_lock);

    return 0;
}

int kl_cxpu_set_instance_mem_limit(kl_cxpu_t *cxpu, char *instance_id, XPUMemoryKind kind,
                                   u64 limit_bytes)
{
    int                 ret;
    kl_cxpu_instance_t *instance;

    if (!cxpu) {
        return -EINVAL;
    }

    instance = kl_cxpu_get_instance_by_id_locked(cxpu, instance_id);
    ret      = kl_cxpu_instance_set_mem_limit_locked(instance, kind, limit_bytes);
    kl_cxpu_put_instance_locked(cxpu, instance);

    return ret;
}

int kl_cxpu_get_memory_info(kl_cxpu_t *cxpu, XPUMemoryKind mem_kind, kl_cxpu_mem_info_t *mem_info)
{
    kl_cxpu_mem_info_t *cxpu_mem;

    if (!cxpu || !mem_info)
        return -EINVAL;

    if (mem_kind == XPU_MEM_MAIN) {
        cxpu_mem = &cxpu->main_mem;
    } else if (mem_kind == XPU_MEM_L3) {
        cxpu_mem = &cxpu->l3_mem;
    } else {
        return -EINVAL;
    }

    mem_info->page_size = cxpu_mem->page_size;
    mem_info->page_used = __actmem_to_showmem(cxpu->ecc_on, mem_kind, cxpu_mem->page_used);
    mem_info->page_cnt  = __actmem_to_showmem(cxpu->ecc_on, mem_kind, cxpu_mem->page_cnt);

    return 0;
}

int kl_cxpu_get_instance_memory_info(kl_cxpu_t *cxpu, char *instance_id, XPUMemoryKind mem_kind,
                                     kl_cxpu_mem_info_t *mem_info)
{
    kl_cxpu_instance_t *instance;

    if (!cxpu || !mem_info)
        return -EINVAL;

    instance = kl_cxpu_get_instance_by_id_locked(cxpu, instance_id);
    if (instance) {
        mem_info->page_size = kl_mm_get_pgsz(cxpu->mm, mem_kind);
        mem_info->page_used = kl_cxpu_instance_get_mem_used(instance, mem_kind);
        mem_info->page_cnt  = kl_cxpu_instance_get_mem_limit(instance, mem_kind);
    } else {
        return -EINVAL;
    }
    kl_cxpu_put_instance_locked(cxpu, instance);

    return 0;
}

u64 kl_cxpu_instance_get_token(kl_cxpu_instance_t *instance)
{
    if (!instance)
        return 0;

    return instance->token;
}

int kl_cxpu_instance_get_refcount(kl_cxpu_instance_t *instance)
{
    if (!instance) {
        return -EINVAL;
    }

    return xref_read(&instance->xref);
}

bool kl_cxpu_instance_is_mem_limit_on(kl_cxpu_instance_t *instance, XPUMemoryKind kind)
{
    if (!instance) {
        return false;
    }

    return (instance->mem_limit_pgcnt[kind] != CXPU_INSTANCE_MEM_UNLIMIT);
}

u64 kl_cxpu_instance_get_mem_used(kl_cxpu_instance_t *instance, XPUMemoryKind kind)
{
    u64 mem_pgcnt;

    if (!instance) {
        return 0;
    }

    mem_pgcnt = atomic64_read(&instance->mem_used_pgcnt[kind]);

    return __actmem_to_showmem(instance->cxpu->ecc_on, kind, mem_pgcnt);
}

u64 kl_cxpu_instance_get_mem_limit(kl_cxpu_instance_t *instance, XPUMemoryKind kind)
{
    u64 mem_pgcnt;

    if (!instance) {
        return CXPU_INSTANCE_MEM_UNLIMIT;
    }

    mem_pgcnt = instance->mem_limit_pgcnt[kind];

    return __actmem_to_showmem(instance->cxpu->ecc_on, kind, mem_pgcnt);
}

void kl_cxpu_instance_add_mem_used(kl_cxpu_instance_t *instance, XPUMemoryKind kind, u64 page_cnt)
{
    if (!instance) {
        return;
    }

    atomic64_add(page_cnt, &instance->mem_used_pgcnt[kind]);
}

void kl_cxpu_instance_sub_mem_used(kl_cxpu_instance_t *instance, XPUMemoryKind kind, u64 page_cnt)
{
    if (!instance) {
        return;
    }

    atomic64_sub(page_cnt, &instance->mem_used_pgcnt[kind]);
}

int kl_cxpu_instance_set_mem_limit_locked(kl_cxpu_instance_t *instance, XPUMemoryKind kind,
                                          u64 limit_bytes)
{
    struct kl_memory   *mem;
    kl_cxpu_mem_info_t *cxpu_mem;
    u64                 mm_page_cnt_all;
    u64                 limit_pgcnt;
    u64                 last_limit_pgcnt;
    u64                 available_pgcnt;
    kl_cxpu_t          *cxpu;

    if (!instance)
        return -EINVAL;

    LOGD("set cxpu mem limit, instance_id: %s\n", (char *)&instance->instance_id);
    cxpu = instance->cxpu;

    mem = __mk2md(cxpu->mm, kind);
    if (!mem) {
        return -EINVAL;
    }

    if (kind == XPU_MEM_MAIN) {
        mm_page_cnt_all = kl_cxpu_get_mainmem_cnt_all(cxpu);
        cxpu_mem        = &cxpu->main_mem;
    } else if (kind == XPU_MEM_L3) {
        mm_page_cnt_all = kl_mm_get_pg_all(cxpu->mm, kind);
        cxpu_mem        = &cxpu->l3_mem;
    } else {
        return -EINVAL;
    }

    if (instance->mem_limit_pgcnt[kind] == CXPU_INSTANCE_MEM_UNLIMIT)
        last_limit_pgcnt = 0;
    else
        last_limit_pgcnt = instance->mem_limit_pgcnt[kind];

    mutex_lock(&instance->lock);
    if (limit_bytes == CXPU_INSTANCE_MEM_UNLIMIT) {
        cxpu_mem->page_used -= last_limit_pgcnt;
        instance->mem_limit_pgcnt[kind] = CXPU_INSTANCE_MEM_UNLIMIT;
        mutex_unlock(&instance->lock);
        return 0;
    }

    limit_pgcnt = __bytes2pgcnt(mem, limit_bytes);
    if (limit_pgcnt == CXPU_INSTANCE_MEM_UNLIMIT) {
        // limit_bytes overflow
        mutex_unlock(&instance->lock);
        return -EINVAL;
    }

    limit_pgcnt = __showmem_to_actmem(cxpu->ecc_on, kind, limit_pgcnt);

    available_pgcnt = mm_page_cnt_all + last_limit_pgcnt - cxpu_mem->page_used;
    if (limit_pgcnt <= available_pgcnt) {
        cxpu_mem->page_used -= last_limit_pgcnt;
        cxpu_mem->page_used += limit_pgcnt;
    } else {
        LOGW("have no enough pages for instance %s, required pages=%lld, available pages=%lld\n",
             instance->instance_id, __actmem_to_showmem(cxpu->ecc_on, kind, limit_pgcnt),
             __actmem_to_showmem(cxpu->ecc_on, kind, available_pgcnt));
        mutex_unlock(&instance->lock);
        return -ENOMEM;
    }

    instance->mem_limit_pgcnt[kind] = limit_pgcnt;
    mutex_unlock(&instance->lock);

    return 0;
}

int kl_cxpu_instance_check_mem_limit(kl_cxpu_instance_t *instance, XPUMemoryKind kind,
                                     u64 page_needed)
{
    if (!instance) {
        return 0;
    }

    if (__exceed_limit(atomic64_read(&instance->mem_used_pgcnt[kind]), page_needed,
                       instance->mem_limit_pgcnt[kind]))
        return -XPUERR_NOMEM;

    return 0;
}

u32 kl_cxpu_instance_get_core_limit(struct kl_cxpu_instance *instance)
{
    if (!instance) {
        return CXPU_INSTANCE_CORE_UNLIMIT;
    }

    return instance->core_limit;
}
