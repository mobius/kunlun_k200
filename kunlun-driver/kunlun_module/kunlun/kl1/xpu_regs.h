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
// xpu_regs.c - XPU Registers
//
#ifndef BAIDU_XPU_RUNTIME_MODULE_XPU_REGS_H
#define BAIDU_XPU_RUNTIME_MODULE_XPU_REGS_H

#define PD0_OFFSET 0x0
#define PD1_OFFSET 0x400000000ULL

#define RARM_BASE 0x00000000UL
#define RSRAM_BASE 0x20000000UL
#define RSYSCON_BASE 0x20040000UL
#define RINTC_BASE 0x20060000UL
#define ROTP_BASE 0x20070000UL

#define RSSE_BASE 0x20100000UL
#define RDMA_BASE 0x20140000UL

#define RXPU0_APB_BASE 0x20180000UL
#define RXPU1_APB_BASE 0x201A0000UL
#define RXPU2_APB_BASE 0x201C0000UL
#define RXPU3_APB_BASE 0x201E0000UL

#define RPCIE_APB_BASE 0x20200000UL

#define RCDNN0_APB_BASE 0x20600000UL
#define RCDNN0CL_APB_BASE 0x20640000UL
#define RCDNN1_APB_BASE 0x20680000UL
#define RCDNN1CL_APB_BASE 0x206C0000UL
#define RCDNN2_APB_BASE 0x20700000UL
#define RCDNN2CL_APB_BASE 0x20740000UL
#define RCDNN3_APB_BASE 0x20780000UL
#define RCDNN3CL_APB_BASE 0x207C0000UL

static const u64 RXPU_APB_BASE[] = {
    RXPU0_APB_BASE,
    RXPU1_APB_BASE,
    RXPU2_APB_BASE,
    RXPU3_APB_BASE,
};

static const u64 RCDNNCL_APB_BASE[] = {
    RCDNN0CL_APB_BASE,
    RCDNN1CL_APB_BASE,
    RCDNN2CL_APB_BASE,
    RCDNN3CL_APB_BASE,
};

#define RHBMCTRL_BASE 0x21000000UL
#define RHBMPHY_BASE 0x21400000UL

#define RCDNN0CL_BASE 0x80000000UL
#define RCDNN1CL_BASE 0x80100000UL
#define RCDNN2CL_BASE 0x80200000UL
#define RCDNN3CL_BASE 0x80300000UL
#define RXPU0_BASE 0x80400000UL
#define RXPU1_BASE 0x80500000UL
#define RXPU2_BASE 0x80600000UL
#define RXPU3_BASE 0x80700000UL

#define ROUND_MODEL0_OFFSET 0x8300UL
#define ROUND_MODEL1_OFFSET 0x8304UL
#define ROUND_MODEL2_OFFSET 0x8308UL

#define ROUND_MODEL0_VALUE 0x08208208UL
#define ROUND_MODEL1_VALUE 0x82082082UL
#define ROUND_MODEL2_VALUE 0x20820820UL

//////// SYSCON ////////

#define RSYSCON_G_R0 0x02280
#define RSYSCON_G_R1 0x02284
#define RSYSCON_G_R2 0x02288
#define RSYSCON_G_R3 0x02288
#define RSYSCON_G_R4 0x02290
#define RSYSCON_G_R5 0x02294
#define RSYSCON_G_R6 0x02298
#define RSYSCON_G_R7 0x0229C
#define RSYSCON_TEM0_STAT 0x00124
#define RSYSCON_TEM1_STAT 0x0012C
#define RSYSCON_TEM2_STAT 0x00134
#define RSYSCON_TEM3_STAT 0x0013C
#define RSYSCON_PPL0_DIV 0x00000
#define RSYSCON_PPL1_DIV 0x0000C
#define RSYSCON_PPL2_DIV 0x00018
#define RSYSCON_PPL3_DIV 0x02000
#define RSYSCON_PPL4_DIV 0x0200C
#define RSYSCON_PPL5_DIV 0x02018
#define RSYSCON_ZP_CUNIT_BITS 0x02244

