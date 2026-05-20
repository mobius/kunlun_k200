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

// Copyright 2018 Baidu Inc. All Rights Reserved.
// authors: Han Jinchen hanjinche@baidu.com
//
// xpu_drv.c - XPU driver module
// implement primary Linux module and driver logic like module entry and exit,
// device probe and remove etc.
//
#define __FILENAME__ "xpu_driver.c"

#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/random.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>

#include "xpurt_priv/ioctl.h"
#include "xpu/version.h"
#include "xpu_drv.h"
#include "xpu_hw.h"

//dev_t g_devno;
//int   g_major        = 0;
//int   g_baseminor    = 0;
//int   g_device_count = 0;
//int   g_errprobe_cnt = 0;

int g_kl1_config_autoreset = 0;
int g_kl1_config_wait_mode = 2;

//u64 g_driver_load_time;

// This bitmap tells which devfile number is available
//unsigned long *g_devfile_id_bitmap;

//struct xpu_device      g_xpu_devs[MAX_DEVICE_NUM] = { { 0 } };
//struct proc_dir_entry *g_xpu_proc_root            = NULL;
//struct class *         g_xpu_class                = NULL;
//struct cdev            g_cdev;

extern struct file_operations xpu_fops;

struct xpu_pd *get_xpd_by_minor(int minor)
{
    struct xpu_pd    *xpd    = NULL;
    struct kl_inode  *kinode = NULL;
    struct kl_device *kdev   = NULL;

    if (minor < 0 || minor >= MAX_DEVINODE_NUM)
        return NULL;
    kinode = &g_devinodes[minor];
    kdev   = kinode->kdev;
    if (kdev->info->kl_code != KL1)
        return NULL;

    xpd = kinode->data;
    if (!xpd || xpd->id == -1) {
        return NULL;
    }
    return xpd;
}

struct xpu_pd *get_xpd_by_devfile_id(int devfile_id)
{
    int minor = 0;

    if (devfile_id < 0 || devfile_id >= MAX_DEVINODE_NUM)
        return NULL;

    minor = g_devfile_id_2_devinode[devfile_id];
    return get_xpd_by_minor(minor);
}

static int __find_devfile_slot(u32 cunit_bits)
{
    int pd_cnt = 0;
    int pd_id  = 0;
    for (pd_id = 0; pd_id < XPU_PD_NUM; ++pd_id) {
        if (cunit_bits & (0xf0f << (pd_id * 4))) {
            ++pd_cnt;
        }
    }

    if (pd_cnt) {
        return bitmap_find_next_zero_area(g_devfile_ids_bitmap, MAX_DEVINODE_NUM, 0, pd_cnt, 0);
    }

    return -1;
}

static int __setup_mcu(struct xpu_device *xdev)
{
    int mcutimeout      = 0;
    u32 mcu_status      = 0;
    int i               = 0;
    u32 redirect_ints[] = {
        0x1100,                                                         // pm_int
        0x1200,                                                         // temp_int
        0x2800, 0x2804, 0x2808, 0x280C, 0x2810, 0x2814, 0x2818, 0x281C, // hbm0 int
        0x3800, 0x3804, 0x3808, 0x380C, 0x3810, 0x3814, 0x3818, 0x381C, // hbm1 int
    };

#ifdef CRASH_DRIVER
    // write 0xFAFAFFFF in syscon general r2 to recovery mcu
    xwritel(0xFAFAFFFF, xdev->syscon_base, RSYSCON_G_R2);
#else
    // write 0xFAFA1111 in syscon general r2 to grant approval for mcu
    xwritel(0xFAFA1111, xdev->syscon_base, RSYSCON_G_R2);
#endif

#ifdef CRASH_DRIVER
    return 0;
#endif

    xdev->flash_version[0] = 0;
    xdev->flash_version[1] = 0;
    xdev->flash_version[2] = 0;
    xdev->cpld_version     = 0;

    while (mcutimeout < 40) { // 30s
        mcu_status = xreadl(xdev->syscon_base, RSYSCON_G_R1);

        if (mcu_status != MCU_STATUS_UNINIT)
            break;

        msleep(1000);
        ++mcutimeout;
    }

    if (mcu_status == MCU_STATUS_UNINIT) {
        LOGW("Firmware init timeout\n");
        return -XPUERR_MCUUNINIT;
    }

    if ((mcu_status >> 16) == 0x5B33) {
        // error: HBM training timeout
        LOGW("Firmware timeout training HBM, retrain needed. status=0x%x\n", mcu_status);
        xdev->hbm_retrain_needed = 1;
    } else if (mcu_status != MCU_STATUS_NORMAL) {
        // other error
        LOGW("Firmware not init success, statue=0x%x\n", mcu_status);
        return -XPUERR_MCUUNINIT;
    }

    LOGL4("redirect %lu interrupts to msi-31\n", sizeof(redirect_ints) / sizeof(u32));

    // redirect interrupts to msi 31, avoid driver handling them
    for (i = 0; i < sizeof(redirect_ints) / sizeof(u32); ++i)
        xwritel(31, xdev->intc_base, redirect_ints[i]);

    xpu_read_mcu_version(xdev);

    if (xdev->flash_version[2] < 15)
        LOGW("Firmware version too old (<15), please update ASAP, "
             "otherwise driver cannot get info like Power/Temp etc.");

    return 0;
}

