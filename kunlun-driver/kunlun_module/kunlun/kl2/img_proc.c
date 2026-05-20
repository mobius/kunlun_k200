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

#ifdef ENABLE_CODEC

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/bitmap.h>
#include <linux/vmalloc.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif
#include "kl2/kl2.h"
#include "kl2/kl2_regs.h"
#include "kl2/img_proc.h"
#include "xpurt_priv/imgproc_defs.h"
#include "xpurt_priv/ioctl_img_proc_kl2.h"

//#define IMGPROC_DEBUG

#ifdef IMGPROC_DEBUG
#define VLOGD(fmt, args...) KL2_LOGI("imgproc: " fmt, ##args)
#else
#define VLOGD(fmt, args...)
#endif
#define VLOGI(fmt, args...) KL2_LOGI("imgproc: " fmt, ##args)
#define VLOGW(fmt, args...) KL2_LOGW("imgproc: " fmt, ##args)
#define VLOGE(fmt, args...) KL2_LOGE("imgproc: " fmt, ##args)

#define IMGPROC_REG_READ(core_id, reg_offset)                                                      \
    ioread32((void *)(pimgproc_dev->imgproc_reg_base_virt_addr[core_id] + reg_offset))

#define IMGPROC_REG_WRITE(core_id, reg_offset, val)                                                \
    iowrite32(val, (void *)(pimgproc_dev->imgproc_reg_base_virt_addr[core_id] + reg_offset))

#define SYSCON_REG_READ(reg_offset)                                                                \
    ioread32((void *)(pimgproc_dev->syscon_reg_base_virt_addr + reg_offset))

#define SYSCON_REG_WRITE(reg_offset, val)                                                          \
    iowrite32(val, (void *)(pimgproc_dev->syscon_reg_base_virt_addr + reg_offset))

// Interrupt status
enum {
    ism_rdma_done         = BIT(0),
    ism_rdma_bus_clean    = BIT(1),
    ism_rdma_time_out     = BIT(2),
    ism_rdma_rresp_slverr = BIT(3),
    ism_rdma_rresp_decerr = BIT(4),
    ism_rdma_not_cont     = BIT(5),
    ism_wdma_done         = BIT(6),
    ism_wdma_bus_clean    = BIT(7),
    ism_wdma_time_out     = BIT(8),
    ism_wdma_bresp_slverr = BIT(9),
    ism_wdma_bresp_decerr = BIT(10),
};

static void reset_video_noc(imgprocdev_t *pimgproc_dev)
{
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    u32                        regval         = SYSCON_REG_READ(VIDEO_RST_CTRL);

    VLOGD("Reset video NOC\n");

    SYSCON_REG_WRITE(VIDEO_RST_CTRL, regval | BIT(19));
    usleep_range(1, 2);

    SYSCON_REG_WRITE(VIDEO_RST_CTRL, regval & ~BIT(19));
    usleep_range(1, 2);
}

