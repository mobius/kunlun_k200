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

#define __FILENAME__ "xpu_monitor.c"

#include <linux/vmalloc.h>
#include <linux/delay.h>
#include "xpu_drv.h"
#include "xpu_hw.h"

////////////////////////////
// XPU Monitor implementation
////////////////////////////

static int write_and_poll_cmd_reg_locked(void *buffer_addr, u32 cmd)
{
    u32               val;
    volatile uint64_t start_jiffies = 0;

    val = reg_readl(buffer_addr);
    val |= cmd;
    reg_writel(buffer_addr, val);

    start_jiffies = jiffies;
    while ((val & cmd) && ((jiffies - start_jiffies) < (5UL * HZ))) {
        // wait 5s
        val = reg_readl(buffer_addr);
        usleep_range(10, 100);
    }
    if (val & cmd) {
        // timeout
        return -XPUERR_TIMEOUT;
    }
    return 0;
}

int xpu_read_temp_sensor(struct xpu_device *xdev)
{
    static const u32 temp_offset[4] = {
        RSYSCON_TEM0_STAT,
        RSYSCON_TEM1_STAT,
        RSYSCON_TEM2_STAT,
        RSYSCON_TEM3_STAT,
    };

    struct xpu_dev_monitor *xds = &xdev->monitor;
    int                     i;

    if (xdev->flash_version[2] < 15)
        return -XPUERR_OLDFW;

    for (i = 0; i < 4; ++i) {
        int reg      = (int)reg_readl(xdev->syscon_base + temp_offset[i]);
        int temp     = (reg - 1656) / 16 + 35;
        xds->temp[i] = temp > 0 ? temp : 0;
    }

    return 0;
}

int xpu_read_frequency(struct xpu_device *xdev)
{
    static const u32 pll_offset[6] = {
        RSYSCON_PPL0_DIV, RSYSCON_PPL1_DIV, RSYSCON_PPL2_DIV,
        RSYSCON_PPL3_DIV, RSYSCON_PPL4_DIV, RSYSCON_PPL5_DIV,
    };

    struct xpu_dev_monitor *xds   = &xdev->monitor;
    const u32               f_fin = 25; // pll related, Ffin = 25Mhz
    int                     i     = 0;

    if (xdev->flash_version[2] < 15)
        return -XPUERR_OLDFW;

    for (i = 0; i < 4; ++i) {
        // pll related calculate, ref: http://wiki.baidu.com/pages/viewpage.action?pageId=504586144
        u32 pll          = reg_readl(xdev->syscon_base + pll_offset[i]);
        u32 s            = (pll >> 0) & 0x7;   // bits: 2:0
        u32 m            = (pll >> 3) & 0x3FF; // bits: 12:3
        u32 p            = (pll >> 13) & 0x3F; // bits: 18:13
        u32 pow2_s       = 1 << s;
        xds->freq[i]     = (p * pow2_s == 0) ? 0 : ((m * f_fin) / (p * pow2_s));
        xds->freq_raw[i] = pll;
    }
    for (; i < 6; ++i) {
        u32 pll          = reg_readl(xdev->syscon_base + pll_offset[i]);
        u32 r            = (pll >> 15) & 0x3F; // bits: 20:15
        u32 f            = (pll >> 6) & 0x1FF; // bits: 14:6
        u32 q            = (pll >> 3) & 0x7;   // bits: 5:3
        u32 pow2_q       = 1 << q;
        xds->freq[i]     = f_fin * (f + 1) / (r + 1) / pow2_q;
        xds->freq_raw[i] = pll;
    }

    return 0;
}

int xpu_read_power(struct xpu_device *xdev)
{
    struct xpu_dev_monitor *xds = &xdev->monitor;
    u32                     raw;
    u8                      cnt, byte1, byte2, checksum, cs;

    int mw_per_bit = 20;                               // mW, for version >= 1.8
    if ((xdev->cpld_version & 0x001700) == 0x001700) { // version 1.7
        mw_per_bit = 40;
    }

    if (xdev->flash_version[2] < 15)
        return -XPUERR_OLDFW;

    raw = reg_readl(xdev->syscon_base + RSYSCON_G_R6);

    cnt      = raw & 0xFF;
    byte1    = (raw >> 8) & 0xFF;
    byte2    = (raw >> 16) & 0xFF;
    checksum = (raw >> 24) & 0xFF;
    if (cnt != 3) {
        //LOGW("[SN: %u] I2C error, raw = %x, cnt=%u != 3\n", xdev->sn, raw, cnt);
        xds->power = 0;
        return -1;
    }
    cs = 0xFF & (0x100 - (cnt + byte1 + byte2));
    if (checksum != cs) {
        //LOGW("[SN: %u] I2C error, raw = %x, checksum wanted=%x, checksum calculated=%x\n",
        //            xdev->sn, raw, checksum, cs);
        xds->power = 0;
        return -1;
    }

    xds->power = ((byte1 << 8) + byte2) * mw_per_bit; // mW

    return 0;
}

int xpu_switch_i3e(struct xpu_device *xdev)
{
    struct kl_device *kdev = xdev->kdev;
    int               ret;
    void             *buffer_addr = NULL;

    LOGI("SWITCH ON/OFF IEEE1500...\n");

    buffer_addr = kdev->bar[0] + (reg_readl(xdev->syscon_base + RSYSCON_G_R0) - RSRAM_BASE);

    mutex_lock(&xdev->firmware_lock);
    ret = write_and_poll_cmd_reg_locked(buffer_addr, HOST_CMD_HBM_TEMP_EN);
    mutex_unlock(&xdev->firmware_lock);

    return ret;
}

