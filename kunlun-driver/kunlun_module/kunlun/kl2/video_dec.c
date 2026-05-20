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
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/vmalloc.h>

#include "kl2/video_cache.h"
#include "xpurt_priv/vdec_dwl_defs.h"
#include "kl2/kl2.h"
#include "kl2/kl2_regs.h"
#include "kl2/video_dec.h"
#include "xpurt_priv/ioctl_vdec_kl2.h"

#ifdef VDEC_DEBUG
#define VLOGD(fmt, args...) KL2_LOGI("video_dec: " fmt, ##args)
#else
#define VLOGD(fmt, args...)
#endif
#define VLOGI(fmt, args...) KL2_LOGI("video_dec: " fmt, ##args)
#define VLOGW(fmt, args...) KL2_LOGW("video_dec: " fmt, ##args)
#define VLOGE(fmt, args...) KL2_LOGE("video_dec: " fmt, ##args)

#define VDEC_REG_READ(core_id, reg_offset)                                                         \
    ioread32((void *)(pvdec_device->reg_base_virt_addr[core_id] + reg_offset))

#define VDEC_REG_WRITE(core_id, reg_offset, val)                                                   \
    iowrite32(val, (void *)(pvdec_device->reg_base_virt_addr[core_id] + reg_offset))

#define SYSCON_REG_READ(reg_offset)                                                                \
    ioread32((void *)(pvdec_device->syscon_reg_base_virt_addr + reg_offset))

#define SYSCON_REG_WRITE(reg_offset, val)                                                          \
    iowrite32(val, (void *)(pvdec_device->syscon_reg_base_virt_addr + reg_offset))

#if 0
/* video decoder reg config */
#define VDEC_REGS_NUM 437 /* total regs */
//#define VDEC_REGS_NUM         359 /* total regs */
#define VDEC_FIRST_REG 0
#define VDEC_LAST_REG (VDEC_REGS_NUM - 1)

#define DWL_CLIENT_TYPE_H264_DEC 1U
#define DWL_CLIENT_TYPE_MPEG4_DEC 2U
#define DWL_CLIENT_TYPE_JPEG_DEC 3U
#define DWL_CLIENT_TYPE_PP 4U
#define DWL_CLIENT_TYPE_VC1_DEC 5U
#define DWL_CLIENT_TYPE_MPEG2_DEC 6U
#define DWL_CLIENT_TYPE_VP6_DEC 7U
#define DWL_CLIENT_TYPE_AVS_DEC 8U
#define DWL_CLIENT_TYPE_RV_DEC 9U
#define DWL_CLIENT_TYPE_VP8_DEC 10U
#define DWL_CLIENT_TYPE_VP9_DEC 11U
#define DWL_CLIENT_TYPE_HEVC_DEC 12U
#define DWL_CLIENT_TYPE_ST_PP 14U
#define DWL_CLIENT_TYPE_H264_MAIN10 15U
#define DWL_CLIENT_TYPE_AVS2_DEC 16U
#define DWL_CLIENT_TYPE_AV1_DEC 17U

typedef struct {
    struct kl2_device *kl2_dev;

    u8               *reg_base_virt_addr[VDEC_MAX_CORES];
    u64               reg_base_hw_addr[VDEC_MAX_CORES];
    u32               hw_id;     /* assume all cores share same HW ID*/
    u32               cores_num; /* total available cores num on the device */
    spinlock_t        owner_lock;
    wait_queue_head_t dec_wait_queue;
    wait_queue_head_t hw_queue;
    struct file      *dec_owner[VDEC_MAX_CORES];
    u32               cfg_fmt[VDEC_MAX_CORES];        /* indicate the supported format */
    u32               cfg_fmt_backup[VDEC_MAX_CORES]; /* back up of cfg_fmt */
    u32               dec_regs[VDEC_MAX_CORES][VDEC_REGS_NUM];
#ifdef VDEC_REG_ACCESS_OPT
    u32 shadow_dec_regs[VDEC_MAX_CORES][VDEC_REGS_NUM];
#endif
#ifdef VDEC_DEBUG
    u32 dec_flush_regs_func_count; /* times of calling of DecFlushRegs */
    u32 total_flushed_regs_count;  /* total number of registers flushed */
#endif

    struct semaphore dec_core_sem;
    int              dec_irq;
    atomic_t         irq_rx;
    atomic_t         irq_tx;
    u32              core_usage[VDEC_MAX_CORES];

#ifdef BALANCE_CORE_USAGE
    atomic_t reserved_count;
#endif
    pvcache_device_t cache_device;
} vdecdev_t;
#endif
static void reset_asic(pvdec_device_t pvdecdev)
{
    vdecdev_t *pvdec_device = (vdecdev_t *)pvdecdev;
    int        i = 0, j = 0;
    u32        status = 0;

    for (j = 0; j < pvdec_device->cores_num; j++) {
        status = VDEC_REG_READ(j, VDEC_IRQ_STAT_DEC_OFF);

        if (status & VDEC_DEC_E) {
            /* abort with IRQ disabled */
            status = VDEC_DEC_ABORT | VDEC_DEC_IRQ_DISABLE;
            VDEC_REG_WRITE(j, VDEC_IRQ_STAT_DEC_OFF, status);
        }

        for (i = 1; i < VDEC_REGS_NUM; i++) {
            VDEC_REG_WRITE(j, i * 4, 0);
        }
    }
}