static int __post_setup_mcu(struct xpu_device *xdev)
{
    if (!xdev->hbm_retrain_needed)
        return 0;

    xwritel(MCU_STATUS_NORMAL, xdev->syscon_base, RSYSCON_G_R1);

    xpu_switch_i3e(xdev);

    return 0;
}

static int __setup_xpd(struct pci_dev *pdev, struct xpu_pd *xpd)
{
    int err = 0;
    int i;
    //long sec;
    //unsigned long ns;
    struct kl_inode *kinode = NULL;

    for (i = 0; i < PROFILER_COUNT; ++i) {
        xpd->prof_cost[i]      = 0;
        xpd->prof_count[i]     = 0;
        xpd->profiler[i].count = 0;
        xpd->profiler[i].cost  = 0;
        xpd->profiler[i].name  = NULL;
    }

    xpd->profiling_enabled = 1;

    snprintf(xpd->dev_name, XPU_MAX_STRLEN, DEVICE_NAME "%d", xpd->devfile_id);
    // compatible with existing deploy tool, proc path must be /proc/xpu/dev{n}/xxx */
    snprintf(xpd->proc_name, XPU_MAX_STRLEN, "dev%d", xpd->devfile_id);

    LOGL4("/dev/xpu%d -- pd= %d devno= %u:%u\n", xpd->devfile_id, xpd->id, xpd->major, xpd->minor);

    xpd->need_clean_etasks = 0;
    spin_lock_init(&xpd->etasks_lock);
    INIT_LIST_HEAD(&xpd->etasks);

    atomic_set(&xpd->xtqs_pending_finish_flag, 0);

    mutex_init(&xpd->xdi_cache_lock);

    // hw init XPU SSE
    xpuhw_sse_init(xpd);

    // HBM Controller
    // base of PD0 HBM channel_N (8 channels in total) is (0x020100000 + N * 0x40000).
    // base of PD1 HBM channel_N (8 channels in total) is (0x420100000 + N * 0x40000).
    // for each channel, write offset 0x3e78 and 0x4000.
    if (xpd->xdev->hbm_retrain_needed) {
        int try = 0;
        while (try < 5) {
            LOGI("Training HBM, pd=%d, try= %d\n", xpd->id, try);
#if defined(PLATFORM_KUNLUN)
            err = xpuhw_train_hbm(xpd);
#elif defined(PLATFORM_PLD) || defined(PLATFORM_ZEBU)
            err = xpuhw_train_hbm_zp(xpd);
#else
            err = 0;
#endif
            if (!err)
                break;
            ++try;
        }

        if (err)
            goto err_out;
    }

    // init cluster round mode
    xpuhw_cluster_round_mode_init(xpd);

    kinode             = &g_devinodes[xpd->minor];
    kinode->devfile_id = xpd->devfile_id;
    strncpy(kinode->name, xpd->dev_name, XPU_MAX_STRLEN);
    kinode->fops                             = &xpu_fops;
    kinode->data                             = (void *)xpd;
    kinode->kdev                             = xpd->xdev->kdev;
    g_devfile_id_2_devinode[xpd->devfile_id] = xpd->minor;

    // create linux device
    xpd->device    = device_create(g_class,
                                   &pdev->dev,                    // parent
                                   MKDEV(xpd->major, xpd->minor), // dev_t
                                   xpd,                           // drvdata
                                   "%s", xpd->dev_name);          // device name
    kinode->device = xpd->device;
    if (IS_ERR_OR_NULL(xpd->device)) {
        LOGE("device_create failed\n");
        err = -XPUERR_DEVINIT;
        goto err_kinode;
    }

    LOGL4("create Linux device /dev/xpu%d\n", xpd->devfile_id);

    // alloc proc entries
    err = xpu_proc_entries_create(xpd, g_proc_root);
    if (err != 0)
        goto err_device_destroy;

    LOGL4("create proc fs /proc/xpu/dev%d\n", xpd->devfile_id);

    // Alloc Dma buffer
    err = dma_setup(xpd);
    if (err)
        goto err_proc_destroy;

    // initialize xpu_ssedma
    mutex_init(&xpd->ssedma.lock);
    xpd->ssedma.xpd        = xpd;
    xpd->ssedma.channel    = 0;
    xpd->ssedma.max_cycles = 0;

    // Allocate memory management bitmap
    err = xpu_mem_setup(xpd);
    if (err)
        goto err_dma_unsetup;

    // init XPU task queue manager
    xtq_init(xpd);

    // init XPU session manager
    session_init(xpd);

    // init XPU task token counter
    atomic_set(&xpd->task_token_counter, 10);

    spin_lock_init(&xpd->state_lock);

    bitmap_zero(xpd->stat_use_ratio_bitmap, USE_RATIO_PN);
    xpd->stat_use_ratio_off = 0;

    xpd->timer_counter = 0;

    spin_lock_init(&xpd->timer_worklock);

    //sec = KL1_PDTIMER_FREQ_MS / 1000;
    //ns = (KL1_PDTIMER_FREQ_MS % 1000) * 1000000;
    //xpd->kt_periode = ktime_set(sec, ns);

    //hrtimer_init(&xpd->hrtimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
    //xpd->hrtimer.function = xpd_timer_handler;
    //hrtimer_start(&xpd->hrtimer, xpd->kt_periode, HRTIMER_MODE_REL);

    xpd_init_ktimer(xpd);

    bitmap_set(g_devinodes_bitmap, xpd->minor, 1);
    bitmap_set(g_devfile_ids_bitmap, xpd->devfile_id, 1);
    xpd->state = XPDS_RUNNING;

    return 0;

err_dma_unsetup:
    dma_unsetup(xpd);

err_proc_destroy:
    xpu_proc_entries_destroy(xpd, g_proc_root);

err_device_destroy:
    device_destroy(g_class, MKDEV(xpd->major, xpd->minor));

err_kinode:
    kinode->device = NULL;

    g_devfile_id_2_devinode[xpd->devfile_id] = -1;
    kinode->kdev                             = NULL;
    kinode->data                             = NULL;
    kinode->fops                             = NULL;
    strncpy(kinode->name, INVALID_DEVFILE_NAME, XPU_MAX_STRLEN);
    kinode->devfile_id = -1;

err_out:
    xpd->state = XPDS_UNUSED;

    return err;
}

