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
#include "xpurt_priv/ioctl_vdec_kl2.h"

// Video L2 Cache: include Read Cache and Write Burst Shaper.
// Read Cache can help to reduce the request frequency and save bandwidth,
// Shaper will align the write request and improve the efficiency.

//#define VCACHE_DEBUG

#define VCACHE_MAX_CORES (9 * 2)

struct kl2_device;

typedef enum { DIR_RD, DIR_WR, DIR_BI } driver_cache_dir;

typedef enum {
    VC8000D_0 = 0,
    VC8000D_1,
    VC8000D_2,
    VC8000D_3,
    VC8000D_4,
    VC8000D_5,
    VC8000D_6,
    VC8000D_7,
    VC8000D_8,
} cache_client_type;

typedef struct {
    cache_client_type client;
    u32               iosize;
    driver_cache_dir  dir;
} vcache_core_config;

typedef void *pvcache_device_t;

typedef struct {
    struct kl2_device *kl2_dev;

    void *vcache_reg_base_virt_addr[VCACHE_MAX_CORES];
    u64   vcache_reg_base_hw_addr[VCACHE_MAX_CORES];
    u32   cores_num;
} vcache_init_para_t;

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

pvcache_device_t vcache_init(vcache_init_para_t *pvcache_init);
void             vcache_uninit(pvcache_device_t pvcachedev);
void             vcache_isr(pvcache_device_t pvcachedev);
long vcache_process_ioctl(pvcache_device_t pvcachedev, struct file *filp, unsigned int cmd,
                          unsigned long arg);
void vcache_on_file_release(pvcache_device_t pvcachedev, struct file *filp, int dec_core_id);