static void reset_asic(imgprocdev_t *pimgproc_dev, int core_id)
{
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    u32                        regval         = 0;
    // u32 tmpval = 0;
    const int     delay_interval = 100; //us
    u32           delay_count    = 0;
    unsigned long flags          = 0;

    VLOGD("Reset core_id = %d, reg[IMG_PROC_INT_STATUS_RAW] = 0x%x\n", core_id,
          IMGPROC_REG_READ(core_id, IMG_PROC_INT_STATUS_RAW));

    // ensure the hw is idle
    while (delay_count < 9000) {
        spin_lock_irqsave(&pimgproc_dev->owner_lock, flags);
        if (pimgproc_dev->hw_busy[core_id] == 0) {
            spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);
            break;
        } else {
            regval = IMGPROC_REG_READ(core_id, IMG_PROC_INT_STATUS_RAW);
            if (regval & ism_wdma_done) {
                pimgproc_dev->hw_busy[core_id] = 0;
                spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);
                VLOGI("core[%d] WDMA done\n", core_id);
                break;
            } else if (regval & (ism_wdma_bresp_slverr | ism_wdma_bresp_decerr |
                                 ism_rdma_rresp_slverr | ism_rdma_rresp_decerr)) {
                // fatal error, may need to reset NOC from soc level.
                pimgproc_dev->hw_busy[core_id] = 0;
                spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);
                VLOGE("imgproc fatal error happened, may need to reset NOC from soc level\n");
                break;
            } else if (regval & (ism_wdma_time_out | ism_rdma_time_out)) {
                pimgproc_dev->hw_busy[core_id] = 0;
                spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);
                VLOGE("imgproc timeout happened\n");
                break;
            } else {
                spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);
                usleep_range(delay_interval, delay_interval + 1);

                if ((delay_count % 1000) == 0) {
                    VLOGW("imgproc core_id[%d] is still busy\n", core_id);
                }

                delay_count++;
            }
        }
    }

    VLOGD("delay_count = %d\n", delay_count);

    // regval = SYSCON_REG_READ(VIDEO_RST_CTRL);
    // tmpval = regval | BIT(13 + core_id);
    // VLOGD("tmpval = 0x%x\n", tmpval);
    // SYSCON_REG_WRITE(VIDEO_RST_CTRL, tmpval);
    // usleep_range(1, 2);

    // tmpval = regval & ~BIT(13 + core_id);
    // VLOGD("tmpval = 0x%x\n", tmpval);
    // SYSCON_REG_WRITE(VIDEO_RST_CTRL, tmpval);
    // usleep_range(1, 2);

    // clear all interrupts, will also clear the RAW int status
    IMGPROC_REG_WRITE(core_id, IMG_PROC_INT_CLEAR, ~0);
    // disable interrupts
    IMGPROC_REG_WRITE(core_id, IMG_PROC_INT_MASK, 0);
    // enable interrupts:wdma done
    IMGPROC_REG_WRITE(core_id, IMG_PROC_INT_MASK, ism_wdma_done);
}

static int reserve_core(imgprocdev_t *pimgproc_dev, struct file *filp)
{
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    int                        core_id        = 0;
    unsigned long              flags          = 0;
    int                        i              = 0;
    long                       start_index    = 0;

    /* reserve a core */
    if (down_interruptible(&pimgproc_dev->core_sem)) {
        return -ERESTARTSYS;
    }

    spin_lock_irqsave(&pimgproc_dev->owner_lock, flags);

#ifdef BALANCE_CORE_USAGE
    start_index = atomic64_inc_return(&pimgproc_dev->reserved_count) - 1;
#endif
    for (i = 0; i < pimgproc_dev->cores_num; i++) {
#ifdef BALANCE_CORE_USAGE
        core_id = (i + start_index) % pimgproc_dev->cores_num;
#else
        core_id = i;
#endif
        if (pimgproc_dev->owner[core_id] == NULL) {
            pimgproc_dev->owner[core_id] = filp;
            atomic_inc(&pimgproc_dev->video_perf->core_in_used);
            break;
        }
    }

    if (likely(core_id < pimgproc_dev->cores_num)) {
        pimgproc_dev->core_usage[core_id]++;
    } else {
        VLOGE("should not happen\n");
        core_id = -1;
    }

    spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);

    return core_id;
}

static void release_core(imgprocdev_t *pimgproc_dev, int core_id)
{
    unsigned long flags = 0;

    reset_asic(pimgproc_dev, core_id);

    spin_lock_irqsave(&pimgproc_dev->owner_lock, flags);
    pimgproc_dev->owner[core_id] = NULL;
    atomic_dec(&pimgproc_dev->video_perf->core_in_used);
    atomic_inc(&pimgproc_dev->video_perf->frames_num);
    spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);

    up(&pimgproc_dev->core_sem);

    return;
}