static void __unsetup_xpd(struct xpu_pd *xpd)
{
    struct kl_inode *kinode = NULL;

    if (xpd->state == XPDS_UNUSED)
        return;

    bitmap_clear(g_devinodes_bitmap, xpd->minor, 1);
    bitmap_clear(g_devfile_ids_bitmap, xpd->devfile_id, 1);

    //hrtimer_cancel(&xpd->hrtimer);
    xpd_del_ktimer(xpd);

    xpu_mem_unsetup(xpd);

    dma_unsetup(xpd);

    xpu_proc_entries_destroy(xpd, g_proc_root);
    LOGL4("remove /proc/xpu/dev%d\n", xpd->devfile_id);

    if (!IS_ERR_OR_NULL(xpd->device)) {
        device_destroy(g_class, MKDEV(xpd->major, xpd->minor));
        LOGL4("remove /dev/xpu%d\n", xpd->devfile_id);
    }

    kinode         = &g_devinodes[xpd->minor];
    kinode->device = NULL;

    g_devfile_id_2_devinode[xpd->devfile_id] = -1;
    kinode->kdev                             = NULL;
    kinode->data                             = NULL;
    kinode->fops                             = NULL;
    strncpy(kinode->name, INVALID_DEVFILE_NAME, XPU_MAX_STRLEN);
    kinode->devfile_id = -1;
}

// XXX(miaotianxiang): 转移到kl_compat.c
//static inline u64 m_pci_bus_address(struct pci_dev *pdev, int bar)
//{
//    struct pci_bus_region region;
//
//    pcibios_resource_to_bus(pdev->bus, &region, &pdev->resource[bar]);
//    return region.start;
//}

