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
// xpu_hw.c - XPU Hardware Interface
// Implement interface for real XPU hardware communication
//
#define __FILENAME__ "xpu_hw.c"
#include <asm/io.h>
#include <linux/delay.h>
#include "xpu_regs.h"
#include "xpu_drv.h"
#include "xpu_hw.h"

#define regwl(base, reg, val)                                                                      \
    do {                                                                                           \
        reg_writel((base) + (reg), (val));                                                         \
        LOGL1("" #reg " (%px) = 0x%x\n", (base) + (reg), (u32)(val));                              \
    } while (0)

#define regwq(base, reg, val)                                                                      \
    do {                                                                                           \
        reg_writeq((base) + (reg), (val));                                                         \
        LOGL1("" #reg " (%px) = 0x%llx\n", (base) + (reg), (u64)(val));                            \
    } while (0)

// query error status
u64 xpuhw_status(void __iomem *bars[])
{
    return 0;
}

#define IATU_BASE 0x100000

#define IATU_INBOUND_CTRL1 0x100
#define IATU_INBOUND_CTRL2 0x104
#define IATU_INBOUND_LOWER_BASE_ADDR 0x108
#define IATU_INBOUND_UPPER_BASE_ADDR 0x10c
#define IATU_INBOUND_LIMIT_ADDR 0x110
#define IATU_INBOUND_LOWER_TARGET_ADDR 0x114
#define IATU_INBOUND_UPPER_TARGET_ADDR 0x118

#define IATU_INBOUND_CTRL2_BAR_NUM(v) (((v)&0x7) << 8)
#define IATU_INBOUND_CTRL2_REGION_EN (0x1U << 31)
#define IATU_INBOUND_CTRL2_BAR_MATCH_MODE (0x1U << 30)
#define IATU_INBOUND_CTRL2_MEM_MATCH_MODE (0x0U << 30)
#define IATU_INBOUND_CTRL2_VFBAR_EN (0x1U << 26)
#define IATU_INBOUND_CTRL2_VFMATCH_EN (0x1U << 20)

// base for each region:
//   0 : 0x100000
//   1 : 0x100200
//   2 : 0x100400
//   3 : 0x100600
#define IATU_REG(region_id, reg_offset) (IATU_BASE + (region_id)*0x200 + (reg_offset))

static int xpuhw_iatu_set_barmode(void __iomem *bars[], int bar, u64 addr)
{
    if ((bars == NULL) || (bars[4] == NULL))
        return -EINVAL;

    if ((bar != 0) && (bar != 2))
        return -EINVAL;

    regwl(bars[4], IATU_REG(bar, IATU_INBOUND_LOWER_TARGET_ADDR), low32(addr));
    regwl(bars[4], IATU_REG(bar, IATU_INBOUND_UPPER_TARGET_ADDR), high32(addr));
    regwl(bars[4], IATU_REG(bar, IATU_INBOUND_CTRL1), 0x0);
    regwl(bars[4], IATU_REG(bar, IATU_INBOUND_CTRL2),
          IATU_INBOUND_CTRL2_REGION_EN | IATU_INBOUND_CTRL2_BAR_MATCH_MODE |
                  IATU_INBOUND_CTRL2_BAR_NUM(bar));

    return 0;
}

void xpuhw_setup_iatu_for_setup(struct xpu_device *xdev)
{
    struct kl_device *kdev = xdev->kdev;

    xpuhw_iatu_set_barmode(kdev->bar, 0, 0x20000000ul);

#define SET_BASE(var, reg) xdev->var = kdev->bar[0] + (reg - 0x20000000ul)
    SET_BASE(syscon_base, RSYSCON_BASE);
    SET_BASE(otp_base, ROTP_BASE);
    SET_BASE(intc_base, RINTC_BASE);
#undef SET_BASE

    udelay(1000);
}

static struct iatu_region_info g_default_iatu = {
    .region_type = KL1_REGION_TYPE_INBOUND,
    .region_en   = 0x3,
    .match_mode  = { KL1_MATCHMODE_MEM, KL1_MATCHMODE_MEM, KL1_MATCHMODE_MEM, KL1_MATCHMODE_MEM },
    .bar         = { 0, 0, 0, 0 },
    .size        = { 0x0200000, 0x0100000, 0, 0 },
    // here base actually serves as offset to bar
    .base   = { 0x0, 0x0200000, 0x0, 0x0 },
    .target = { 0x020000000ULL, 0x420100000ULL, 0x0, 0x0 }
};

static int xpuhw_iatu_set_inbound_region(struct xpu_device *xdev, int region, u32 bar, u64 offset,
                                         u64 target, u32 size)
{
    struct kl_device *kdev             = xdev->kdev;
    void __iomem     *region_ctrl_base = NULL;
    u64               region_base      = 0;

    if ((xdev == NULL) || (xdev->iatu_base == NULL))
        return -XPUERR_INVALID_PARAM;

    if ((region < 0) || (region >= KL1_INBOUND_REGION_NUM))
        return -XPUERR_INVALID_PARAM;

    region_ctrl_base = xdev->iatu_base + region * 0x200;
    region_base      = kdev->bar_info.bus_addr[bar] + offset;

    // regsion must not across a 4G border
    if (high32(low32(region_base) + size) != 0)
        return -XPUERR_INVALID_PARAM;

    regwl(region_ctrl_base, IATU_INBOUND_LOWER_BASE_ADDR, low32(region_base));
    regwl(region_ctrl_base, IATU_INBOUND_UPPER_BASE_ADDR, high32(region_base));
    regwl(region_ctrl_base, IATU_INBOUND_LOWER_TARGET_ADDR, low32(target));
    regwl(region_ctrl_base, IATU_INBOUND_UPPER_TARGET_ADDR, high32(target));
    regwl(region_ctrl_base, IATU_INBOUND_LIMIT_ADDR, (low32(region_base) + size - 1));
    regwl(region_ctrl_base, IATU_INBOUND_CTRL1, 0x0);
    regwl(region_ctrl_base, IATU_INBOUND_CTRL2,
          IATU_INBOUND_CTRL2_REGION_EN | IATU_INBOUND_CTRL2_MEM_MATCH_MODE);

    xdev->iatu_inbound_info.region_en |= (0x1 << region);
    xdev->iatu_inbound_info.match_mode[region] = KL1_MATCHMODE_MEM;
    xdev->iatu_inbound_info.bar[region]        = bar;
    xdev->iatu_inbound_info.size[region]       = size;
    xdev->iatu_inbound_info.base[region]       = region_base;
    xdev->iatu_inbound_info.target[region]     = target;

    return 0;
}

int xpuhw_iatu_set_inbound_regions(struct xpu_device *xdev, const struct iatu_region_info *info)
{
    int i, err;
    for (i = 0; i < KL1_MAX_REGION_NUM; ++i) {
        err = xpuhw_iatu_set_inbound_region(xdev, i, info->bar[i], info->base[i], info->target[i],
                                            info->size[i]);
        if (err)
            return err;
    }

    msleep(1000);
    return 0;
}

static u32 __find_bar_for_region_base(struct xpu_device *xdev, u64 base)
{
    struct kl_device *kdev = xdev->kdev;
    struct bar_info  *bi   = &kdev->bar_info;
    int               i;
    for (i = 0; i < PCIE_BAR_NUM; ++i)
        if ((bi->bus_addr[i] <= base) && (base < bi->bus_addr[i] + bi->bar_size[i]))
            return i;
    return -1;
}

static int xpuhw_iatu_get_inbound_region(struct xpu_device *xdev, int region)
{
    struct kl_device *kdev             = xdev->kdev;
    void __iomem     *region_ctrl_base = NULL;
    u64               region_base      = 0;
    u64               target           = 0;
    u32               bar;
    u32               size = 0;
    u32               hi   = 0;
    u32               lo   = 0;

    if ((xdev == NULL) || (xdev->iatu_base == NULL))
        return -XPUERR_INVALID_PARAM;

    if ((region < 0) || (region >= KL1_INBOUND_REGION_NUM))
        return -XPUERR_INVALID_PARAM;

    region_ctrl_base = xdev->iatu_base + region * 0x200;

    lo = reg_readl(region_ctrl_base + IATU_INBOUND_CTRL2);

    if (lo == ~0x0)
        return -XPUERR_PCIE;

    if ((lo & IATU_INBOUND_CTRL2_REGION_EN) == 0) {
        xdev->iatu_inbound_info.region_en &= ~(0x1 << region);
        return 0;
    }

    xdev->iatu_inbound_info.region_en |= (0x1 << region);

    if (lo & IATU_INBOUND_CTRL2_BAR_MATCH_MODE) {
        xdev->iatu_inbound_info.match_mode[region] = KL1_MATCHMODE_BAR;

        bar = (lo >> 8) & 0x7;
        if (bar != 0 && bar != 2) {
            LOGW("Invalid BAR %d for region %d\n", bar, region);
            return -XPUERR_PCIE;
        }

        region_base = kdev->bar_info.bus_addr[bar];
        size        = kdev->bar_info.bar_size[bar];
    } else {
        xdev->iatu_inbound_info.match_mode[region] = KL1_MATCHMODE_MEM;

        lo          = reg_readl(region_ctrl_base + IATU_INBOUND_LOWER_BASE_ADDR);
        hi          = reg_readl(region_ctrl_base + IATU_INBOUND_UPPER_BASE_ADDR);
        region_base = makeu64(hi, lo);

        lo   = reg_readl(region_ctrl_base + IATU_INBOUND_LIMIT_ADDR);
        size = lo - low32(region_base) + 1;

        bar = __find_bar_for_region_base(xdev, region_base);
        if (bar != 0 && bar != 2) {
            LOGW("Cannot find BAR for region %d addr 0x%llx\n", region, region_base);
            return -XPUERR_PCIE;
        }
    }

    lo     = reg_readl(region_ctrl_base + IATU_INBOUND_LOWER_TARGET_ADDR);
    hi     = reg_readl(region_ctrl_base + IATU_INBOUND_UPPER_TARGET_ADDR);
    target = makeu64(hi, lo);

    xdev->iatu_inbound_info.bar[region]    = bar;
    xdev->iatu_inbound_info.base[region]   = region_base;
    xdev->iatu_inbound_info.size[region]   = size;
    xdev->iatu_inbound_info.target[region] = target;

    LOGL4("region%d base  = 0x%llx\n", region, region_base);
    LOGL4("region%d target= 0x%llx\n", region, target);
    LOGL4("region%d bar   = %u\n", region, bar);
    LOGL4("region%d size  = 0x%x\n", region, size);

    return 0;
}

int xpuhw_iatu_get_inbound_regions(struct xpu_device *xdev)
{
    int i, err;
    for (i = 0; i < KL1_MAX_REGION_NUM; ++i)
        if ((err = xpuhw_iatu_get_inbound_region(xdev, i)) != 0)
            return err;
    return 0;
}

static void *__iatu_xpu2cpu(struct xpu_device *xdev, u64 addr)
{
    struct iatu_region_info *info = &xdev->iatu_inbound_info;
    struct kl_device        *kdev = xdev->kdev;
    struct bar_info         *bi   = &kdev->bar_info;
    int                      i    = 0;

    for (i = 0; i < KL1_MAX_REGION_NUM; ++i) {
        if (((info->region_en >> i) & 0x1) && (addr >= info->target[i]) &&
            (addr - info->target[i] < info->size[i])) {
            u32 bar_id                = info->bar[i];
            u64 addr_to_region_offset = addr - info->target[i];
            u64 region_to_bar_offset  = info->base[i] - bi->bus_addr[bar_id];
            u64 addr_to_bar_offset    = region_to_bar_offset + addr_to_region_offset;
            LOGL4("Mapping region%d 0x%llx --> 0x%llx (bar%d + 0x%llx)\n", i, addr,
                  (u64)kdev->bar[bar_id] + addr_to_bar_offset, bar_id, addr_to_bar_offset);
            return kdev->bar[bar_id] + addr_to_bar_offset;
        }
    }

    return NULL;
}

#define BASE_SET(base, reg)                                                                        \
    LOGL4("Set " #base "\n");                                                                      \
    (base) = __iatu_xpu2cpu(xdev, (reg));                                                          \
    if (!(base)) {                                                                                 \
        LOGE("Cannot find mapping for " #reg);                                                     \
        return -XPUERR_NODEV;                                                                      \
    }

int xpuhw_setup_iatu(struct xpu_device *xdev)
{
    struct kl_device *kdev = xdev->kdev;
    int               err  = 0;

    if ((xdev == NULL) || (kdev->bar[4] == NULL))
        return -XPUERR_INVALID_PARAM;

    if (kdev->bar[0] == NULL) {
        LOGW("should setup BARs before iATU\n");
        return -XPUERR_UNEXPECT;
    }

    err = xpuhw_iatu_set_inbound_regions(xdev, &g_default_iatu);
    if (err)
        return err;

    // setup register base for each XPU module
    BASE_SET(xdev->syscon_base, RSYSCON_BASE);
    BASE_SET(xdev->intc_base, RINTC_BASE);
    BASE_SET(xdev->otp_base, ROTP_BASE);

    // setup PD0 register base
    BASE_SET(xdev->xpd[0].sse_base, RSSE_BASE);
    BASE_SET(xdev->xpd[0].ssedma.base, RDMA_BASE);

    // setup PD1 register base
    BASE_SET(xdev->xpd[1].sse_base, PD1_OFFSET + RSSE_BASE);
    BASE_SET(xdev->xpd[1].ssedma.base, PD1_OFFSET + RDMA_BASE);

    return 0;
}

// TODO:
void xpuhw_softreset(struct xpu_device *dev)
{
}

// TODO:
void xpuhw_migreset(struct xpu_device *dev)
{
}

void xpuhw_cluster_round_mode_init(struct xpu_pd *xpd)
{
    struct xpu_device *xdev = xpd->xdev;
    u64                base;
    int                i;
    for (i = 0; i < XPD_CLUSTER_COUNT; ++i) {
        base = xpd->rbase + RXPU_APB_BASE[i];
        xpuhw_edma_rwl(xdev, base + ROUND_MODEL0_OFFSET, ROUND_MODEL0_VALUE);
        xpuhw_edma_rwl(xdev, base + ROUND_MODEL1_OFFSET, ROUND_MODEL1_VALUE);
        xpuhw_edma_rwl(xdev, base + ROUND_MODEL2_OFFSET, ROUND_MODEL2_VALUE);

        base = xpd->rbase + RCDNNCL_APB_BASE[i];
        xpuhw_edma_rwl(xdev, base + ROUND_MODEL0_OFFSET, ROUND_MODEL0_VALUE);
        xpuhw_edma_rwl(xdev, base + ROUND_MODEL1_OFFSET, ROUND_MODEL1_VALUE);
        xpuhw_edma_rwl(xdev, base + ROUND_MODEL2_OFFSET, ROUND_MODEL2_VALUE);
    }
}

///////
// eDMA
///////
#define edma_regwl(base, reg, val)                                                                 \
    do {                                                                                           \
        reg_writel((base) + (reg), (val));                                                         \
        LOGL1("" #reg "(%px) = 0x%x\n", (base) + (reg), (u32)(val));                               \
    } while (0)
#define edma_regwq(base, reg, val)                                                                 \
    do {                                                                                           \
        reg_writeq((base) + (reg), (val));                                                         \
        LOGL1("" #reg "(%px) = 0x%llx\n", (base) + (reg), (u64)(val));                             \
    } while (0)

// require:
// 1. bar mapped
// 2. msi data and address set
//    this part is skipped as we could use LIE(intc) only
int xpuhw_edma_init(struct xpu_device *xdev)
{
    void __iomem *base = xdev->edma_base;
    int           ch   = 0;

    // Init eDMA read engine
    // DMA Read Engine Enable (0x2c)
    edma_regwl(base, 0x2c, 0x1);
    // DMA Read Interrupt Mask register (0xa8)
    edma_regwl(base, 0xa8, 0x0);
    // DMA Read Done IMWr Address Low register (0xcc)
    //edma_regwl(base, 0xcc, dev->msi_lo);
    // DMA Read Done IMWr Address High register (0xd0)
    //edma_regwl(base, 0xd0, dev->msi_hi);
    // DMA Read Abort IMWr Address Low register (0xd4)
    //edma_regwl(base, 0xd4, dev->msi_lo);
    // DMA Read Abort IMWr Address High register (0xd8)
    //edma_regwl(base, 0xd8, dev->msi_hi);
    // DMA Read Channel 0 IMWr Data register (0xdc)
    //edma_regwl(base, 0xdc, dev->msi_data);

#define REDMA_CC_RIE 0x10
#define REDMA_CC_LIE 0x08
    for (ch = 0; ch < KL1_EDMA_CHANNEL_NUM; ch += 1) {
        // DMA Channel Control 1 register register (0x300)
        edma_regwl(base, 0x300 + ch * 0x200, REDMA_CC_LIE);
        //edma_regwl(base, 0x300 + ch * 0x200, 0);
        // DMA Channel Control 2 register register (0x304)
        edma_regwl(base, 0x304 + ch * 0x200, 0x0);
    }

    // Init eDMA write engine
    // DMA Write Engine Enable register (0xc)
    edma_regwl(base, 0xc, 0x1);
    // DMA Write Interrupt Mask register (0x54)
    edma_regwl(base, 0x54, 0x0);
    // DMA Write Done IMWr Address Low register (0x60)
    //edma_regwl(base, 0x60, dev->msi_lo);
    // DMA Write Done IMWr Address High register (0x64)
    //edma_regwl(base, 0x64, dev->msi_hi);
    // DMA Write Abort IMWr Address Low register (0x68)
    //edma_regwl(base, 0x68, dev->msi_lo);
    // DMA Write Abort IMWr Address High register (0x6c)
    //edma_regwl(base, 0x6c, dev->msi_hi);
    // DMA Write Channel 0 IMWr Data register (0x70)
    //edma_regwl(base, 0x70, dev->msi_data);
    for (ch = 0; ch < KL1_EDMA_CHANNEL_NUM; ch += 1) {
        // DMA Channel Control 1 register register (0x200)
        edma_regwl(base, 0x200 + ch * 0x200, REDMA_CC_LIE);
        //edma_regwl(base, 0x200 + ch * 0x200, 0);
        // DMA Channel Control 2 register register (0x204)
        edma_regwl(base, 0x204 + ch * 0x200, 0x0);
    }

    return 0;
}

void xpuhw_edma_uninit(struct xpu_device *xdev)
{
    void __iomem *base = xdev->edma_base;
    edma_regwl(base, 0x2c, 0x0);
    edma_regwl(base, 0xc, 0x0);
}

// RETURN VALUE
// * 0
// * -XPUERR_DMAABORT
// * -XPUERR_DMATIMEOUT
static inline int xpuhw_edma_poll_wait(void __iomem *reg, u32 channel, u64 *cycles)
{
    int          i             = 0;
    u32          DMA_DONE      = 0x1 << channel;
    u32          DMA_ABORT     = 0x1 << (channel + 16);
    u32          val           = reg_readl(reg);
    volatile u64 start_jiffies = jiffies;
    u64          t0            = 0;
    u64          t1            = 0;

    // wait 10s
    t0 = get_cycles();
    while (((val & (DMA_DONE | DMA_ABORT)) == 0) && ((jiffies - start_jiffies) < (20UL * HZ))) {
        val = reg_readl(reg);
        udelay(2);
        ++i;
    }
    t1 = get_cycles();

    LOGL1("poll val= %x ite= %d\n", val & (DMA_DONE | DMA_ABORT), i);

    if (cycles)
        *cycles = (((t1 - t0) == 0) ? 1 : (t1 - t0));

    if (val & DMA_DONE)
        return 0;

    if (val & DMA_ABORT)
        return -XPUERR_DMAABORT;

    return -XPUERR_DMATIMEOUT;
}

static inline void __edma_read_start(void __iomem *base, u64 dst_dev, u64 src_host, size_t sz, int ch)
{
    // DMA Transfer Size register (0x308)
    edma_regwl(base, 0x308 + ch * 0x200, sz);
    // SAR Low register (0x30c)
    edma_regwl(base, 0x30c + ch * 0x200, low32(src_host));
    // SAR High register (0x310)
    edma_regwl(base, 0x310 + ch * 0x200, high32(src_host));
    // DAR Low register (0x314)
    edma_regwl(base, 0x314 + ch * 0x200, low32(dst_dev));
    // DAR High register (0x318)
    edma_regwl(base, 0x318 + ch * 0x200, high32(dst_dev));

    // Clear DMA Read Interrupt Status before Doorbell
    edma_regwl(base, 0xac, ((0x1 << ch) | (0x1 << (ch + 16))));
    // DMA Read Doorbell register (0x030)
    edma_regwl(base, 0x30, (ch & 0x7));
}

static inline int __edma_read_wait(void __iomem *base, int ch, u64 *cycles)
{
    int ret;

    // polling on DMA Read Interrupt Status Register (0xa0)
    ret = xpuhw_edma_poll_wait(base + 0xa0, ch, cycles);
    if (ret < 0) {
        LOGW("edma poll error= %d\n", ret);
        if (ret == -XPUERR_DMAABORT)
            LOGW("RD_ERR_STATUS_HI= %x LO= %x\n", reg_readl(base + 0xb8), reg_readl(base + 0xb4));
    }

    edma_regwl(base, 0xac, ((0x1 << ch) | (0x1 << (ch + 16))));
    return ret;
}

static inline int __edma_read_locked(void __iomem *base, u64 dst_dev, u64 src_host, size_t sz,
                                     int ch, u64 *cycles)
{
    __edma_read_start(base, dst_dev, src_host, sz, ch);
    return __edma_read_wait(base, ch, cycles);
}

static inline void __edma_write_start(void __iomem *base, u64 dst_host, u64 src_dev, size_t sz,
                                      int ch)
{
    // DMA Transfer Size register (0x208)
    edma_regwl(base, 0x208 + ch * 0x200, sz);
    // SAR Low register (0x20c)
    edma_regwl(base, 0x20c + ch * 0x200, low32(src_dev));
    // SAR High register (0x210)
    edma_regwl(base, 0x210 + ch * 0x200, high32(src_dev));
    // DAR Low register (0x214)
    edma_regwl(base, 0x214 + ch * 0x200, low32(dst_host));
    // DAR High register (0x218)
    edma_regwl(base, 0x218 + ch * 0x200, high32(dst_host));

    // Clear DMA Write Interrupt Status before Doorbell
    edma_regwl(base, 0x58, ((0x1 << ch) | (0x1 << (ch + 16))));
    // DMA Write Doorbell register (0x010)
    edma_regwl(base, 0x10, (ch & 0x7));
}

static inline int __edma_write_wait(void __iomem *base, int ch, u64 *cycles)
{
    int ret;

    // polling on DMA Write Interrupt Status Register (0x4c)
    ret = xpuhw_edma_poll_wait(base + 0x4c, ch, cycles);
    if (ret < 0) {
        LOGW("edma poll error= %d\n", ret);
        if (ret == -XPUERR_DMAABORT)
            LOGW("WR_ERR_STATUS= %x\n", reg_readl(base + 0x5c));
    }

    edma_regwl(base, 0x58, ((0x1 << ch) | (0x1 << (ch + 16))));
    return ret;
}

static inline int __edma_write_locked(void __iomem *base, u64 dst_host, u64 src_dev, size_t sz,
                                      int ch, u64 *cycles)
{
    __edma_write_start(base, dst_host, src_dev, sz, ch);
    return __edma_write_wait(base, ch, cycles);
}

static inline int edma_precheck(struct xpu_edma *edma)
{
    if (xpu_device_disabled_or_in_reset(edma->xpd->xdev))
        return -XPUERR_PEERRESET;
    if (!edma->enable)
        return -XPUERR_PEERRESET;
    return 0;
}

inline int xpuhw_edma_read_start(struct xpu_edma *edma, u64 dst_dev, u64 src_host, size_t sz)
{
    void __iomem *base = edma->xpd->xdev->edma_base;
    int           ch   = edma->channel;
    int           ret;

    ret = edma_precheck(edma);
    if (ret)
        return ret;

    LOGL1("[xpu_%d] RD start 0x%llx -> 0x%llx sz= 0x%zx ch= %d\n", edma->xpd->devfile_id, src_host,
          dst_dev, sz, ch);
    __edma_read_start(base, dst_dev, src_host, sz, ch);
    return 0;
}

inline int xpuhw_edma_read_wait(struct xpu_edma *edma, u64 *cycles)
{
    void __iomem *base = edma->xpd->xdev->edma_base;
    int           ch   = edma->channel;
    int           ret;

    ret = edma_precheck(edma);
    if (ret)
        return ret;

    ret = __edma_read_wait(base, ch, cycles);
    if (cycles)
        LOGL1("RD wait reg= %px cycles= %llu\n", base + 0xa0, *cycles);
    return ret;
}

inline int xpuhw_edma_write_start(struct xpu_edma *edma, u64 dst_host, u64 src_dev, size_t sz)
{
    void __iomem *base = edma->xpd->xdev->edma_base;
    int           ch   = edma->channel;
    int           ret;

    ret = edma_precheck(edma);
    if (ret)
        return ret;

    LOGL1("[xpu_%d] WR start 0x%llx -> 0x%llx sz= 0x%zx ch= %d\n", edma->xpd->devfile_id, src_dev,
          dst_host, sz, ch);
    __edma_write_start(base, dst_host, src_dev, sz, ch);
    return 0;
}

inline int xpuhw_edma_write_wait(struct xpu_edma *edma, u64 *cycles)
{
    void __iomem *base = edma->xpd->xdev->edma_base;
    int           ch   = edma->channel;
    int           ret;

    ret = edma_precheck(edma);
    if (ret)
        return ret;

    ret = __edma_write_wait(base, ch, cycles);
    if (cycles)
        LOGL1("WR wait reg= %px cycles= %llu\n", base + 0x4c, *cycles);
    return ret;
}

inline int xpuhw_edma_read_locked(struct xpu_edma *edma, u64 dst_dev, u64 src_host, size_t sz,
                                  u64 *cycles)
{
    int ret;

    if (cycles == NULL)
        return -XPUERR_INVALID_PARAM;

    ret = xpuhw_edma_read_start(edma, dst_dev, src_host, sz);
    if (ret)
        return ret;
    return xpuhw_edma_read_wait(edma, cycles);
}

inline int xpuhw_edma_write_locked(struct xpu_edma *edma, u64 dst_host, u64 src_dev, size_t sz,
                                   u64 *cycles)
{
    int ret;

    if (cycles == NULL)
        return -XPUERR_INVALID_PARAM;

    ret = xpuhw_edma_write_start(edma, dst_host, src_dev, sz);
    if (ret)
        return ret;
    return xpuhw_edma_write_wait(edma, cycles);
}

// u32 Register Write using edma engine
// This func should only be used while probing, as this func use edma engine
// without acquire a lock, as it is called before lock is inited
int xpuhw_edma_rwl(struct xpu_device *xdev, u64 reg_addr, u32 value)
{
    unsigned long flags;
    int           ret;

    spin_lock_irqsave(&xdev->edma_rw_lock, flags);
    xdev->edma_rw_kbuf[0] = value;
    ret = __edma_read_locked(xdev->edma_base, reg_addr, xdev->edma_rw_dma_addr, 4, 7, NULL);
    spin_unlock_irqrestore(&xdev->edma_rw_lock, flags);

    return ret;
}

// u32 Register Read using edma engine
// This func should only be used while probing, as this func use edma engine
// without acquire a lock, as it is called before lock is inited
int xpuhw_edma_rrl(struct xpu_device *xdev, u64 reg_addr, u32 *value)
{
    unsigned long flags;
    int           ret = 0;

    if (value == NULL)
        return -XPUERR_INVALID_PARAM;

    spin_lock_irqsave(&xdev->edma_rr_lock, flags);
    ret = __edma_write_locked(xdev->edma_base, xdev->edma_rr_dma_addr, reg_addr, 4, 7, NULL);
    if (ret == 0)
        *value = xdev->edma_rr_kbuf[0];
    spin_unlock_irqrestore(&xdev->edma_rr_lock, flags);

    return ret;
}

#define ssedma_regwl(base, reg, val)                                                               \
    do {                                                                                           \
        reg_writel((base) + (reg), (val));                                                         \
        LOGL1("" #reg "(%px) = 0x%x\n", (base) + (reg), (u32)(val));                               \
    } while (0)
#define ssedma_regwq(base, reg, val)                                                               \
    do {                                                                                           \
        reg_writeq((base) + (reg), (val));                                                         \
        LOGL1("" #reg "(%px) = 0x%llx\n", (base) + (reg), (u64)(val));                             \
    } while (0)

#define DMA_KT_GM2GM 0
#define DMA_KT_INT 1
#define RDMA_KT 0x0000
#define RDMA_DEST_LO 0x0004
#define RDMA_DEST_HI 0x0008
#define RDMA_SRC_LO 0x000C
#define RDMA_SRC_HI 0x0010
#define RDMA_SZ 0x0014
int xpuhw_ssedma_locked(struct xpu_ssedma *ssedma, u64 dest, u64 src, u64 sz, u64 *cost)
{
    // write GM2GM memcpy task
    int           channel       = ssedma->channel;
    void __iomem *cmdbase       = ssedma->base + 0x20000 + channel * 0x1000;
    void __iomem *pollreg       = ssedma->base + 0x2000 + channel * 4;
    volatile u64  start_jiffies = jiffies;
    unsigned long t0            = 0;
    unsigned long t1            = 0;
    u32           val           = 0;

    if (cost == NULL)
        return -XPUERR_INVALID_PARAM;

    ssedma_regwl(cmdbase, RDMA_KT, DMA_KT_GM2GM);
    ssedma_regwl(cmdbase, RDMA_DEST_LO, low32(dest));
    ssedma_regwl(cmdbase, RDMA_DEST_HI, high32(dest));
    ssedma_regwl(cmdbase, RDMA_SRC_LO, low32(src));
    ssedma_regwl(cmdbase, RDMA_SRC_HI, high32(src));
    ssedma_regwl(cmdbase, RDMA_SZ, sz);

    ssedma_regwl(cmdbase, RDMA_KT, 1);
    ssedma_regwl(cmdbase, RDMA_SZ, 0);

    // poll for the GM2GM dma to finish
    t0  = get_cycles();
    val = reg_readl(pollreg);
    // timed out after 5 mins
    while ((val == 0) && ((jiffies - start_jiffies) < 1 * 60 * HZ)) {
        usleep_range(10 /* min us */, 1000 /* max us */);
        val = reg_readl(pollreg);
    }
    t1 = get_cycles();

    if (val == 0) {
        LOGW("gm2gm 0x%llx --> 0x%llx on ch_%d timed out\n", src, dest, channel);
        return -XPUERR_TIMEOUT;
    }

    if (val != 1) {
        LOGW("gm2gm 0x%llx --> 0x%llx on ch_%d received %d intrs\n", src, dest, channel, val);
        return -XPUERR_UNEXPECT;
    }

    LOGL2("gm2gm 0x%llx --> 0x%llx on ch_%d, cycles= %lu\n", src, dest, channel, (t1 - t0));

    if (cost)
        *cost = t1 - t0;

    return 0;
}

static inline u32 __read_xpu_token(struct xpu_device *xdev, u64 base)
{
    u32 v0, v1;
    xpuhw_edma_rrl(xdev, base + 0x801C, &v0);
    xpuhw_edma_rrl(xdev, base + 0x8020, &v1);
    return (v1 & 0xffff0000) | ((v0 >> 16) & 0xffff);
}

static inline u32 __read_xpu_err(struct xpu_device *xdev, u64 base)
{
    u32 v;
    xpuhw_edma_rrl(xdev, base + 0x800C, &v);
    return v;
}

inline u32 xpuhw_get_xpu_token(struct xpu_pd *xpd, int idx)
{
    return __read_xpu_token(xpd->xdev, xpd->rbase + RXPU_APB_BASE[idx]);
}

inline u32 xpuhw_get_xpu_err(struct xpu_pd *xpd, int idx)
{
    return __read_xpu_err(xpd->xdev, xpd->rbase + RXPU_APB_BASE[idx]);
}

inline u32 xpuhw_get_cdnn_token(struct xpu_pd *xpd, int idx)
{
    return __read_xpu_token(xpd->xdev, xpd->rbase + RCDNNCL_APB_BASE[idx]);
}

inline u32 xpuhw_get_cdnn_cl_err(struct xpu_pd *xpd, int idx)
{
    return __read_xpu_err(xpd->xdev, xpd->rbase + RCDNNCL_APB_BASE[idx]);
}

inline u32 xpuhw_get_cdnn_err(struct xpu_pd *xpd, int idx)
{
    u32 v;
    xpuhw_edma_rrl(xpd->xdev, xpd->rbase + RCDNNCL_APB_BASE[idx] + 0x0018, &v);
    return v;
}