static void flush_regs(imgprocdev_t *pimgproc_dev, int core_id, u32 *regs, int start_hw)
{
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    int                        i              = 0;
    unsigned long              flags          = 0;

    // flush RDMA registers, but don't enable it
    for (i = 0; i < IMGPROC_RDMA_REGS_NUM - 1; i++) {
        IMGPROC_REG_WRITE(core_id, i * 4 + IMGPROC_RDMA_REGS_BASE, regs[i]);
    }

    // flush RESIZE registers
    for (i = 0; i < IMGPROC_RESIZE_REGS_NUM; i++) {
        IMGPROC_REG_WRITE(core_id, i * 4 + IMGPROC_RESIZE_REGS_BASE,
                          regs[i + IMGPROC_RDMA_REGS_NUM]);
    }

    // flush WDMA registers
    for (i = 0; i < IMGPROC_WDMA_REGS_NUM; i++) {
        IMGPROC_REG_WRITE(core_id, i * 4 + IMGPROC_WDMA_REGS_BASE,
                          regs[i + IMGPROC_RDMA_REGS_NUM + IMGPROC_RESIZE_REGS_NUM]);
    }

    // enable interrupts
    IMGPROC_REG_WRITE(core_id, IMG_PROC_INT_MASK, 0x077c);

    if (start_hw) {
        spin_lock_irqsave(&pimgproc_dev->owner_lock, flags);
        pimgproc_dev->hw_busy[core_id] = 1;
        spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);

        //force update
        IMGPROC_REG_WRITE(core_id, IMG_PROC_FORCE_UPD, 0x1);

        //start RDMA
        IMGPROC_REG_WRITE(core_id, IMG_RDMA_START, 0x1);
    }

    VLOGD("flushed registers on core %d, start_hw = %d\n", core_id, start_hw);

    return;
}

static int __maybe_unused check_imgproc_irq(imgprocdev_t *pimgproc_dev, int core_id, int *status)
{
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    unsigned long              flags          = 0;
    int                        rdy            = 0;
    const u32                  irq_mask       = (1 << core_id);

    spin_lock_irqsave(&pimgproc_dev->owner_lock, flags);

    if (pimgproc_dev->imgproc_irq & irq_mask) {
        /* reset the wait condition(s) */
        pimgproc_dev->imgproc_irq &= ~irq_mask;
        rdy = 1;

        if (pimgproc_dev->int_status[core_id] & ism_wdma_done) {
            *status = IMGPROC_WAIT_OK;
        } else if ((pimgproc_dev->int_status[core_id] & ism_rdma_time_out) ||
                   (pimgproc_dev->int_status[core_id] & ism_wdma_time_out)) {
            VLOGE("imgproc DMA timeout happened, raw_int_status = 0x%x\n",
                  pimgproc_dev->raw_int_status[core_id]);
            *status = IMGPROC_WAIT_TIME_OUT;
        } else {
            VLOGE("imgproc fatal error happened, may need to reset NOC\n");
            *status = IMGPROC_WAIT_ERR;
        }

        pimgproc_dev->int_status[core_id]     = 0;
        pimgproc_dev->raw_int_status[core_id] = 0;
    }

    spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);

    return rdy;
}

static long wait_imgproc_ready(imgprocdev_t *pimgproc_dev, int core_id)
{
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    int                        ret            = 0;
    int                        status         = 0;

    VLOGD("wait_event_interruptible IMGPROC[%d]\n", core_id);
#if 0
    ret = wait_event_interruptible(pimgproc_dev->wait_queue,
                                     check_imgproc_irq(pimgproc_dev, core_id, &status));
#endif
    ret = wait_event_interruptible(pimgproc_dev->wait_queue[core_id],
                                   pimgproc_dev->got_event[core_id]);
    if (ret) {
        VLOGI("IMGPROC[%d]  wait_event_interruptible interrupted, ret = %d\n", core_id, ret);
        return -ERESTARTSYS;
    }
    pimgproc_dev->got_event[core_id] = 0;

    //atomic_inc(&pimgproc_dev->irq_tx);
    if (signal_pending(current)) {
        VLOGI("IMGPROC[%d]  wait_event_interruptible interrupted by signal, ret = %d\n", core_id,
              ret);
        return -ERESTARTSYS;
    }
    return status;
}

