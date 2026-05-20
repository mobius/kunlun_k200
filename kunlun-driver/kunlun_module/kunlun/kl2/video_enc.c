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
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/vmalloc.h>

#include "xpurt_priv/venc_ewl_defs.h"
#include "kl2/kl2.h"
#include "kl2/kl2_regs.h"
#include "kl2/video_enc.h"
#include "xpurt_priv/ioctl_venc_kl2.h"

typedef struct {
    u32           core_type;
    unsigned long offset;
    u32           reg_size;
} CORE_CONFIG;

/*------------------------------------------------------------------------
*****************************PORTING LAYER********************************
-------------------------------------------------------------------------*/
#define RESOURCE_SHARED_INTER_SUBSYS                                                               \
    0                              /* 0:no resource sharing inter subsystems
                                                      1: existing resource sharing
                                                    */
#define SUBSYS_IO_SIZE (40000 * 4) /* bytes */

/*assume all subsystem share same config except base_addr*/
/*base_addr, iosize, resource_shared*/
const SUBSYS_CONFIG g_subsys_cfg = { 0, SUBSYS_IO_SIZE, RESOURCE_SHARED_INTER_SUBSYS };

/*here config all core in each subsystem*/
/*assume all subsystem share same core(s) type and number*/
/*core_type, offset, reg_size*/
const CORE_CONFIG g_core_cfg_array[] = {
    { CORE_VC8000E, 0, 500 * 4 },
    { CORE_CUTREE, 0x800, 500 * 4 },
};
/*------------------------------END-------------------------------------*/

#ifdef VENC_DEBUG
#define VLOGD(fmt, args...) KL2_LOGI("video_enc: " fmt, ##args)
#else
#define VLOGD(fmt, args...)
#endif
#define VLOGI(fmt, args...) KL2_LOGI("video_enc: " fmt, ##args)
#define VLOGW(fmt, args...) KL2_LOGW("video_enc: " fmt, ##args)
#define VLOGE(fmt, args...) KL2_LOGE("video_enc: " fmt, ##args)

#define VENC_REG_READ(subsys_id, reg_offset)                                                       \
    ioread32((void *)(pvenc_device->enc_data[subsys_id].hwregs + reg_offset))

#define VENC_REG_WRITE(subsys_id, reg_offset, val)                                                 \
    iowrite32(val, (void *)(pvenc_device->enc_data[subsys_id].hwregs + reg_offset))

#define SYSCON_REG_READ(reg_offset) ioread32(pvenc_device->syscon_reg_base_virt_addr + reg_offset)

#define SYSCON_REG_WRITE(reg_offset, val)                                                          \
    iowrite32(val, pvenc_device->syscon_reg_base_virt_addr + reg_offset)

/******************************************************************************/
static void ResetAsic(enc_dev_t *pvenc_device, int subsys_id, int core_type)
{
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    u32 reg_offset = pvenc_device->enc_data[subsys_id].subsys_data.core_info.offset[core_type];
    // u32 reg_val = 0;

    if (pvenc_device->enc_data[subsys_id].is_valid == 0) {
        return;
    }

    VLOGD("reset subsys_id = %d, core_type = %d\n", subsys_id, core_type);

    //reg_val = VENC_REG_READ(subsys_id, reg_offset + 0x14);
    //VENC_REG_WRITE(subsys_id, reg_offset + 0x14, reg_val & (~0x01));
    VENC_REG_WRITE(subsys_id, reg_offset + 0x14, 0);
}

static int CheckEncIrq(enc_dev_t *pvenc_device, u32 *core_info, u32 *irq_status)
{
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    unsigned long              flags          = 0;
    int                        rdy            = 0;
    u8                         core_type      = 0;
    u8                         subsys_idx     = 0;

    core_type  = (u8)(*core_info & 0x0F);
    subsys_idx = (u8)(*core_info >> 4);

    if (subsys_idx > pvenc_device->total_subsys_num - 1) {
        *core_info  = -1;
        *irq_status = 0;
        VLOGE("invalid subsys_idx: %d\n", subsys_idx);
        return 1;
    }

    VLOGD("check subsys[%d][%d]...\n", subsys_idx, core_type);

    spin_lock_irqsave(&pvenc_device->owner_lock, flags);

    if (pvenc_device->enc_data[subsys_idx].irq_received[core_type]) {
        /* reset the wait condition(s) */
        VLOGD("check subsys[%d][%d] irq ready\n", subsys_idx, core_type);
        pvenc_device->enc_data[subsys_idx].irq_received[core_type] = 0;
        rdy                                                        = 1;
        *core_info                                                 = subsys_idx;
        *irq_status = pvenc_device->enc_data[subsys_idx].irq_status[core_type];
    }

    spin_unlock_irqrestore(&pvenc_device->owner_lock, flags);

    return rdy;
}

