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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_DBGM_H
#define BAIDU_XPU_RUNTIME_MODULE_KL2_DBGM_H

#include <linux/types.h>

#define KL2_DBGM_SGDESC_DWORD 5

struct kl2_session;
struct kl2_userprocess;
struct kl2_device;

struct kl2_debug_master {
    struct {
        u64  noc_addr;
        u32  size;
        bool is_device;
    } sgdesc;

    u32 time_stamp;
    u32 stamp_interval;
    u32 lost_interval;
    u32 port;

    struct kl2_session *sess; // owner session
    bool                in_use;

    struct {
        bool enable;
        int  message;
    } mbox;

    struct mutex      lock;
    wait_queue_head_t wait_queue;
};

void kl2_dbgm_init(struct kl2_device *kl2_dev);

int kl2_dbgm_enable(struct kl2_session *sess, void __user *argp, u64 sz);
int kl2_dbgm_disable(struct kl2_session *sess);

int kl2_dbgm_start(struct kl2_userprocess *uproc);
int kl2_dbgm_stop(struct kl2_userprocess *uproc);

int kl2_dbgm_mbox_write(struct kl2_session *sess, void __user *argp, u64 sz);
int kl2_dbgm_mbox_read(struct kl2_session *sess, void __user *argp, u64 sz);

int kl2_dbgm_sleep_until_new_cmd(struct kl2_session *sess);

int kl2_dbgm_in_use(struct kl2_session *sess, void __user *argp, u64 sz);

#endif