pimgproc_device_t imgproc_init(struct kl2_device *kl2_dev)
{
    imgprocdev_t       *pimgproc_dev      = NULL;
    int                 i                 = 0;
    imgproc_init_para_t imgproc_init_para = { 0 };
    struct kl2_df_spec *spec;
    struct kl_device   *kdev;

    ASSERT_RET_VAL(kl2_dev != NULL, NULL);

    spec = &kl2_dev->spec;
    kdev = kl2_dev->kdev;
    CHECK_NULL_RET_VAL(spec, NULL);
    CHECK_NULL_RET_VAL(kdev, NULL);

    imgproc_init_para.cores_num = bitcount(spec->imgproc_bits);

    ASSERT_RET_VAL(imgproc_init_para.cores_num <= IMGPROC_MAX_CORES, NULL);

    for (i = 0; i < imgproc_init_para.cores_num; i++) {
        imgproc_init_para.syscon0_reg_base_virt_addr = kdev->bar[2] + KL2_REG_SYSCON0_BAR2_BASE;
        imgproc_init_para.img_proc_reg_base_hw_addr[i] =
                kdev->bar_info.pcie_addr[2] + KL2_REG_IMGPROC_BAR2_BASE + i * 0x2000;
        imgproc_init_para.img_proc_reg_base_virt_addr[i] =
                (u8 *)kdev->bar[2] + KL2_REG_IMGPROC_BAR2_BASE + i * 0x2000;
    }

    pimgproc_dev = vzalloc(sizeof(imgprocdev_t));
    CHECK_NULL_RET_VAL(pimgproc_dev, NULL);

    pimgproc_dev->kl2_dev   = kl2_dev;
    pimgproc_dev->cores_num = imgproc_init_para.cores_num;

    spin_lock_init(&pimgproc_dev->owner_lock);
    sema_init(&pimgproc_dev->core_sem, pimgproc_dev->cores_num);

    pimgproc_dev->syscon_reg_base_virt_addr = imgproc_init_para.syscon0_reg_base_virt_addr;
    VLOGD("syscon_reg_base_virt_addr = %px\n", pimgproc_dev->syscon_reg_base_virt_addr);

    for (i = 0; i < pimgproc_dev->cores_num; i++) {
        pimgproc_dev->imgproc_reg_base_virt_addr[i] =
                imgproc_init_para.img_proc_reg_base_virt_addr[i];

        VLOGD("imgproc_reg_base_virt_addr[%d] = %px\n", i,
              pimgproc_dev->imgproc_reg_base_virt_addr[i]);
        init_waitqueue_head(&pimgproc_dev->wait_queue[i]);
        reset_asic(pimgproc_dev, i);
    }

    // dangerous!
    // FIXME: should not do this here
    reset_video_noc(pimgproc_dev);

    return pimgproc_dev;
}

void imgproc_uninit(pimgproc_device_t pimgprocdev)
{
    int                        i              = 0;
    imgprocdev_t              *pimgproc_dev   = pimgprocdev;
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;

    CHECK_NULL_RETURN(pimgproc_dev);

    for (i = 0; i < pimgproc_dev->cores_num; i++) {
        VLOGD("core[%d] usage: %d\n", i, pimgproc_dev->core_usage[i]);
    }

    for (i = 0; i < pimgproc_dev->cores_num; i++) {
        reset_asic(pimgproc_dev, i);
    }

    vfree(pimgproc_dev);

    return;
}
#if 0
void imgproc_isr(pimgproc_device_t pimgprocdev)
{
    imgprocdev_t *pimgproc_dev = (imgprocdev_t*)pimgprocdev;
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    unsigned long flags = 0;
    int i = 0;
    u32 int_status = 0;
    u32 raw_int_status = 0;

    CHECK_NULL_RETURN(pimgproc_dev);

    spin_lock_irqsave(&pimgproc_dev->owner_lock, flags);

    for (i = 0; i < pimgproc_dev->cores_num; i++) {
        /* interrupt status register read */
        int_status = IMGPROC_REG_READ(i, IMG_PROC_INT_STATUS_MASKED);
        raw_int_status = IMGPROC_REG_READ(i, IMG_PROC_INT_STATUS_RAW);

        if (int_status != 0) {
            /* clear IRQ */
            IMGPROC_REG_WRITE(i, IMG_PROC_INT_CLEAR, int_status);
#ifdef IMGPROC_DEBUG
            VLOGD("imgproc IRQ received! core %d, int_status = 0x%x, raw_int_status = 0x%x\n",
                    i, int_status, raw_int_status);
#endif
            pimgproc_dev->raw_int_status[i] |= raw_int_status;

            if (pimgproc_dev->hw_busy[i]
                && (raw_int_status & (ism_wdma_done
                                        | ism_rdma_time_out
                                        | ism_rdma_rresp_slverr
                                        | ism_rdma_rresp_decerr
                                        | ism_wdma_time_out
                                        | ism_wdma_bresp_slverr
                                        | ism_wdma_bresp_decerr))) {
                pimgproc_dev->hw_busy[i] = 0;
            }

            if (int_status & ~(ism_rdma_done | ism_rdma_bus_clean)) {
                pimgproc_dev->int_status[i] |= int_status;
                //atomic_inc(&pimgproc_dev->irq_rx);
                pimgproc_dev->imgproc_irq |= (1 << i);
                wake_up_interruptible_all(&pimgproc_dev->wait_queue);
            }
        }
    }

    spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);

    return;
}
#endif
void imgproc_isr(pimgproc_device_t pimgprocdev)
{
    imgprocdev_t              *pimgproc_dev   = (imgprocdev_t *)pimgprocdev;
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    unsigned long              flags          = 0;
    int                        i              = 0;
    u32                        int_status     = 0;
    u32                        raw_int_status = 0;

    CHECK_NULL_RETURN(pimgproc_dev);

    spin_lock_irqsave(&pimgproc_dev->owner_lock, flags);

    for (i = 0; i < pimgproc_dev->cores_num; i++) {
        /* interrupt status register read */
        int_status     = IMGPROC_REG_READ(i, IMG_PROC_INT_STATUS_MASKED);
        raw_int_status = IMGPROC_REG_READ(i, IMG_PROC_INT_STATUS_RAW);

        if (int_status != 0) {
            /* clear IRQ */
            IMGPROC_REG_WRITE(i, IMG_PROC_INT_CLEAR, int_status);
            VLOGD("imgproc IRQ received! core %d, int_status = 0x%x, raw_int_status = 0x%x\n", i,
                  int_status, raw_int_status);
            pimgproc_dev->raw_int_status[i] |= raw_int_status;
            pimgproc_dev->int_status[i] |= int_status;
            pimgproc_dev->imgproc_irq |= (1 << i);

            if (pimgproc_dev->hw_busy[i] && (raw_int_status & ism_wdma_done)) {
                pimgproc_dev->hw_busy[i]   = 0;
                pimgproc_dev->got_event[i] = 1;
                VLOGD("imgproc isr .......\n");
                wake_up(&pimgproc_dev->wait_queue[i]);
            } else {
                VLOGD("imgproc IRQ received, but not wdma done! core id %d, int_status 0x%x, \
                    raw_int_status 0x%x\n",
                      i, int_status, raw_int_status);
            }
        }
    }

    spin_unlock_irqrestore(&pimgproc_dev->owner_lock, flags);

    return;
}

