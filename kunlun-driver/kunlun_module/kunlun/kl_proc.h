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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL_PROC_H
#define BAIDU_XPU_RUNTIME_MODULE_KL_PROC_H

#include <linux/proc_fs.h>
#include <linux/version.h>

struct kl_proc_entry {
    const char *name;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
    const struct proc_ops fops;
#else
    const struct file_operations fops;
#endif
};

struct proc_dir_entry *proc_entries_create(struct proc_dir_entry *proc_parent,
                                           const char *proc_name, struct kl_proc_entry *dir_entries,
                                           size_t dir_entry_cnt, void *data);

void proc_entries_destroy(struct proc_dir_entry *proc_parent, const char *proc_name,
                          struct proc_dir_entry *proc_entry, struct kl_proc_entry *dir_entries,
                          size_t dir_entry_cnt);

struct proc_dir_entry *kl_proc_create(void);

void kl_proc_destroy(struct proc_dir_entry *proc_root);

//
// Driver proc entry helper functions
// drv procs are files locates in /proc/xpu
//
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
#define XPU_RO_DRVPROC_ENTRY(pname)                                                                \
    {                                                                                              \
        .name = "" #pname, .fops = {                                                               \
            .proc_open    = xpu_drvproc_##pname##_open,                                            \
            .proc_read    = seq_read,                                                              \
            .proc_release = single_release,                                                        \
        }                                                                                          \
    }

#define XPU_RW_DRVPROC_ENTRY(pname)                                                                \
    {                                                                                              \
        .name = "" #pname, .fops = {                                                               \
            .proc_open    = xpu_drvproc_##pname##_open,                                            \
            .proc_read    = seq_read,                                                              \
            .proc_write   = xpu_drvproc_##pname##_write,                                           \
            .proc_release = single_release,                                                        \
        }                                                                                          \
    }
#else
#define XPU_RO_DRVPROC_ENTRY(pname)                                                                \
    {                                                                                              \
        .name = "" #pname, .fops = {                                                               \
            .owner   = THIS_MODULE,                                                                \
            .open    = xpu_drvproc_##pname##_open,                                                 \
            .read    = seq_read,                                                                   \
            .release = single_release,                                                             \
        }                                                                                          \
    }

#define XPU_RW_DRVPROC_ENTRY(pname)                                                                \
    {                                                                                              \
        .name = "" #pname, .fops = {                                                               \
            .owner   = THIS_MODULE,                                                                \
            .open    = xpu_drvproc_##pname##_open,                                                 \
            .read    = seq_read,                                                                   \
            .write   = xpu_drvproc_##pname##_write,                                                \
            .release = single_release,                                                             \
        }                                                                                          \
    }
#endif

#define DEFINE_DRVPROC_WR(procname)                                                                \
    static ssize_t xpu_drvproc_##procname##_write(struct file *file, const char __user *buffer,    \
                                                  size_t count, loff_t *pos)

#define DEFINE_DRVPROC_SHOW(procname)                                                              \
    static int xpu_drvproc_##procname##_show(struct seq_file *m, void *v);                         \
    static int xpu_drvproc_##procname##_open(struct inode *inode, struct file *file)               \
    {                                                                                              \
        return single_open(file, xpu_drvproc_##procname##_show, NV_PDE_DATA(inode));               \
    }                                                                                              \
    static int xpu_drvproc_##procname##_show(struct seq_file *m, void *v)

//
// Device proc entry helper functions
// dev procs are files locates in /proc/xpu/dev{N}
//
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
#define XPU_RO_PROC_ENTRY(pname)                                                                   \
    {                                                                                              \
        .name = "" #pname, .fops = {                                                               \
            .proc_open    = xpu_proc_##pname##_open,                                               \
            .proc_read    = seq_read,                                                              \
            .proc_release = single_release,                                                        \
        }                                                                                          \
    }

#define XPU_RW_PROC_ENTRY(pname)                                                                   \
    {                                                                                              \
        .name = "" #pname, .fops = {                                                               \
            .proc_open    = xpu_proc_##pname##_open,                                               \
            .proc_read    = seq_read,                                                              \
            .proc_write   = xpu_proc_##pname##_write,                                              \
            .proc_release = single_release,                                                        \
        }                                                                                          \
    }
#else
#define XPU_RO_PROC_ENTRY(pname)                                                                   \
    {                                                                                              \
        .name = "" #pname, .fops = {                                                               \
            .owner   = THIS_MODULE,                                                                \
            .open    = xpu_proc_##pname##_open,                                                    \
            .read    = seq_read,                                                                   \
            .release = single_release,                                                             \
        }                                                                                          \
    }

#define XPU_RW_PROC_ENTRY(pname)                                                                   \
    {                                                                                              \
        .name = "" #pname, .fops = {                                                               \
            .owner   = THIS_MODULE,                                                                \
            .open    = xpu_proc_##pname##_open,                                                    \
            .read    = seq_read,                                                                   \
            .write   = xpu_proc_##pname##_write,                                                   \
            .release = single_release,                                                             \
        }                                                                                          \
    }
#endif

#define DEFINE_DEVPROC_SHOW(procname)                                                              \
    static int xpu_proc_##procname##_show(struct seq_file *m, void *v);                            \
    static int xpu_proc_##procname##_open(struct inode *inode, struct file *file)                  \
    {                                                                                              \
        return single_open(file, xpu_proc_##procname##_show, NV_PDE_DATA(inode));                  \
    }                                                                                              \
    static int xpu_proc_##procname##_show(struct seq_file *m, void *v)

#define DEFINE_DEVPROC_WR(procname)                                                                \
    static ssize_t xpu_proc_##procname##_write(struct file *file, const char __user *buffer,       \
                                               size_t count, loff_t *pos)

#endif //BAIDU_XPU_RUNTIME_MODULE_KL_PROC_H