#define HOST_CMD_I2C (0x1U)
#define HOST_CMD_HBM_TEMP_EN (0x1U << 1)
#define HOST_CMD_USER_RESET (0x1U << 2)
#define HOST_CMD_USER_RESET_DONE (0x1U << 3)
#define HOST_CMD_ONLINE_UPDATE (0x1U << 4)
#define HOST_CMD_HW_VERSION (0x1U << 5)
#define HOST_CMD_OTP_WRITE (0x1U << 6)
#define HOST_CMD_OTHER (0x1U << 7)

#define DEVICE_CMD_HOST_PAUSE (0x1U)
#define DEVICE_CMD_HOST_RESUME (0x1U << 1)
#define DEVICE_CMD_OTHER (0x1U << 2)

//////// OTP ////////
#define ROTP_SN_LO 0x10B0
#define ROTP_SN_HI 0x10AC
#define ROTP_SN_OLDFMT 0x1004
#define ROTP_PRODUCT_NUM 0x100C

//////// SSE ////////

#define RSSE_VERSION 0x00000
#define RSSE_TEST 0x00004
#define RSSE_INTR_STATUS 0x00008
#define RSSE_SCHEDULER 0x01000
#define RSSE_KERNEL_CACHE
#define RSSE_ERR_HANDLING_MODE
#define RSSE_CLDISABLE 0x01010
#define RSSE_READ_QOS
#define RSSE_WRITE_QOS
#define RSSE_XPU0_BASE_LO 0x03000
#define RSSE_XPU0_BASE_HI 0x03004
#define RSSE_XPU1_BASE_LO 0x03008
#define RSSE_XPU1_BASE_HI 0x0300C
#define RSSE_XPU2_BASE_LO 0x03010
#define RSSE_XPU2_BASE_HI 0x03014
#define RSSE_XPU3_BASE_LO 0x03018
#define RSSE_XPU3_BASE_HI 0x0301C
#define RSSE_CDNN0_BASE_LO 0x03020
#define RSSE_CDNN0_BASE_HI 0x03024
#define RSSE_CDNN1_BASE_LO 0x03028
#define RSSE_CDNN1_BASE_HI 0x0302C
#define RSSE_CDNN2_BASE_LO 0x03030
#define RSSE_CDNN2_BASE_HI 0x03034
#define RSSE_CDNN3_BASE_LO 0x03038
#define RSSE_CDNN3_BASE_HI 0x0303C

#define RSSE_TQ_INTR_CNT(tq_id) (0x02000 + (tq_id)*0x4)
#define RSSE_TQ_MAXID(tq_id) (0x20000 + (tq_id)*0x1000)
#define RSSE_TQ_TOKEN(tq_id) (0x20000 + (tq_id)*0x1000 + 0x4)
#define RSSE_TQ_KTYPE(tq_id) (0x20000 + (tq_id)*0x1000 + 0x8)
#define RSSE_TQ_KADDR_LO(tq_id) (0x20000 + (tq_id)*0x1000 + 0xC)
#define RSSE_TQ_KADDR_HI(tq_id) (0x20000 + (tq_id)*0x1000 + 0x10)
#define RSSE_TQ_KSIZE(tq_id) (0x20000 + (tq_id)*0x1000 + 0x14)

//////// INTC ////////

#define RINTC_STAT(set_id) (0x0000 + (set_id)*0x4)
#define RINTC_CLR(set_id) (0x0100 + (set_id)*0x4)
#define RINTC_SET(set_id) (0x0200 + (set_id)*0x4)
#define RINTC_MASK(set_id) (0x0300 + (set_id)*0x4)
#define RINTC_MCU_MASK(set_id) (0x0400 + (set_id)*0x4)

#define RINT_MSI_EN 0x0500
#define RINT_MSI_EN_TRUE 0x1
#define RINT_MSI_EN_FALSE 0x0

#define RINT_MCU_EN 0x0600
#define RINT_MCU_EN_TRUE 0x1
#define RINT_MCU_EN_FALSE 0x0