static int WaitEncReady(enc_dev_t *pvenc_device, u32 *core_info, u32 *irq_status)
{
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    int                        ret            = 0;

    VLOGD("WaitEncReady\n");

    ret = wait_event_interruptible(pvenc_device->enc_wait_queue,
                                   CheckEncIrq(pvenc_device, core_info, irq_status));
    if (ret) {
        VLOGD("ENC wait_event_interruptible interrupted, ret = %d\n", ret);
        return -ERESTARTSYS;
    }

    return 0;
}

static long EncFlushRegs(enc_dev_t *pvenc_device, struct subsys_enc *subsys)
{
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    long                       ret            = 0;
    u32                        i              = 0;
    u32                        subsys_id      = subsys->id;
    u32                        reg_offset =
            pvenc_device->enc_data[subsys_id].subsys_data.core_info.offset[subsys->core_type];

    if (subsys->core_type != CORE_CUTREE) {
        ret = copy_from_user(pvenc_device->enc_regs[subsys_id], subsys->regs,
                             ASIC_SWREG_AMOUNT * 4);
        if (ret) {
            VLOGE("copy_from_user failed, returned %li\n", ret);
            return -EFAULT;
        }

        pvenc_device->enc_regs[subsys_id][ASIC_REG_INDEX_STATUS] &= ~ASIC_STATUS_ENABLE;

        for (i = 1; i < ASIC_SWREG_AMOUNT; i++) {
#ifdef OPTIMIZE_REG
            if (i == 80 || i == 82 || (i >= 183 && i <= 184) || (i >= 214 && i <= 223) ||
                (i >= 226 && i <= 234) || i == 287 || (i >= 290 && i <= 298) ||
                (i >= 308 && i <= 317) || i == 319 || (i >= 395 && i <= 396) ||
                i == 430 //RO
                //Regs After FRAME READY and not be written
                || i == 133 || i == 137 || (i >= 262 && i <= 264) || (i >= 283 && i <= 286)) {
                continue;
            } else if ((pvenc_device->enc_regs[subsys_id][i] !=
                        pvenc_device->enc_regs_tmp[subsys_id][i]) &&
                       i != ASIC_REG_INDEX_STATUS) {
                VENC_REG_WRITE(subsys_id, reg_offset + i * 4, pvenc_device->enc_regs[subsys_id][i]);
            } else if ((pvenc_device->enc_regs[subsys_id][i] ==
                        pvenc_device->enc_regs_tmp[subsys_id][i]) &&
                       (i == 1 || i == 7 || i == 9 || i == 45 || i == 113 || i == 136 || i == 185 ||
                        (i >= 195 && i <= 198) || i == 200 || i == 243 || i == 244 ||
                        (i >= 247 && i <= 249) || i == 321)) {
                VENC_REG_WRITE(subsys_id, reg_offset + i * 4, pvenc_device->enc_regs[subsys_id][i]);
            }
#else
            VENC_REG_WRITE(subsys_id, reg_offset + i * 4, pvenc_device->enc_regs[subsys_id][i]);
#endif
        }

        memcpy(pvenc_device->enc_regs_tmp[subsys_id], pvenc_device->enc_regs[subsys_id],
               ASIC_SWREG_AMOUNT * 4);

        // VLOGD("reg[ASIC_REG_INDEX_STATUS] = 0x%x\n", pvenc_device->enc_regs[subsys_id][ASIC_REG_INDEX_STATUS]);
        pvenc_device->enc_regs[subsys_id][ASIC_REG_INDEX_STATUS] |= ASIC_STATUS_ENABLE;
        VENC_REG_WRITE(subsys_id, reg_offset + ASIC_REG_INDEX_STATUS * 4,
                       pvenc_device->enc_regs[subsys_id][ASIC_REG_INDEX_STATUS]);
        // VLOGD("enable vce HW, reg[ASIC_REG_INDEX_STATUS] = 0x%x\n", pvenc_device->enc_regs[subsys_id][ASIC_REG_INDEX_STATUS]);
    } else {
        // CUTREE
        ret = copy_from_user(pvenc_device->cutree_regs[subsys_id], subsys->regs,
                             ASIC_SWREG_AMOUNT * 4);
        if (ret) {
            VLOGE("copy_from_user failed, returned %li\n", ret);
            return -EFAULT;
        }

        pvenc_device->cutree_regs[subsys_id][ASIC_REG_INDEX_STATUS] &= ~ASIC_STATUS_ENABLE;

        for (i = 1; i < ASIC_SWREG_AMOUNT; i++) {
#ifdef OPTIMIZE_REG
            if ((pvenc_device->cutree_regs[subsys_id][i] !=
                 pvenc_device->cutree_regs_tmp[subsys_id][i]) &&
                i != ASIC_REG_INDEX_STATUS) {
                VENC_REG_WRITE(subsys_id, reg_offset + i * 4,
                               pvenc_device->cutree_regs[subsys_id][i]);
            } else if ((pvenc_device->cutree_regs[subsys_id][i] ==
                        pvenc_device->cutree_regs_tmp[subsys_id][i]) &&
                       (i == 1 || (i >= 20 && i <= 37))) {
                VENC_REG_WRITE(subsys_id, reg_offset + i * 4,
                               pvenc_device->cutree_regs[subsys_id][i]);
            }
#else
            VENC_REG_WRITE(subsys_id, reg_offset + i * 4, pvenc_device->cutree_regs[subsys_id][i]);
#endif
        }

        memcpy(pvenc_device->cutree_regs_tmp[subsys_id], pvenc_device->cutree_regs[subsys_id],
               ASIC_SWREG_AMOUNT * 4);
        pvenc_device->cutree_regs[subsys_id][ASIC_REG_INDEX_STATUS] |= ASIC_STATUS_ENABLE;
        VENC_REG_WRITE(subsys_id, reg_offset + ASIC_REG_INDEX_STATUS * 4,
                       pvenc_device->cutree_regs[subsys_id][ASIC_REG_INDEX_STATUS]);
    }

    VLOGD("flushed registers on subsys %d, core_type=%d\n", subsys_id, subsys->core_type);
    return 0;
}

