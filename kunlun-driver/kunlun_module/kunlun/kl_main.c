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

#include "kl_drv.h"

#include <linux/bitmap.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>

#include "kl_proc.h"
#include "kl_hcm.h"

extern int __init kl1_module_init(void);
extern int __init kl2_module_post(void);

extern int  ctrldev_create(unsigned int major, unsigned int minor);
extern void ctrldev_destroy(void);

extern struct file_operations kl_fops;

extern const struct kl_info  kl1_info;
extern const struct kl_info  kl2_info;
static const struct kl_info *kl_info_table[] = {
    [0] = NULL, [KL1_MPW] = NULL, [KL1] = &kl1_info, [KL2_MPW] = NULL, [KL2] = &kl2_info,
};

// proc root for kunlun device /proc/xpu
struct proc_dir_entry *g_proc_root;
// linux device class for kunlun device
struct class *g_class;
// kunlun cdev number
dev_t g_devno;
// kunlun cdev major number
int g_major;
// linux cdev for kunlun device (not for control node)
struct cdev g_cdev;
// 全局workqueue，用于一些非紧急delayed work处理，如升降频率打印
struct workqueue_struct *g_kunlun_wq;

char g_devs_bitmap_char[(MAX_DEVICE_NUM + 7) >> 3]          = { 0 };
char g_devinodes_bitmap_char[(MAX_DEVINODE_NUM + 7) >> 3]   = { 0 };
char g_devfile_ids_bitmap_char[(MAX_DEVINODE_NUM + 7) >> 3] = { 0 };

// list of kunlun devices probed
struct kl_device g_devs[MAX_DEVICE_NUM] = { { 0 } };
// list of device file inode (/dev/xpu*) created
struct kl_inode g_devinodes[MAX_DEVINODE_NUM] = { { 0 } };

// TODO(miaotianxiang): 并发控制，加锁保护？
// number of kunlun devices probed
int g_devs_count = 0;
// bitmap to record device slots that are occupied
unsigned long *g_devs_bitmap = (unsigned long *)g_devs_bitmap_char;
// bitmap to record inode minor numbers that are occupied
unsigned long *g_devinodes_bitmap   = (unsigned long *)g_devinodes_bitmap_char;
unsigned long *g_devfile_ids_bitmap = (unsigned long *)g_devfile_ids_bitmap_char;
int            g_devfile_id_2_devinode[MAX_DEVINODE_NUM];
// error number used in device probe
int g_errno = 0;

u64 g_driver_load_time = 0;

struct kl_wars g_kl_wars = { 0 };

struct kl_device *get_kdev_by_devfile_id(int devfile_id)
{
    int               minor;
    struct kl_inode  *kinode;
    struct kl_device *kdev;

    if (devfile_id < 0 || devfile_id >= MAX_DEVINODE_NUM)
        return NULL;

    minor = g_devfile_id_2_devinode[devfile_id];
    if (minor < 0 || minor >= MAX_DEVINODE_NUM)
        return NULL;

    kinode = &g_devinodes[minor];
    kdev   = kinode->kdev;
    LOGD("devfile_id= %d, minor= %d, kinode= %px, kdev= %px, kdev->name= %s\n", devfile_id, minor,
         kinode, kdev, kdev->name);
    return kdev;
}

// get kdev from g_devs
struct kl_device *get_kdev_by_id(int id)
{
    if (id < 0 || id >= MAX_DEVICE_NUM)
        return NULL;

    return test_bit(id, g_devs_bitmap) ? &(g_devs[id]) : NULL;
}

