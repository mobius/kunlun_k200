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

#define __FILENAME__ "xpu_hbm.c"
#include <asm/io.h>
#include <linux/delay.h>
#include "xpu_regs.h"
#include "xpu_hw.h"
#include "xpu_drv.h"

int xpuhw_train_hbm(struct xpu_pd *xpd)
{
    struct xpu_device *xdev    = xpd->xdev;
    uint32_t           channel = 0;
    uint32_t           dout    = 0;
    uint32_t           soft_lane_repair_info[3];
    //uint32_t hbm_freq = 0x13D3;//500MHz
    //uint32_t hbm_freq = 0xFCB; //800MHz
    //uint32_t hbm_freq = 0x11CB;//900MHz
    //uint32_t hbm_freq = 0x12CB;//950MHz
    uint32_t hbm_freq = 0x13CB; //1GHz

#define SYSCON_WRITE(off, v) xpuhw_edma_rwl(xdev, RSYSCON_BASE + (off), (v))

#define SYSCON_READ(off, v) xpuhw_edma_rrl(xdev, RSYSCON_BASE + (off), &(v))

#define CHANNEL_OFFSET(ch, offset) (((ch) << 18) + (offset))

#define HBM_PHY_APB_WRITE(off, v) xpuhw_edma_rwl(xdev, xpd->rbase + RHBMPHY_BASE + (off), (v))

#define HBM_PHY_APB_READ(off, v) xpuhw_edma_rrl(xdev, xpd->rbase + RHBMPHY_BASE + (off), &(v))

#define HBM_CTRL_APB_WRITE(ch, off, v)                                                             \
    xpuhw_edma_rwl(xdev, xpd->rbase + RHBMCTRL_BASE + ((ch) << 18) + (off), (v))

#define HBM_CTRL_APB_READ(ch, off, v)                                                              \
    xpuhw_edma_rrl(xdev, xpd->rbase + RHBMCTRL_BASE + ((ch) << 18) + (off), &(v))

#define ERROR_CODE 0

#define MCU_POLL(dout, cond, errno, expression)                                                    \
    do {                                                                                           \
        u32     timeout_us = 10 * 1000 * 1000;                                                     \
        u32     sleep_us   = 1000;                                                                 \
        ktime_t __timeout;                                                                         \
        __timeout = ktime_add_us(ktime_get(), timeout_us);                                         \
        might_sleep_if(sleep_us);                                                                  \
        dout = 0;                                                                                  \
        for (;;) {                                                                                 \
            expression;                                                                            \
            if (cond)                                                                              \
                break;                                                                             \
            if (timeout_us && ktime_compare(ktime_get(), __timeout) > 0)                           \
                break;                                                                             \
            if (sleep_us)                                                                          \
                usleep_range((sleep_us >> 2) + 1, sleep_us);                                       \
        }                                                                                          \
        if (!cond)                                                                                 \
            return -XPUERR_HBM_INIT;                                                               \
    } while (0)

    if (xpd->id == 0) {
        SYSCON_WRITE(0x200C, hbm_freq); // pd0
        // pll_reset
        SYSCON_WRITE(0x2014, 0x1);
        SYSCON_WRITE(0x2220, 0x1);
    } else {
        SYSCON_WRITE(0x2018, hbm_freq); // pd1
        // pll_reset
        SYSCON_WRITE(0x2020, 0x1);
        SYSCON_WRITE(0x2220, 0x2);
    }
    // delay 1.5us
    msleep(100);

    SYSCON_WRITE(0x2014, 0x0);
    SYSCON_WRITE(0x2020, 0x0);
    msleep(100);
    SYSCON_WRITE(0x2220, 0x0);

    for (channel = 0; channel < 8; ++channel) {
        //Step 7.1.1 Turn off power down setting
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0904 << 2), 0x0); //ADDR_IPD_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a04 << 2), 0x0); //ADDR_IPD_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a04 << 2), 0x0); //ADDR_IPD_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a04 << 2), 0x0); //ADDR_IPD_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca04 << 2), 0x0); //ADDR_IPD_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0901 << 2), 0x0); //ADDR_PD_IREF_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a01 << 2), 0x0); //ADDR_PD_IREF_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a01 << 2), 0x0); //ADDR_PD_IREF_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a01 << 2), 0x0); //ADDR_PD_IREF_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca01 << 2), 0x0); //ADDR_PD_IREF_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0902 << 2), 0x0); //ADDR_PDREF_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a02 << 2), 0x0); //ADDR_PDREF_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a02 << 2), 0x0); //ADDR_PDREF_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a02 << 2), 0x0); //ADDR_PDREF_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca02 << 2), 0x0); //ADDR_PDREF_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x090d << 2), 0x0); //ADDR_IPD_E_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a0c << 2), 0x0); //ADDR_IPD_E_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a0c << 2), 0x0); //ADDR_IPD_E_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a0c << 2), 0x0); //ADDR_IPD_E_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca0c << 2), 0x0); //ADDR_IPD_E_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x090e << 2), 0x0); //ADDR_IPD_R_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a0d << 2), 0x0); //ADDR_IPD_R_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a0d << 2), 0x0); //ADDR_IPD_R_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a0d << 2), 0x0); //ADDR_IPD_R_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca0d << 2), 0x0); //ADDR_IPD_R_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a0e << 2), 0x2); //ADDR_IPD_DQS_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a0e << 2), 0x2); //ADDR_IPD_DQS_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a0e << 2), 0x2); //ADDR_IPD_DQS_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca0e << 2), 0x2); //ADDR_IPD_DQS_3
        //Step 7.1.2 High speed configuration for IO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x090b << 2), 0x0); //ADDR_ENAB_LV_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a0a << 2), 0x0); //ADDR_ENAB_LV_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a0a << 2), 0x0); //ADDR_ENAB_LV_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a0a << 2), 0x0); //ADDR_ENAB_LV_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca0a << 2), 0x0); //ADDR_ENAB_LV_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x090c << 2), 0x1); //ADDR_ENAB_NP_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a0b << 2), 0x1); //ADDR_ENAB_NP_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a0b << 2), 0x1); //ADDR_ENAB_NP_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a0b << 2), 0x1); //ADDR_ENAB_NP_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca0b << 2), 0x1); //ADDR_ENAB_NP_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0906 << 2), 0x4); //ADDR_DR_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a06 << 2), 0x4); //ADDR_DR_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a06 << 2), 0x4); //ADDR_DR_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a06 << 2), 0x4); //ADDR_DR_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca06 << 2), 0x4); //ADDR_DR_3
        //Step 7.1.3 VREF setting
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0900 << 2), 0x0); //ADDR_OP_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a00 << 2), 0x0); //ADDR_OP_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a00 << 2), 0x0); //ADDR_OP_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a00 << 2), 0x0); //ADDR_OP_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca00 << 2), 0x0); //ADDR_OP_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0907 << 2), 0x0); //ADDR_MODE_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a07 << 2), 0x0); //ADDR_MODE_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a07 << 2), 0x0); //ADDR_MODE_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a07 << 2), 0x0); //ADDR_MODE_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca07 << 2), 0x0); //ADDR_MODE_3
        //Step 7.1.4.1 Output enable
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0908 << 2), 0x1); //ADDR_OE_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0030 << 2), 0x3); //ADDR_OE_DQS_EN
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0031 << 2), 0x3); //ADDR_OE_DQ_EN
        //Step 7.1.4.2 Input enable
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0905 << 2), 0x1); //ADDR_IE_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0a05 << 2), 0x1); //ADDR_IE_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4a05 << 2), 0x1); //ADDR_IE_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8a05 << 2), 0x1); //ADDR_IE_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xca05 << 2), 0x1); //ADDR_IE_3
        //Step 7.1.4.3 Turn off clock disable feature
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0033 << 2), 0x0); //ADDR_CH_CK_OFF
        //Step 7.1.4.4 Parity Latency
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0003 << 2), 0x0); //ADDR_PAR_LAT
        //Step 7.2 HBM2PHY DLL configuration
        //Belong to Step 7.2, Prior to DLL setting, Disable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x0); //ADDR_FIFO_ENB
        //Belong to Step 7.2, Prior to DLL setting, Power on DLL
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0700 << 2), 0x0); //ADDR_SLAVE_PD_AW
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0800 << 2), 0x0); //ADDR_SLAVE_PD_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4800 << 2), 0x0); //ADDR_SLAVE_PD_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8800 << 2), 0x0); //ADDR_SLAVE_PD_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc800 << 2), 0x0); //ADDR_SLAVE_PD_3
        //Belong to Step 7.2, Setting DLL if using slave code program 0, else program 1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0702 << 2),
                          0x0); //ADDR_SLAVE_MULT_OUT_FLOP_ENAB
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0802 << 2),
                          0x0); //ADDR_WDQS_MULT_OUT_FLOP_ENAB_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4802 << 2),
                          0x0); //ADDR_WDQS_MULT_OUT_FLOP_ENAB_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8802 << 2),
                          0x0); //ADDR_WDQS_MULT_OUT_FLOP_ENAB_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc802 << 2),
                          0x0); //ADDR_WDQS_MULT_OUT_FLOP_ENAB_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0842 << 2),
                          0x0); //ADDR_RDQS_MULT_OUT_FLOP_ENAB_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4842 << 2),
                          0x0); //ADDR_RDQS_MULT_OUT_FLOP_ENAB_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8842 << 2),
                          0x0); //ADDR_RDQS_MULT_OUT_FLOP_ENAB_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc842 << 2),
                          0x0); //ADDR_RDQS_MULT_OUT_FLOP_ENAB_3
        //Belong to Step 7.2, Setting DLL if using slave code program 1, else program 0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0706 << 2), 0x1); //ADDR_SLAVE_DFT_EXT_ADJ_ENAB
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0806 << 2), 0x1); //ADDR_WDQS_DFT_EXT_ADJ_ENAB_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4806 << 2), 0x1); //ADDR_WDQS_DFT_EXT_ADJ_ENAB_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8806 << 2), 0x1); //ADDR_WDQS_DFT_EXT_ADJ_ENAB_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc806 << 2), 0x1); //ADDR_WDQS_DFT_EXT_ADJ_ENAB_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0846 << 2), 0x1); //ADDR_RDQS_DFT_EXT_ADJ_ENAB_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4846 << 2), 0x1); //ADDR_RDQS_DFT_EXT_ADJ_ENAB_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8846 << 2), 0x1); //ADDR_RDQS_DFT_EXT_ADJ_ENAB_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc846 << 2), 0x1); //ADDR_RDQS_DFT_EXT_ADJ_ENAB_3
        //Belong to Step 7.2, Program desired delay if using slave code
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0705 << 2), 0x0); //ADDR_SLAVE_DFT_EXT_ADJ
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0805 << 2), 0x0); //ADDR_WDQS_DFT_EXT_ADJ_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4805 << 2), 0x0); //ADDR_WDQS_DFT_EXT_ADJ_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8805 << 2), 0x0); //ADDR_WDQS_DFT_EXT_ADJ_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc805 << 2), 0x0); //ADDR_WDQS_DFT_EXT_ADJ_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0845 << 2), 0x0); //ADDR_RDQS_DFT_EXT_ADJ_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4845 << 2), 0x0); //ADDR_RDQS_DFT_EXT_ADJ_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8845 << 2), 0x0); //ADDR_RDQS_DFT_EXT_ADJ_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc845 << 2), 0x0); //ADDR_RDQS_DFT_EXT_ADJ_3
        //Belong to Step 7.2, Additional 9 settings according to eSilicon feedback 2019/06/14
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0809 << 2), 0x3); //ADDR_DS_P_AW_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x084a << 2), 0x3); //ADDR_RDS_P_0_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x484a << 2), 0x3); //ADDR_RDS_P_0_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x884a << 2), 0x3); //ADDR_RDS_P_0_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc84a << 2), 0x3); //ADDR_RDS_P_0_3
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0849 << 2), 0x3); //ADDR_WDS_P_0_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4849 << 2), 0x3); //ADDR_WDS_P_0_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8849 << 2), 0x3); //ADDR_WDS_P_0_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc849 << 2), 0x3); //ADDR_WDS_P_0_3
    }

    //assert WRST_N
    if (xpd->id == 0) {
        SYSCON_WRITE(0x222c, 0x1);
    } else {
        SYSCON_WRITE(0x222c, 0x2);
    }

    //Step 8 Assert HBM DRAM reset
    //does not reset DFI when the controller is in reset
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4268, 0x1);
    }

    //WSRT_n must keep low at least 500us(5 us for fast sim) after RESET_n is deassert
    msleep(100);

    //disable WRCK
    if (xpd->id == 0) {
        SYSCON_WRITE(0x222c, 0x5);
    } else {
        SYSCON_WRITE(0x222c, 0xa);
    }

    msleep(100);

    //deassert WRST_N
    if (xpd->id == 0) {
        SYSCON_WRITE(0x222c, 0x4);
    } else {
        SYSCON_WRITE(0x222c, 0x8);
    }
    msleep(100);

    //enable WRCK
    SYSCON_WRITE(0x222c, 0x0);

    //Step 9 PHY IO Calibration
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0020 << 2), 0x0); //ADDR_GLOBAL_CFG_CAL_PD
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0023 << 2), 0x0); //ADDR_GLOBAL_ZQCAL_GO

    MCU_POLL(dout, (dout & 0x1), ERROR_CODE,
             HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0024 << 2), dout)); //ADDR_GLOBAL_ZQCAL_DONE

    HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0025 << 2), dout); //ADDR_GLOBAL_CFG_CAL_FAULT
    if ((dout & 0x1) != 0) {
        LOGW("ZQCAL not success: value = %x\n", dout & 0x1);
        return -XPUERR_HBM_INIT;
    }

    //disable WRCK
    if (xpd->id == 0) {
        SYSCON_WRITE(0x222c, 0x4);
    } else {
        SYSCON_WRITE(0x222c, 0x8);
    }
    msleep(100);

    //set CS to 2, for Step 10
    SYSCON_WRITE(0x2238, 0x2); //cs
    msleep(100);

    //enable WRCK
    SYSCON_WRITE(0x222C, 0x0);

    //Step 10, Load HARD LANE REPAIR config from HBM DRAM
    for (channel = 0; channel < 8; ++channel) {
        SYSCON_WRITE(0x2230, 0x5); //event:[0]=SelectWIR, [1]=CaptureWR, [2]=ShiftWR, [3]=UpdateWR
        SYSCON_WRITE(0x2234, 11);  //shift_len
        SYSCON_WRITE(0x2268, xpd->id);                 //pd
        SYSCON_WRITE(0x2244, ((channel << 8) | 0x13)); //data_wsi
        SYSCON_WRITE(0x223C, 0x1);                     //trigger

        MCU_POLL(dout, (dout & 0x1), ERROR_CODE, SYSCON_READ(0x2240, dout)); //rdy

        SYSCON_WRITE(0x2230, 0x9); //event:[0]=SelectWIR, [1]=CaptureWR, [2]=ShiftWR, [3]=UpdateWR
        SYSCON_WRITE(0x2268, xpd->id); //pd
        SYSCON_WRITE(0x223C, 0x1);     //trigger

        MCU_POLL(dout, (dout & 0x1), ERROR_CODE, SYSCON_READ(0x2240, dout)); //rdy

        SYSCON_WRITE(0x2230, 0x2); //event:[0]=SelectWIR, [1]=CaptureWR, [2]=ShiftWR, [3]=UpdateWR
        SYSCON_WRITE(0x2268, xpd->id); //pd
        SYSCON_WRITE(0x223C, 0x1);     //trigger

        MCU_POLL(dout, (dout & 0x1), ERROR_CODE, SYSCON_READ(0x2240, dout)); //rdy

        SYSCON_WRITE(0x2230, 0x4); //event:[0]=SelectWIR, [1]=CaptureWR, [2]=ShiftWR, [3]=UpdateWR
        SYSCON_WRITE(0x2234, 31);  //shift_len
        SYSCON_WRITE(0x2268, xpd->id); //pd
        SYSCON_WRITE(0x223C, 0x1);     //trigger

        MCU_POLL(dout, (dout & 0x1), ERROR_CODE, SYSCON_READ(0x2240, dout)); //rdy

        SYSCON_READ(0x2248 + 4 * channel, soft_lane_repair_info[0]);
        SYSCON_WRITE(0x2230, 0x4); //event:[0]=SelectWIR, [1]=CaptureWR, [2]=ShiftWR, [3]=UpdateWR
        SYSCON_WRITE(0x2234, 31);  //shift_len
        SYSCON_WRITE(0x2268, xpd->id); //pd
        SYSCON_WRITE(0x223C, 0x1);     //trigger

        MCU_POLL(dout, (dout & 0x1), ERROR_CODE, SYSCON_READ(0x2240, dout)); //rdy

        SYSCON_READ(0x2248 + 4 * channel, soft_lane_repair_info[1]);
        SYSCON_WRITE(0x2230, 0x4); //event:[0]=SelectWIR, [1]=CaptureWR, [2]=ShiftWR, [3]=UpdateWR
        SYSCON_WRITE(0x2234, 0x7); //shift_len
        SYSCON_WRITE(0x2268, xpd->id); //pd
        SYSCON_WRITE(0x223C, 0x1);     //trigger

        MCU_POLL(dout, (dout & 0x1), ERROR_CODE, SYSCON_READ(0x2240, dout)); //rdy

        SYSCON_READ(0x2248 + 4 * channel, dout);

        soft_lane_repair_info[2] = 0x000000ff & (dout >> 24);

        //ADDR_CH_AWORD_CA
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x2601 << 2),
                          soft_lane_repair_info[1] & 0x0000000f);
        //ADDR_CH_AWORD_RA
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x2602 << 2),
                          (soft_lane_repair_info[1] & 0x000000f0) >> 4);
        //ADDR_CH_DRBYTE0_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x2603 << 2),
                          soft_lane_repair_info[0] & 0x0000ffff);
        //ADDR_CH_DRBYTE0_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x2604 << 2),
                          (soft_lane_repair_info[0] & 0xffff0000) >> 16);
        //ADDR_CH_DRBYTE1_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x2605 << 2),
                          (soft_lane_repair_info[1] & 0x00ffff00) >> 8);
        //ADDR_CH_DRBYTE1_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x2606 << 2),
                          ((soft_lane_repair_info[2] << 8) & 0x0000ff00) |
                                  ((soft_lane_repair_info[1] >> 24) & 0x000000ff));
    }

    //disable WRCK
    if (xpd->id == 0) {
        SYSCON_WRITE(0x222c, 0x4);
    } else {
        SYSCON_WRITE(0x222c, 0x8);
    }
    msleep(100);

    //set CS to 0, otherwise PHY can't use IEEE1500 to set HBM to loopback mode
    SYSCON_WRITE(0x2238, 0x0); //cs
    msleep(100);

    //enable WRCK
    SYSCON_WRITE(0x222C, 0x0);

    //Skip Step 11 ~ 13, only need in manufacturing test

    //Step 14, CK training
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0006 << 2), 0xff); //ADDR_GLOBAL_CHANNEL_ENAB
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0002 << 2), 0x0);  //ADDR_GLOBAL_TRAIN_DONE
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0003 << 2), 0x0);  //ADDR_GLOBAL_TRAIN_OK

    for (channel = 0; channel < 8; ++channel) {
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0038 << 2), 0x28); //ADDR_CK_MINRANGE
    }

    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003a << 2), 0x1);   //ADDR_GLOBAL_DOREPAIR
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x8056 << 2), 0x3e8); //ADDR_CK_COUNT_LO
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x8057 << 2), 0x0);   //ADDR_CK_COUNT_HI
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003e << 2), 0x0);   //ADDR_GLOBAL_SLAVE_START
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003f << 2), 0xff);  //ADDR_GLOBAL_SLAVE_END
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0001 << 2), 0x0);   //ADDR_GLOBAL_TRAIN_TYPE
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0000 << 2), 0x1);   //ADDR_GLOBAL_TRAIN_GO

    MCU_POLL(dout, (dout & 0x1), ERROR_CODE,
             HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0002 << 2), dout)); //ADDR_GLOBAL_TRAIN_DONE

    HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0003 << 2), dout); //ADDR_GLOBAL_TRAIN_OK
    if ((dout & 0xFF) != 0xFF) {
        LOGW("ADDR TRAIN not success: value = %x\n", dout & 0xFF);
        return -XPUERR_HBM_INIT;
    }

    //Step 15 Reset FIFO
    for (channel = 0; channel < 8; ++channel) {
        //Disable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x0); //ADDR_FIFO_ENB
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x1); //ADDR_FIFO_ENB
    }

    //Step 16 Program MRS
    //work around bug 1806, change tRC to 49
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4054, 49);
    }
    //work around bug 1685, change tRP to 16
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4050, 16);
    }
    //mask temperature change
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x2950, 0x40000);
    }
    //enable single bank refresh, disable in default
    //for (channel = 0; channel < 8; ++channel) {
    //    HBM_CTRL_APB_WRITE(channel, 0x3e78, 0x1);
    //}
    //disable wdata parity check for axi port
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x12c00, 0x0);
    }
    //enable rid out of order
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x13694, 0x1);
    }
    //enable rid interleaving
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x136a0, 0x1);
    }
    //set HBM driver strength to 18mA
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x40b4, 0x4);
    }
    //start init FSM
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4000, 0x1);
    }

    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(0, 0x403c, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(1, 0x403c, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(2, 0x403c, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(3, 0x403c, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(4, 0x403c, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(5, 0x403c, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(6, 0x403c, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(7, 0x403c, dout));

    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x294c, 0x40000);
    }
    //unmask temperature change
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x2950, 0x0);
    }

    //Step 17 MC program CKE to Low to enter power down mode
    //phy_apb_master need ctrl_apb_master to set CKE low
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x401c, 0x1);
    }

    //Step 18.1 WDQS training
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0006 << 2), 0xff); //ADDR_GLOBAL_CHANNEL_ENAB
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0002 << 2), 0x0);  //ADDR_GLOBAL_TRAIN_DONE
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0003 << 2), 0x0);  //ADDR_GLOBAL_TRAIN_OK

    for (channel = 0; channel < 8; ++channel) {
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0039 << 2), 0x28); //ADDR_WDQS_MINRANGE
    }

    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003a << 2), 0x1);   //ADDR_GLOBAL_DOREPAIR
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x2600 << 2), 0x1);   //ADDR_CH_MODE2
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x805a << 2), 0x3e8); //ADDR_WDQS_COUNT_LO
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x805b << 2), 0x0);   //ADDR_WDQS_COUNT_HI
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003e << 2), 0x0);   //ADDR_GLOBAL_SLAVE_START
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003f << 2), 0xff);  //ADDR_GLOBAL_SLAVE_END
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0001 << 2), 0x1);   //ADDR_GLOBAL_TRAIN_TYPE
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0000 << 2), 0x1);   //ADDR_GLOBAL_TRAIN_GO

    MCU_POLL(dout, (dout & 0x1), ERROR_CODE,
             HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0002 << 2), dout)); //ADDR_GLOBAL_TRAIN_DONE

    HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0003 << 2), dout); //ADDR_GLOBAL_TRAIN_OK
    if ((dout & 0xFF) != 0xFF) {
        LOGW("WDQS TRAIN not success: value = %x\n", dout & 0xFF);
        return -XPUERR_HBM_INIT;
    }

    //Step 18.2 RDQS training
    for (channel = 0; channel < 8; ++channel) {
        //Disable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x0); //ADDR_FIFO_ENB
        //Enable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x1); //ADDR_FIFO_ENB
    }

    for (channel = 0; channel < 8; ++channel) {
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0006 << 2),
                          1 << channel);                          //ADDR_GLOBAL_CHANNEL_ENAB
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0002 << 2), 0x0); //ADDR_GLOBAL_TRAIN_DONE
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0003 << 2), 0x0); //ADDR_GLOBAL_TRAIN_OK

        //HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x003a << 2), 0x28);      //ADDR_RDQS_MINRANGE
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x003a << 2), 0x14); //ADDR_RDQS_MINRANGE

        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003a << 2), 0x1);   //ADDR_GLOBAL_DOREPAIR
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x8058 << 2), 0x3e8); //ADDR_RDQS_COUNT_LO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x8059 << 2), 0x0);   //ADDR_RDQS_COUNT_HI
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003e << 2), 0x0);   //ADDR_GLOBAL_SLAVE_START
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x003f << 2), 0xff);  //ADDR_GLOBAL_SLAVE_END
        //Additional 2 settings according to eSilicon feedback 2019/06/14
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0043 << 2), 0x22); //ADDR_RDSEL_START
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0044 << 2), 0x24); //ADDR_RDSEL_END
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0001 << 2), 0x2);  //ADDR_GLOBAL_TRAIN_TYPE
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0000 << 2), 0x1);  //ADDR_GLOBAL_TRAIN_GO

        MCU_POLL(dout, (dout & 0x1), ERROR_CODE,
                 HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0002 << 2), dout)); //ADDR_GLOBAL_TRAIN_DONE

        HBM_PHY_APB_READ(CHANNEL_OFFSET(channel, 0x0845 << 2), dout);  //ADDR_RDQS_DFT_EXT_ADJ_0
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x4845 << 2), dout); //ADDR_RDQS_DFT_EXT_ADJ_1
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x8845 << 2), dout); //ADDR_RDQS_DFT_EXT_ADJ_2
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0xc845 << 2), dout); //ADDR_RDQS_DFT_EXT_ADJ_3
    }

    for (channel = 0; channel < 8; ++channel) {
        //Disable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x0); //ADDR_FIFO_ENB
        //Enable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x1); //ADDR_FIFO_ENB
    }

    //Step 18.3 RDSEL training
    for (channel = 0; channel < 8; ++channel) {
        //Disable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x0); //ADDR_FIFO_ENB
        //Enable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x1); //ADDR_FIFO_ENB
    }

    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0006 << 2), 0xff);  //ADDR_GLOBAL_CHANNEL_ENAB
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0002 << 2), 0x0);   //ADDR_GLOBAL_TRAIN_DONE
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0003 << 2), 0x0);   //ADDR_GLOBAL_TRAIN_OK
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x8058 << 2), 0x3e8); //ADDR_RDQS_COUNT_LO
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x8059 << 2), 0x0);   //ADDR_RDQS_COUNT_HI
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0043 << 2), 0x20);  //ADDR_GLOBAL_RDSEL_START
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0044 << 2), 0x24);  //ADDR_GLOBAL_RDSEL_END
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0001 << 2), 0x3);   //ADDR_GLOBAL_TRAIN_TYPE
    HBM_PHY_APB_WRITE(CHANNEL_OFFSET(0x8, 0x0000 << 2), 0x1);   //ADDR_GLOBAL_TRAIN_GO

    MCU_POLL(dout, (dout & 0x1), ERROR_CODE,
             HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0002 << 2), dout)); //ADDR_GLOBAL_TRAIN_DONE

    HBM_PHY_APB_READ(CHANNEL_OFFSET(0x8, 0x0003 << 2), dout); //ADDR_GLOBAL_TRAIN_OK
    if ((dout & 0xFF) != 0xFF) {
        LOGW("RDSEL TRAIN not success: value = %x\n", dout & 0xFF);
        return -XPUERR_HBM_INIT;
    }

    for (channel = 0; channel < 8; ++channel) {
        //Disable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x0); //ADDR_FIFO_ENB
        //Enable FIFO
        HBM_PHY_APB_WRITE(CHANNEL_OFFSET(channel, 0x0040 << 2), 0x1); //ADDR_FIFO_ENB
    }

    //Step 19 MC program CKE to High to exit power down mode
    //phy_apb_master need ctrl_apb_master to set CKE high
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x401c, 0x0);
    }
    msleep(100);

    //Step 21, Write zeros to all memory space
    //set MT_ADDR_PATTERN to Counting address pattern
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4418, 0x0);
    }
    //set MT_DATA_PATTERN to user-specified
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4414, 0x6);
    }
    //set MT_USER_DATA_PATTERN to zero
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4540, 0x0);
    }
    //set MT_ADDR_BITS to 30 (1GB per channel)
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4420, 30);
    }
    //set MT_EN to 1
    for (channel = 0; channel < 8; ++channel) {
        HBM_CTRL_APB_WRITE(channel, 0x4404, 0x1);
    }

    //poll MT_DONE_ACK
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(0, 0x4428, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(1, 0x4428, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(2, 0x4428, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(3, 0x4428, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(4, 0x4428, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(5, 0x4428, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(6, 0x4428, dout));
    MCU_POLL(dout, (dout & 0x1), ERROR_CODE, HBM_CTRL_APB_READ(7, 0x4428, dout));

    return 0;
}

int xpuhw_train_hbm_zp(struct xpu_pd *xpd)
{
    struct xpu_device *xdev = xpd->xdev;
    int                i;

    for (i = 0; i < KL1_HBM_CHANNEL_NUM; ++i) {
        u64 base = xpd->id * PD1_OFFSET + RHBMCTRL_BASE + i * 0x40000;

        // mask temperature change
        xpuhw_edma_rwl(xdev, base + 0x2950, 0x40000);
        // enable single bank refresh, disable in default
        xpuhw_edma_rwl(xdev, base + 0x3e78, 0x1);
        // disable wdata parity check for axi port
        xpuhw_edma_rwl(xdev, base + 0x12c00, 0x0);
        // enable rdata out of order
        xpuhw_edma_rwl(xdev, base + 0x13694, 0x1);
        // enable rid interleaving
        xpuhw_edma_rwl(xdev, base + 0x136a0, 0x1);
        // start init FSM
        xpuhw_edma_rwl(xdev, base + 0x4000, 0x1);
    }

    for (i = 0; i < KL1_HBM_CHANNEL_NUM; ++i) {
        u64 base = xpd->id * PD1_OFFSET + RHBMCTRL_BASE + i * 0x40000;
        u32 val  = 0;

        // wait until FSM done
        int timeout = 0;
        while (timeout < 1000) {
            xpuhw_edma_rrl(xdev, base + 0x403c, &val);
            if (val == 1)
                break;

            usleep_range(1000, 2000);
            ++timeout;
        }

        if (val != 1) {
            LOGW("HBM%d FSM done timedout on ch %d\n", xpd->id, i);
            return -XPUERR_HBM_INIT;
        }

        // clear temperature change
        xpuhw_edma_rwl(xdev, base + 0x294c, 0x40000);
        // clear temperature change
        xpuhw_edma_rwl(xdev, base + 0x2950, 0x0);
    }

    return 0;
}

static inline bool __hbm_stat_check_ucecc(u32 v)
{
    return ((v & (0x1 << 5)) != 0);
}

bool xpuhw_hbm_has_ucecc(struct xpu_pd *xpd)
{
    bool has_ucecc = false;
    u64  base      = xpd->id * PD1_OFFSET + RHBMCTRL_BASE;
    int  ch;

    for (ch = 0; ch < KL1_HBM_CHANNEL_NUM; ++ch) {
        u32 val;
        xpuhw_edma_rrl(xpd->xdev, base + ch * 0x40000 + 0x294C, &val);
        if (__hbm_stat_check_ucecc(val)) {
            has_ucecc = true;
            break;
        }
    }

    return has_ucecc;
}
