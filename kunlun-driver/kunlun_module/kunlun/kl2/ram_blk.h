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

#ifndef BAIDU_XPU_RUNTIME_MODULE_RAM_BLK_H
#define BAIDU_XPU_RUNTIME_MODULE_RAM_BLK_H

#include <linux/types.h>
#include <linux/blkdev.h>

#define MAX_XPU_MINOR_NUM 16
#define RB_FIRST_MINOR 0
#define RB_MINOR_CNT 16
#define RB_SECTOR_SIZE 512

struct dma_engine;

int  kl_rb_init(struct dma_engine *dma_engine, int xpu_minor, u64 rb_dev_addr, u64 rb_size);
void kl_rb_cleanup(int xpu_minor);

struct rb_device {
    /* minor */
    int xpu_minor;
    /* major */
    int major;
    /* dev addr */
    u64 dev_addr;
    /* Size is the size of the device (in sectors) */
    u64 size;
    /* For exclusive access to our request queue */
    spinlock_t lock;
    /* Our request queue */
    struct request_queue *rb_queue;
    /* This is kernel's representation of an individual disk device */
    struct gendisk *rb_disk;
    /* dma engine */
    struct dma_engine *dma_engine;
};

#endif