// TODO(miaotianxiang): 完善后给kl1使用
// create \p count system device files under /dev folder
// the minor numbers of created files are guaranteed to be contineous
// return the first minor number allocated on success, negtive errno on error
int kl_create_device(struct device *dev, int start, int count, struct file_operations *fops,
                     void *data, struct kl_device *kdev)
{
    struct kl_inode *kinode     = NULL;
    int              minor      = 0;
    int              devfile_id = 0;
    int              err        = 0;
    int              i          = 0;

    minor = bitmap_find_next_zero_area(g_devinodes_bitmap, MAX_DEVINODE_NUM, start, count, 0);
    if (minor >= MAX_DEVINODE_NUM) {
        LOGE("no device file minor available\n");
        return -XPUERR_MAXDEV;
    }

    bitmap_set(g_devinodes_bitmap, minor, count);

    // 无需额外检查
    devfile_id = bitmap_find_next_zero_area(g_devfile_ids_bitmap, MAX_DEVINODE_NUM, 0, count, 0);
    BUG_ON(devfile_id >= MAX_DEVINODE_NUM);
    bitmap_set(g_devfile_ids_bitmap, devfile_id, count);

    for (i = 0; i < count; ++i) {
        kinode             = &g_devinodes[minor + i];
        kinode->devfile_id = devfile_id + i;
        snprintf(kinode->name, XPU_MAX_STRLEN, DEVICE_NAME "%d", kinode->devfile_id);
        kinode->fops                                = fops;
        kinode->data                                = data;
        kinode->kdev                                = kdev;
        g_devfile_id_2_devinode[kinode->devfile_id] = minor + i;

        kinode->device = device_create(g_class,
                                       dev,                       // parent
                                       MKDEV(g_major, minor + i), // dev_t
                                       kinode,                    // drvdata
                                       "%s", kinode->name);       // device name
        if (IS_ERR_OR_NULL(kinode->device)) {
            LOGE("device_create failed\n");
            err = -XPUERR_DEVINIT;
            goto err_destroy_dev;
        }
    }

    return minor;

err_destroy_dev:
    while (i >= 0) {
        kinode = &g_devinodes[minor + i];
        if (!IS_ERR_OR_NULL(kinode->device)) {
            device_destroy(g_class, MKDEV(g_major, minor + i));
            kinode->device = NULL;

            g_devfile_id_2_devinode[kinode->devfile_id] = -1;
            kinode->kdev                                = NULL;
            kinode->data                                = NULL;
            kinode->fops                                = NULL;
            strncpy(kinode->name, INVALID_DEVFILE_NAME, XPU_MAX_STRLEN);
            kinode->devfile_id = -1;
        }
        --i;
    }

    bitmap_clear(g_devinodes_bitmap, minor, count);
    bitmap_clear(g_devfile_ids_bitmap, devfile_id, count);

    return err;
}

int kl_destroy_device(int minor, int count)
{
    struct kl_inode *kinode     = NULL;
    int              devfile_id = 0;
    int              i          = 0;

    devfile_id = g_devinodes[minor].devfile_id;
    for (i = 0; i < count; ++i) {
        kinode = &g_devinodes[minor + i];
        if (!IS_ERR_OR_NULL(kinode->device)) {
            device_destroy(g_class, MKDEV(g_major, minor + i));
            kinode->device = NULL;

            g_devfile_id_2_devinode[kinode->devfile_id] = -1;
            kinode->kdev                                = NULL;
            kinode->data                                = NULL;
            kinode->fops                                = NULL;
            strncpy(kinode->name, INVALID_DEVFILE_NAME, XPU_MAX_STRLEN);
            kinode->devfile_id = -1;
        }
    }

    bitmap_clear(g_devinodes_bitmap, minor, count);
    bitmap_clear(g_devfile_ids_bitmap, devfile_id, count);
    return 0;
}

#ifdef NV_CLASS_DEV_UEVENT_HAS_CONST_DEV_ARGS
int kl_dev_uevent(const struct device *dev, struct kobj_uevent_env *env)
#else  /* NV_CLASS_DEV_UEVENT_HAS_CONST_DEV_ARGS */
int kl_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
#endif /* NV_CLASS_DEV_UEVENT_HAS_CONST_DEV_ARGS */
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

void kl_init_profiler(struct kl_device *kl_dev)
{
    int i = 0;

    for (i = 0; i < PROFILER_COUNT; ++i) {
        kl_dev->prof_cost[i]      = 0;
        kl_dev->prof_count[i]     = 0;
        kl_dev->profiler[i].count = 0;
        kl_dev->profiler[i].cost  = 0;
        kl_dev->profiler[i].name  = NULL;
    }
    kl_dev->profiling_enabled = 1;
}

