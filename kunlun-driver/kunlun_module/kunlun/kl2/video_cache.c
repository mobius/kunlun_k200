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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "kl2/kl2.h"
#include "xpurt_priv/vdec_cwl_defs.h"
#include "kl2/video_cache.h"

#ifdef VCACHE_DEBUG
#define VLOGD(fmt, args...) KL2_LOGI("video_cache: " fmt, ##args)
#else
#define VLOGD(fmt, args...)
#endif
#define VLOGI(fmt, args...) KL2_LOGI("video_cache: " fmt, ##args)
#define VLOGW(fmt, args...) KL2_LOGW("video_cache: " fmt, ##args)
#define VLOGE(fmt, args...) KL2_LOGE("video_cache: " fmt, ##args)

#define VCACHE_REG_READ(core_id, reg_offset)                                                       \
    ioread32((void *)(pvcache_dev->cache_data[core_id].hwregs + reg_offset))

#define VCACHE_REG_WRITE(core_id, reg_offset, val)                                                 \
    iowrite32(val, (void *)(pvcache_dev->cache_data[core_id].hwregs + reg_offset))

#define RESOURCE_SHARED_INTER_CORES 0

#define CORE_CACHE_IO_SIZE ((6 + 4 * 8) * 4) /* bytes */  //cache register size
#define CORE_SHAPER_IO_SIZE ((5 + 5 * 8) * 4) /* bytes */ //shaper register size

/*for all cores, the core info should be listed here for later use*/
/*base_addr, iosize, irq*/
static const vcache_core_config g_core_array[] = {
    { VC8000D_0, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_0, CORE_SHAPER_IO_SIZE, DIR_WR },
    { VC8000D_1, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_1, CORE_SHAPER_IO_SIZE, DIR_WR },
    { VC8000D_2, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_2, CORE_SHAPER_IO_SIZE, DIR_WR },
    { VC8000D_3, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_3, CORE_SHAPER_IO_SIZE, DIR_WR },
    { VC8000D_4, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_4, CORE_SHAPER_IO_SIZE, DIR_WR },
    { VC8000D_5, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_5, CORE_SHAPER_IO_SIZE, DIR_WR },
    { VC8000D_6, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_6, CORE_SHAPER_IO_SIZE, DIR_WR },
    { VC8000D_7, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_7, CORE_SHAPER_IO_SIZE, DIR_WR },
    { VC8000D_8, CORE_CACHE_IO_SIZE, DIR_RD }, { VC8000D_8, CORE_SHAPER_IO_SIZE, DIR_WR },
};

#if 0
typedef struct {
    vcache_core_config core_cfg;    //config of each core,such as base addr, irq,etc
    unsigned long      hw_id;       //hw id to indicate project
    u32                core_id;     //core id for driver and sw internal use
    u32                is_valid;    //indicate this core is hantro's core or not
    u32                is_reserved; //indicate this core is occupied by user or not
    int                pid;         //indicate which process is occupying the core
    struct file       *owner_fp;
    u32                irq_received; //indicate this core receives irq
    u32                irq_status;
    char              *buffer;
    unsigned int       buffsize;
    volatile u8       *hwregs;
    unsigned long      com_base_addr; //common base addr of each L2
} cache_data_t;

typedef struct {
    struct kl2_device *kl2_dev;

    cache_data_t          cache_data[VCACHE_MAX_CORES];
    int                   total_core_num;
    volatile unsigned int asic_status;
    wait_queue_head_t     hw_queue;
    wait_queue_head_t     cache_wait_queue;
    spinlock_t            owner_lock;
} vcache_dev_t;
#endif
static int check_cache_irq(vcache_dev_t *pvcache_dev, int core_id)
{
    unsigned long flags = 0;
    int           rdy   = 0;

    spin_lock_irqsave(&pvcache_dev->owner_lock, flags);

    if (pvcache_dev->cache_data[core_id].irq_received) {
        /* reset the wait condition(s) */
        pvcache_dev->cache_data[core_id].irq_received = 0;
        rdy                                           = 1;
    }

    spin_unlock_irqrestore(&pvcache_dev->owner_lock, flags);

    return rdy;
}