long imgproc_process_ioctl(pimgproc_device_t pimgprocdev, struct file *filp, unsigned int cmd,
                           unsigned long arg)
{
    imgprocdev_t              *pimgproc_dev   = (imgprocdev_t *)pimgprocdev;
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    int                        err            = 0;
    long                       tmp            = 0;

    CHECK_NULL_RET_VAL(pimgproc_dev, -EINVAL);

    /*
    * extract the type and number bitfields, and don't decode
    * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
    */
    if (_IOC_TYPE(cmd) != IOCTL_IMGPROC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > IOCTL_IMGPROC_MAXNR)
        return -ENOTTY;

    /*
    * the direction is a bitmask, and VERIFY_WRITE catches R/W
    * transfers. `Type' is user-oriented, while
    * access_ok is kernel-oriented, so the concept of "read" and
    * "write" is reversed
    */
    if (_IOC_DIR(cmd) & _IOC_READ) {
        //err = !access_ok(VERIFY_WRITE, (void *) arg, _IOC_SIZE(cmd));
    } else if (_IOC_DIR(cmd) & _IOC_WRITE) {
        //err = !access_ok(VERIFY_READ, (void *) arg, _IOC_SIZE(cmd));
    } else {
        // OK
    }

    if (err) {
        return -EFAULT;
    }

    VLOGD("ioctl cmd 0x%08x:\n", cmd);

    switch (cmd) {
    case IOCTL_IMGPROC_RESET: {
        VLOGD("IOCTL_IMGPROC_RESET\n");
        if ((arg >= 0) && (arg < pimgproc_dev->cores_num)) {
            reset_asic(pimgproc_dev, (int)arg);
        } else {
            return -EINVAL;
        }
    } break;
    case IOCTL_IMGPROC_CORE_NUM: {
        VLOGD("IOCTL_IMGPROC_CORE_NUM\n");
        //__put_user(pimgproc_dev->cores_num, (u32 *)arg);
        VLOGD("pimgproc_dev->cores_num=%d\n", pimgproc_dev->cores_num);
        return pimgproc_dev->cores_num;
    } break;
    case IOCTL_IMGPROC_RESERVE: {
        VLOGD("IOCTL_IMGPROC_RESERVE\n");
        return reserve_core(pimgproc_dev, filp);
    } break;
    case IOCTL_IMGPROC_RELEASE: {
        VLOGD("IOCTL_IMGPROC_RELEASE\n");
        if ((arg >= pimgproc_dev->cores_num) || (pimgproc_dev->owner[arg] != filp)) {
            VLOGE("bogus release, core = %li\n", arg);
            return -EFAULT;
        }

        release_core(pimgproc_dev, (int)arg);
    } break;
    case IOCTL_IMGPROC_RUN: {
        struct imgproc_ioctl_run_param run_param;
        u32                            regs[IMGPROC_REGS_NUM];

        VLOGD("IOCTL_IMGPROC_RUN\n");

        /* get parameter struct from user space*/
        tmp = copy_from_user(&run_param, (void *)arg, sizeof(struct imgproc_ioctl_run_param));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        if ((run_param.core_id >= pimgproc_dev->cores_num) || (run_param.core_id < 0)) {
            return -EINVAL;
        }

        /* Get registers from user space */
        tmp = copy_from_user(regs, run_param.regs, IMGPROC_REGS_NUM * 4);
        if (tmp) {
            VLOGD("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        flush_regs(pimgproc_dev, run_param.core_id, regs, 1);
    } break;
    case IOCTL_IMGPROC_WAIT: {
        VLOGD("IOCTL_IMGPROC_WAIT\n");

        if ((arg >= 0) && (arg < pimgproc_dev->cores_num)) {
            return wait_imgproc_ready(pimgproc_dev, (int)arg);
        } else {
            return -EINVAL;
        }
    } break;
    case IOCTL_IMGPROC_WRITE_REG: {
        struct imgproc_ioctl_rw_reg_param rw_reg_param;

        VLOGD("IOCTL_IMGPROC_WRITE_REG\n");

        /* get parameter struct from user space*/
        tmp = copy_from_user(&rw_reg_param, (void *)arg, sizeof(struct imgproc_ioctl_rw_reg_param));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        if ((rw_reg_param.core_id >= pimgproc_dev->cores_num) || (rw_reg_param.core_id < 0)) {
            return -EINVAL;
        }

        IMGPROC_REG_WRITE(rw_reg_param.core_id, rw_reg_param.reg_offset, rw_reg_param.data);
    } break;
    case IOCTL_IMGPROC_READ_REG: {
        struct imgproc_ioctl_rw_reg_param  rw_reg_param;
        struct imgproc_ioctl_rw_reg_param *p_user_arg = (struct imgproc_ioctl_rw_reg_param *)arg;

        VLOGD("IOCTL_IMGPROC_READ_REG\n");

        /* get parameter struct from user space*/
        tmp = copy_from_user(&rw_reg_param, (void *)arg, sizeof(struct imgproc_ioctl_rw_reg_param));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        if ((rw_reg_param.core_id >= pimgproc_dev->cores_num) || (rw_reg_param.core_id < 0)) {
            return -EINVAL;
        }

        rw_reg_param.data = IMGPROC_REG_READ(rw_reg_param.core_id, rw_reg_param.reg_offset);

        tmp = copy_to_user(&p_user_arg->data, &rw_reg_param.data, sizeof(u32));
        if (tmp) {
            VLOGE("copy_to_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        return 0;
    } break;
    default: {
        VLOGE("Invalid cmd\n");
    } break;
    }

    return 0;
}

void imgproc_on_file_release(pimgproc_device_t pimgprocdev, struct file *filp)
{
    imgprocdev_t              *pimgproc_dev   = (imgprocdev_t *)pimgprocdev;
    struct kl2_device *kl2_dev __maybe_unused = pimgproc_dev->kl2_dev;
    int                        i              = 0;

    CHECK_NULL_RETURN(pimgproc_dev);

    for (i = 0; i < pimgproc_dev->cores_num; i++) {
        if (pimgproc_dev->owner[i] == filp) {
            VLOGD("releasing core %d\n", i);
            release_core(pimgproc_dev, i);
        }
    }

    VLOGD("dev closed\n");

    return;
}

#endif
