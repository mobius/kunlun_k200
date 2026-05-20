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

// Copyright 2018 Baidu Inc. All Rights Reserved.
// authors: Han Jinchen hanjinche@baidu.com
//
// xpu_hw.h - XPU basic hardware interface
//
#ifndef BAIDU_XPU_RUNTIME_MODULE_XPU_HW_H
#define BAIDU_XPU_RUNTIME_MODULE_XPU_HW_H

#include <linux/types.h>
#include "xpu_regs.h"
#include "xpu_drv.h"

#define align_up(v, alignment) (((v) + (alignment)-1) & (~((alignment)-1)))

u64 xpuhw_status(void __iomem *bars[]);

void xpuhw_setup_iatu_for_setup(struct xpu_device *xdev);
int  xpuhw_setup_iatu(struct xpu_device *dev);

int  xpuhw_train_hbm(struct xpu_pd *xpd);
int  xpuhw_train_hbm_zp(struct xpu_pd *xpd);
bool xpuhw_hbm_has_ucecc(struct xpu_pd *xpd);

void xpuhw_softreset(struct xpu_device *dev);

void xpuhw_migreset(struct xpu_device *dev);

void xpuhw_cluster_round_mode_init(struct xpu_pd *xpd);

// eDMA interface
int  xpuhw_edma_init(struct xpu_device *);
void xpuhw_edma_uninit(struct xpu_device *);
int  xpuhw_edma_read_locked(struct xpu_edma *, u64 dst, u64 src, size_t sz, u64 *cycles);
int  xpuhw_edma_write_locked(struct xpu_edma *, u64 dst, u64 src, size_t sz, u64 *cycles);
int  xpuhw_edma_rwl(struct xpu_device *xdev, u64 reg_addr, u32 value);
int  xpuhw_edma_rrl(struct xpu_device *xdev, u64 reg_addr, u32 *value);

// Performs a GM2GM memcpy
// returns -XPUERR_TIMEOUT if timed out
//         -XPUERR_UNEXPECT if XPU behaves unexpected
//         cycles_count if success
int xpuhw_ssedma_locked(struct xpu_ssedma *, u64 dst, u64 src, u64 sz, u64 *cycles);

// Get SSE version info
u32 xpuhw_sse_version(struct xpu_pd *xpd);
// Init SSE module
void xpuhw_sse_init(struct xpu_pd *xpd);

u32 xpuhw_sse_last_dispatched(struct xpu_pd *xpd, int tq_id);
u32 xpuhw_sse_last_finished(struct xpu_pd *xpd, int tq_id);
u64 xpuhw_sse_last_cycles(struct xpu_pd *xpd, int tq_id);

u64 xpuhw_sse_rc_error(struct xpu_pd *xpd);
// Read and clear sse tq's stack intr cnt
u32 xpuhw_sse_rc_tq_intr_cnt(struct xpu_pd *xpd, int tq_id);
int xpuhw_sse_enqueue_task_locked(struct xpu_task *xtask, int tq_id);

void xpu_sse_print_errmsg(struct xpu_pd *xpd, u64 err);

//////// Interrupt Controller ////////
// Initialize intc
void xpuhw_intc_init(struct xpu_device *xdev);

void xpuhw_intc_enable_msi(struct xpu_device *xdev);
void xpuhw_intc_disable_msi(struct xpu_device *xdev);
void xpuhw_intc_toggle_msi(struct xpu_device *xdev);

// INTC SCM (status, clear, mask) interface
// For all 64-bit INTC SCM interface (with suffix q), idx should be either 0 or 2

// Read STATUS
u32 xpuhw_intc_status(struct xpu_device *xdev, int idx);
u64 xpuhw_intc_statusq(struct xpu_device *xdev, int idx);

// Write to CLEAR
void xpuhw_intc_clear(struct xpu_device *xdev, int idx, u32 v);
void xpuhw_intc_clearq(struct xpu_device *xdev, int idx, u64 v);

// Write MASK
void xpuhw_intc_mask(struct xpu_device *xdev, int idx, u32 v);
void xpuhw_intc_maskq(struct xpu_device *xdev, int idx, u64 v);

// Remove given bits from MASK
void xpuhw_intc_unmask(struct xpu_device *xdev, int idx, u32 v);
void xpuhw_intc_unmaskq(struct xpu_device *xdev, int idx, u64 v);

// Add bits in {v} into MASK and clear them from STATUS
void xpuhw_intc_mask_and_clear(struct xpu_device *xdev, int idx, u32 v);
void xpuhw_intc_mask_and_clearq(struct xpu_device *xdev, int idx, u64 v);

int xpuhw_intc_mask_all(struct xpu_device *xdev);

// Query intr status of all the sse task queues
u32 xpuhw_intc_sse_status_all(struct xpu_device *xdev);

// Query intr status of a specific sse task queues
u32 xpuhw_intc_sse_status(struct xpu_device *xdev, int id);

// Clear intr status of a specific sse task queues
void xpuhw_intc_sse_clear(struct xpu_device *xdev, int id);

// Mask intr status of a specific sse task queues
void xpuhw_intc_sse_mask(struct xpu_device *xdev, int id);

// Unmask intr status of a specific sse task queues
void xpuhw_intc_sse_unmask(struct xpu_device *xdev, int id);

// Set UserInterrupt_idx
void xpuhw_intc_set_usrintr(struct xpu_device *xdev, int idx);

u32 xpuhw_get_xpu_token(struct xpu_pd *xpd, int idx);
u32 xpuhw_get_xpu_err(struct xpu_pd *xpd, int idx);
u32 xpuhw_get_cdnn_token(struct xpu_pd *xpd, int idx);
u32 xpuhw_get_cdnn_cl_err(struct xpu_pd *xpd, int idx);
u32 xpuhw_get_cdnn_err(struct xpu_pd *xpd, int idx);

#endif
