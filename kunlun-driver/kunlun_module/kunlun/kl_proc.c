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

#include "kl_proc.h"
#include <linux/version.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "kl_drv.h"
#include "kl_hcm.h"

/*
    mkdir dir and entries under proc_parent:

    params:
        proc_parent : parent dir entry
        proc_name   : current dir name
        dir_entries : files in current dir
        data        : private data
    return:
        proc_entry  : current dir entry

*/
struct proc_dir_entry *proc_entries_create(struct proc_dir_entry *proc_parent,
                                           const char *proc_name, struct kl_proc_entry *dir_entries,
                                           size_t dir_entry_cnt, void *data)
{
    int                    i          = 0;
    int                    n          = dir_entry_cnt;
    struct proc_dir_entry *proc_entry = NULL;

    proc_entry = proc_mkdir(proc_name, proc_parent);
    if (proc_entry == NULL) {
        LOGE("create proc dir %s error!\n", proc_name);
        return NULL;
    }

    for (i = 0; i < n; i++) {
        const struct kl_proc_entry *entry = &dir_entries[i];
        if (proc_create_data(entry->name, 0666, proc_entry, &entry->fops, data) == NULL) {
            LOGE("create entry %s/%s error!\n", proc_name, entry->name);
            goto err_proc_entry;
        }
    }

    return proc_entry;

err_proc_entry:
    for (--i; i >= 0; --i) {
        remove_proc_entry(dir_entries[i].name, proc_entry);
    }

    remove_proc_entry(proc_name, proc_parent);

    return NULL;
}

/*
 *   remove dir_entry under proc_entry
 */
void proc_entries_destroy(struct proc_dir_entry *proc_parent, const char *proc_name,
                          struct proc_dir_entry *proc_entry, struct kl_proc_entry *dir_entries,
                          size_t dir_entry_cnt)
{
    int i = 0;
    int n = dir_entry_cnt;

    if (!proc_entry) {
        LOGE("[%s] invalid proc_entry to remove\n", proc_name);
        return;
    }

    for (i = 0; i < n; i++) {
        remove_proc_entry(dir_entries[i].name, proc_entry);
    }

    remove_proc_entry(proc_name, proc_parent);
}

/*
    /proc/xpu/  entries
*/

DEFINE_DRVPROC_SHOW(load_time)
{
    seq_printf(m, "jiffies: %llu\n", g_driver_load_time);
    return 0;
}

DEFINE_DRVPROC_SHOW(probe_error)
{
    int i       = 0;
    int err_cnt = 0;
    for (i = 0; i < MAX_DEVICE_NUM; ++i) {
        struct kl_device *kdev = &g_devs[i];
        if (kdev->probe_errno == 0)
            continue;

        seq_printf(m, "%04x:%02x:%02x.%x %llx %d %s %d %s\n", kdev->domain, kdev->bus, kdev->slot,
                   kdev->func, kdev->sn, kdev->idx,
                   kdev->info ? kdev->info->canonical_name : "(nil)", kdev->probe_errno,
                   xpu_strerror(kdev->probe_errno));
        err_cnt++;
    }

    if (err_cnt == 0)
        seq_printf(m, "\n");

    return 0;
}

DEFINE_DRVPROC_SHOW(version)
{
    seq_printf(m, "%u.%u.%u\n", XPURT_VERSION_MAJOR, XPURT_VERSION_MINOR, XPURT_VERSION_THIRD);
    return 0;
}

static struct kl_proc_entry kl_proc_entries[] = {
    XPU_RO_DRVPROC_ENTRY(load_time),
    XPU_RO_DRVPROC_ENTRY(probe_error),
    XPU_RO_DRVPROC_ENTRY(version),

};

extern int  xpu_drvproc_entries_create(void);
extern void xpu_drvproc_entries_destroy(void);

struct proc_dir_entry *kl_proc_create()
{
    struct proc_dir_entry *proc_root;
    proc_root = proc_entries_create(NULL, PROC_ROOT_DIR, kl_proc_entries,
                                    ARRAY_SIZE(kl_proc_entries), NULL);
    if (!proc_root)
        return NULL;

    g_proc_root = proc_root;
    xpu_drvproc_entries_create();
    hcm_proc_create(proc_root);

    return proc_root;
}

void kl_proc_destroy(struct proc_dir_entry *proc_root)
{
    hcm_proc_destroy(proc_root);
    xpu_drvproc_entries_destroy();

    proc_entries_destroy(NULL, PROC_ROOT_DIR, proc_root, kl_proc_entries,
                         ARRAY_SIZE(kl_proc_entries));
}
