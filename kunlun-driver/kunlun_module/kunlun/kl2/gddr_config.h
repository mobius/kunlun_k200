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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_GDDR_CONFIG_H
#define BAIDU_XPU_RUNTIME_MODULE_KL2_GDDR_CONFIG_H

#define GDDRCFG_NUM_16G 280
extern const unsigned int gddr_ctrl_cfg_16g[GDDRCFG_NUM_16G];

#define GDDRCFG_NUM_16G_ECC 280
extern const unsigned int gddr_ctrl_cfg_16g_ecc[GDDRCFG_NUM_16G_ECC];

#define GDDRCFG_NUM_16G_X8 280
extern const unsigned int gddr_ctrl_cfg_16g_x8[GDDRCFG_NUM_16G_X8];

static const unsigned int gddr_mem_cfg[15][2] = {
    { 0x1, 0xbff }, { 0x0, 0xa2e }, { 0x1, 0x380 }, { 0x2, 0xc00 }, { 0x3, 0x800 },
    { 0x4, 0x015 }, { 0x5, 0xf00 }, { 0x6, 0x0 },   { 0x7, 0x0 },   { 0x8, 0x300 },
    { 0x9, 0x0 },   { 0xa, 0x0 },   { 0xb, 0x000 }, { 0xc, 0x0 },   { 0xf, 0x0 },
};

#endif