static int CheckCoreOccupation(enc_dev_t *pvenc_device, struct file *filp, u8 subsys_idx,
                               u32 core_type)
{
    int           ret   = 0;
    unsigned long flags = 0;

    core_type = (core_type == CORE_VC8000EJ ? CORE_VC8000E : core_type);

    spin_lock_irqsave(&pvenc_device->owner_lock, flags);
    if (!pvenc_device->enc_data[subsys_idx].is_reserved[core_type]) {
        pvenc_device->enc_data[subsys_idx].is_reserved[core_type] = 1;
        pvenc_device->enc_data[subsys_idx].pid[core_type]         = current->pid;
        pvenc_device->enc_data[subsys_idx].enc_owner[core_type]   = filp;
        ret                                                       = 1;
        pvenc_device->enc_data[subsys_idx].usage++;
        atomic_inc(&pvenc_device->video_perf->core_in_used);
    }

    spin_unlock_irqrestore(&pvenc_device->owner_lock, flags);

    return ret;
}

static int GetWorkableCore(enc_dev_t *pvenc_device, struct file *filp, u32 *core_info,
                           u32 *core_info_tmp)
{
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    int                        ret            = 0;
    u32                        i              = 0;
    u32                        cores          = 0;
    u8                         core_type      = 0;
    u32                        required_num   = 0;
    int                        subsys_id      = 0;
#ifdef BALANCE_CORE_USAGE
    long start_index = atomic64_inc_return(&pvenc_device->reserved_count) - 1;
#endif

    /*input core_info[32 bit]:
        mode[1bit](1:all 0:specified)+amount[3bit](the needing amount -1)+reserved+core_type[8bit].
      output core_info[32 bit]: the reserved core info to user space and defined as below.
        mode[1bit](1:all 0:specified)+amount[3bit](reserved total core num)+reserved+subsys_mapping[8bit]
    */
    cores        = *core_info;
    required_num = ((cores >> CORE_INFO_AMOUNT_OFFSET) & 0x7) + 1;
    core_type    = (u8)(cores & 0xFF);

    if (*core_info_tmp == 0) {
        *core_info_tmp = required_num << CORE_INFO_AMOUNT_OFFSET;
    } else {
        required_num = (*core_info_tmp >> CORE_INFO_AMOUNT_OFFSET);
    }

    VLOGD("GetWorkableCore:required_num=%d,core_info=%x\n", required_num, *core_info);

    if (required_num) {
        /* a valid free Core with specified core type */
        for (i = 0; i < pvenc_device->total_subsys_num; i++) {
#ifdef BALANCE_CORE_USAGE
            subsys_id = (i + start_index) % pvenc_device->total_subsys_num;
#else
            subsys_id = i;
#endif
            if (pvenc_device->enc_data[subsys_id].subsys_data.core_info.type_info &
                (1 << core_type)) {
                if (pvenc_device->enc_data[subsys_id].is_valid &&
                    CheckCoreOccupation(pvenc_device, filp, subsys_id, core_type)) {
                    *core_info_tmp = ((((*core_info_tmp >> CORE_INFO_AMOUNT_OFFSET) - 1)
                                       << CORE_INFO_AMOUNT_OFFSET) |
                                      (*core_info_tmp & 0x0FF));
                    *core_info_tmp = (*core_info_tmp | (1 << subsys_id));
                    if ((*core_info_tmp >> CORE_INFO_AMOUNT_OFFSET) == 0) {
                        ret            = 1;
                        *core_info     = (*core_info & 0xFFFFFF00) | (*core_info_tmp & 0xFF);
                        *core_info_tmp = 0;
                        required_num   = 0;
                        break;
                    }
                }
            }
        }
    } else {
        ret = 1;
    }

    VLOGD("*core_info = %x\n", *core_info);
    return ret;
}