int kl1_probe(struct kl_device *kdev)
{
    struct pci_dev    *pdev       = kdev->pdev;
    struct xpu_device *xdev       = NULL;
    int                dev_id     = 0;
    int                pd_id      = 0;
    int                err        = 0;
    int                devfile_id = 0;

    LOGL4("probe pdev %px\n", pdev);

    if (pdev == NULL) {
        LOGE("pdev is NULL\n");
        return -XPUERR_INVALID_PARAM;
    }

    if (pdev->bus == NULL) {
        LOGE("pdev->bus is NULL\n");
        return -XPUERR_INVALID_PARAM;
    }

    xdev = kzalloc(sizeof(*xdev), GFP_KERNEL);
    if (!xdev) {
        err = -ENOMEM;
        goto err_ret;
    }

    dev_id       = kdev->idx;
    xdev->kdev   = kdev;
    xdev->owner  = THIS_MODULE;
    xdev->pdev   = pdev;
    xdev->domain = pci_domain_nr(pdev->bus);
    xdev->bus    = pdev->bus->number;
    xdev->slot   = PCI_SLOT(pdev->devfn);
    xdev->func   = PCI_FUNC(pdev->devfn);

    xdev->disabled = 1;
    atomic_set(&xdev->in_reset, 0);

    LOGI("pdev %04x:%02x:%02x.%x -- xdev%d\n", xdev->domain & 0xffff, xdev->bus & 0xff,
         xdev->slot & 0x1f, xdev->func & 0x7, dev_id);

    xdev->errno = 0;
    mutex_init(&xdev->state_lock);
    mutex_init(&xdev->firmware_lock);
    init_completion(&xdev->irq_disable_done);
    init_completion(&xdev->reset_done);
    INIT_WORK(&xdev->reset_work, xpu_device_reset_work);
    spin_lock_init(&xdev->brw_lock);

    // make sure bar0, 2, 4 are correctly assigned
    if ((kdev->bars_en & 0x15) != 0x15) {
        LOGE("bars=0x%x PCI resource memory not assigned\n", kdev->bars_en);
        err = -ENODEV;
        goto err_release_selected_regions;
    }

    LOGL4("bars= 0x%px request success\n", kdev->bar);

    xdev->edma_base = kdev->bar[4] + 0x180000;
    xdev->iatu_base = kdev->bar[4] + 0x100000;

    xpuhw_setup_iatu_for_setup(xdev);

    // hw init PCIe eDMA
    xpuhw_edma_init(xdev);
    spin_lock_init(&xdev->edma_rw_lock);
    xdev->edma_rw_kbuf = dma_alloc_coherent(&xdev->pdev->dev, 4 * 1024, &xdev->edma_rw_dma_addr,
                                            GFP_KERNEL | __GFP_ZERO);
    if (!xdev->edma_rw_kbuf) {
        err = -XPUERR_NOCPUMEM;
        goto err_unmap_bars;
    }

    spin_lock_init(&xdev->edma_rr_lock);
    xdev->edma_rr_kbuf = dma_alloc_coherent(&xdev->pdev->dev, 4 * 1024, &xdev->edma_rr_dma_addr,
                                            GFP_KERNEL | __GFP_ZERO);
    if (!xdev->edma_rr_kbuf) {
        err = -XPUERR_NOCPUMEM;
        goto err_unmap_bars;
    }

#if defined(PLATFORM_KUNLUN)
    {
        u32 lo, hi;
        lo = xreadl(xdev->otp_base, 0x10b0);
        hi = xreadl(xdev->otp_base, 0x10ac);
        if ((lo == 0) && (hi == 0))
            xdev->sn = xreadl(xdev->otp_base, 0x1004);
        else
            xdev->sn = ((u64)hi << 32) | lo;
        LOGI("SN: %llx\n", xdev->sn);
    }
    xdev->product_num = xreadl(xdev->otp_base, 0x100C);
    if (xdev->product_num & (0x1 << 24)) {
        xdev->cunit_bits = 0x0f0f;
    } else {
        xdev->cunit_bits = 0xffff;
    }
#elif defined(PLATFORM_PLD) || defined(PLATFORM_ZEBU)
    xdev->sn          = 0;
    xdev->product_num = 0;
    xdev->cunit_bits  = xreadl(xdev->syscon_base, RSYSCON_ZP_CUNIT_BITS);
    LOGI("cunit_bits= 0x%u\n", xdev->cunit_bits);
#else
#error Must specify a TARGET_PLATFORM envrion with pld, zebu or kunlun
#endif

#if defined(PLATFORM_KUNLUN)
    err = __setup_mcu(xdev);
    if (err)
        goto err_unmap_bars;
#elif defined(PLATFORM_PLD) || defined(PLATFORM_ZEBU)
    xdev->hbm_retrain_needed = 1;
#endif

    // hw setup PCIe iATU for normal use
    err = xpuhw_setup_iatu(xdev);
    if (err)
        goto err_unmap_bars;

    // hear means pd_count
    devfile_id = __find_devfile_slot(xdev->cunit_bits);

    // initialize each power domain
    xdev->pd_num = 0;
    for (pd_id = 0; pd_id < XPU_PD_NUM; ++pd_id) {
        struct xpu_pd *xpd = &xdev->xpd[pd_id];

        if ((xdev->cunit_bits & (0xf0f << (pd_id * 4))) == 0) {
            // skip this pd if it does not contain any xpu and cdnn
            LOGL4("skip pd= %d\n", pd_id);
            // mark this pd as not used
            xpd->id = -1;
            continue;
        }

        xpd->id         = pd_id;
        xpd->xdev       = xdev;
        xpd->devfile_id = devfile_id + xpd->id;
        xpd->major      = g_major;
        xpd->minor      = kdev->idx * XPU_PD_NUM + xpd->id;
        xpd->state      = XPDS_UNUSED;
        xpd->rbase      = (pd_id) ? PD1_OFFSET : 0;

        err = __setup_xpd(pdev, xpd);
        if (err)
            goto err_cleanup_pd;

        ++xdev->pd_num;
    }

    __post_setup_mcu(xdev);

    // hw init XPU INTC
    xpuhw_intc_mask_all(xdev);

    msleep(1000);

    // Register MSI interrput handler
    err = xpu_msi_register(xdev);
    if (err)
        goto err_cleanup_pd;

    xpuhw_intc_init(xdev);

    xdev->state = XDS_RUNNING;

    xdev->disabled = 0;

    kdev->data = xdev;
    LOGI("xdev%d probe done.\n", kdev->idx);
    LOGI("------------------\n");

    return 0;

err_cleanup_pd:
    for (pd_id = 0; pd_id < XPU_PD_NUM; ++pd_id) {
        struct xpu_pd *xpd = &xdev->xpd[pd_id];

        if (xpd->id == -1)
            continue;

        __unsetup_xpd(xpd);
    }

err_unmap_bars:
    if (xdev->edma_rr_kbuf) {
        dma_free_coherent(&xdev->pdev->dev, 4 * 1024, xdev->edma_rr_kbuf, xdev->edma_rr_dma_addr);
        xdev->edma_rr_kbuf = NULL;
    }

    if (xdev->edma_rw_kbuf) {
        dma_free_coherent(&xdev->pdev->dev, 4 * 1024, xdev->edma_rw_kbuf, xdev->edma_rw_dma_addr);
        xdev->edma_rw_kbuf = NULL;
    }

err_release_selected_regions:
    xdev->state = XDS_ERRPROBE;
    xdev->errno = abs(err);

    LOGW("xdev%d probe error= %d\n", kdev->idx, xdev->errno);
    kfree(xdev);

err_ret:
    return err;
}