// INTC_REG_0
//  | bit     | description         | handle  |
//  | ------  | ------------------- | ------- |
//  | **0**   | cdnn_intr_0_0       | IGNORED |
//  | **1**   | cdnn_intr_0_1       | IGNORED |
//  | **2**   | cdnn_intr_0_2       | IGNORED |
//  | **3**   | cdnn_intr_0_3       | IGNORED |
//  | 4       | clstr_intr_0_0      | IGNORED |
//  | 5       | clstr_intr_0_1      | IGNORED |
//  | 6       | cdnn_clstr_expt_0_0 | EXCEPT  |
//  | 7       | cdnn_expt_0_0       | EXCEPT  |
//  | **8**   | cdnn_clstr_expt_0_1 | EXCEPT  |
//  | **9**   | cdnn_expt_0_1       | EXCEPT  |
//  | **10**  | cdnn_clstr_expt_0_2 | EXCEPT  |
//  | **11**  | cdnn_expt_0_2       | EXCEPT  |
//  | 12      | cdnn_clstr_expt_0_3 | EXCEPT  |
//  | 13      | cdnn_expt_0_3       | EXCEPT  |
//  | 14      | clstr_expt_0_0      | EXCEPT  |
//  | 15      | clstr_expt_0_1      | EXCEPT  |
//  | [31:16] | same as [15:0] but on PD1 |   |
#define INTC_IGNORED_0 0x003f003fULL
#define INTC_MCU_0 0x00000000ULL
#define INTC_RRM_0 0x00000000ULL
#define INTC_EXCEPT_0 0xffc0ffc0ULL
#define INTC_NORMAL_0 0x00000000ULL

// INTC_REG_1
// | bit     | description    | handle  |
// | ------  | -------------- | ------- |
// | **0**   | sse_intr_0_0   | NORMAL  |
// | **1**   | sse_intr_0_1   | NORMAL  |
// | **2**   | sse_intr_0_2   | NORMAL  |
// | **3**   | sse_intr_0_3   | NORMAL  |
// | 4       | sse_intr_0_4   | NORMAL  |
// | 5       | sse_intr_0_5   | NORMAL  |
// | 6       | sse_intr_0_6   | NORMAL  |
// | 7       | sse_intr_0_7   | NORMAL  |
// | **8**   | ssedma_fin_0   | IGNORED |
// | **9**   | sse_expt_0     | NORMAL  |
// | **10**  | ssedma_expt_0  | NORMAL  |
// | **11**  | clstr_intr_0_2 | IGNORED |
// | 12      | clstr_intr_0_3 | IGNORED |
// | 13      | clstr_expt_0_2 | EXCEPT  |
// | 14      | clstr_expt_0_3 | EXCEPT  |
// | 15      | reserved       | IGNORED |
// | [31:16] | same as [15:0] but on PD1 | |
#define INTC_IGNORED_1 0x99009900ULL
#define INTC_MCU_1 0x00000000ULL
#define INTC_RRM_1 0x00000000ULL
#define INTC_EXCEPT_1 0x60006000ULL
#define INTC_NORMAL_1 0x00ff00ffULL

#define INTC_SSE_EXCEPTION_10 0x0600060000000000ull

// INTC_REG_2
// | bit    | description    | handle  |
// | ------ | -------------- | ------- |
// | **0**  | noc_intr_0 | RRM |
// | **1**  | pm_intr | RRM |
// | **2**  | tmprtr_intr_0 | RRM |
// | **3**  | tmprtr_intr_1 | RRM |
// | 4      | tmprtr_intr_2 | RRM |
// | 5      | tmprtr_intr_3 | RRM |
// | 6      | noc_intr_1 | RRM |
// | 7      | reserved | IGNORED |
// | **8**  | reserved | IGNORED |
// | **9**  | reserved | IGNORED |
// | **10** | reserved | IGNORED |
// | **11** | reserved | IGNORED |
// | 12     | reserved | IGNORED |
// | 13     | reserved | IGNORED |
// | 14     | reserved | IGNORED |
// | 15     | reserved | IGNORED |
// | **16**  | edma_0 | IGNORED |
// | **17**  | edma_1 | IGNORED |
// | **18**  | edma_2 | IGNORED |
// | **19**  | edma_3 | IGNORED |
// | 20      | edma_4 | IGNORED |
// | 21      | edma_5 | IGNORED |
// | 22      | edma_6 | IGNORED |
// | 23      | edma_7 | IGNORED |
// | **24**  | edma_8 | IGNORED |
// | **25**  | edma_9 | IGNORED |
// | **26** | edma_10 | IGNORED |
// | **27** | edma_11 | IGNORED |
// | 28     | edma_12 | IGNORED |
// | 29     | edma_13 | IGNORED |
// | 30     | edma_14 | IGNORED |
// | 31     | edma_15  | IGNORED |
#define INTC_IGNORED_2 0xffffff80ULL
#define INTC_MCU_2 0x00000000ULL
#define INTC_RRM_2 0x0000007fULL
#define INTC_EXCEPT_2 0x00000000ULL
#define INTC_NORMAL_2 0x00000000ULL