static long ReserveEncoder(enc_dev_t *pvenc_device, struct file *filp, u32 *core_info)
{
    u32 core_info_tmp = 0;
    /*If HW resources are shared inter cores, just make sure only one is using the HW*/
    if (pvenc_device->enc_data[0].subsys_data.cfg.resouce_shared) {
        if (down_interruptible(&pvenc_device->enc_core_sem)) {
            return -ERESTARTSYS;
        }
    }

    /* lock a core that has specified core id*/
    if (wait_event_interruptible(pvenc_device->hw_queue,
                                 GetWorkableCore(pvenc_device, filp, core_info, &core_info_tmp) !=
                                         0)) {
        return -ERESTARTSYS;
    }

    return 0;
}

static void ReleaseEncoder(enc_dev_t *pvenc_device, struct file *filp, u32 *core_info)
{
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    unsigned long              flags          = 0;
    u8                         core_type      = 0;
    u8                         subsys_idx     = 0;

    subsys_idx = (u8)((*core_info & 0xF0) >> 4);
    core_type  = (u8)(*core_info & 0x0F);

    VLOGD("ReleaseEncoder:subsys_idx=%d, core_type=%x\n", subsys_idx, core_type);
    /* release specified subsys and core type */

    if (pvenc_device->enc_data[subsys_idx].subsys_data.core_info.type_info & (1 << core_type)) {
        core_type = (core_type == CORE_VC8000EJ ? CORE_VC8000E : core_type);
        spin_lock_irqsave(&pvenc_device->owner_lock, flags);
        VLOGD("subsys[%d].pid[%d]=%d,current->pid=%d\n", subsys_idx, core_type,
              pvenc_device->enc_data[subsys_idx].pid[core_type], current->pid);
        if (pvenc_device->enc_data[subsys_idx].is_reserved[core_type] &&
            pvenc_device->enc_data[subsys_idx].enc_owner[core_type] == filp) {
            pvenc_device->enc_data[subsys_idx].pid[core_type]          = -1;
            pvenc_device->enc_data[subsys_idx].enc_owner[core_type]    = NULL;
            pvenc_device->enc_data[subsys_idx].is_reserved[core_type]  = 0;
            pvenc_device->enc_data[subsys_idx].irq_received[core_type] = 0;
            pvenc_device->enc_data[subsys_idx].irq_status[core_type]   = 0;
            atomic_dec(&pvenc_device->video_perf->core_in_used);
            atomic_inc(&pvenc_device->video_perf->frames_num);

            //ResetAsic(pvenc_device, subsys_idx, core_type);
        }
        /*
        else if (pvenc_device->enc_data[subsys_idx].pid[core_type] != current->pid)
        {
            VLOGE("WARNING:pid(%d) is trying to release core reserved by pid(%d)\n",
                                        current->pid, pvenc_device->enc_data[subsys_idx].pid[core_type]);
        }
        else
        {
            // do nothing
        }
        */

        spin_unlock_irqrestore(&pvenc_device->owner_lock, flags);
    }

    wake_up_interruptible_all(&pvenc_device->hw_queue);

    if (pvenc_device->enc_data[subsys_idx].subsys_data.cfg.resouce_shared) {
        up(&pvenc_device->enc_core_sem);
    }

    return;
}

static int reset_enc_core(enc_dev_t *pvenc_device, int core_index)
{
    u32                regval               = SYSCON_REG_READ(VIDEO_RST_CTRL);
    struct kl2_device *kl2_dev              = NULL;
    u32                noc_video_no_pending = 0;

    ASSERT_RET_VAL(pvenc_device != NULL, -1);

    if (core_index < 0 || core_index > 2) {
        VLOGW("invalid reset video enc core[%d]\n", core_index);
        return -1;
    }
    kl2_dev = pvenc_device->kl2_dev;
    if (NULL == kl2_dev) {
        VLOGW("invalid kl2_dev\n");
        return -1;
    }
    noc_video_no_pending = kl2_readl(kl2_dev, kl2_dev->iomem_base.syscon1_base + 0x284);
    if (!(noc_video_no_pending & BIT(core_index + 9))) {
        VLOGW("enc core[%d] is pending, noc_video_no_pending=0x%x \n", core_index,
              noc_video_no_pending);
        return -1;
    }

    SYSCON_REG_WRITE(VIDEO_RST_CTRL, regval | BIT(core_index));
    regval = SYSCON_REG_READ(VIDEO_RST_CTRL);
    if (!(regval & BIT(core_index))) {
        VLOGW("reset video enc core[%d] failed!\n", core_index);
        return -1;
    }

    SYSCON_REG_WRITE(VIDEO_RST_CTRL, regval & ~BIT(core_index));
    regval = SYSCON_REG_READ(VIDEO_RST_CTRL);
    if (regval & BIT(core_index)) {
        VLOGW("reset video enc core[%d] low failed!\n", core_index);
        return -1;
    }

    return 0;
}