static long wait_cache_ready(vcache_dev_t *pvcache_dev, int core_id)
{
    int                        ret            = 0;
    struct kl2_device *kl2_dev __maybe_unused = pvcache_dev->kl2_dev;
    VLOGD("wait_cache_ready\n");

    ret = wait_event_interruptible(pvcache_dev->cache_wait_queue,
                                   check_cache_irq(pvcache_dev, core_id));
    if (ret) {
        VLOGW("Cache wait_event_interruptible interrupted core_id %d\n", core_id);
        return -ERESTARTSYS;
    }

    return 0;
}

static int check_core_occupation(vcache_dev_t *pvcache_dev, struct file *filp, int core_id)
{
    int           ret   = 0;
    unsigned long flags = 0;

    spin_lock_irqsave(&pvcache_dev->owner_lock, flags);
    if (!pvcache_dev->cache_data[core_id].is_reserved) {
        pvcache_dev->cache_data[core_id].is_reserved = 1;
        pvcache_dev->cache_data[core_id].pid         = current->pid;
        pvcache_dev->cache_data[core_id].owner_fp    = filp;
        ret                                          = 1;
    }

    spin_unlock_irqrestore(&pvcache_dev->owner_lock, flags);

    return ret;
}

static int get_workable_core(vcache_dev_t *pvcache_dev, struct file *filp, u32 *core_id)
{
    struct kl2_device *kl2_dev __maybe_unused = pvcache_dev->kl2_dev;
    int                        ret            = 0;
    u32                        i              = 0;
    driver_cache_dir           dir            = (*core_id) & 0x01;
    cache_client_type          client         = (*core_id) >> 1;

    VLOGD("dir = %d, client = %d\n", dir, client);

    for (i = 0; i < pvcache_dev->total_core_num; i++) {
        /* a valid free Core*/
        if (pvcache_dev->cache_data[i].is_valid &&
            pvcache_dev->cache_data[i].core_cfg.client == client &&
            pvcache_dev->cache_data[i].core_cfg.dir == dir &&
            check_core_occupation(pvcache_dev, filp, i)) {
            ret      = 1;
            *core_id = i;
            break;
        }
    }

    return ret;
}

static long reserve_core(vcache_dev_t *pvcache_dev, struct file *filp, u32 *core_id)
{
    struct kl2_device *kl2_dev __maybe_unused = pvcache_dev->kl2_dev;
    driver_cache_dir           dir            = (*core_id) & 0x01;
    cache_client_type          client         = (*core_id) >> 1;
    int                        i              = 0;
    int                        status         = 0;
    int                        ret            = 0;

    for (i = 0; i < pvcache_dev->total_core_num; i++) {
        /* a valid core supports such client and dir*/
        if (pvcache_dev->cache_data[i].is_valid &&
            pvcache_dev->cache_data[i].core_cfg.client == client &&
            pvcache_dev->cache_data[i].core_cfg.dir == dir) {
            status = 1;
            break;
        }
    }

    if (status == 0) {
        VLOGW("No any core support client:%d,dir:%d.\n", dir, client);
        return -1;
    }

    /* lock a core that has specified core id*/
    ret = wait_event_interruptible(pvcache_dev->hw_queue,
                                   get_workable_core(pvcache_dev, filp, core_id));
    if (ret != 0) {
        VLOGW("VCACHE wait_event_interruptible interrupted, ret = %d\n", ret);
        return -ERESTARTSYS;
    }

    return 0;
}

