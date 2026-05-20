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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL_DRV_H
#define BAIDU_XPU_RUNTIME_MODULE_KL_DRV_H

#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include "kl_profiler.h"
#include "kl_util.h"
#include "kl_compat.h"
#include "kl_cxpu.h"
#include "xpu/defs.h"
#include "xpu/version.h"
#include "xpurt_priv/ioctl.h"
#include "xpurt_priv/defs_private.h"

#define DEVICE_NAME "xpu"
#define PROC_ROOT_DIR "xpu"
#define INVALID_DEVFILE_NAME "invalid"

struct kl_inode;
struct kl_device;
struct kl_info;
struct xpu_pd;

enum {
    KL1_MPW = 1,
    KL1,
    KL2_MPW,
    KL2,
};

struct kl_device {
    int  idx;
    char name[XPU_MAX_STRLEN];

    struct pci_dev             *pdev;
    const struct pci_device_id *ident;
    int                         bars_en;
    void __iomem               *bar[PCIE_BAR_NUM];
    struct bar_info             bar_info;

    int domain;
    u8  bus;
    u8  slot;
    u8  func;
    u64 sn;
    u32 product_num;
    int probe_errno;

    u64                  prof_cost[PROFILER_COUNT];
    u32                  prof_count[PROFILER_COUNT];
    int                  profiling_enabled;
    struct profiler_stat profiler[PROFILER_COUNT];

    struct kl_device *pf;
    u16               num_vfs;
    u16               vf_id;

    const struct kl_info *info;
    void                 *data;

    // cache for query_device_proc_info
    struct mutex                xdprocs_lock;
    struct xpu_device_processes xdprocs;

    // cache for query_device_info
    struct mutex           xdi_lock;
    struct xpu_device_info xdi;

    // cxpu
    kl_cxpu_t cxpu;
};

// describe a kunlun device's system file node
struct kl_inode {
    int minor;
    int devfile_id;

    struct device *device;
    char           name[XPU_MAX_STRLEN];
    // this inode's fops
    struct file_operations *fops;
    // this inode's private data
    // for kl1, this is struct xpu_pd, for kl2, this is struct kl2_device
    // it is fops handlers' charge to convert this data
    void *data;

    struct kl_device *kdev;
};

// describe how to probe and remove a kunlun device, this info
// is combined with pci id <vendor, device, subvendor, subdevice>
struct kl_info {
    int         kl_code;
    const char *canonical_name;
    const char *device_name;
    int (*probe)(struct kl_device *kdev);
    int (*remove)(struct kl_device *kdev);
    int (*sriov_configure)(struct kl_device *kdev, int num_vfs);
    int (*query_device_info_v1)(struct kl_device *kdev, union xpu_device_info_v1 *i);
    int (*query_device_proc_info)(struct kl_device *kdev, struct ioc_qproc_info_in *i,
                                  struct xpu_device_processes *dp);
};

struct kl_device *get_kdev_by_devfile_id(int devfile_id);
struct kl_device *get_kdev_by_id(int id);
int  kl_create_device(struct device *dev, int start, int count, struct file_operations *fops,
                      void *data, struct kl_device *kdev);
int  kl_destroy_device(int minor, int count);
int  kl_soc_register(void);
void kl_soc_unregister(void);
void kl_init_profiler(struct kl_device *kl_dev);

extern struct proc_dir_entry   *g_proc_root;
extern struct class            *g_class;
extern int                      g_major;
extern struct workqueue_struct *g_kunlun_wq;
extern struct kl_device         g_devs[MAX_DEVICE_NUM];
extern int                      g_devs_count;
extern struct kl_inode          g_devinodes[MAX_DEVINODE_NUM];
extern unsigned long           *g_devinodes_bitmap;
extern unsigned long           *g_devfile_ids_bitmap;
extern int                      g_devfile_id_2_devinode[MAX_DEVINODE_NUM];
extern u64                      g_driver_load_time;

static inline struct kl_inode *inode_to_kinode(struct inode *inode)
{
    int minor = MINOR(inode->i_rdev);
    return &g_devinodes[minor];
}

struct kl_wars {
    int add_tiny_mem_read_after_d2h_war;
};

extern struct kl_wars g_kl_wars;

int kl_init_war(void);

#endif
