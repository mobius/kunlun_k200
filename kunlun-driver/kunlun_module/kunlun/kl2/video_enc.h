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

#pragma once

#include "xpurt_priv/ioctl_venc_kl2.h"
#include "kl2/video_perf.h"

//#define VENC_DEBUG
//#define OPTIMIZE_REG

// round-robin method to allocate the cores
#define BALANCE_CORE_USAGE

#define ASIC_SWREG_AMOUNT 479
#define ASIC_STATUS_ENABLE 0x001
#define ASIC_REG_INDEX_STATUS (20 / 4)

#define ENC_HW_ID1 0x48320100
#define ENC_HW_ID2 0x80006000
#define CORE_INFO_MODE_OFFSET 31
#define CORE_INFO_AMOUNT_OFFSET 28

struct kl2_device;

typedef struct {
    unsigned long base_addr;
    u32           iosize;
    u32           resouce_shared; // indicate the core share resources with other cores or not.
                                  // If 1, means cores can not work at the same time.
} SUBSYS_CONFIG;
typedef struct {
    SUBSYS_CONFIG    cfg;
    SUBSYS_CORE_INFO core_info;
} SUBSYS_DATA;

typedef struct {
    SUBSYS_DATA  subsys_data;            //config of each core,such as base addr, iosize,etc
    u32          hw_id;                  //VC8000E/VC8000EJ hw id to indicate project
    u32          subsys_id;              //subsys id for driver and sw internal use
    u32          is_valid;               //indicate this subsys is valid or not
    int          pid[CORE_MAX];          //indicate which process is occupying the subsys
    struct file *enc_owner[CORE_MAX];    //indicate who is occupying the subsys
    u32          is_reserved[CORE_MAX];  //indicate this subsys is occupied by user or not
    u32          irq_received[CORE_MAX]; //indicate which core receives irq
    u32          irq_status[CORE_MAX];   //IRQ status of each core
    char        *buffer;
    u32          buffsize;
    volatile u8 *hwregs;
    u32          usage; //statistics the amount times this subsys was used
} enc_data_t;

typedef struct {
    struct kl2_device *kl2_dev;

    void             *syscon_reg_base_virt_addr;
    enc_data_t        enc_data[VENC_MAX_SUBSYS];
    int               total_subsys_num;
    u32               enc_regs[VENC_MAX_SUBSYS][ASIC_SWREG_AMOUNT];
    u32               enc_regs_tmp[VENC_MAX_SUBSYS][ASIC_SWREG_AMOUNT];
    u32               cutree_regs[VENC_MAX_SUBSYS][ASIC_SWREG_AMOUNT];
    u32               cutree_regs_tmp[VENC_MAX_SUBSYS][ASIC_SWREG_AMOUNT];
    wait_queue_head_t hw_queue;
    wait_queue_head_t enc_wait_queue;
    struct semaphore  enc_core_sem;
    spinlock_t        owner_lock;

#ifdef BALANCE_CORE_USAGE
    atomic64_t reserved_count;
#endif
    video_perf_t *video_perf;
} enc_dev_t;

typedef void *pvenc_device_t;

typedef struct {
    void *syscon0_reg_base_virt_addr;
    void *reg_base_virt_addr[VENC_MAX_SUBSYS];
    u64   reg_base_hw_addr[VENC_MAX_SUBSYS];
    u32   total_subsys_num;
} venc_init_para_t;

pvenc_device_t venc_init(struct kl2_device *kl2_dev);
void           venc_uninit(pvenc_device_t pvencdev);
void           venc_isr(pvenc_device_t pvencdev);
long           venc_process_ioctl(pvenc_device_t pvencdev, struct file *filp, unsigned int cmd,
                                  unsigned long arg);
void           venc_on_file_release(pvenc_device_t pvencdev, struct file *filp);