static void release_core(vcache_dev_t *pvcache_dev, u32 core_id)
{
    struct kl2_device *kl2_dev __maybe_unused = pvcache_dev->kl2_dev;
    unsigned long              flags          = 0;

    VLOGD("Release, core = %d\n", core_id);

    spin_lock_irqsave(&pvcache_dev->owner_lock, flags);

    if (pvcache_dev->cache_data[core_id].is_reserved
        /*&& pvcache_dev->cache_data[core_id].pid == current->pid*/) {
        pvcache_dev->cache_data[core_id].pid         = -1;
        pvcache_dev->cache_data[core_id].is_reserved = 0;
        pvcache_dev->cache_data[core_id].owner_fp    = NULL;
    }

    pvcache_dev->cache_data[core_id].irq_received = 0;
    pvcache_dev->cache_data[core_id].irq_status   = 0;
    spin_unlock_irqrestore(&pvcache_dev->owner_lock, flags);

    wake_up_interruptible_all(&pvcache_dev->hw_queue);

    return;
}

void vcache_isr(pvcache_device_t pvcachedev)
{
    vcache_dev_t *pvcache_dev   = (vcache_dev_t *)pvcachedev;
    u32           irq_status    = 0;
    unsigned long flags         = 0;
    u32           irq_triggered = 0;
    int           i             = 0;

    CHECK_NULL_RETURN(pvcache_dev);

    for (i = 0; i < pvcache_dev->total_core_num; i++) {
        irq_triggered = 0;
        if (pvcache_dev->cache_data[i].core_cfg.dir == DIR_RD) {
            irq_status = VCACHE_REG_READ(i, 0x04);
            if (irq_status & 0x28) {
                irq_triggered = 1;
                VCACHE_REG_WRITE(i, 0x04, irq_status); //clear irq
            }
        } else {
            irq_status = VCACHE_REG_READ(i, 0x0C);
            if (irq_status) {
                irq_triggered = 1;
                VCACHE_REG_WRITE(i, 0x0C, irq_status); //clear irq
            }
        }

        if (irq_triggered == 1) { //irq has been triggered
            /* clear all IRQ bits. IRQ is cleared by writting 1 */
            spin_lock_irqsave(&pvcache_dev->owner_lock, flags);

            if (pvcache_dev->cache_data[i].is_reserved) {
                pvcache_dev->cache_data[i].irq_received = 1;
                pvcache_dev->cache_data[i].irq_status   = irq_status;

                spin_unlock_irqrestore(&pvcache_dev->owner_lock, flags);
                wake_up_interruptible_all(&pvcache_dev->cache_wait_queue);
            } else {
                /*If core is not reserved by any user, but irq is received, just ignore it*/
                spin_unlock_irqrestore(&pvcache_dev->owner_lock, flags);
            }
        }
    }
}