pvenc_device_t venc_init(struct kl2_device *kl2_dev)
{
    struct kl2_df_spec *spec;
    venc_init_para_t    venc_init_para;
    struct kl_device   *kdev;
    enc_dev_t          *pvenc_device     = NULL;
    int                 i                = 0;
    int                 j                = 0;
    u32                 VC8000E_core_idx = 0;
    u32                 hwid             = 0;
    u32                 hw_cfg           = 0;

    ASSERT_RET_VAL(kl2_dev != NULL, NULL);

    spec = &kl2_dev->spec;
    kdev = kl2_dev->kdev;
    CHECK_NULL_RET_VAL(spec, NULL);
    CHECK_NULL_RET_VAL(kdev, NULL);

    if (!is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
        kl2_writel(kl2_dev, 0x05, kl2_dev->iomem_base.syscon1_base + 0x30);
    }
#ifdef CODEC_BRING_UP_TEST_ON
    ASSERT_RET_VAL(kdev->num_vfs == 0, NULL); //only pf mode
    venc_init_para.total_subsys_num = 1;      //Only one hardware core is tested at a time
    int test_core_idx               = 0;      //core index is a variable:0~2
#else
    venc_init_para.total_subsys_num = bitcount(spec->enc_bits);
#endif

    ASSERT_RET_VAL(venc_init_para.total_subsys_num <= VENC_MAX_SUBSYS, NULL);

    venc_init_para.syscon0_reg_base_virt_addr = kdev->bar[2] + KL2_REG_SYSCON0_BAR2_BASE;
    for (i = 0; i < venc_init_para.total_subsys_num; i++) {
#ifdef CODEC_BRING_UP_TEST_ON
        venc_init_para.reg_base_hw_addr[i] =
                kdev->bar_info.pcie_addr[2] + KL2_REG_ENC_BAR2_BASE + 0x2000 * (i + test_core_idx);
        venc_init_para.reg_base_virt_addr[i] =
                kdev->bar[2] + KL2_REG_ENC_BAR2_BASE + 0x2000 * (i + test_core_idx);
#else
        venc_init_para.reg_base_hw_addr[i] =
                kdev->bar_info.pcie_addr[2] + KL2_REG_ENC_BAR2_BASE + 0x2000 * i;
        venc_init_para.reg_base_virt_addr[i] = kdev->bar[2] + KL2_REG_ENC_BAR2_BASE + 0x2000 * i;
#endif
    }

    pvenc_device = vzalloc(sizeof(enc_dev_t));
    CHECK_NULL_RET_VAL(pvenc_device, NULL);

    pvenc_device->kl2_dev                   = kl2_dev;
    pvenc_device->total_subsys_num          = venc_init_para.total_subsys_num;
    pvenc_device->syscon_reg_base_virt_addr = venc_init_para.syscon0_reg_base_virt_addr;

    for (i = 0; i < pvenc_device->total_subsys_num; i++) {
        pvenc_device->enc_data[i].subsys_data.cfg           = g_subsys_cfg;
        pvenc_device->enc_data[i].subsys_data.cfg.base_addr = venc_init_para.reg_base_hw_addr[i];

        VLOGD("reg_base_virt_addr[%d] = %px, reg_base_hw_addr[%d] = 0x%llx\n", i,
              venc_init_para.reg_base_virt_addr[i], i, venc_init_para.reg_base_hw_addr[i]);

        for (j = 0; j < sizeof(g_core_cfg_array) / sizeof(CORE_CONFIG); j++) {
            pvenc_device->enc_data[i].subsys_data.core_info.type_info |=
                    (1 << (g_core_cfg_array[j].core_type));
            pvenc_device->enc_data[i].subsys_data.core_info.offset[g_core_cfg_array[j].core_type] =
                    g_core_cfg_array[j].offset;
            pvenc_device->enc_data[i].subsys_data.core_info.regSize[g_core_cfg_array[j].core_type] =
                    g_core_cfg_array[j].reg_size;

            VLOGD("subsys_id = %d, core_type = %d, offset[core_type] = 0x%x, size = %u\n", i,
                  g_core_cfg_array[j].core_type,
                  pvenc_device->enc_data[i]
                          .subsys_data.core_info.offset[g_core_cfg_array[j].core_type],
                  pvenc_device->enc_data[i]
                          .subsys_data.core_info.regSize[g_core_cfg_array[j].core_type]);
        }

        pvenc_device->enc_data[i].subsys_id = i;
        pvenc_device->enc_data[i].hwregs    = (u8 *)venc_init_para.reg_base_virt_addr[i];

        /*read hwid and check validness and store it*/
        VC8000E_core_idx =
                GET_ENCODER_IDX(pvenc_device->enc_data[i].subsys_data.core_info.type_info);
        hwid = VENC_REG_READ(
                i, pvenc_device->enc_data[i].subsys_data.core_info.offset[VC8000E_core_idx] + 0);
        VLOGD("hwid = 0x%08x\n", hwid);

        /* check for encoder HW ID */
        if (((((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID1 >> 16) & 0xFFFF))) &&
            ((((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID2 >> 16) & 0xFFFF)))) {
            VLOGI("HW not found at %px\n",
                  (void *)pvenc_device->enc_data[i].subsys_data.cfg.base_addr);

            pvenc_device->enc_data[i].is_valid = 0;
            continue;
        }

        pvenc_device->enc_data[i].hw_id    = hwid;
        pvenc_device->enc_data[i].is_valid = 1;

        hw_cfg = VENC_REG_READ(
                i, pvenc_device->enc_data[i].subsys_data.core_info.offset[VC8000E_core_idx] + 320);
        pvenc_device->enc_data[i].subsys_data.core_info.type_info &= 0xFFFFFFFC;
        if (hw_cfg & 0x88000000) {
            pvenc_device->enc_data[i].subsys_data.core_info.type_info |= (1 << CORE_VC8000E);
        }
        if (hw_cfg & 0x00008000) {
            pvenc_device->enc_data[i].subsys_data.core_info.type_info |= (1 << CORE_VC8000EJ);
        }

        memset(pvenc_device->enc_regs, 0, VENC_MAX_SUBSYS * ASIC_SWREG_AMOUNT * 4);
        memset(pvenc_device->cutree_regs, 0, VENC_MAX_SUBSYS * ASIC_SWREG_AMOUNT * 4);
        memset(pvenc_device->enc_regs_tmp, 0, VENC_MAX_SUBSYS * ASIC_SWREG_AMOUNT * 4);
        memset(pvenc_device->cutree_regs_tmp, 0, VENC_MAX_SUBSYS * ASIC_SWREG_AMOUNT * 4);

        for (j = 0; j < sizeof(g_core_cfg_array) / sizeof(CORE_CONFIG); j++) {
            ResetAsic(pvenc_device, i, g_core_cfg_array[j].core_type);
        }

        VLOGD("HW at base <%px> with ID <0x%08x>\n",
              (void *)pvenc_device->enc_data[i].subsys_data.cfg.base_addr, hwid);
    }

    spin_lock_init(&pvenc_device->owner_lock);
    init_waitqueue_head(&pvenc_device->enc_wait_queue);
    init_waitqueue_head(&pvenc_device->hw_queue);
    sema_init(&pvenc_device->enc_core_sem, 1);

    return pvenc_device;
}