// INTC_REG_3
// | bit    | description    | handle  |
// | ------ | -------------- | ------- |
// | **0**  | hbm_intr_or_c0_0 | IGNORED |
// | **1**  | hbm_intr_or_c1_0 | IGNORED |
// | **2**  | hbm_intr_or_c2_0 | IGNORED |
// | **3**  | hbm_intr_or_c3_0 | IGNORED |
// | 4      | hbm_intr_or_c4_0 | IGNORED |
// | 5      | hbm_intr_or_c5_0 | IGNORED |
// | 6      | hbm_intr_or_c6_0 | IGNORED |
// | 7      | hbm_intr_or_c7_0 | IGNORED |
// | **8**  | hbm_intr_or_c0_1 | IGNORED |
// | **9**  | hbm_intr_or_c1_1 | IGNORED |
// | **10** | hbm_intr_or_c2_1 | IGNORED |
// | **11** | hbm_intr_or_c3_1 | IGNORED |
// | 12     | hbm_intr_or_c4_1 | IGNORED |
// | 13     | hbm_intr_or_c5_1 | IGNORED |
// | 14     | hbm_intr_or_c6_1 | IGNORED |
// | 15     | hbm_intr_or_c7_1 | IGNORED |
// | **16**  | usr_0 | MCU FEEDBACK |
// | **17**  | usr_1 | MCU REQUEST  |
// | **18**  | usr_2 | MCU WARNING  |
// | **19**  | usr_3 | IGNORED |
// | 20      | usr_4 | IGNORED |
// | 21      | usr_5 | IGNORED |
// | 22      | usr_6 | IGNORED |
// | 23      | usr_7 | IGNORED |
// | **24**  | usr_8 | IGNORED |
// | **25**  | usr_9 | IGNORED |
// | **26** | usr_10 | IGNORED |
// | **27** | usr_11 | IGNORED |
// | 28     | usr_12 | TEST |
// | 29     | usr_13 | TEST |
// | 30     | usr_14 | TEST |
// | 31     | usr_15 trigger_pause | TEST |
#define INTC_IGNORED_3 0x00000000ull
#define INTC_MCU_3 0x00070000ULL
#define INTC_TEST_3 0xf0070000ULL
#define INTC_RRM_3 0x0ff80000ULL
#define INTC_EXCEPT_3 0x0000FFFFull
#define INTC_NORMAL_3 0x00000000ULL

#define INTC_IGNORED_10 ((INTC_IGNORED_1 << 32) | (INTC_IGNORED_0))
#define INTC_IGNORED_32 ((INTC_IGNORED_3 << 32) | (INTC_IGNORED_2))
#define INTC_MCU_32 (INTC_MCU_3 << 32)
#define INTC_TEST_32 (INTC_TEST_3 << 32)
#define INTC_RRM_10 ((INTC_RRM_1 << 32) | (INTC_RRM_0))
#define INTC_RRM_32 ((INTC_RRM_3 << 32) | (INTC_RRM_2))
#define INTC_EXCEPT_10 ((INTC_EXCEPT_1 << 32) | (INTC_EXCEPT_0))
#define INTC_EXCEPT_32 ((INTC_EXCEPT_3 << 32) | (INTC_EXCEPT_2))
#define INTC_NORMAL_10 ((INTC_NORMAL_1 << 32) | (INTC_NORMAL_0))
#define INTC_NORMAL_32 ((INTC_NORMAL_3 << 32) | (INTC_NORMAL_2))

#define INTC_EHBM_32 0x0000FFFF00000000ull

#endif