#ifndef __devinit
#define __devinit
#endif
static int __devinit kl_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    u32 domain = pci_domain_nr(pdev->bus);
    u32 bus    = pdev->bus->number;
    u32 slot   = PCI_SLOT(pdev->devfn);
    u32 func   = PCI_FUNC(pdev->devfn);

    const struct kl_info *kinfo = kl_info_table[ent->driver_data];
    struct kl_device     *kdev  = NULL;
    int                   dev_idx;
    int                   bar_idx;
    int                   err;
    u16                   subsys_vendor;

    // 检查PCI配置空间，如遇非法情况尽早报错return
    err = pci_read_config_word(pdev, 0x2c /* Subsystem Vendor ID */, &subsys_vendor);
    if (err || subsys_vendor > 0x1000u) {
        LOGE("device [%04x:%02x:%02x.%x] not accessible, subsys_vendor= %04x, are you sure?\n",
             domain, bus, slot, func, subsys_vendor);
        return -XPUERR_INVALID_DEVICE;
    }

    if (!kinfo) {
        LOGI("device [%04x:%02x:%02x.%x] not supported, are you sure?\n", domain, bus, slot, func);
        return -XPUERR_INVALID_DEVICE;
    }

    dev_idx = bitmap_find_free_region(g_devs_bitmap, MAX_DEVICE_NUM, 0);
    if (dev_idx < 0) {
        LOGE("cannot find avaiable slot in device list\n");
        err = -XPUERR_MAXDEV;
        goto err_out;
    }

    kdev = &g_devs[dev_idx];
    memset(kdev, 0, sizeof(*kdev));

    kdev->idx = dev_idx;
    snprintf(kdev->name, XPU_MAX_STRLEN, "%s%d", kinfo->device_name, kdev->idx);
    kdev->pdev   = pdev;
    kdev->ident  = ent;
    kdev->domain = domain;
    kdev->bus    = bus;
    kdev->slot   = slot;
    kdev->func   = func;
    kdev->info   = kinfo;

    mutex_init(&kdev->xdprocs_lock);
    mutex_init(&kdev->xdi_lock);

    if (pdev->is_physfn) {
        // 为PF，且pci_iov_init初始化成功
        kdev->pf = kdev;
    } else if (pdev->is_virtfn) {
        // 为VF，且与PF处于同一os下
        kdev->pf = pci_get_drvdata(pci_physfn(pdev));
    } else {
        // TODO(miaotianxiang):
        // 存在以下情形：
        //   1. 为PF，但pci_iov_init初始化失败，不具备开启SR-IOV能力，如PF
        //      被屏蔽SR-IOV capability后再透传给了vm
        //   2. 为VF，被透传给了vm，与PF处于不同os下
        kdev->pf = NULL;
    }

    LOGI("kunlun probe device [%04x:%02x:%02x.%x] idx=%d pdev=%px pf_pdev=%px\n"
         "..Vendor=0x%x Device=0x%x SubVendor=0x%x SubDevice=0x%x Class=0x%x RVSN=0x%x\n"
         "..CanonicalName=%s Name=%s\n",
         domain, bus, slot, func, kdev->idx, pdev, pci_physfn(pdev), pdev->vendor, pdev->device,
         pdev->subsystem_vendor, pdev->subsystem_device, pdev->class, pdev->revision,
         kinfo->canonical_name, kdev->name);

    // general PCIe configuration
    err = pci_enable_device_mem(pdev);
    if (err) {
        LOGE("pci_enable_device_mem= %d\n", err);
        goto err_release_bitmap;
    }

    // select MEM type bars
    kdev->bars_en = pci_select_bars(pdev, IORESOURCE_MEM);
    err           = pci_request_selected_regions(pdev, kdev->bars_en, kdev->name);
    if (err) {
        LOGE("pci_request_selected_regions= %d, kdev->bars_en= %x\n", err, kdev->bars_en);
        goto err_disable_dev;
    }

    // enables bus-mastering for device dev
    pci_set_master(pdev);

    // set 64 DMA model
    err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
    if (err) {
        LOGE("dma_set_mask= %d\n", err);
        goto err_release_selected_regions;
    }
    // XXX(miaotianxiang): 解决kylin 4.19.90-23.8.v2101.ky10.x86_64内核使能intel iommu时的崩溃问题
    // 详情参考 https://console.cloud.baidu-int.com/devops/icafe/issue/xpu-runtime-888/show
    err = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(64));
    if (err) {
        LOGE("dma_set_coherent_mask= %d\n", err);
        goto err_release_selected_regions;
    }

    // map bar space
    for (bar_idx = 0; bar_idx < PCIE_BAR_NUM; ++bar_idx) {
        if (pci_resource_len(pdev, bar_idx) == 0) {
            continue;
        }

        kdev->bar[bar_idx] = pci_ioremap_bar(pdev, bar_idx);
        if (!kdev->bar[bar_idx]) {
            LOGE("pci_ioremap_bar= %d\n", bar_idx);
            goto err_unmap_bars;
        }

        kdev->bar_info.bar_en |= (0x1 << bar_idx);
        kdev->bar_info.pcie_addr[bar_idx] = pci_resource_start(pdev, bar_idx);
        kdev->bar_info.bar_size[bar_idx]  = pci_resource_len(pdev, bar_idx);
        kdev->bar_info.bus_addr[bar_idx]  = nv_pci_bus_address(pdev, bar_idx);

        LOGI(" -  bar[%d]: len= 0x%016llx pcie_addr= %016llx bus_addr= %016llx remap_addr= %px\n",
             bar_idx, kdev->bar_info.bar_size[bar_idx], kdev->bar_info.pcie_addr[bar_idx],
             kdev->bar_info.bus_addr[bar_idx], kdev->bar[bar_idx]);
    }

    // check bars mapping
    if (kdev->bar_info.bar_en == 0) {
        LOGE("invalid BAR mapping or BAR size, are you sure?\n");
        goto err_unmap_bars;
    }

    pci_set_drvdata(pdev, kdev);
    kl_init_profiler(kdev);

    err = kinfo->probe(kdev);
    if (err)
        goto err_unmap_bars;

    ++g_devs_count;
    return 0;