static void read_core_config(pvdec_device_t pvdecdev)
{
    vdecdev_t                 *pvdec_device   = (vdecdev_t *)pvdecdev;
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    int                        core_id        = 0;
    u32                        reg = 0, tmp = 0, mask = 0;

    for (core_id = 0; core_id < pvdec_device->cores_num; core_id++) {
        /* Decoder configuration */
        reg = VDEC_REG_READ(core_id, VDEC_SYNTH_CFG * 4);
#ifdef VDEC_DEBUG
        VLOGD("core_id:%d, reg[VDEC_SYNTH_CFG] = 0x%08x\n", core_id, reg);
#endif
        tmp = (reg >> DWL_H264_E) & 0x3U;
        if (tmp) {
            VLOGD("core[%d] has H264\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_H264_DEC : 0;

        tmp = (reg >> DWL_H264HIGH10_E) & 0x01U;
        if (tmp) {
            VLOGD("core[%d] has H264HIGH10\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_H264_DEC : 0;

        tmp = (reg >> DWL_AVS2_E) & 0x03U;
        if (tmp) {
            VLOGD("core[%d] has AVS2\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_AVS2_DEC : 0;

        tmp = (reg >> DWL_JPEG_E) & 0x01U;
        if (tmp) {
            VLOGD("core[%d] has JPEG\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_JPEG_DEC : 0;

        tmp = (reg >> DWL_HJPEG_E) & 0x01U;
        if (tmp) {
            VLOGD("core[%d] has HJPEG\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_JPEG_DEC : 0;

        tmp = (reg >> DWL_MPEG4_E) & 0x3U;
        if (tmp) {
            VLOGD("core[%d] has MPEG4\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_MPEG4_DEC : 0;

        tmp = (reg >> DWL_VC1_E) & 0x3U;
        if (tmp) {
            VLOGD("core[%d] has VC1\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_VC1_DEC : 0;

        tmp = (reg >> DWL_MPEG2_E) & 0x01U;
        if (tmp) {
            VLOGD("core[%d] has MPEG2\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_MPEG2_DEC : 0;

        tmp = (reg >> DWL_VP6_E) & 0x01U;
        if (tmp) {
            VLOGD("core[%d] has VP6\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_VP6_DEC : 0;

        reg = VDEC_REG_READ(core_id, VDEC_SYNTH_CFG_2 * 4);

        /* VP7 and WEBP is part of VP8 */
        mask = (1 << DWL_VP8_E) | (1 << DWL_VP7_E) | (1 << DWL_WEBP_E);
        tmp  = (reg & mask);
        if (tmp & (1 << DWL_VP8_E)) {
            VLOGD("core[%d] has VP8\n", core_id);
        }
        if (tmp & (1 << DWL_VP7_E)) {
            VLOGD("core[%d] has VP7\n", core_id);
        }
        if (tmp & (1 << DWL_WEBP_E)) {
            VLOGD("core[%d] has WebP\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_VP8_DEC : 0;

        tmp = (reg >> DWL_AVS_E) & 0x01U;
        if (tmp) {
            VLOGD("core[%d] has AVS\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_AVS_DEC : 0;

        tmp = (reg >> DWL_RV_E) & 0x03U;
        if (tmp) {
            VLOGD("core[%d] has RV\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_RV_DEC : 0;

        reg = VDEC_REG_READ(core_id, VDEC_SYNTH_CFG_3 * 4);

        tmp = (reg >> DWL_HEVC_E) & 0x07U;
        if (tmp) {
            VLOGD("core[%d] has HEVC\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_HEVC_DEC : 0;

        tmp = (reg >> DWL_VP9_E) & 0x07U;
        if (tmp) {
            VLOGD("core[%d] has VP9\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_VP9_DEC : 0;

        /* Post-processor configuration */
        reg = VDEC_REG_READ(core_id, VDECPP_CFG_STAT * 4);

        tmp = (reg >> DWL_PP_E) & 0x01U;
        if (tmp) {
            VLOGD("core[%d] has PP\n", core_id);
        }
        pvdec_device->cfg_fmt[core_id] |= tmp ? 1 << DWL_CLIENT_TYPE_PP : 0;

        pvdec_device->cfg_fmt[core_id] |= 1 << DWL_CLIENT_TYPE_ST_PP;
    }
}

static int core_has_format(const vdecdev_t *pvdec_device, int core_id, u32 format)
{
    return (pvdec_device->cfg_fmt[core_id] & (1 << format)) ? 1 : 0;
}

int get_dec_core(long core_id, vdecdev_t *pvdec_device, struct file *filp, unsigned long format)
{
    int           success = 0;
    unsigned long flags   = 0;

    spin_lock_irqsave(&pvdec_device->owner_lock, flags);
    if (core_has_format(pvdec_device, core_id, format) &&
        (pvdec_device->dec_owner[core_id] == NULL)) {
        pvdec_device->dec_owner[core_id] = filp;
        success                          = 1;
        pvdec_device->core_usage[core_id]++;
    }

    spin_unlock_irqrestore(&pvdec_device->owner_lock, flags);

    return success;
}

static int get_dec_core_any(long *core, vdecdev_t *pvdec_device, struct file *filp,
                            unsigned long format)
{
    int  success = 0;
    long c       = 0;
#ifdef BALANCE_CORE_USAGE
    long start_index = atomic64_inc_return(&pvdec_device->reserved_count) - 1;
#endif
    int core_id = 0;

    *core = -1;

    for (c = 0; c < pvdec_device->cores_num; c++) {
        /* a free core that has format */
#ifdef BALANCE_CORE_USAGE
        core_id = (c + start_index) % pvdec_device->cores_num;
#else
        core_id = c;
#endif
        if (get_dec_core(core_id, pvdec_device, filp, format)) {
            success = 1;
            *core   = core_id;
            break;
        }
    }

    return success;
}

/* a core that has format */
static int get_dec_core_id(vdecdev_t *pvdec_device, unsigned long format)
{
    long c       = 0;
    int  core_id = -1;

    for (c = 0; c < pvdec_device->cores_num; c++) {
        if (core_has_format(pvdec_device, c, format)) {
            core_id = c;
            break;
        }
    }

    return core_id;
}

static long reserve_decoder(vdecdev_t *pvdec_device, struct file *filp, unsigned long format)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    long                       core_id        = -1;

    /* reserve a core */
    if (down_interruptible(&pvdec_device->dec_core_sem)) {
        return -ERESTARTSYS;
    }
    if (wait_event_interruptible(pvdec_device->hw_queue,
                                 get_dec_core_any(&core_id, pvdec_device, filp, format) != 0)) {
        return -ERESTARTSYS;
    }

    atomic_inc(&pvdec_device->video_perf->core_in_used);
    return core_id;
#if 0
    if (0 != get_dec_core_any(&core_id, pvdec_device, filp, format)) {
        VLOGD("Reserved DEC %ld\n", core_id);
        return core_id;
    } else {
        VLOGE("no core available for format = 0x%lx\n", format);
        up(&pvdec_device->dec_core_sem);
        return -ERESTARTSYS;
    }
#endif
}

static void release_decoder(vdecdev_t *pvdec_device, long core_id)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    u32                        status         = 0;
    unsigned long              flags          = 0;
    u32                        loop           = 0;

    status = VDEC_REG_READ(core_id, VDEC_IRQ_STAT_DEC_OFF);
    while (status & 0x1) {
        status = VDEC_REG_READ(core_id, VDEC_IRQ_STAT_DEC_OFF);

        if (loop == 5000) {
            printk("---in release_decoder: status 0x%x, core_id %ld\n", status, core_id);
            if (((status >> 0) & 0x1) || ((status >> 1) & 0x1) || ((status >> 14) & 0x1)) {
                printk("in release_decoder, stream error dected! status reg: 0x%x\n", status);
                status &= (~VDEC_DEC_E);
                VDEC_REG_WRITE(core_id, VDEC_IRQ_STAT_DEC_OFF, status);
            }
        }
        loop++;
    }

    spin_lock_irqsave(&pvdec_device->owner_lock, flags);
    pvdec_device->dec_owner[core_id] = NULL;
    pvdec_device->dec_irq &= ~(1 << core_id);
    spin_unlock_irqrestore(&pvdec_device->owner_lock, flags);

    up(&pvdec_device->dec_core_sem);
    wake_up_interruptible_all(&pvdec_device->hw_queue);
    atomic_dec(&pvdec_device->video_perf->core_in_used);
    atomic_inc(&pvdec_device->video_perf->frames_num);

    VLOGD("DEC core %ld released\n", core_id);

    return;
}

static long flush_regs(vdecdev_t *pvdec_device, struct vdec_core_desc *core)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    long                       ret = 0, i = 0;
    u32                        id = core->id;
#ifdef VDEC_DEBUG
    int reg_wr = 2;
#endif

    ret = copy_from_user(pvdec_device->dec_regs[id], core->regs, VDEC_REGS_NUM * 4);
    if (ret) {
        VLOGE("copy_from_user failed, returned %li\n", ret);
        return -EFAULT;
    }

    /* write all regs but the status reg[1] to hardware */
#ifdef VDEC_REG_ACCESS_OPT
    for (i = 3; i < VDEC_REGS_NUM; i++) {
        /* check whether register value is updated. */
        if (pvdec_device->dec_regs[id][i] != pvdec_device->shadow_dec_regs[id][i]) {
            VDEC_REG_WRITE(id, i * 4, pvdec_device->dec_regs[id][i]);
            pvdec_device->shadow_dec_regs[id][i] = pvdec_device->dec_regs[id][i];
#ifdef VDEC_DEBUG
            reg_wr++;
#endif
        }
    }
#else // VDEC_REG_ACCESS_OPT

    for (i = 3; i < VDEC_REGS_NUM; i++) {
        VDEC_REG_WRITE(id, i * 4, pvdec_device->dec_regs[id][i]);
#ifdef VALIDATE_REGS_WRITE
        if (pvdec_device->dec_regs[id][i] != VDEC_REG_READ(id, i * 4)) {
            KL2_LOGW("swreg[%ld]: read %08x != write %08x *\n", i, VDEC_REG_READ(id, i * 4),
                     pvdec_device->dec_regs[id][i]);
        }
#endif
    }

#ifdef VDEC_DEBUG
    reg_wr = VDEC_REGS_NUM - 3;
#endif
#endif // VDEC_REG_ACCESS_OPT

    /* write swreg2 for AV1, in which bit0 is the start bit */
    VDEC_REG_WRITE(id, 8, pvdec_device->dec_regs[id][2]);
#ifdef VDEC_REG_ACCESS_OPT
    pvdec_device->shadow_dec_regs[id][2] = pvdec_device->dec_regs[id][2];
#endif

    /* write the status register, which may start the decoder */
    VDEC_REG_WRITE(id, 4, pvdec_device->dec_regs[id][1]);
#ifdef VDEC_REG_ACCESS_OPT
    pvdec_device->shadow_dec_regs[id][1] = pvdec_device->dec_regs[id][1];
#endif

#ifdef VDEC_DEBUG
    pvdec_device->dec_flush_regs_func_count++;
    pvdec_device->total_flushed_regs_count += reg_wr;

    VLOGD("%d DecFlushRegs: flushed %d/%d registers (dec_mode = %d, avg %d regs per flush)\n",
          pvdec_device->dec_flush_regs_func_count, reg_wr, pvdec_device->total_flushed_regs_count,
          pvdec_device->dec_regs[id][3] >> 27,
          pvdec_device->total_flushed_regs_count / pvdec_device->dec_flush_regs_func_count);
#endif

    VLOGD("flushed registers on core %d\n", id);

    return 0;
}

static long write_reg(vdecdev_t *pvdec_device, struct vdec_core_desc *core)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    long                       ret            = 0;
    u32                        id             = core->id;
    long                       i              = core->reg_id;

    ret = copy_from_user(pvdec_device->dec_regs[id] + core->reg_id, core->regs + core->reg_id, 4);

    if (ret) {
        VLOGE("copy_from_user failed, returned %li\n", ret);
        return -EFAULT;
    }

    VDEC_REG_WRITE(id, i * 4, pvdec_device->dec_regs[id][i]);
#ifdef VDEC_REG_ACCESS_OPT
    pvdec_device->shadow_dec_regs[id][i] = pvdec_device->dec_regs[id][i];
#endif
    return 0;
}

static long read_reg(vdecdev_t *pvdec_device, struct vdec_core_desc *core)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    long                       ret            = 0;
    u32                        id             = core->id;
    long                       i              = core->reg_id;

    pvdec_device->dec_regs[id][i] = VDEC_REG_READ(id, i * 4);
#ifdef VDEC_REG_ACCESS_OPT
    pvdec_device->shadow_dec_regs[id][i] = pvdec_device->dec_regs[id][i];
#endif

    /* put registers to user space*/
    ret = copy_to_user(core->regs + core->reg_id, pvdec_device->dec_regs[id] + core->reg_id, 4);
    if (ret) {
        VLOGE("copy_to_user failed, returned %li\n", ret);
        return -EFAULT;
    }

    return 0;
}

static long dec_refresh_regs(vdecdev_t *pvdec_device, struct vdec_core_desc *core)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    long                       ret = 0, i = 0;
    u32                        id = core->id;

#ifdef VDEC_REG_ACCESS_OPT
    // only need to read swreg1,62(?),63,168,169
#define REFRESH_REG(idx)                                                                           \
    i                                    = (idx);                                                  \
    pvdec_device->shadow_dec_regs[id][i] = pvdec_device->dec_regs[id][i] = VDEC_REG_READ(id, i * 4)

    REFRESH_REG(0);
    REFRESH_REG(1);
    REFRESH_REG(62);
    REFRESH_REG(63);
    REFRESH_REG(168);
    REFRESH_REG(169);
#undef REFRESH_REG
#else  // VDEC_REG_ACCESS_OPT
    for (i = 0; i < VDEC_REGS_NUM; i++) {
        pvdec_device->dec_regs[id][i] = VDEC_REG_READ(id, i * 4);
    }
#endif // VDEC_REG_ACCESS_OPT

    ret = copy_to_user(core->regs, pvdec_device->dec_regs[id], MIN(VDEC_REGS_NUM * 4, core->size));
    if (ret) {
        VLOGE("copy_to_user failed, returned %li\n", ret);
        return -EFAULT;
    }

    return 0;
}

static int check_dec_irq(vdecdev_t *pvdec_device, int core_id)
{
    unsigned long flags    = 0;
    int           rdy      = 0;
    const u32     irq_mask = (1 << core_id);

    spin_lock_irqsave(&pvdec_device->owner_lock, flags);

    if (pvdec_device->dec_irq & irq_mask) {
        /* reset the wait condition(s) */
        pvdec_device->dec_irq &= ~irq_mask;
        rdy = 1;
    }

    spin_unlock_irqrestore(&pvdec_device->owner_lock, flags);

    return rdy;
}

static long wait_dec_ready_and_refresh_regs(vdecdev_t *pvdec_device, struct vdec_core_desc *core)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    u32                        id             = core->id;
    long                       ret            = 0;

#ifdef USE_SW_TIMEOUT
    u32 status = 0;

    VLOGD("wait_event_interruptible DEC[%d]\n", id);
    ret = wait_event_interruptible_timeout(pvdec_device->dec_wait_queue,
                                           check_dec_irq(pvdec_device, id),
                                           msecs_to_jiffies(SW_TIMEOUT_MILLIS));
    if (ret < 0) {
        VLOGI("DEC[%d]  wait_event_interruptible interrupted\n", id);
        return -ERESTARTSYS;
    } else if (ret == 0) {
        VLOGD("DEC[%d]  wait_event_interruptible timeout\n", id);
        status = VDEC_REG_READ(id, VDEC_IRQ_STAT_DEC_OFF);
        /* check if HW is enabled */
        if (status & VDEC_DEC_E) {
            VLOGI("DEC[%d] reset becuase of timeout\n", id);

            /* abort decoder */
            status |= VDEC_DEC_ABORT | VDEC_DEC_IRQ_DISABLE;
            VDEC_REG_WRITE(id, VDEC_IRQ_STAT_DEC_OFF, status);
        }
    } else {
        // do nothing
    }
#else
    VLOGD("wait_event_interruptible DEC[%d]\n", id);
    ret = wait_event_interruptible(pvdec_device->dec_wait_queue, check_dec_irq(pvdec_device, id));
    if (ret) {
        VLOGI("DEC[%d]  wait_event_interruptible interrupted, ret = %ld\n", id, ret);
        return -ERESTARTSYS;
    }
#endif
    atomic64_inc(&pvdec_device->irq_tx);

    /* refresh registers */
    return dec_refresh_regs(pvdec_device, core);
}

static int check_core_irq(vdecdev_t *pvdec_device, const struct file *filp, int *core_id)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    unsigned long              flags          = 0;
    int                        rdy = 0, n = 0;

    do {
        u32 irq_mask = (1 << n);

        spin_lock_irqsave(&pvdec_device->owner_lock, flags);

        if (pvdec_device->dec_irq & irq_mask) {
            if (pvdec_device->dec_owner[n] == filp) {
                /* we have an IRQ for our client */
                /* reset the wait condition(s) */
                pvdec_device->dec_irq &= ~irq_mask;

                /* signal ready core no. for our client */
                *core_id = n;

                rdy = 1;

                spin_unlock_irqrestore(&pvdec_device->owner_lock, flags);
                break;
            } else if (pvdec_device->dec_owner[n] == NULL) {
                /* zombie IRQ */
                VLOGI("IRQ on core[%d], but no owner!!!\n", n);

                /* reset the wait condition(s) */
                pvdec_device->dec_irq &= ~irq_mask;
            }
        }

        spin_unlock_irqrestore(&pvdec_device->owner_lock, flags);

        n++; /* next core */
    } while (n < pvdec_device->cores_num);

    return rdy;
}

static long wait_core_ready(vdecdev_t *pvdec_device, const struct file *filp, int *core_id)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    long                       ret            = 0;

#ifdef USE_SW_TIMEOUT
    u32 i = 0, status = 0;

    VLOGD("wait_event_interruptible CORE\n");

    ret = wait_event_interruptible_timeout(pvdec_device->dec_wait_queue,
                                           check_core_irq(pvdec_device, filp, core_id),
                                           msecs_to_jiffies(SW_TIMEOUT_MILLIS));
    if (ret < 0) {
        VLOGI("CORE  wait_event_interruptible interrupted\n");
        return -ERESTARTSYS;
    } else if (ret == 0) {
        VLOGD("CORE wait_event_interruptible timeout\n");
        for (i = 0; i < pvdec_device->cores_num; i++) {
            status = VDEC_REG_READ(i, VDEC_IRQ_STAT_DEC_OFF);
            /* check if HW is enabled */
            if ((status & VDEC_DEC_E) && pvdec_device->dec_owner[i] == filp) {
                VLOGI("CORE[%d] reset becuase of timeout\n", i);
                *core_id = i;
                /* abort decoder */
                status |= VDEC_DEC_ABORT | VDEC_DEC_IRQ_DISABLE;
                VDEC_REG_WRITE(i, VDEC_IRQ_STAT_DEC_OFF, status);
                break;
            }
        }
    }
#else
    VLOGD("wait_event_interruptible CORE\n");

    ret = wait_event_interruptible(pvdec_device->dec_wait_queue,
                                   check_core_irq(pvdec_device, filp, core_id));
    if (ret) {
        VLOGD("CORE  wait_event_interruptible interrupted, ret = %ld\n", ret);
        return -ERESTARTSYS;
    }
#endif

    atomic64_inc(&pvdec_device->irq_tx);

    return 0;
}

static void init_cache(vdecdev_t *pvdec_device)
{
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    vcache_init_para_t         cache_init_para;
    int                        i = 0;

    cache_init_para.kl2_dev   = kl2_dev;
    cache_init_para.cores_num = pvdec_device->cores_num * 2; // cache and shaper

    for (i = 0; i < pvdec_device->cores_num; i++) {
        cache_init_para.vcache_reg_base_virt_addr[2 * i] =
                pvdec_device->reg_base_virt_addr[i] + 0x2000;
        cache_init_para.vcache_reg_base_virt_addr[2 * i + 1] =
                pvdec_device->reg_base_virt_addr[i] + 0x2000;

        cache_init_para.vcache_reg_base_hw_addr[2 * i] = pvdec_device->reg_base_hw_addr[i] + 0x2000;
        cache_init_para.vcache_reg_base_hw_addr[2 * i + 1] =
                pvdec_device->reg_base_hw_addr[i] + 0x2000;
    }

    pvdec_device->cache_device = vcache_init(&cache_init_para);
}

pvdec_device_t vdec_init(struct kl2_device *kl2_dev)
{
    struct kl2_df_spec *spec;
    vdec_init_para_t    vdec_init_para;
    struct kl_device   *kdev;
    vdecdev_t          *pvdec_device = NULL;
    int                 i            = 0;
    u32                 val;

    ASSERT_RET_VAL(kl2_dev != NULL, NULL);

    spec = &kl2_dev->spec;
    kdev = kl2_dev->kdev;
    CHECK_NULL_RET_VAL(spec, NULL);
    CHECK_NULL_RET_VAL(kdev, NULL);

    // set the video APB clock to 800MHz, the default clock is 500MHz
    if (!is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
        kl2_writel(kl2_dev, 0x05, kl2_dev->iomem_base.syscon1_base + 0x38);
    }

#ifdef CODEC_BRING_UP_TEST_ON
    ASSERT_RET_VAL(kdev->num_vfs == 0, NULL); //only pf mode
    vdec_init_para.cores_num = 1;             //Only one hardware core is tested at a time
    int test_core_idx        = 1;             //core index is a variable:0~8
#else
    vdec_init_para.cores_num = bitcount(spec->dec_bits);
#endif
    ASSERT_RET_VAL(vdec_init_para.cores_num <= VDEC_MAX_CORES, NULL);

    vdec_init_para.syscon0_reg_base_virt_addr = kdev->bar[2] + KL2_REG_SYSCON0_BAR2_BASE;
    for (i = 0; i < vdec_init_para.cores_num; i++) {
#ifdef CODEC_BRING_UP_TEST_ON
        vdec_init_para.vdec_reg_base_hw_addr[i] =
                kdev->bar_info.pcie_addr[2] + KL2_REG_DEC_BAR2_BASE + 0x4000 * (i + test_core_idx);
        vdec_init_para.vdec_reg_base_virt_addr[i] =
                kdev->bar[2] + KL2_REG_DEC_BAR2_BASE + 0x4000 * (i + test_core_idx);
#else
        vdec_init_para.vdec_reg_base_hw_addr[i] =
                kdev->bar_info.pcie_addr[2] + KL2_REG_DEC_BAR2_BASE + 0x4000 * i;
        vdec_init_para.vdec_reg_base_virt_addr[i] =
                kdev->bar[2] + KL2_REG_DEC_BAR2_BASE + 0x4000 * i;
#endif
    }

    pvdec_device = vzalloc(sizeof(vdecdev_t));
    CHECK_NULL_RET_VAL(pvdec_device, NULL);

    pvdec_device->kl2_dev                   = kl2_dev;
    pvdec_device->cores_num                 = vdec_init_para.cores_num;
    pvdec_device->syscon_reg_base_virt_addr = vdec_init_para.syscon0_reg_base_virt_addr;

    for (i = 0; i < pvdec_device->cores_num; i++) {
        pvdec_device->reg_base_virt_addr[i] = (u8 *)vdec_init_para.vdec_reg_base_virt_addr[i];
        pvdec_device->reg_base_hw_addr[i]   = vdec_init_para.vdec_reg_base_hw_addr[i];
        VLOGD("reg_base_virt_addr[%d] = %px, reg_base_hw_addr[%d] = 0x%llx\n", i,
              pvdec_device->reg_base_virt_addr[i], i, pvdec_device->reg_base_hw_addr[i]);
    }

    pvdec_device->hw_id = VDEC_REG_READ(0, 0);
    spin_lock_init(&pvdec_device->owner_lock);
    init_waitqueue_head(&pvdec_device->dec_wait_queue);
    init_waitqueue_head(&pvdec_device->hw_queue);
    sema_init(&pvdec_device->dec_core_sem, pvdec_device->cores_num);

    read_core_config(pvdec_device);
    reset_asic(pvdec_device);

    init_cache(pvdec_device);

    // decoder L2cache interrupt resp err workaround (all regs are already been init to 0)
    KL2_LOGI("decoder L2cache interrupt resp err workaround\n");
    for (i = 0; i < vdec_init_para.cores_num; i++) {
        iowrite32(0x00000010, (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x0018));
        iowrite32(0x0C000000, (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x0050));
        iowrite32(0x60000000, (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x000C));
        // XXX(miaotianxiang): decoder workaround由gddr改用L3
        //
        // R300 fw初始化gddr有概率失败，此处访问gddr可能超时，进而总线hang，造成CmpltTO和uce等严重后果
        //iowrite32(0x00000008,
        //          (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x02A0)); // MSB
        //iowrite32(0x00000010,
        //          (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x02A4)); // LSB
        //iowrite32(0x00000008,
        //          (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x0298)); // MSB
        //iowrite32(0x00000020,
        //          (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x029C)); // LSB
        iowrite32(0x00000000,
                  (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x02A0)); // MSB
        iowrite32(0xC0001010,
                  (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x02A4)); // LSB
        iowrite32(0x00000000,
                  (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x0298)); // MSB
        iowrite32(0xC0001020,
                  (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x029C)); // LSB
        iowrite32(0x00000010,
                  (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x00E8)); // reg58
        iowrite32(0x00000001, (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x0004));
    }
    usleep_range(5000, 10000);
    for (i = 0; i < vdec_init_para.cores_num; i++) {
        val = ioread32((void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x04BC));
        if (val != 2) {
            KL2_LOGE("decoder[%d] workaround error, swreg303 == 0x%x ( == 0x2 expected)\n", i, val);
        }

        val = ioread32((void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x0004));
        val &= 0xFFFFFFFE;
        if (val == 0x0) {
            KL2_LOGE("decoder[%d] workaround error, no interrupt highlight in swreg1\n", i);
        }

        iowrite32(0x00000000, (void *)(vdec_init_para.vdec_reg_base_virt_addr[i] + 0x0004));
    }
    KL2_LOGI("decoder workaround done!\n");

    return pvdec_device;
}

void vdec_uninit(pvdec_device_t pvdecdev)
{
    vdecdev_t *pvdec_device = pvdecdev;
    int        i            = 0;

    CHECK_NULL_RETURN(pvdec_device);

    reset_asic(pvdec_device);

    for (i = 0; i < pvdec_device->cores_num; i++) {
        VLOGD("core[%d] usage: %d\n", i, pvdec_device->core_usage[i]);
    }

    vcache_uninit(pvdec_device->cache_device);

    vfree(pvdec_device);

    return;
}

void vdec_isr(pvdec_device_t pvdecdev)
{
    vdecdev_t    *pvdec_device   = (vdecdev_t *)pvdecdev;
    unsigned long flags          = 0;
    unsigned int  handled        = 0;
    int           i              = 0;
    u32           irq_status_dec = 0;

    CHECK_NULL_RETURN(pvdec_device);

    spin_lock_irqsave(&pvdec_device->owner_lock, flags);

    for (i = 0; i < pvdec_device->cores_num; i++) {
        /* interrupt status register read */
        irq_status_dec = VDEC_REG_READ(i, VDEC_IRQ_STAT_DEC_OFF);

        if (irq_status_dec & VDEC_DEC_IRQ) {
            /* clear dec IRQ */
            irq_status_dec &= (~VDEC_DEC_IRQ);
            VDEC_REG_WRITE(i, VDEC_IRQ_STAT_DEC_OFF, irq_status_dec);
#ifdef VDEC_DEBUG
            // FIXME：delete
            VLOGD("decoder IRQ received! core %d\n", i);
#endif
            atomic64_inc(&pvdec_device->irq_rx);

            pvdec_device->dec_irq |= (1 << i);

            wake_up_interruptible_all(&pvdec_device->dec_wait_queue);
            handled++;
        }
    }

    spin_unlock_irqrestore(&pvdec_device->owner_lock, flags);

    // cache has interrupt?
    //vcache_isr(pvdec_device->cache_device);

    return;
}

long vdec_process_ioctl(pvdec_device_t pvdecdev, struct file *filp, unsigned int cmd,
                        unsigned long arg)
{
    vdecdev_t                 *pvdec_device   = (vdecdev_t *)pvdecdev;
    struct kl2_device *kl2_dev __maybe_unused = pvdec_device->kl2_dev;
    int                        err            = 0;
    long                       tmp            = 0;
#ifdef CLK_CFG
    unsigned long flags = 0;
#endif

#ifdef HW_PERFORMANCE
    struct timeval *end_time_arg = NULL;
#endif
    CHECK_NULL_RET_VAL(pvdec_device, -EINVAL);

    /*
    * extract the type and number bitfields, and don't decode
    * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
    */
    if (_IOC_TYPE(cmd) != IOCTL_VDEC_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > IOCTL_VDEC_IOC_MAXNR)
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
    }

    if (err)
        return -EFAULT;

#ifdef CLK_CFG
    spin_lock_irqsave(&pvdec_device->clk_lock, flags);
    if (pvdec_device->clk_cfg != NULL && !IS_ERR(pvdec_device->clk_cfg) &&
        (pvdec_device->is_clk_on == 0)) {
        VLOGD("turn on clock by user\n");
        if (clk_enable(pvdec_device->clk_cfg)) {
            spin_unlock_irqrestore(&pvdec_device->clk_lock, flags);
            return -EFAULT;
        } else
            pvdec_device->is_clk_on = 1;
    }
    spin_unlock_irqrestore(&pvdec_device->clk_lock, flags);
    mod_timer(&timer, jiffies + 10 * HZ); /*the interval is 10s*/
#endif

    VLOGD("ioctl cmd 0x%08x:\n", cmd);

    switch (cmd) {
    case IOCTL_VDEC_IOC_CLI: {
        __u32 id = 0;
        get_user(id, (__u32 *)arg);

        VLOGD("IOCTL_VDEC_IOC_CLI\n");

        if (id >= pvdec_device->cores_num) {
            return -EFAULT;
        }
        //disable_irq(hantrodec_data.irq[id]);
        //break;
        VLOGE("Unsupported\n");
        return -EINVAL;
    }
    case IOCTL_VDEC_IOC_STI: {
        __u32 id = 0;
        get_user(id, (__u32 *)arg);

        VLOGD("IOCTL_VDEC_IOC_STI\n");

        if (id >= pvdec_device->cores_num) {
            return -EFAULT;
        }
        //enable_irq(hantrodec_data.irq[id]);
        //break;
        VLOGE("Unsupported\n");
        return -EINVAL;
    }
    case IOCTL_VDEC_IOCGHWOFFSET: {
        __u32 id = 0;
        get_user(id, (__u32 *)arg);

        VLOGD("IOCTL_VDEC_IOCGHWOFFSET\n");

        if (id >= pvdec_device->cores_num) {
            return -EFAULT;
        }

        __put_user(pvdec_device->reg_base_hw_addr[id], (unsigned long *)arg);
        break;
    }
    case IOCTL_VDEC_IOCGHWIOSIZE: {
        __u32 id      = 0;
        __u32 io_size = VDEC_REGS_NUM * 4;
        get_user(id, (__u32 *)arg);

        VLOGD("IOCTL_VDEC_IOCGHWIOSIZE\n");

        if (id >= pvdec_device->cores_num) {
            return -EFAULT;
        }
        __put_user(io_size, (u32 *)arg);

        return 0;
    }
    case IOCTL_VDEC_IOC_MC_OFFSETS: {
        VLOGD("IOCTL_VDEC_IOC_MC_OFFSETS\n");

        tmp = copy_to_user((unsigned long *)arg, pvdec_device->reg_base_hw_addr,
                           sizeof(pvdec_device->reg_base_hw_addr));
        if (err) {
            VLOGE("copy_to_user failed, returned %li\n", tmp);
            return -EFAULT;
        }
        break;
    }
    case IOCTL_VDEC_IOC_MC_CORES:
        VLOGD("IOCTL_VDEC_IOC_MC_CORES\n");
        __put_user(pvdec_device->cores_num, (unsigned int *)arg);
        VLOGD("pvdec_device->cores_num=%d\n", pvdec_device->cores_num);
        break;
    case IOCTL_VDEC_IOCS_DEC_PUSH_REG: {
        struct vdec_core_desc core;

        VLOGD("IOCTL_VDEC_IOCS_DEC_PUSH_REG\n");

        /* get registers from user space*/
        tmp = copy_from_user(&core, (void *)arg, sizeof(struct vdec_core_desc));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        flush_regs(pvdec_device, &core);
        break;
    }
    case IOCTL_VDEC_IOCS_DEC_WRITE_REG: {
        struct vdec_core_desc core;

        VLOGD("IOCTL_VDEC_IOCS_DEC_WRITE_REG\n");

        /* get registers from user space*/
        tmp = copy_from_user(&core, (void *)arg, sizeof(struct vdec_core_desc));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        write_reg(pvdec_device, &core);
        break;
    }
    case IOCTL_VDEC_IOCS_PP_PUSH_REG: {
        struct vdec_core_desc core;

        VLOGD("IOCTL_VDEC_IOCS_PP_PUSH_REG\n");

        /* get registers from user space*/
        tmp = copy_from_user(&core, (void *)arg, sizeof(struct vdec_core_desc));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        //PPFlushRegs(pvdec_device, &core);
        //break;
        VLOGE("Unsupported\n");
        return -EINVAL;
    }
    case IOCTL_VDEC_IOCS_DEC_PULL_REG: {
        struct vdec_core_desc core;

        VLOGD("IOCTL_VDEC_IOCS_DEC_PULL_REG\n");

        /* get registers from user space*/
        tmp = copy_from_user(&core, (void *)arg, sizeof(struct vdec_core_desc));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        return dec_refresh_regs(pvdec_device, &core);
    }
    case IOCTL_VDEC_IOCS_DEC_READ_REG: {
        struct vdec_core_desc core;

        VLOGD("IOCTL_VDEC_IOCS_DEC_READ_REG\n");

        /* get registers from user space*/
        tmp = copy_from_user(&core, (void *)arg, sizeof(struct vdec_core_desc));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        return read_reg(pvdec_device, &core);
    }
    case IOCTL_VDEC_IOCS_PP_PULL_REG: {
        struct vdec_core_desc core;

        VLOGD("IOCTL_VDEC_IOCS_PP_PULL_REG\n");

        /* get registers from user space*/
        tmp = copy_from_user(&core, (void *)arg, sizeof(struct vdec_core_desc));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        //return PPRefreshRegs(pvdec_device, &core);
        VLOGE("Unsupported\n");
        return -EINVAL;
    }
    case IOCTL_VDEC_IOCH_DEC_RESERVE: {
        VLOGD("IOCTL_VDEC_IOCH_DEC_RESERVE\n");
        VLOGD("Reserve DEC core, format = 0x%lx\n", arg);
        return reserve_decoder(pvdec_device, filp, arg);
    }
    case IOCTL_VDEC_IOCT_DEC_RELEASE: {
        if ((arg >= pvdec_device->cores_num) || (pvdec_device->dec_owner[arg] != filp)) {
            VLOGE("bogus DEC release, core = %li\n", arg);
            return -EFAULT;
        }

        VLOGD("IOCTL_VDEC_IOCT_DEC_RELEASE\n");
        VLOGD("Release DEC, core = %li\n", arg);

        release_decoder(pvdec_device, arg);

        break;
    }
    case IOCTL_VDEC_IOCQ_PP_RESERVE:
        VLOGD("IOCTL_VDEC_IOCQ_PP_RESERVE\n");
        VLOGE("Unsupported\n");
        return -EINVAL;
        //return ReservePostProcessor(pvdec_device, filp);
    case IOCTL_VDEC_IOCT_PP_RELEASE: {
        /*
        if(arg != 0 || pp_owner[arg] != filp) {
            PDEBUG("bogus PP release %li\n", arg);
            return -EFAULT;
        }

        ReleasePostProcessor(pvdec_device, arg);
        break;*/
        VLOGD("IOCTL_VDEC_IOCT_PP_RELEASE\n");
        VLOGE("Unsupported\n");
        return -EINVAL;
    }
    case IOCTL_VDEC_IOCX_DEC_WAIT: {
        struct vdec_core_desc core;

        VLOGD("IOCTL_VDEC_IOCX_DEC_WAIT\n");

        /* get registers from user space */
        tmp = copy_from_user(&core, (void *)arg, sizeof(struct vdec_core_desc));
        if (tmp) {
            VLOGE("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        return wait_dec_ready_and_refresh_regs(pvdec_device, &core);
    }
    case IOCTL_VDEC_IOCX_PP_WAIT: {
        /*
        struct vdec_core_desc core;

        // get registers from user space
        tmp = copy_from_user(&core, (void*)arg, sizeof(struct vdec_core_desc));
        if (tmp) {
            VLOGD("copy_from_user failed, returned %li\n", tmp);
            return -EFAULT;
        }

        return WaitPPReadyAndRefreshRegs(pvdec_device, &core);*/
        VLOGD("IOCTL_VDEC_IOCX_PP_WAIT\n");
        VLOGE("Unsupported\n");
        return -EINVAL;
    }
    case IOCTL_VDEC_IOCG_CORE_WAIT: {
        int id = 0;

        VLOGD("IOCTL_VDEC_IOCG_CORE_WAIT\n");
        tmp = wait_core_ready(pvdec_device, filp, &id);
        if (tmp == 0) {
            __put_user(id, (int *)arg);
        }
        return tmp;
    }
    case IOCTL_VDEC_IOX_ASIC_ID: {
        u32 id = 0;

        VLOGD("IOCTL_VDEC_IOX_ASIC_ID\n");

        get_user(id, (u32 *)arg);

        if (id >= pvdec_device->cores_num) {
            return -EFAULT;
        }
        id = VDEC_REG_READ(id, 0);
        __put_user(id, (u32 *)arg);
        return 0;
    }
    case IOCTL_VDEC_IOCG_CORE_ID: {
        VLOGD("IOCTL_VDEC_IOCG_CORE_ID\n");
        VLOGD("Get DEC Core_id, format = 0x%lx\n", arg);
        return get_dec_core_id(pvdec_device, arg);
    }
    case IOCTL_VDEC_IOX_ASIC_BUILD_ID: {
        u32 id = 0, hw_id = 0;

        VLOGD("IOCTL_VDEC_IOX_ASIC_BUILD_ID\n");
        get_user(id, (u32 *)arg);

        if (id >= pvdec_device->cores_num) {
            return -EFAULT;
        }

        hw_id = VDEC_REG_READ(id, VDEC_HW_BUILD_ID_OFF);
        __put_user(hw_id, (u32 *)arg);
        return 0;
    }
    case IOCTL_VDEC_DEBUG_STATUS: {
        VLOGD("IOCTL_VDEC_DEBUG_STATUS\n");
        VLOGI("dec_irq     = 0x%08x \n", pvdec_device->dec_irq);
        //VLOGI("IRQs received/sent2user = %llu/ %llu\n", atomic64_read(&pvdec_device->irq_rx),
        //      atomic64_read(&pvdec_device->irq_tx));

        for (tmp = 0; tmp < pvdec_device->cores_num; tmp++) {
            VLOGI("dec_core[%li] %s\n", tmp,
                  pvdec_device->dec_owner[tmp] == NULL ? "FREE" : "RESERVED");
        }
        break;
    }
    default: {
        VLOGD("vcache_process_ioctl\n");
        return vcache_process_ioctl(pvdec_device->cache_device, filp, cmd, arg);
    }
    }

    return 0;
}

static int reset_dec_core(vdecdev_t *pvdec_device, int core_index)
{
    u32                regval               = SYSCON_REG_READ(VIDEO_RST_CTRL);
    struct kl2_device *kl2_dev              = NULL;
    u32                noc_video_no_pending = 0;

    if (core_index < 0 || core_index > 8) {
        VLOGW("invalid reset video dec core[%d]\n", core_index);
        return -1;
    }
    kl2_dev = pvdec_device->kl2_dev;
    if (NULL == kl2_dev) {
        VLOGW("invalid kl2_dev\n");
        return -1;
    }
    noc_video_no_pending = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon1_base + 0x284);
    if (!(noc_video_no_pending & BIT(core_index))) {
        VLOGW("dec core[%d] is pending, noc_video_no_pending=0x%x \n", core_index,
              noc_video_no_pending);
        return -1;
    }

    SYSCON_REG_WRITE(VIDEO_RST_CTRL, regval | BIT(4 + core_index));
    regval = SYSCON_REG_READ(VIDEO_RST_CTRL);
    if (!(regval & BIT(4 + core_index))) {
        VLOGW("reset video dec core[%d] failed!\n", core_index);
        return -1;
    }

    SYSCON_REG_WRITE(VIDEO_RST_CTRL, regval & ~BIT(4 + core_index));
    regval = SYSCON_REG_READ(VIDEO_RST_CTRL);
    if (regval & BIT(4 + core_index)) {
        VLOGW("reset video dec core[%d] low failed!\n", core_index);
        return -1;
    }

    return 0;
}

void vdec_on_file_release(pvdec_device_t pvdecdev, struct file *filp)
{
    vdecdev_t *pvdec_device = (vdecdev_t *)pvdecdev;
    int        n            = 0;
    u32        status       = 0;
    u32        loop         = 0;
    int        idx          = 0;

    CHECK_NULL_RETURN(pvdec_device);

    for (n = 0; n < pvdec_device->cores_num; n++) {
        if (pvdec_device->dec_owner[n] == filp) {
            status = VDEC_REG_READ(n, VDEC_IRQ_STAT_DEC_OFF);
            /* make sure HW is disabled */
            status |= VDEC_DEC_IRQ_DISABLE;
            status &= (~VDEC_DEC_IRQ);
            VDEC_REG_WRITE(n, VDEC_IRQ_STAT_DEC_OFF, status);

            if (status & VDEC_DEC_E) {
                /* abort decoder */
                while (status & 0x1) {
                    status = VDEC_REG_READ(n, VDEC_IRQ_STAT_DEC_OFF);
                    if (loop == 3000) {
                        pr_err("in vdec_on_file_release, abort dec error! status reg: 0x%x\n",
                               status);
                        if (((status >> 0) & 0x1) || ((status >> 1) & 0x1) ||
                            ((status >> 14) & 0x1)) {
                            pr_err("in vdec_on_file_release, stream error dected! status reg: 0x%x\n",
                                   status);
                            status &= (~VDEC_DEC_E);
                            VDEC_REG_WRITE(n, VDEC_IRQ_STAT_DEC_OFF, status);
                        }
                    } else if (loop >= 4000) {
                        if (reset_dec_core(pvdec_device, n) == 0) {
                            break;
                        }
                    } else if (loop >= 5000) {
                        break;
                    }
                    loop++;
                }
            }
            vcache_on_file_release(pvdec_device->cache_device, filp, n);
            release_decoder(pvdec_device, n);
        }

        /*reset shadow dec regs for every dec core*/
        for (idx = 0; idx < VDEC_REGS_NUM; idx++) {
            pvdec_device->shadow_dec_regs[n][idx] = 0xdeadbeef;
        }
    }

    VLOGD("dev closed\n");

    return;
}

#endif