long vcache_process_ioctl(pvcache_device_t pvcachedev, struct file *filp, unsigned int cmd,
                          unsigned long arg)
{
    vcache_dev_t              *pvcache_dev    = (vcache_dev_t *)pvcachedev;
    struct kl2_device *kl2_dev __maybe_unused = pvcache_dev->kl2_dev;
    int                        ret            = 0;

    CHECK_NULL_RET_VAL(pvcache_dev, -EINVAL);

    VLOGD("cmd = 0x%08ux:\n", cmd);

    switch (cmd) {
    case IOCTL_VDEC_CACHE_IOCGHWOFFSET: {
        u32 id;

        VLOGD("IOCTL_VDEC_CACHE_IOCGHWOFFSET\n");
        get_user(id, (u32 *)arg);

        if (id >= pvcache_dev->total_core_num || pvcache_dev->cache_data[id].is_valid == 0) {
            return -EFAULT;
        }
        __put_user(pvcache_dev->cache_data[id].com_base_addr, (unsigned long *)arg);

    } break;
    case IOCTL_VDEC_CACHE_IOCGHWIOSIZE: {
        u32 id;
        u32 io_size;

        VLOGD("IOCTL_VDEC_CACHE_IOCGHWIOSIZE\n");

        get_user(id, (u32 *)arg);

        if (id >= pvcache_dev->total_core_num || pvcache_dev->cache_data[id].is_valid == 0) {
            return -EFAULT;
        }

        io_size = pvcache_dev->cache_data[id].core_cfg.iosize;
        __put_user(io_size, (u32 *)arg);
    } break;
    case IOCTL_VDEC_CACHE_IOCG_CORE_NUM: {
        VLOGD("IOCTL_VDEC_CACHE_IOCG_CORE_NUM\n");
        __put_user(pvcache_dev->total_core_num, (unsigned int *)arg);
    } break;
    case IOCTL_VDEC_CACHE_IOCH_HW_RESERVE: {
        u32 core_id = 0;

        VLOGD("IOCTL_VDEC_CACHE_IOCH_HW_RESERVE\n");

        get_user(core_id, (u32 *)arg); //get client and direction info

        ret = reserve_core(pvcache_dev, filp, &core_id);
        if (ret == 0) {
            __put_user(core_id, (unsigned int *)arg);
        } else {
            VLOGW("reserve_core failed, ret = %d\n", ret);
        }

        return ret;
    } break;
    case IOCTL_VDEC_CACHE_IOCH_HW_RELEASE: {
        u32 core_id = 0;

        VLOGD("IOCTL_VDEC_CACHE_IOCH_HW_RELEASE\n");

        get_user(core_id, (u32 *)arg);

        release_core(pvcache_dev, core_id);
    } break;
    case IOCTL_VDEC_CACHE_IOCG_ABORT_WAIT: {
        u32 id = 0;

        VLOGD("IOCTL_VDEC_CACHE_IOCG_ABORT_WAIT\n");

        get_user(id, (u32 *)arg);

        ret = wait_cache_ready(pvcache_dev, id);
        if (ret == 0) {
            __put_user(pvcache_dev->cache_data[id].irq_status, (unsigned int *)arg);
        }
    } break;
    default:
        VLOGE("Unknown cmd\n");
        return -EINVAL;
    }

    return ret;
}

static void reset_asic(vcache_dev_t *pvcache_dev)
{
    struct kl2_device *kl2_dev __maybe_unused = pvcache_dev->kl2_dev;
    int                        i              = 0;
    int                        n              = 0;

    for (n = 0; n < pvcache_dev->total_core_num; n++) {
        if (pvcache_dev->cache_data[n].is_valid == 0) {
            continue;
        }

        VLOGD("reset core: %d\n", n);
        for (i = 0; i < pvcache_dev->cache_data[n].core_cfg.iosize; i += 4) {
            VCACHE_REG_WRITE(n, i, 0);
        }
    }
}

pvcache_device_t vcache_init(vcache_init_para_t *pvcache_init)
{
    int           i           = 0;
    int           hwid        = 0;
    vcache_dev_t *pvcache_dev = vzalloc(sizeof(vcache_dev_t));

    CHECK_NULL_RET_VAL(pvcache_dev, NULL);

    pvcache_dev->kl2_dev        = pvcache_init->kl2_dev;
    pvcache_dev->total_core_num = pvcache_init->cores_num;

    VLOGD("total_core_num = %d\n", pvcache_dev->total_core_num);

    spin_lock_init(&pvcache_dev->owner_lock);
    init_waitqueue_head(&pvcache_dev->hw_queue);
    init_waitqueue_head(&pvcache_dev->cache_wait_queue);

    for (i = 0; i < pvcache_dev->total_core_num; i++) {
        int hw_cfg = 0;

        pvcache_dev->cache_data[i].core_cfg      = g_core_array[i];
        pvcache_dev->cache_data[i].hwregs        = pvcache_init->vcache_reg_base_virt_addr[i];
        pvcache_dev->cache_data[i].core_id       = i;
        pvcache_dev->cache_data[i].is_valid      = 0;
        pvcache_dev->cache_data[i].com_base_addr = pvcache_init->vcache_reg_base_hw_addr[i];

        VLOGD("cache_data[%d].hwregs = %px\n", i, (void *)pvcache_dev->cache_data[i].hwregs);

        hwid   = VCACHE_REG_READ(i, 0x00);
        hw_cfg = (hwid & 0xF0000) >> 16;

        VLOGD("core_id = %d, hwid = 0x%x, hw_cfg = 0x%x\n", i, hwid, hw_cfg);

        if (hw_cfg > 2) {
            continue;
        }

        if ((hw_cfg == 1) && (pvcache_dev->cache_data[i].core_cfg.dir == DIR_WR)) { //cache only
            pvcache_dev->cache_data[i].is_valid = 0;
        } else if ((hw_cfg == 2) &&
                   (pvcache_dev->cache_data[i].core_cfg.dir == DIR_RD)) { //shaper only
            pvcache_dev->cache_data[i].is_valid = 0;
        } else {
            pvcache_dev->cache_data[i].is_valid = 1;
        }

        if (pvcache_dev->cache_data[i].is_valid == 0) {
            continue;
        }

        if ((hwid == 0) && (pvcache_dev->cache_data[i].core_cfg.dir == DIR_RD)) {
            pvcache_dev->cache_data[i].hwregs += CACHE_WITH_SHAPER_OFFSET;
        } else if (hwid != 0) {
            if (pvcache_dev->cache_data[i].core_cfg.dir == DIR_WR) {
                pvcache_dev->cache_data[i].hwregs += SHAPER_OFFSET;
            } else if ((pvcache_dev->cache_data[i].core_cfg.dir == DIR_RD) && (hw_cfg == 0)) {
                pvcache_dev->cache_data[i].hwregs += CACHE_WITH_SHAPER_OFFSET;
            } else if ((pvcache_dev->cache_data[i].core_cfg.dir == DIR_RD) && (hw_cfg == 1)) {
                pvcache_dev->cache_data[i].hwregs += CACHE_ONLY_OFFSET;
            } else {
                // do nothing
            }
        } else {
            // do nothing
        }
    }

    reset_asic(pvcache_dev);

    VLOGD("init finished\n");

    return pvcache_dev;
}

