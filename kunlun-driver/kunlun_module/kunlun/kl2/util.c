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

#include "kl2/util.h"
#include "kl2/kl2.h"

#ifdef REG_DEBUG

enum __kl2_iomem_range {
    __dma_range     = 0x200,
    __intc_range    = 0x10000,
    __sse_range     = 0x40000,
    __vac_range     = 0x1000,
    __aes_range     = 0x10000,
    __cluster_range = 0x340000,
    __sdnn_range    = 0x300000,
    __syscon0_range = 0x40000,
    __syscon1_range = 0x40000,
    __gddr_range    = 0x80000,
    __l3_range      = 0x4000000,
};

#define kl2_addr_in_range(addr, name)                                                              \
    ((addr >= kl2_dev->iomem_base.name##_base) &&                                                  \
     (addr < kl2_dev->iomem_base.name##_base + __##name##_range))

#define kl2_readl_debug_print(name)                                                                \
    if (kl2_addr_in_range(addr, name)) {                                                           \
        KL2_LOGI("RR " #name "_base+%08lx= %08x\n", addr - kl2_dev->iomem_base.name##_base, val);  \
    }

#define kl2_writel_debug_print(name)                                                               \
    if (kl2_addr_in_range(addr, name)) {                                                           \
        KL2_LOGI("RW " #name "_base+%08lx= %08x\n", addr - kl2_dev->iomem_base.name##_base, val);  \
    }

u32 kl2_readl(struct kl2_device *kl2_dev, const volatile void __iomem *addr)
{
    u32 val = readl(addr);
    kl2_readl_debug_print(dma);
    kl2_readl_debug_print(intc);
    kl2_readl_debug_print(sse);
    kl2_readl_debug_print(aes);
    kl2_readl_debug_print(vac);
    kl2_readl_debug_print(sdnn);
    kl2_readl_debug_print(cluster);
    kl2_readl_debug_print(syscon0);
    kl2_readl_debug_print(syscon1);
    kl2_readl_debug_print(gddr);
    kl2_readl_debug_print(l3);
    return val;
}

void kl2_writel(struct kl2_device *kl2_dev, u32 val, volatile void __iomem *addr)
{
    kl2_writel_debug_print(dma);
    kl2_writel_debug_print(intc);
    kl2_writel_debug_print(sse);
    kl2_writel_debug_print(aes);
    kl2_writel_debug_print(vac);
    kl2_writel_debug_print(sdnn);
    kl2_writel_debug_print(cluster);
    kl2_writel_debug_print(syscon0);
    kl2_writel_debug_print(syscon1);
    kl2_writel_debug_print(gddr);
    kl2_writel_debug_print(l3);
    return writel(val, addr);
}

#endif