int kl1_remove(struct kl_device *kdev)
{
    //struct pci_dev *   pdev  = kdev->pdev;
    struct xpu_device *xdev = kdev->data;
    int                i    = 0;

    LOGL4("[%04x:%02x:%02x] remove device %d\n", xdev->bus, xdev->slot, xdev->func, xdev->id);

    xpuhw_intc_disable_msi(xdev);

    if (xdev->edma_rr_kbuf) {
        dma_free_coherent(&xdev->pdev->dev, 4 * 1024, xdev->edma_rr_kbuf, xdev->edma_rr_dma_addr);
        xdev->edma_rr_kbuf = NULL;
    }
    if (xdev->edma_rw_kbuf) {
        dma_free_coherent(&xdev->pdev->dev, 4 * 1024, xdev->edma_rw_kbuf, xdev->edma_rw_dma_addr);
        xdev->edma_rw_kbuf = NULL;
    }
    xpuhw_edma_uninit(xdev);

    LOGL4("disable intc and edma.\n");

    for (i = 0; i < XPU_PD_NUM; ++i) {
        struct xpu_pd *xpd = &xdev->xpd[i];

        if (xpd->id == -1)
            continue;

        __unsetup_xpd(&xdev->xpd[i]);
    }

    xpu_msi_unregister(xdev);

    kfree(xdev);
    return 0;
}

const struct kl_info kl1_info = {
    .kl_code                = KL1,
    .canonical_name         = "Kunlun1",
    .device_name            = "kl1_dev",
    .probe                  = kl1_probe,
    .remove                 = kl1_remove,
    .query_device_info_v1   = kl1_query_device_info_v1,
    .query_device_proc_info = kl1_query_device_proc_info,
};

// 在这里添加kl1需要额外初始化的内容
int __init kl1_module_init(void)
{
    return 0;
}