void vcache_uninit(pvcache_device_t pvcachedev)
{
    vcache_dev_t *pvcache_dev = (vcache_dev_t *)pvcachedev;
    int           i           = 0;

    CHECK_NULL_RETURN(pvcache_dev);

    for (i = 0; i < pvcache_dev->total_core_num; i++) {
        if (pvcache_dev->cache_data[i].is_valid == 0) {
            continue;
        }

        VCACHE_REG_WRITE(i, 0x04, 0);   /* disable HW */
        VCACHE_REG_WRITE(i, 0x14, 0xF); /* clear IRQ */
    }

    vfree(pvcache_dev);

    return;
}

void vcache_on_file_release(pvcache_device_t pvcachedev, struct file *filp, int dec_core_id)
{
    vcache_dev_t *pvcache_dev = (vcache_dev_t *)pvcachedev;
    int           core_id     = 0;
    int           i           = 0;
    u32           tmp         = 0;

    CHECK_NULL_RETURN(pvcache_dev);

    for (core_id = 2 * dec_core_id; core_id <= 2 * dec_core_id + 1; core_id++) {
        if (pvcache_dev->cache_data[core_id].owner_fp == filp) {
            if (pvcache_dev->cache_data[core_id].core_cfg.dir == DIR_RD) {
                if (VCACHE_REG_READ(core_id, 0x04) & 0x1) {
                    VCACHE_REG_WRITE(core_id, 0x04, 0); /*release cache*/
                }
            } else {
                if (VCACHE_REG_READ(core_id, 0x0) & 0x1) {
                    VCACHE_REG_WRITE(core_id, 0x0, 0); /*release shaper*/
                    for (i = 0; i < 10000; i++) {
                        tmp = VCACHE_REG_READ(core_id, 0x0c);
                        if (tmp & 0x2) {
                            //printk(KERN_INFO "L2 cache: DEC[%d] disabled shaper DONE\n", core_id);
                            VCACHE_REG_WRITE(core_id, 0x0c, tmp);
                            break;
                        }
                    }
                    if (i == 10000) {
                        pr_err("L2 cache: core[%d]:disabled shaper FAILED!\n", core_id);
                    }
                }
            }
            release_core(pvcache_dev, core_id);
        }
    }

    VLOGD("dev closed\n");
    return;
}

#endif
