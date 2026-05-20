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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_UTIL_H
#define BAIDU_XPU_RUNTIME_MODULE_KL2_UTIL_H

#include <linux/io.h>

// 解注释下一行，打开REG_DEBUG打印
//#define REG_DEBUG

#ifdef REG_DEBUG

struct kl2_device;

#define kl2_readl kl2_readl
#define kl2_writel kl2_writel
u32  kl2_readl(struct kl2_device *kl2_dev, const volatile void __iomem *addr);
void kl2_writel(struct kl2_device *kl2_dev, u32 val, volatile void __iomem *addr);

#else

#define kl2_readl(kl2_dev, addr) readl(addr)
#define kl2_writel(kl2_dev, val, addr) writel(val, addr)

#endif

#endif