void venc_uninit(pvenc_device_t pvencdev)
{
    enc_dev_t                 *pvenc_device   = pvencdev;
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    int                        i              = 0;

    CHECK_NULL_RETURN(pvenc_device);

    for (i = 0; i < pvenc_device->total_subsys_num; i++) {
        VLOGD("subsys[%d] usage: %d\n", i, pvenc_device->enc_data[i].usage);
    }

    vfree(pvenc_device);
}

void venc_isr(pvenc_device_t pvencdev)
{
    enc_dev_t                 *pvenc_device   = pvencdev;
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    int                        i              = 0;
    int                        j              = 0;
    u32                        irq_status     = 0;
    unsigned long              flags          = 0;
    int                        core_type      = 0;
    unsigned long              reg_offset     = 0;
    u32                        hwId = 0, majorId = 0, wClr = 0;

    CHECK_NULL_RETURN(pvenc_device);

    for (i = 0; i < pvenc_device->total_subsys_num; i++) {
        for (j = 0; j < sizeof(g_core_cfg_array) / sizeof(CORE_CONFIG); j++) {
            core_type  = g_core_cfg_array[j].core_type;
            reg_offset = pvenc_device->enc_data[i].subsys_data.core_info.offset[core_type];

            irq_status = VENC_REG_READ(i, reg_offset + 0x04);

            //VLOGD("subsys_id = %d, core_type = %d, reg_offset = %d, irq_status = 0x%x\n",
            //        i, core_type, reg_offset, irq_status);

            /*If core is not reserved by any user, but irq is received, just clean it*/
            spin_lock_irqsave(&pvenc_device->owner_lock, flags);
            if (!pvenc_device->enc_data[i].is_reserved[core_type]) {
                if (irq_status & 0x01) {
                    VLOGI("venc_isr:received IRQ but core is not reserved!, core_type=%d\n",
                          core_type);

                    /*  Disable HW when buffer over-flow happen
                     *  HW behavior changed in over-flow
                     *    in-pass, HW cleanup HWIF_ENC_E auto
                     *    new version:  ask SW cleanup HWIF_ENC_E when buffer over-flow
                     */
                    if (irq_status & 0x20) {
                        VENC_REG_WRITE(i, reg_offset + 0x14, 0);
                    }

                    /* clear all IRQ bits. (hwId >= 0x80006100) means IRQ is cleared by writting 1 */
                    hwId    = VENC_REG_READ(i, reg_offset + 0);
                    majorId = (hwId & 0x0000FF00) >> 8;
                    wClr    = (majorId >= 0x61) ? irq_status : (irq_status & (~0x1FD));
                    VENC_REG_WRITE(i, reg_offset + 0x04, wClr);
                }
                spin_unlock_irqrestore(&pvenc_device->owner_lock, flags);
                continue;
            }
            spin_unlock_irqrestore(&pvenc_device->owner_lock, flags);

            if (irq_status & 0x01) {
                VLOGD("venc_isr:received IRQ!\n");
                VLOGD("irq_status of subsys %d core %d is:%x\n",
                      pvenc_device->enc_data[i].subsys_id, core_type, irq_status);

                /*  Disable HW when buffer over-flow happen
                *  HW behavior changed in over-flow
                *    in-pass, HW cleanup HWIF_ENC_E auto
                *    new version:  ask SW cleanup HWIF_ENC_E when buffer over-flow
                */
                if (irq_status & 0x20) {
                    VENC_REG_WRITE(i, reg_offset + 0x14, 0);
                }

                /* clear all IRQ bits. (hwId >= 0x80006100) means IRQ is cleared by writting 1 */
                hwId    = VENC_REG_READ(i, reg_offset + 0);
                majorId = (hwId & 0x0000FF00) >> 8;
                wClr    = (majorId >= 0x61) ? irq_status : (irq_status & (~0x1FD));
                VENC_REG_WRITE(i, reg_offset + 0x04, wClr);

                spin_lock_irqsave(&pvenc_device->owner_lock, flags);
                pvenc_device->enc_data[i].irq_received[core_type] = 1;
                pvenc_device->enc_data[i].irq_status[core_type]   = irq_status & (~0x01);
                spin_unlock_irqrestore(&pvenc_device->owner_lock, flags);

                wake_up_interruptible_all(&pvenc_device->enc_wait_queue);
            }

            /* hw bus error*/
            if (irq_status & 0x08) {
                VLOGD("venc_isr:received HW bus error reset encode core !\n");
                reset_enc_core(pvenc_device, i);
            }
        }
    }

    return;
}

