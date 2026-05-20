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

#define __FILENAME__ "exception.c"

#include "xpu_drv.h"

char *g_clstr_reason[32] = {
    /* 00 */ "UND_INSTRUCTION",
    /* 01 */ "RD_OVER_IBUF",
    /* 02 */ "LM_MULTI_READ",
    /* 03 */ "LM_MULTI_WRITE",
    /* 04 */ "LM_RDWR_CONFLICT",
    /* 05 */ "RD_OVER_LM",
    /* 06 */ "WR_OVER_LM",
    /* 07 */ "LM_POPCNT_RLM_NOALIGN",
    /* 08 */ "SIMD_RLM_NOALIGN",
    /* 09 */ "SIMD_WLM_NOALIGN",
    /* 10 */ "LD_LM_NOALIGN",
    /* 11 */ "ST_LM_NOALIGN",
    /* 12 */ "SM2LM_LMNOALIGN",
    /* 13 */ "LM2SM_LMNOALIGN",
    /* 14 */ "GM2LM_LMNOALIGN",
    /* 15 */ "LM2GM_LMNOALIGN",
    /* 16 */ "INT_DIV0",
    /* 17 */ "FP_DIV0",
    /* 18 */ "GM2SM_SMNOALIGN",
    /* 19 */ "SM2GM_SMNOALIGN",
    /* 20 */ "SM2LM_SMNOALIGN",
    /* 21 */ "LM2SM_SMNOALIGN",
    /* 22 */ "reserved",
    /* 23 */ "reserved",
    /* 24 */ "reserved",
    /* 25 */ "reserved",
    /* 26 */ "reserved",
    /* 27 */ "RBRESP",
    /* 28 */ "WBRESP",
    /* 29 */ "SM_CONFILICT",
    /* 30 */ "RD_OVER_SM",
    /* 31 */ "WR_OVER_SM",
};

char *g_cdnn_reason[32] = {
    /* 00 */ "DS_MUX_EXCP",
    /* 01 */ "DMAO_EXCP",
    /* 02 */ "RS_EXCP",
    /* 03 */ "EW_EXCP",
    /* 04 */ "MAC_EXCP",
    /* 05 */ "DS_1_EXCP",
    /* 06 */ "DS_0_EXCP",
    /* 07 */ "DMA_I1_EXCP",
    /* 08 */ "DMA_I0_EXCP",
    /* 09 */ "SCH_EXCP",
    /* 10 */ "reserved",
    /* 11 */ "reserved",
    /* 12 */ "reserved",
    /* 13 */ "reserved",
    /* 14 */ "reserved",
    /* 15 */ "reserved",
    /* 16 */ "reserved",
    /* 17 */ "reserved",
    /* 18 */ "reserved",
    /* 19 */ "reserved",
    /* 20 */ "reserved",
    /* 21 */ "reserved",
    /* 22 */ "reserved",
    /* 23 */ "reserved",
    /* 24 */ "reserved",
    /* 25 */ "reserved",
    /* 26 */ "reserved",
    /* 27 */ "reserved",
    /* 28 */ "reserved",
    /* 29 */ "reserved",
    /* 30 */ "reserved",
    /* 31 */ "reserved",
};

// pd0 debug信息
// pd1 信息：pd1.bit_idx = pd0.bit_idx + 16
//           pd1.debug_reg = pd0.debug_reg + 0x4000000
struct exception_info g_clstr_dbgs[12] = {
    { 0, 14, "cl-0", g_clstr_reason },   { 0, 15, "cl-1", g_clstr_reason },
    { 1, 13, "cl-2", g_clstr_reason },   { 1, 14, "cl-3", g_clstr_reason },
    { 0, 6, "sdcl-0", g_clstr_reason },  { 0, 8, "sdcl-1", g_clstr_reason },
    { 0, 10, "sdcl-2", g_clstr_reason }, { 0, 12, "sdcl-3", g_clstr_reason },
    { 0, 7, "sd-0", g_cdnn_reason },     { 0, 9, "sd-1", g_cdnn_reason },
    { 0, 11, "sd-2", g_cdnn_reason },    { 0, 13, "sd-3", g_cdnn_reason },
};