int xpu_read_hbm_temp(struct xpu_device *xdev)
{
    struct xpu_dev_monitor *xds = &xdev->monitor;

    if (xdev->flash_version[2] < 15)
        return -XPUERR_OLDFW;

    xds->hbm_temp[0] = reg_readl(xdev->syscon_base + RSYSCON_G_R4);
    xds->hbm_temp[1] = reg_readl(xdev->syscon_base + RSYSCON_G_R5);

    return 0;
}

int xpu_read_mcu_version(struct xpu_device *xdev)
{
    struct kl_device *kdev        = xdev->kdev;
    void             *buffer_addr = NULL;
    u32               mcu_status  = 0;

    if (xdev->cpld_version != 0)
        return 0;

    buffer_addr = kdev->bar[0] + (reg_readl(xdev->syscon_base + RSYSCON_G_R0) - RSRAM_BASE);

    mcu_status = reg_readl(xdev->syscon_base + RSYSCON_G_R1);
    if (mcu_status == MCU_STATUS_UNINIT) {
        LOGW("MCU uninited, statue=0x%x\n", mcu_status);
        xdev->flash_version[0] = 0;
        xdev->flash_version[1] = 0;
        xdev->flash_version[2] = 0;
        xdev->cpld_version     = 0;
        return -XPUERR_MCUUNINIT;
    }

    // read version from mcu sram
    xdev->flash_version[0] = reg_readl(buffer_addr + 0x14 + 3 * 4);
    xdev->flash_version[1] = reg_readl(buffer_addr + 0x14 + 4 * 4);
    xdev->flash_version[2] = reg_readl(buffer_addr + 0x14 + 5 * 4);
    xdev->cpld_version     = reg_readl(buffer_addr + 0x14 + 6 * 4);

    //LOGI("Flash version: %x.%x.%x, CPLD version: %x\n",
    //        xdev->flash_version[0],
    //        xdev->flash_version[1],
    //        xdev->flash_version[2],
    //        xdev->cpld_version);

    return 0;
}

int static_pll_set(struct xpu_device *xdev, u32 pll_index)
{
    const u32 pll_list[5] = {
        0x66C1, //900
        0x8ffa, //800
        0x6a82, //700
        0x6902, //600
        0x6782  //500
    };
    const u32 bypass_addr[4] = { 0x4, 0x10, 0x1C, 0x2004 };
    const u32 rst_addr[4]    = { 0x8, 0x14, 0x20, 0x2008 };
    const u32 pms_addr[4]    = {
        RSYSCON_PPL0_DIV,
        RSYSCON_PPL1_DIV,
        RSYSCON_PPL2_DIV,
        RSYSCON_PPL3_DIV,
    };
    int i;

    if (pll_index < 0 || pll_index >= 5) {
        return -1;
    }

    for (i = 0; i < 4; ++i) {
        u32 val;
        // source_sel : 0-ref_clk
        reg_writel(xdev->syscon_base + 0x0100, 0x0);
        // config bypass mode
        val = reg_readl(xdev->syscon_base + bypass_addr[i]);
        reg_writel(xdev->syscon_base + bypass_addr[i], (val | 0x4));
        // pll rst
        reg_writel(xdev->syscon_base + rst_addr[i], 0);
        // pll p/m/s
        reg_writel(xdev->syscon_base + pms_addr[i], pll_list[pll_index]);
        // pll non-rst
        reg_writel(xdev->syscon_base + rst_addr[i], 0x1);
        // delay 30 ns, here we make it more
        usleep_range(1, 2);
        // config pll out of bypass mode
        val = reg_readl(xdev->syscon_base + bypass_addr[i]);
        reg_writel(xdev->syscon_base + bypass_addr[i], (val & (~(0x1 << 2))));
        // source_sel:1---pll_out
        reg_writel(xdev->syscon_base + 0x0100, 0xf);
    }
    return 0;
}

// is_add:
//      true:   s += 1, freq / 2
//      false:  s -= 1, freq * 2
int dynamic_pll_set(struct xpu_device *xdev, u32 is_add)
{
    const u32 pms_addr[4] = {
        RSYSCON_PPL0_DIV,
        RSYSCON_PPL1_DIV,
        RSYSCON_PPL2_DIV,
        RSYSCON_PPL3_DIV,
    };
    int i;
    for (i = 0; i < 4; ++i) {
        const u32 mask_p   = 0x3F;  // bits: 18:13
        const u32 mask_m   = 0x3FF; // bits: 12:3
        const u32 mask_s   = 0x7;   // bits: 2:0
        const u32 f_fin    = 25;    // pll related, Ffin = 25Mhz
        const u32 up_bound = 900;

        u32 pll, p, m, s, pow2_s, freq;
        pll    = reg_readl(xdev->syscon_base + pms_addr[i]);
        p      = (pll >> 13) & mask_p;
        m      = (pll >> 3) & mask_m;
        s      = (pll >> 0) & mask_s;
        pow2_s = 1 << s;
        freq   = (p * pow2_s == 0) ? 0 : ((m * f_fin) / (p * pow2_s));

        if (is_add == 0 && (freq * 2 > up_bound)) {
            // can not make freq larger than 'up_bound'
            return -1;
        }

        s += (is_add > 0 ? 1 : -1);

        if (s == mask_s) {
            // dynamic pll up overflow
            return -1;
        } else if (s == 0) {
            // dynamic pll down underflow
            return -1;
        }
        pll = (pll & (~mask_s)) | s;
        reg_writel(xdev->syscon_base + pms_addr[i], pll);
    }
    return 0;
}
