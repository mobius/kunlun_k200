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

#include "xpurt_priv/ioctl_vdec_kl2.h"
#include "video_cache.h"
#include "kl2/video_perf.h"
//#define VDEC_DEBUG
//#define USE_SW_TIMEOUT
//#define SW_TIMEOUT_MILLIS (2000)
#define SW_TIMEOUT_MILLIS (60000)

// round-robin method to allocate the cores for decoding frames
#define BALANCE_CORE_USAGE

// Use buffer to cache the registers' value
// in order to improve the performance
#define VDEC_REG_ACCESS_OPT

// KL2 soc has 9 hardware decoder instances
#define VDEC_MAX_CORES (9)

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

    u8               *syscon_reg_base_virt_addr;
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
    atomic64_t       irq_rx;
    atomic64_t       irq_tx;
    u32              core_usage[VDEC_MAX_CORES];

#ifdef BALANCE_CORE_USAGE
    atomic64_t reserved_count;
#endif
    pvcache_device_t cache_device;

    video_perf_t *video_perf;
} vdecdev_t;

struct kl2_device;

typedef void *pvdec_device_t;

typedef struct {
    void *syscon0_reg_base_virt_addr;
    void *vdec_reg_base_virt_addr[VDEC_MAX_CORES];
    u64   vdec_reg_base_hw_addr[VDEC_MAX_CORES];
    u32   cores_num;
} vdec_init_para_t;

pvdec_device_t vdec_init(struct kl2_device *kl2_dev);
void           vdec_uninit(pvdec_device_t pvdecdev);
void           vdec_isr(pvdec_device_t pvdecdev);
long           vdec_process_ioctl(pvdec_device_t pvdecdev, struct file *filp, unsigned int cmd,
                                  unsigned long arg);
void           vdec_on_file_release(pvdec_device_t pvdecdev, struct file *filp);
