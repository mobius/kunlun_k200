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
// xpu_intc.c - XPU INTC (interrupt controller) hw interface
//
#define __FILENAME__ "xpu_intc.c"

#include <linux/types.h>
#include "xpu_hw.h"
#include "xpu_regs.h"
#include "xpu_drv.h"

///////////////////////
// Interrupt Controller
///////////////////////

#define intc_regwl(reg, val)                                                                       \
    do {                                                                                           \
        reg_writel(xdev->intc_base + (reg), (val));                                                \
        LOGL1("" #reg "(%px) = 0x%x\n", xdev->intc_base + (reg), (u32)(val));                      \
    } while (0)
#define intc_regwq(val, base, reg)                                                                 \
    do {                                                                                           \
        reg_writeq(xdev->intc_base + (reg), (val));                                                \
        LOGL1("" #reg "(%px) = 0x%llx\n", xdev->intc_base + (reg), (u64)(val));                    \
    } while (0)

void xpuhw_intc_init(struct xpu_device *xdev)
{
    intc_regwl(RINTC_MASK(0), INTC_IGNORED_0);
    intc_regwl(RINTC_MASK(1), INTC_IGNORED_1);
    intc_regwl(RINTC_MASK(2), INTC_IGNORED_2);
    intc_regwl(RINTC_MASK(3), INTC_IGNORED_3);
    intc_regwl(RINTC_CLR(0), 0xffffffff);
    intc_regwl(RINTC_CLR(1), 0xffffffff);
    intc_regwl(RINTC_CLR(2), 0xffffffff);
    intc_regwl(RINTC_CLR(3), 0xffffffff);
    intc_regwl(RINT_MSI_EN, RINT_MSI_EN_TRUE);
}

inline void xpuhw_intc_disable_msi(struct xpu_device *xdev)
{
    reg_writel(xdev->intc_base + RINT_MSI_EN, RINT_MSI_EN_FALSE);
}

inline void xpuhw_intc_enable_msi(struct xpu_device *xdev)
{
    reg_writel(xdev->intc_base + RINT_MSI_EN, RINT_MSI_EN_TRUE);
}

inline void xpuhw_intc_toggle_msi(struct xpu_device *xdev)
{
    xpuhw_intc_disable_msi(xdev);
    xpuhw_intc_enable_msi(xdev);
}

inline u32 xpuhw_intc_status(struct xpu_device *xdev, int idx)
{
    return reg_readl(xdev->intc_base + RINTC_STAT(idx));
}

// Clear given bits {v} from STATUS_{idx}
inline void xpuhw_intc_clear(struct xpu_device *xdev, int idx, u32 v)
{
    reg_writel(xdev->intc_base + RINTC_CLR(idx), v);
}

// Add given bits {v} into mask of STATUS_{idx}
inline void xpuhw_intc_mask(struct xpu_device *xdev, int idx, u32 v)
{
    reg_writel(xdev->intc_base + RINTC_MASK(idx), reg_readl(xdev->intc_base + RINTC_MASK(idx)) | v);
}

// Remove given bits {v} from mask of STATUS_{idx}
inline void xpuhw_intc_unmask(struct xpu_device *xdev, int idx, u32 v)
{
    reg_writel(xdev->intc_base + RINTC_MASK(idx),
               reg_readl(xdev->intc_base + RINTC_MASK(idx)) & ~v);
}

// Add given bits {v} into mask of STATUS_{idx} and clear them from STATUS_{idx}
inline void xpuhw_intc_mask_and_clear(struct xpu_device *xdev, int idx, u32 v)
{
    xpuhw_intc_mask(xdev, idx, v);
    xpuhw_intc_clear(xdev, idx, v);
}

int xpuhw_intc_mask_all(struct xpu_device *xdev)
{
    u32 val;
    int i, err, ret = 0;

    for (i = 0; i < 4; ++i)
        intc_regwl(RINTC_MASK(i), ~0x0u);

    for (i = 0; i < 4; ++i) {
        err = xpu_poll_reg_timeout(xdev->intc_base + RINTC_MASK(i), val, val == ~0x0u, 100,
                                   1000 * 1000);
        if (err)
            ++ret;
    }

    return ret;
}

// For all 64-bit INTC SCM interface, idx should be either 0 or 2
// Read STATUS_{idx} {idx+1}
inline u64 xpuhw_intc_statusq(struct xpu_device *xdev, int idx)
{
    return reg_readq(xdev->intc_base + RINTC_STAT(idx));
}

// Clear given bits {v} from STATUS_{idx} {idx+1}
inline void xpuhw_intc_clearq(struct xpu_device *xdev, int idx, u64 v)
{
    reg_writeq(xdev->intc_base + RINTC_CLR(idx), v);
}

// Add given bits {v} into mask of STATUS_{idx} {idx+1}
inline void xpuhw_intc_maskq(struct xpu_device *xdev, int idx, u64 v)
{
    reg_writeq(xdev->intc_base + RINTC_MASK(idx), reg_readq(xdev->intc_base + RINTC_MASK(idx)) | v);
}

// Remove given bits {v} from mask of STATUS_{idx} {idx+1}
inline void xpuhw_intc_unmaskq(struct xpu_device *xdev, int idx, u64 v)
{
    reg_writeq(xdev->intc_base + RINTC_MASK(idx),
               reg_readq(xdev->intc_base + RINTC_MASK(idx)) & ~v);
}

// Add given bits {v} into mask of STATUS_{idx} {idx+1} and clear them from STATUS
inline void xpuhw_intc_mask_and_clearq(struct xpu_device *xdev, int idx, u64 v)
{
    xpuhw_intc_maskq(xdev, idx, v);
    xpuhw_intc_clearq(xdev, idx, v);
}

inline u32 xpuhw_intc_sse_status_all(struct xpu_device *xdev)
{
    return reg_readl(xdev->intc_base + RINTC_STAT(1)) & 0xff;
}

// Query intr status of a specific sse task queues
inline u32 xpuhw_intc_sse_status(struct xpu_device *xdev, int id)
{
    return ((reg_readl(xdev->intc_base + RINTC_STAT(1)) & 0xff) >> id) & 0x1;
}

// Clear intr status of a specific sse task queues
inline void xpuhw_intc_sse_clear(struct xpu_device *xdev, int id)
{
    reg_writel(xdev->intc_base + RINTC_CLR(1), 0x1 << id);
}

// Mask intr status of a specific sse task queues
inline void xpuhw_intc_sse_mask(struct xpu_device *xdev, int id)
{
    u32 value = reg_readl(xdev->intc_base + RINTC_MASK(1));
    value |= (0x1 << id);
    reg_writel(xdev->intc_base + RINTC_MASK(1), value);
}

// Unmask intr status of a specific sse task queues
inline void xpuhw_intc_sse_unmask(struct xpu_device *xdev, int id)
{
    u32 value = reg_readl(xdev->intc_base + RINTC_MASK(1));
    value &= ~(0x1 << id);
    reg_writel(xdev->intc_base + RINTC_MASK(1), value);
}

inline void xpuhw_intc_set_usrintr(struct xpu_device *xdev, int idx)
{
    intc_regwl(RINTC_SET(3), 0x1 << (16 + idx));
}
