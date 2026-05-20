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

#include "kl2/kl2_regs.h"

DEFINE_KL_BITWISE_DESC_BEGIN(KL2_REG_SSE_EXCEPT_STATUS)
DEFINE_KL_BITWISE_DESC_ENTRY(0, "stream_overflow_err", "stream overflow error")
DEFINE_KL_BITWISE_DESC_ENTRY(1, "desc_err", "descriptor error (nclusters==0 or ncores==0)")
DEFINE_KL_BITWISE_DESC_ENTRY(2, "wresp_err", "axi wresp error")
DEFINE_KL_BITWISE_DESC_END(KL2_REG_SSE_EXCEPT_STATUS)

DEFINE_KL_BITWISE_DESC_BEGIN(KL2_REG_SSE_EXCEPT_MASK)
DEFINE_KL_BITWISE_DESC_ENTRY(0, "stream_overflow_err", "stream overflow error")
DEFINE_KL_BITWISE_DESC_ENTRY(1, "desc_err", "descriptor error (nclusters==0 or ncores==0)")
DEFINE_KL_BITWISE_DESC_ENTRY(2, "wresp_err", "axi wresp error")
DEFINE_KL_BITWISE_DESC_END(KL2_REG_SSE_EXCEPT_MASK)

DEFINE_KL_BITWISE_DESC_BEGIN(KL2_REG_SSE_WRESP_ERR_MAP)
DEFINE_KL_BITWISE_DESC_ENTRY(0, "wresp_exception_map", "the last wresp error map")
DEFINE_KL_BITWISE_DESC_END(KL2_REG_SSE_WRESP_ERR_MAP)

DEFINE_KL_BITWISE_DESC_BEGIN(KL2_REG_SSE_SSE_BUSY_STATUS)
DEFINE_KL_BITWISE_DESC_ENTRY(0, "stream_queue_proc_0_busy", "stream_queue_proc[0] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(1, "stream_queue_proc_1_busy", "stream_queue_proc[1] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(2, "stream_queue_proc_2_busy", "stream_queue_proc[2] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(3, "stream_queue_proc_3_busy", "stream_queue_proc[3] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(4, "stream_queue_proc_4_busy", "stream_queue_proc[4] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(5, "stream_queue_proc_5_busy", "stream_queue_proc[5] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(6, "stream_queue_proc_6_busy", "stream_queue_proc[6] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(7, "stream_queue_proc_7_busy", "stream_queue_proc[7] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(8, "stream_queue_proc_8_busy", "stream_queue_proc[8] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(9, "stream_queue_proc_9_busy", "stream_queue_proc[9] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(10, "stream_queue_proc_10_busy", "stream_queue_proc[10] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(11, "stream_queue_proc_11_busy", "stream_queue_proc[11] is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(12, "sch_sel_u00_busy", "sch_sel_u00 is busy (user0 cluster)")
DEFINE_KL_BITWISE_DESC_ENTRY(13, "sch_sel_u01_busy", "sch_sel_u01 is busy (user0 sdnn)")
DEFINE_KL_BITWISE_DESC_ENTRY(14, "sch_sel_u10_busy", "sch_sel_u10 is busy (user1 cluster)")
DEFINE_KL_BITWISE_DESC_ENTRY(15, "sch_sel_u11_busy", "sch_sel_u11 is busy (user1 sdnn)")
DEFINE_KL_BITWISE_DESC_ENTRY(16, "sch_sel_u20_busy", "sch_sel_u20 is busy (user2 cluster)")
DEFINE_KL_BITWISE_DESC_ENTRY(17, "sch_sel_u21_busy", "sch_sel_u21 is busy (user2 sdnn)")
DEFINE_KL_BITWISE_DESC_ENTRY(18, "cid_proc_u00_busy", "cid_proc_u00 is busy (user0 cluster)")
DEFINE_KL_BITWISE_DESC_ENTRY(19, "cid_proc_u01_busy", "cid_proc_u01 is busy (user0 sdnn)")
DEFINE_KL_BITWISE_DESC_ENTRY(20, "cid_proc_u10_busy", "cid_proc_u10 is busy (user1 cluster)")
DEFINE_KL_BITWISE_DESC_ENTRY(21, "cid_proc_u11_busy", "cid_proc_u11 is busy (user1 sdnn)")
DEFINE_KL_BITWISE_DESC_ENTRY(22, "cid_proc_u20_busy", "cid_proc_u20 is busy (user2 cluster)")
DEFINE_KL_BITWISE_DESC_ENTRY(23, "cid_proc_u21_busy", "cid_proc_u21 is busy (user2 sdnn)")
DEFINE_KL_BITWISE_DESC_ENTRY(24, "task_issue_busy", "task_issue is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(25, "axi_write_busy", "axi_write is busy")
DEFINE_KL_BITWISE_DESC_END(KL2_REG_SSE_SSE_BUSY_STATUS)

DEFINE_KL_BITWISE_DESC_BEGIN(KL2_REG_SSE_XPU_BUSY_STATUS)
DEFINE_KL_BITWISE_DESC_ENTRY(0, "xpu_cluster_0_busy", "xpu_cluster_0 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(1, "xpu_cluster_1_busy", "xpu_cluster_1 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(2, "xpu_cluster_2_busy", "xpu_cluster_2 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(3, "xpu_cluster_3_busy", "xpu_cluster_3 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(4, "xpu_cluster_4_busy", "xpu_cluster_4 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(5, "xpu_cluster_5_busy", "xpu_cluster_5 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(6, "xpu_cluster_6_busy", "xpu_cluster_6 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(7, "xpu_cluster_7_busy", "xpu_cluster_7 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(8, "xpu_sdnn_0_busy", "xpu_sdnn_0 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(9, "xpu_sdnn_1_busy", "xpu_sdnn_1 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(10, "xpu_sdnn_2_busy", "xpu_sdnn_2 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(11, "xpu_sdnn_3_busy", "xpu_sdnn_3 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(12, "xpu_sdnn_4_busy", "xpu_sdnn_4 is busy")
DEFINE_KL_BITWISE_DESC_ENTRY(13, "xpu_sdnn_5_busy", "xpu_sdnn_5 is busy")
DEFINE_KL_BITWISE_DESC_END(KL2_REG_SSE_XPU_BUSY_STATUS)

DEFINE_KL_BITWISE_DESC_BEGIN(KL2_REG_SSE_EXCEPT_CLR)
DEFINE_KL_BITWISE_DESC_ENTRY(0, "stream_overflow_exception_clear",
                             "stream overflow exception clear")
DEFINE_KL_BITWISE_DESC_ENTRY(1, "descriptor_exception_clear", "descriptor exception clear")
DEFINE_KL_BITWISE_DESC_ENTRY(2, "wresp_exception_clear", "wresp exception clear")
DEFINE_KL_BITWISE_DESC_END(KL2_REG_SSE_EXCEPT_CLR)

DEFINE_KL_BITWISE_DESC_BEGIN(KL2_REG_SSE_QUEUE0_STATUS_3)
DEFINE_KL_BITWISE_DESC_ENTRY(0, "stream_overflow_err", "stream overflow error")
DEFINE_KL_BITWISE_DESC_ENTRY(1, "desc_err", "descriptor error (nclusters==0 or ncores==0)")
DEFINE_KL_BITWISE_DESC_ENTRY(2, "wresp_err", "axi wresp error")
DEFINE_KL_BITWISE_DESC_END(KL2_REG_SSE_QUEUE0_STATUS_3)