err_unmap_bars:
    pci_set_drvdata(pdev, NULL);

    for (bar_idx = 0; bar_idx < PCIE_BAR_NUM; ++bar_idx) {
        if (kdev->bar[bar_idx]) {
            pci_iounmap(pdev, kdev->bar[bar_idx]);
            kdev->bar[bar_idx] = NULL;
        }
    }

err_release_selected_regions:
    pci_clear_master(pdev);
    pci_release_selected_regions(pdev, kdev->bars_en);

err_disable_dev:
    pci_disable_device(pdev);

err_release_bitmap:
    bitmap_release_region(g_devs_bitmap, kdev->idx, 0);

    kdev->pdev    = NULL;
    kdev->ident   = NULL;
    kdev->bars_en = 0;
    kdev->pf      = NULL;
    kdev->info    = NULL;
    kdev->data    = NULL;

err_out:
    g_errno = abs(err);
    if (kdev)
        kdev->probe_errno = g_errno;

    return err;
}

#ifndef __devexit
#define __devexit
#endif
static void __devexit kl_remove(struct pci_dev *pdev)
{
    struct kl_device *kdev   = pci_get_drvdata(pdev);
    u32               domain = kdev->domain;
    u32               bus    = kdev->bus;
    u32               slot   = kdev->slot;
    u32               func   = kdev->func;
    int               bar_idx;

    LOGI("kunlun remove device [%04x:%02x:%02x.%x] idx=%d\n", domain, bus, slot, func, kdev->idx);

    kdev->info->remove(kdev);

    pci_set_drvdata(pdev, NULL);

    for (bar_idx = 0; bar_idx < PCIE_BAR_NUM; ++bar_idx) {
        if (kdev->bar[bar_idx]) {
            pci_iounmap(pdev, kdev->bar[bar_idx]);
            kdev->bar[bar_idx] = NULL;
        }
    }

    pci_clear_master(pdev);
    pci_release_selected_regions(pdev, kdev->bars_en);

    pci_disable_device(pdev);

    bitmap_release_region(g_devs_bitmap, kdev->idx, 0);

    kdev->pdev    = NULL;
    kdev->ident   = NULL;
    kdev->bars_en = 0;
    kdev->pf      = NULL;
    kdev->info    = NULL;
    kdev->data    = NULL;

    --g_devs_count;
}

int kl_pci_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
    struct kl_device *kdev = pci_get_drvdata(pdev);

    if (!kdev->info)
        return 0;
    if (!kdev->info->sriov_configure)
        return 0;

    return kdev->info->sriov_configure(kdev, num_vfs);
}

#define GEN_PCI_DEVICE_ID(vend, dev, subvend, subdev, data)                                        \
    .vendor = (vend), .device = (dev), .subvendor = (subvend), .subdevice = (subdev),              \
    .driver_data = (kernel_ulong_t)(data)

#define KL_DEVICE_1d22_3684(subsys_vendor, subsys_device, data)                                    \
    .vendor = 0x1d22, .device = 0x3684, .subvendor = (subsys_vendor),                              \
    .subdevice = (subsys_device), .driver_data = (kernel_ulong_t)(data)

#define KL_DEVICE_1d22_3685(subsys_vendor, subsys_device, data)                                    \
    .vendor = 0x1d22, .device = 0x3685, .subvendor = (subsys_vendor),                              \
    .subdevice = (subsys_device), .driver_data = (kernel_ulong_t)(data)

