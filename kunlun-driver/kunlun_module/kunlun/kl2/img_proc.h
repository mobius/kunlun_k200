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

#include <linux/fs.h>
#include "xpurt_priv/ioctl_img_proc_kl2.h"
#include "xpurt_priv/imgproc_defs.h"
#include "kl2/video_perf.h"

typedef struct {
    struct kl2_device *kl2_dev;

    u8               *syscon_reg_base_virt_addr;
    u8               *imgproc_reg_base_virt_addr[IMGPROC_MAX_CORES];
    u32               cores_num; /* total available cores num on the device */
    spinlock_t        owner_lock;
    wait_queue_head_t wait_queue[IMGPROC_MAX_CORES];
    int               got_event[IMGPROC_MAX_CORES];
    struct file      *owner[IMGPROC_MAX_CORES];
    struct semaphore  core_sem;
    int               imgproc_irq;
    u32               int_status[IMGPROC_MAX_CORES];     // record interrupt status
    u32               raw_int_status[IMGPROC_MAX_CORES]; // record RAW interrupt status
    volatile u32      hw_busy[IMGPROC_MAX_CORES];        // indicate if hw is busy
    //atomic_t           irq_rx;
    //atomic_t           irq_tx;
    u32 core_usage[IMGPROC_MAX_CORES];

#ifdef BALANCE_CORE_USAGE
    atomic64_t reserved_count;
#endif
    video_perf_t *video_perf;
} imgprocdev_t;

// round-robin method to allocate the cores
#define BALANCE_CORE_USAGE
#define IMG_PROC_MAX_CORES (6)

typedef void *pimgproc_device_t;

typedef struct {
    void *syscon0_reg_base_virt_addr;
    void *img_proc_reg_base_virt_addr[IMG_PROC_MAX_CORES];
    u64   img_proc_reg_base_hw_addr[IMG_PROC_MAX_CORES];
    u32   cores_num;
} imgproc_init_para_t;

pimgproc_device_t imgproc_init(struct kl2_device *kl2_dev);
void              imgproc_uninit(pimgproc_device_t pimgprocdev);
void              imgproc_isr(pimgproc_device_t pimgprocdev);
long imgproc_process_ioctl(pimgproc_device_t pimgprocdev, struct file *filp, unsigned int cmd,
                           unsigned long arg);
void imgproc_on_file_release(pimgproc_device_t pimgprocdev, struct file *filp);
