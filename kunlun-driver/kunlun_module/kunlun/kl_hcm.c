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

#include "kl_hcm.h"
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include "kl_drv.h"
#include "kl_proc.h"

#define HCM_PROC_DIRNAME "hcm"

static struct proc_dir_entry *hcm_proc_root;
static int                    enable;
static unsigned long          pa;
static unsigned long          size;
static struct mutex           init_lock;

void hcm_init(void)
{
    enable = 0;
    pa     = 0x100000000ul; // 4G
    size   = 0x80000000ul;  // 2G
    mutex_init(&init_lock);
}

static int __hcm_enable_locked(void)
{
    int                         i;
    int err                     __maybe_unused;
    struct iommu_domain *domain __maybe_unused;

    if (enable)
        return 0;

    for (i = 0; i < g_devs_count; ++i) {
#ifdef NV_IOMMU_GET_DOMAIN_FOR_DEV_PRESENT
        domain = iommu_get_domain_for_dev(&g_devs[i].pdev->dev);
        if (!domain) {
            // intel_iommu=off
            LOGI("dev%d: iommu_domain= NULL, pls make sure phys mem 2G$4G is reserved, skip settings\n",
                 i);
            continue;
        }
        if (domain->type == IOMMU_DOMAIN_IDENTITY) {
            // intel_iommu=on iommu=pt
            LOGI("dev%d: iommu_domain->type= IOMMU_DOMAIN_IDENTITY, pls make sure phys mem 2G$4G is reserved, skip settings\n",
                 i);
            continue;
        }

        // intel_iommu=on iommu=nopt
#ifdef NV_IOMMU_MAP_HAS_GFP_ARG
        err = iommu_map(domain, pa, pa, size, IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
#else
        err = iommu_map(domain, pa, pa, size, IOMMU_READ | IOMMU_WRITE);
#endif
        LOGI("dev%d: iommu_map, pa= %lx, size= %lx, iova= %lx\n", i, pa, size, pa);
        if (err == -EINVAL) {
            LOGI("dev%d: iommu_map= %d, maybe in passthrough domain, continue anyway\n", i, err);
        } else if (err) {
            LOGW("dev%d: iommu_map= %d, abort\n", i, err);
            goto err_out;
        }
#else  /* NV_IOMMU_GET_DOMAIN_FOR_DEV_PRESENT */
        LOGI("dev%d: pre-v4.1 kernel detected, pls make sure phys mem 2G$4G is reserved, skip settings\n",
             i);
#endif /* NV_IOMMU_GET_DOMAIN_FOR_DEV_PRESENT */
    }

    enable = 1;
    return 0;

#ifdef NV_IOMMU_GET_DOMAIN_FOR_DEV_PRESENT
err_out:
    for (i = i - 1; i >= 0; --i) {
        domain = iommu_get_domain_for_dev(&g_devs[i].pdev->dev);
        if (!domain) {
            continue;
        }
        iommu_unmap(domain, pa, size);
        LOGI("dev%d: iommu_unmap\n", i);
    }

    return err;
#endif /* NV_IOMMU_GET_DOMAIN_FOR_DEV_PRESENT */
}

static int hcm_enable(void)
{
    int err;

    LOGD("try to enable hcm, pa= %lx, size= %lx, iova= %lx\n", pa, size, pa);
    mutex_lock(&init_lock);
    err = __hcm_enable_locked();
    mutex_unlock(&init_lock);
    return err;
}

void hcm_disable(void)
{
    int                         i;
    struct iommu_domain *domain __maybe_unused;

    LOGD("disable hcm\n");
    mutex_lock(&init_lock);
    if (enable) {
        for (i = 0; i < g_devs_count; ++i) {
#ifdef NV_IOMMU_GET_DOMAIN_FOR_DEV_PRESENT
            domain = iommu_get_domain_for_dev(&g_devs[i].pdev->dev);
            if (!domain) {
                continue;
            }
            iommu_unmap(domain, pa, size);
            LOGI("dev%d: iommu_unmap\n", i);
#endif /* NV_IOMMU_GET_DOMAIN_FOR_DEV_PRESENT */
        }
        enable = 0;
    }
    mutex_unlock(&init_lock);
}

/**
 * enable
 */
DEFINE_DEVPROC_SHOW(enable)
{
    seq_printf(m, "%d\n", enable);
    return 0;
}

DEFINE_DEVPROC_WR(enable)
{
    char buf[2] = { 0 };
    int  rc;

    // 2: 0/1 w/ \0
    if (count > 2)
        return -EINVAL;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    if (buf[0] == '0') {
        // disable
        // cannot disable dynamically
        return count;
    }

    // enable
    rc = hcm_enable();
    if (rc)
        return rc;

    return count;
}

/**
 * pa
 */
DEFINE_DEVPROC_SHOW(pa)
{
    seq_printf(m, "0x%lx\n", pa);
    return 0;
}

DEFINE_DEVPROC_WR(pa)
{
    char buf[32] = { 0 };
    int  rc;

    // 21: (~0ull) in dec, with \0
    if (count > 21)
        return -EINVAL;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    if (strncmp(buf, "0x", 2) == 0) {
        rc = kstrtoul(buf, 16, &pa);
    } else {
        rc = kstrtoul(buf, 10, &pa);
    }
    if (rc)
        return rc;

    return count;
}

/**
 * size
 */
DEFINE_DEVPROC_SHOW(size)
{
    seq_printf(m, "0x%lx\n", size);
    return 0;
}

DEFINE_DEVPROC_WR(size)
{
    char buf[32] = { 0 };
    int  rc;

    // 21: (~0ull) in dec, with \0
    if (count > 21)
        return -EINVAL;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    if (strncmp(buf, "0x", 2) == 0) {
        rc = kstrtoul(buf, 16, &size);
    } else {
        rc = kstrtoul(buf, 10, &size);
    }
    if (rc)
        return rc;

    return count;
}

/**
 * status
 */
DEFINE_DEVPROC_SHOW(status)
{
    return 0;
}

static struct kl_proc_entry hcm_proc_entries[] = {
    XPU_RW_PROC_ENTRY(enable),
    XPU_RW_PROC_ENTRY(pa),
    XPU_RW_PROC_ENTRY(size),
    XPU_RO_PROC_ENTRY(status),
};

int hcm_proc_create(struct proc_dir_entry *proc_root)
{
    hcm_proc_root = proc_entries_create(proc_root, HCM_PROC_DIRNAME, hcm_proc_entries,
                                        ARRAY_SIZE(hcm_proc_entries), NULL);
    if (hcm_proc_root == NULL) {
        return -EINVAL;
    }

    return 0;
}

void hcm_proc_destroy(struct proc_dir_entry *proc_root)
{
    proc_entries_destroy(proc_root, HCM_PROC_DIRNAME, hcm_proc_root, hcm_proc_entries,
                         ARRAY_SIZE(hcm_proc_entries));
}