static struct pci_device_id kl_pci_id_tbl[] = { { KL_DEVICE_1d22_3684(0, 0, KL1) }, /* K200/K100 */
                                                { KL_DEVICE_1d22_3684(2, 1, KL2) }, /* R200 */
                                                { KL_DEVICE_1d22_3684(2, 2, KL2) }, /* R100 */
                                                { KL_DEVICE_1d22_3684(2, 3, KL2) }, /* R300/RM80 */
                                                { KL_DEVICE_1d22_3684(2, 4, KL2) }, /* R200-8F */
                                                { KL_DEVICE_1d22_3684(2, 5, KL2) }, /* R200-8FS */
                                                { KL_DEVICE_1d22_3684(2, 6, KL2) }, /* R420 */
                                                { KL_DEVICE_1d22_3684(2, 7, KL2) }, /* RG800 */
                                                { KL_DEVICE_1d22_3684(2, 8, KL2) }, /* RG800-PRO */
                                                { KL_DEVICE_1d22_3685(2, PCI_ANY_ID,
                                                                      KL2) }, /* KL2 VF */
                                                { 0 } };
MODULE_DEVICE_TABLE(pci, kl_pci_id_tbl);

static struct pci_driver kl_pci_driver __maybe_unused = {
    .name            = "kunlun",
    .id_table        = kl_pci_id_tbl,
    .probe           = kl_probe,
    .remove          = kl_remove,
    .sriov_configure = kl_pci_sriov_configure,
};

static int __init kl_init(void)
{
    int err;
    int i;

    LOGI("load kunlun driver %u.%u.%u, commit %s(%s), agile pipeline %s\n", XPURT_VERSION_MAJOR,
         XPURT_VERSION_MINOR, XPURT_VERSION_THIRD, XPURT_COMMIT, XPURT_COMMIT_TIME,
         XPURT_AGILE_PIPELINE);
    g_driver_load_time = jiffies;

    kl_proc_create();
    if (IS_ERR_OR_NULL(g_proc_root)) {
        LOGE("create proc fs under /proc/" PROC_ROOT_DIR " failed!\n");
        err = -XPUERR_NOCPUMEM;
        goto err_out;
    }
    LOGI("create /proc/" PROC_ROOT_DIR "\n");

    // 在这里添加kl1/kl2需要额外初始化的内容
    kl_init_war();
    hcm_init();
    kl1_module_init();

    g_kunlun_wq = alloc_workqueue("kunlun_wq", WQ_UNBOUND, 1);
    if (!g_kunlun_wq) {
        LOGE("alloc_workqueue failed\n");
        err = -ENOMEM;
        goto err_remove_proc_entry;
    }

    // create linux device class
#if defined(NV_CLASS_CREATE_HAS_OWNER_ARG)
    g_class = class_create(THIS_MODULE, DEVICE_NAME);
#else
    g_class = class_create(DEVICE_NAME);
#endif

    if (IS_ERR_OR_NULL(g_class)) {
        err = PTR_ERR(g_class);
        goto err_destroy_wq;
    }
    g_class->dev_uevent = kl_dev_uevent;

    // allocate device number
    // one for each XPU power domain, and one more for the control node
    err = alloc_chrdev_region(&g_devno, 0, INODE_NUM, DEVICE_NAME);
    if (err) {
        LOGE("alloc_chrdev_region= %d\n", err);
        goto err_release_class;
    }
    g_major = MAJOR(g_devno);
    LOGI("device number major= %d minor= %d ~ %d\n", g_major, 0, INODE_NUM - 1);

    // create linux char device node
    cdev_init(&g_cdev, &kl_fops);
    g_cdev.owner = THIS_MODULE;

    // this cdev handles all the fops except the last one (for ctrlnode)
    err = cdev_add(&g_cdev, g_devno, MAX_DEVINODE_NUM);
    if (err) {
        LOGE("cdev_add= %d\n", err);
        goto err_unregister_chrdev_reg;
    }
    LOGI("create cdev for minor 0 ~ %d\n", MAX_DEVINODE_NUM - 1);

    g_devs_count = 0;
    memset(g_devs, 0, sizeof(g_devs));
    for (i = 0; i < MAX_DEVICE_NUM; ++i) {
        g_devs[i].idx = i;
    }
    bitmap_clear(g_devs_bitmap, 0, MAX_DEVICE_NUM);

    memset(g_devinodes, 0, sizeof(g_devinodes));
    for (i = 0; i < MAX_DEVINODE_NUM; ++i) {
        g_devinodes[i].minor      = i;
        g_devinodes[i].devfile_id = -1;
        strncpy(g_devinodes[i].name, INVALID_DEVFILE_NAME, XPU_MAX_STRLEN);
    }
    bitmap_clear(g_devinodes_bitmap, 0, MAX_DEVINODE_NUM);

    for (i = 0; i < MAX_DEVINODE_NUM; ++i) {
        g_devfile_id_2_devinode[i] = -1;
    }
    bitmap_clear(g_devfile_ids_bitmap, 0, MAX_DEVINODE_NUM);

    err = ctrldev_create(g_major, INODE_NUM - 1);
    if (err) {
        LOGE("ctrldev_create= %d\n", err);
        goto err_cdev_del;
    }

    // register xpu driver
    // PCIe devices are probed when register pci driver
    err = pci_register_driver(&kl_pci_driver);
    if (err) {
        LOGW("pci_register_driver= %d\n", err);
        goto err_controller_del;
    }

    if (g_devs_count == 0) {
        LOGE("no supported device found\n");
        if (g_errno != 0) {
            err = -g_errno;
            goto err_unregister_driver;
        }
        // No device was found, but module install success
    }

    err = kl2_module_post();
    // XXX(miaotianxiang): 即使ccix reinit失败，仍保持现状
    //if (err) {
    //    goto err_unregister_driver;
    //}

    LOGI("module load success\n");
    return 0;

err_unregister_driver:
    pci_unregister_driver(&kl_pci_driver);

err_controller_del:
    ctrldev_destroy();

err_cdev_del:
    cdev_del(&g_cdev);

err_unregister_chrdev_reg:
    unregister_chrdev_region(g_devno, INODE_NUM);

err_release_class:
    class_destroy(g_class);

err_destroy_wq:
    destroy_workqueue(g_kunlun_wq);

err_remove_proc_entry:
    kl_proc_destroy(g_proc_root);
    LOGI("remove /proc/" PROC_ROOT_DIR "\n");

err_out:
    return err;
}
module_init(kl_init);