long venc_process_ioctl(pvenc_device_t pvencdev, struct file *filp, unsigned int cmd,
                        unsigned long arg)
{
    enc_dev_t                 *pvenc_device   = pvencdev;
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    int                        err            = 0;
    long                       ret            = 0;

    CHECK_NULL_RET_VAL(pvenc_device, -EINVAL);

    /*
     * extract the type and number bitfields, and don't encode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != IOCTL_VENC_IOC_MAGIC) {
        return -ENOTTY;
    }

    if ((_IOC_TYPE(cmd) == IOCTL_VENC_IOC_MAGIC && _IOC_NR(cmd) > IOCTL_VENC_IOC_MAXNR)) {
        return -ENOTTY;
    }

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
    if (err) {
        return -EFAULT;
    }

    VLOGD("ioctl cmd 0x%08x:\n", cmd);

    switch (cmd) {
    case IOCTL_VENC_IOCGHWOFFSET: {
        u32 id = 0;

        VLOGD("IOCTL_VENC_IOCGHWOFFSET\n");
        get_user(id, (u32 *)arg);
        if (id >= pvenc_device->total_subsys_num) {
            return -EFAULT;
        }

        __put_user(pvenc_device->enc_data[id].subsys_data.cfg.base_addr, (unsigned long *)arg);
    } break;
    case IOCTL_VENC_IOCGHWIOSIZE: {
        u32 id      = 0;
        u32 io_size = 0;

        VLOGD("IOCTL_VENC_IOCGHWIOSIZE\n");
        get_user(id, (u32 *)arg);
        if (id >= pvenc_device->total_subsys_num) {
            return -EFAULT;
        }

        io_size = pvenc_device->enc_data[id].subsys_data.cfg.iosize;
        __put_user(io_size, (u32 *)arg);

        return 0;
    }
    case IOCTL_VENC_IOCG_CORE_NUM:
        VLOGD("IOCTL_VENC_IOCG_CORE_NUM\n");
        __put_user(pvenc_device->total_subsys_num, (unsigned int *)arg);
        break;
    case IOCTL_VENC_IOCG_CORE_INFO: {
        u32              idx = 0;
        SUBSYS_CORE_INFO in_data;

        VLOGD("IOCTL_VENC_IOCG_CORE_INFO\n");
        ret = copy_from_user(&in_data, (void *)arg, sizeof(SUBSYS_CORE_INFO));
        if (ret) {
            VLOGE("copy_from_user failed, returned %li\n", ret);
            return -EFAULT;
        }
        idx = in_data.type_info;
        if (idx > pvenc_device->total_subsys_num - 1) {
            return -1;
        }

        ret = copy_to_user((void *)arg, &pvenc_device->enc_data[idx].subsys_data.core_info,
                           sizeof(SUBSYS_CORE_INFO));
        if (ret) {
            VLOGE("copy_to_user failed, returned %li\n", ret);
            return -EFAULT;
        }
    } break;
    case IOCTL_VENC_IOCH_ENC_RESERVE: {
        u32 core_info = 0;
        int ret       = 0;

        VLOGD("IOCTL_VENC_IOCH_ENC_RESERVE\n");
        get_user(core_info, (u32 *)arg);
        ret = ReserveEncoder(pvenc_device, filp, &core_info);
        if (ret == 0) {
            __put_user(core_info, (u32 *)arg);
        }
        return ret;
    } break;
    case IOCTL_VENC_IOCH_ENC_RELEASE: {
        u32 core_info = 0;
        VLOGD("IOCTL_VENC_IOCH_ENC_RELEASE\n");
        get_user(core_info, (u32 *)arg);
        VLOGD("Release ENC Core\n");
        ReleaseEncoder(pvenc_device, filp, &core_info);
    } break;
    case IOCTL_VENC_IOCG_CORE_WAIT: {
        u32 core_info  = 0;
        u32 irq_status = 0;

        get_user(core_info, (u32 *)arg);

        VLOGD("IOCTL_VENC_IOCG_CORE_WAIT, subsys_id=%d, core_type=%d\n", (u8)(core_info >> 4),
              (u8)(core_info & 0x0F));

        ret = WaitEncReady(pvenc_device, &core_info, &irq_status);
        if (ret == 0) {
            __put_user(irq_status, (unsigned int *)arg);
            return core_info;
        } else {
            // ret is -ERESTARTSYS, user space will restart this syscall again.
            // so don't destroy the content in arg.
            return ret;
        }
    } break;
    case IOCTL_VENC_IOCS_ENC_PUSH_REG: {
        struct subsys_enc subsys;

        VLOGD("IOCTL_VENC_IOCS_ENC_PUSH_REG\n");

        ret = copy_from_user(&subsys, (void *)arg, sizeof(struct subsys_enc));
        if (ret) {
            VLOGE("copy_from_user failed, returned %li\n", ret);
            return -EFAULT;
        }

        EncFlushRegs(pvenc_device, &subsys);
    } break;
    default:
        VLOGE("Unknown cmd\n");
        return -EINVAL;
    }
    return 0;
}

void venc_on_file_release(pvenc_device_t pvencdev, struct file *filp)
{
    enc_dev_t                 *pvenc_device   = pvencdev;
    struct kl2_device *kl2_dev __maybe_unused = pvenc_device->kl2_dev;
    u32                        core_type      = 0;
    u32                        i              = 0;
    unsigned long              flags          = 0;

    CHECK_NULL_RETURN(pvenc_device);

    VLOGD("dev closed\n");

    for (i = 0; i < pvenc_device->total_subsys_num; i++) {
        for (core_type = 0; core_type < sizeof(pvenc_device->enc_data[0].is_reserved) /
                                                sizeof(pvenc_device->enc_data[0].is_reserved[0]);
             core_type++) {
            spin_lock_irqsave(&pvenc_device->owner_lock, flags);
            if ((pvenc_device->enc_data[i].is_reserved[core_type] == 1) &&
                (pvenc_device->enc_data[i].enc_owner[core_type] == filp)) {
                pvenc_device->enc_data[i].pid[core_type]          = -1;
                pvenc_device->enc_data[i].enc_owner[core_type]    = NULL;
                pvenc_device->enc_data[i].is_reserved[core_type]  = 0;
                pvenc_device->enc_data[i].irq_received[core_type] = 0;
                pvenc_device->enc_data[i].irq_status[core_type]   = 0;

                //ResetAsic(pvenc_device, i, core_type);
                atomic_dec(&pvenc_device->video_perf->core_in_used);
                atomic_inc(&pvenc_device->video_perf->frames_num);
                VLOGD("release reserved core\n");
            }
            spin_unlock_irqrestore(&pvenc_device->owner_lock, flags);
        }
    }

    wake_up_interruptible_all(&pvenc_device->hw_queue);

    if (pvenc_device->enc_data[0].subsys_data.cfg.resouce_shared) {
        up(&pvenc_device->enc_core_sem);
    }

    return;
}

#endif