static void __exit kl_exit(void)
{
    pci_unregister_driver(&kl_pci_driver);

    ctrldev_destroy();

    cdev_del(&g_cdev);

    unregister_chrdev_region(g_devno, INODE_NUM);

    class_destroy(g_class);

    destroy_workqueue(g_kunlun_wq);

    hcm_disable();

    kl_proc_destroy(g_proc_root);
    LOGI("remove /proc/" PROC_ROOT_DIR "\n");
}
module_exit(kl_exit);

int kl1_p2p_stub;
module_param(kl1_p2p_stub, int, 0644);
MODULE_PARM_DESC(kl1_p2p_stub, "KL1 P2P stub mode (1=instant success, debug only)");

int kl1_dma_direct = 1;
module_param(kl1_dma_direct, int, 0644);
MODULE_PARM_DESC(kl1_dma_direct, "KL1 direct EDMA for xpu_host_alloc (S4, 1=on, 0=bounce)");

/* S6/S9: overlap copy_{from,to}_user with EDMA. 0 = legacy serial. */
int kl1_bounce_pipe = 1;
module_param(kl1_bounce_pipe, int, 0644);
MODULE_PARM_DESC(kl1_bounce_pipe,
                 "KL1 pageable bounce pipeline (1=on if 2+ ch free, 0=serial)");

/*
 * S9: D2H pipeline style when kl1_bounce_pipe=1.
 * 1 = single-issue: at most one write EDMA in flight; copy overlaps next EDMA
 *     (matches H2D shape; avoids dual write-engine contention — default).
 * 2 = S6 dual-concurrent: start next EDMA before wait prev (more aggressive).
 */
int kl1_bounce_d2h = 1;
module_param(kl1_bounce_d2h, int, 0644);
MODULE_PARM_DESC(kl1_bounce_d2h,
                 "KL1 D2H bounce style (1=single-issue S9, 2=dual-concurrent S6)");

/*
 * S9 pin-direct for pageable: only helps phys-contig/huge user memory.
 * Default OFF — 4K-scattered malloc + per-page EDMA is much slower than bounce.
 */
int kl1_pageable_pin = 0;
module_param(kl1_pageable_pin, int, 0644);
MODULE_PARM_DESC(kl1_pageable_pin,
                 "KL1 pin pageable pages for direct EDMA (0=default bounce; 1=pin path)");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BAIDU-ISA");
MODULE_DESCRIPTION("kunlun");

#ifdef MODULE_VERSION
MODULE_VERSION(XPURT_VERSION_STR);
MODULE_VERSION(XPURT_COMMIT);
#endif
