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

#include "kl2/hw.h"
#include "kl2/kl2_regs.h"
#include "kl2/gddr_config.h"
#include <linux/delay.h>

// clang-format off
static u64 kl2_atslt_config[][4] = {
    // WIN , OFFSET    , TARGET        , ATR_SIZE(9=1KB 19=1MB)
    { 0    , 0x0       , 0x26000000ull , 21 }, // BAR2 , NOC_VIDEO   , 0~4M
    { 0    , 0x0400000 , 0x24000000ull , 21 }, // BAR2 , NOC_CU/GDDR , 4M~8M
    { 0    , 0x0800000 , 0x25000000ull , 22 }, // BAR2 , NOC_CLUSTER , 8M~16M
    { 0    , 0x1000000 , 0x27000000ull , 22 }, // BAR2 , NOC_HUB     , 16M~24M
    { 0    , 0x1800000 , 0x23000000ull , 20 }, // BAR2 , NOC_CPU     , 24M~26M

    { 1    , 0x0       , 0xC0000000ull , 27 }, // BAR4 , L3          , 0~64M
    { 1    , 0x800000000ull , 0x800000000ull , 34 }, // BAR4 , GDDR  , 32G~64G
};
// clang-format on

#define RAT_BASE(win, tbl) (0x600 + (win)*0x100 + (tbl)*0x20)
#define RAT_SRC_ADDR_ATR_PARAM 0x00
#define RAT_TRSL_ADDR 0x08
#define RAT_TRSL_PARAM 0x10
#define RAT_TRSL_MASK 0x18

static inline u64 __src_addr_atr_param(u64 src, u32 enable, u32 atr_size)
{
    return ((enable & 0x1) | ((atr_size & 0x3f) << 1) | (src & 0xfffffffffffff000ull));
}

static inline u32 __trsl_param(u32 trsl_id, u32 trsf_param)
{
    return ((trsl_id & 0xf) | ((trsf_param & 0xfff) << 16));
}

int kl2_plda_addr_trans_init(struct kl2_device *kl2_dev, void __iomem *base)
{
    void __iomem *tbl_base   = NULL;
    int           tbl        = 0;
    u32           trsf_param = 0;

    KL2_LOGD("ARRAY_SIZE(kl2_atslt_config)=%lu\n", ARRAY_SIZE(kl2_atslt_config));
    for (tbl = 0; tbl < ARRAY_SIZE(kl2_atslt_config); ++tbl) {
        tbl_base = base + RAT_BASE(kl2_atslt_config[tbl][0], tbl);

        writeq(__src_addr_atr_param(kl2_atslt_config[tbl][1], 1, kl2_atslt_config[tbl][3]),
               tbl_base + RAT_SRC_ADDR_ATR_PARAM);
        writeq(kl2_atslt_config[tbl][2], tbl_base + RAT_TRSL_ADDR);
        writeq(__trsl_param(4, 0), tbl_base + RAT_TRSL_PARAM);
    }

    // setup outbound
    tbl_base = base + RAT_BASE(2, 0);

    writeq(__src_addr_atr_param(0x0, 0x1, 31), tbl_base + RAT_SRC_ADDR_ATR_PARAM);
    writeq(0x100000000ull, tbl_base + RAT_TRSL_ADDR);
    trsf_param = kl2_readl(kl2_dev, tbl_base + RAT_TRSL_PARAM);
    kl2_writel(kl2_dev, __trsl_param(0, 0), tbl_base + RAT_TRSL_PARAM);

    msleep(100);
    KL2_LOGD("kl2_plda_addr_trans_init done\n");
    return 0;
}

int kl2_plda_dma_init(struct kl2_device *kl2_dev, int chbits)
{
    struct kl_device *kdev = kl2_dev->kdev;
    int               ch;

    // only setup for virt fn
    if (!is_vf_id(kl2_dev->dev_info.sriov_func_id))
        return 0;

    // Refer to PLDA spec "XpressCCIX-AXI Controller IP for PCIe 4.0",
    // DMA share access register: physical function num[17:13] | virtual function num[12:4]
    //                            dma acesss grantd[1] | dma access locked[0]
    for (ch = 0; ch < KL2_DMACH_CNT; ++ch) {
        void __iomem *shaccess = kdev->bar[0] + 0x400 + ch * 0x40 + RDMA_SHARE_ACCESS;

        // give dma write access to current vf
        if (chbits & BIT(ch))
            kl2_writel(kl2_dev, ((kdev->vf_id + 1) << 4) | 0x1, shaccess);
    }

    return 0;
}

int kl2_plda_dma_uninit(struct kl2_device *kl2_dev)
{
    int ch;

    for (ch = 0; ch < KL2_DMACH_CNT; ++ch)
        kl2_writel(kl2_dev, 0, kl2_dev->iomem_base.dma_base + ch * 0x40 + RDMA_SHARE_ACCESS);

    return 0;
}

#include "kl2/disable_reg_debug.h"
static inline int __dma_poll_wait(struct kl2_device *kl2_dev, void __iomem *base)
{
    void __iomem *rstatus         = base + RDMA_STATUS;
    static u32    DONE            = BIT(0) | BIT(1) | BIT(2);
    static u32    ERROR           = BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7);
    u32           val             = 0;
    int           ite             = 0;
    u64           start_jiffies   = jiffies;
    u64           elapsed_jiffies = 0;
    int           in_reset_state  = 0;

    do {
        val = kl2_readl(kl2_dev, rstatus);
        if (val & (DONE | ERROR)) {
            break;
        }
        in_reset_state = kl2_get_in_reset_state(kl2_dev);
        if (in_reset_state) {
            break;
        }

        ++ite;
        cpu_relax();

        if (elapsed_jiffies > HZ /* 1s */)
            usleep_range(10, 20);
    } while ((elapsed_jiffies = (jiffies - start_jiffies)) < (20UL * HZ) /* 20s */);

    if (val & DONE)
        return 0;

    // Abort anyway
    kl2_writel(kl2_dev, 0, base + RDMA_CONTROL);
    if (val & ERROR) {
        KL2_LOGW("DMA error, rstatus= %px, val= %08x, ite= %d\n", rstatus, val, ite);
        return -XPUERR_DMAABORT;
    } else if (in_reset_state) {
        KL2_LOGW("DMA abort due to reset, rstatus= %px, val= %08x, ite= %d\n", rstatus, val, ite);
        return -XPUERR_DEVRESET;
    } else {
        KL2_LOGW("DMA timeout, rstatus= %px, val= %08x, ite= %d\n", rstatus, val, ite);
        return -XPUERR_DMATIMEOUT;
    }
}

static inline void __do_dma(struct kl2_device *kl2_dev, void __iomem *base, u64 dest, u32 destparam,
                            u64 src, u32 srcparam, u32 length)
{
    u32 ctrl = DIRECT_DMA_CTRL_REG_VAL;

    kl2_writel(kl2_dev, srcparam, base + RDMA_SRCPARAM);
    kl2_writel(kl2_dev, destparam, base + RDMA_DESTPARAM);
    writeq(src, base + RDMA_SRCADDR);
    writeq(dest, base + RDMA_DESTADDR);
    kl2_writel(kl2_dev, length, base + RDMA_LENGTH);
    kl2_writel(kl2_dev, ctrl, base + RDMA_CONTROL);
}

static void plda_ddma_to_host_locked(void *data, u64 dest, u64 src, u64 sz, int ch)
{
    struct kl2_device *kl2_dev = data;
    void __iomem      *base    = kl2_dev->iomem_base.dma_base + ch * 0x40;

    KL2_LOGD("dma_to_host %llx -> %llx sz= %llx on channel %d\n", src, dest, sz, ch);

    __do_dma(kl2_dev, base, dest, PARAMSRC_PCIE, src, PARAMSRC_AXI, sz);
}

static void plda_ddma_from_host_locked(void *data, u64 dest, u64 src, u64 sz, int ch)
{
    struct kl2_device *kl2_dev = data;
    void __iomem      *base    = kl2_dev->iomem_base.dma_base + ch * 0x40;

    KL2_LOGD("dma_from_host %llx -> %llx sz= %llx on channel %d\n", src, dest, sz, ch);

    __do_dma(kl2_dev, base, dest, PARAMSRC_AXI, src, PARAMSRC_PCIE, sz);
}

static void plda_ddma_device_to_device(void *data, u64 dest, u64 src, u64 sz, int ch)
{
    struct kl2_device *kl2_dev = data;
    void __iomem      *base    = kl2_dev->iomem_base.dma_base + ch * 0x40;

    KL2_LOGD("dma_device_to_device %llx -> %llx sz= %llx on channel %d\n", src, dest, sz, ch);

    __do_dma(kl2_dev, base, dest, PARAMSRC_AXI, src, PARAMSRC_AXI, sz);
}

static int plda_sg_dma(void *data, u64 desc_dma_addr, u64 dma_len, int ch, int is_from_host,
                       int nowait, u32 ctrl)
{
    struct kl2_device *kl2_dev = data;
    void __iomem      *base    = kl2_dev->iomem_base.dma_base + ch * 0x40;

    KL2_LOGD("sg_dma on channel %d\n", ch);

    if (dma_len > (4 * 1024 * 1024 * 1024ULL)) {
        KL2_LOGW("dma_len= %lld, should less than 4GB\n", dma_len);
        return -XPUERR_INVALID_PARAM;
    }

    if (is_from_host) {
        kl2_writel(kl2_dev, PARAMSRC_PCIE, base + RDMA_SRCPARAM);
        kl2_writel(kl2_dev, PARAMSRC_AXI, base + RDMA_DESTPARAM);
    } else {
        kl2_writel(kl2_dev, PARAMSRC_AXI, base + RDMA_SRCPARAM);
        kl2_writel(kl2_dev, PARAMSRC_PCIE, base + RDMA_DESTPARAM);
    }

    writeq(desc_dma_addr, base + RDMA_SRCADDR);
    writeq(desc_dma_addr, base + RDMA_DESTADDR);
    kl2_writel(kl2_dev, dma_len, base + RDMA_LENGTH);
    kl2_writel(kl2_dev, ctrl, base + RDMA_CONTROL);

    if (nowait) {
        return 0;
    } else {
        //return __dma_poll_wait(kl2_dev, base);
        return 0;
    }
}

//static int plda_sg_dma_update_desc(void *data, int ch)
//{
//    struct kl2_device *kl2_dev = data;
//    void __iomem *base         = kl2_dev->iomem_base.dma_base + ch * 0x40;
//
//    kl2_writel(kl2_dev, SG_DMA_ADVANCED_CTRL_REG_VAL, base + RDMA_CONTROL);
//    return 0;
//}

static int plda_wait_dma_finished(void *data, int ch)
{
    struct kl2_device *kl2_dev = data;
    void __iomem      *base    = kl2_dev->iomem_base.dma_base + ch * 0x40;

    return __dma_poll_wait(kl2_dev, base);
}
#include "kl2/enable_reg_debug.h"

struct dma_ops kl2_dma_ops = {
    .ddma_to_host          = plda_ddma_to_host_locked,
    .ddma_from_host        = plda_ddma_from_host_locked,
    .ddma_device_to_device = plda_ddma_device_to_device,
    .sg_dma                = plda_sg_dma,
    //.sg_dma_update_desc = plda_sg_dma_update_desc,
    .wait_dma_finished = plda_wait_dma_finished,
};

static int __gddr_mrw(struct kl2_device *kl2_dev, void __iomem *base, u32 mr_num, u32 mr_ops)
{
    u32 v             = 0;
    u64 start_jiffies = 0;

    KL2_LOGD("MRW 0x%x 0x%x\n", mr_num, mr_ops);

    kl2_writel(kl2_dev, mr_ops, base + 4 * 106);
    kl2_writel(kl2_dev, mr_num + 0x2800000, base + 4 * 99);

    start_jiffies = jiffies;
    while ((v != 0x8) && ((jiffies - start_jiffies) < (2 * HZ))) {
        v = kl2_readl(kl2_dev, base + 4 * 185);
        udelay(50);
    }

    if (v != 0x8)
        KL2_LOGW("failed, %px = 0x%x\n", base + 4 * 185, v);

    return 0;
}

static int __gddr_init_controller(struct kl2_device *kl2_dev, int ch)
{
    u32           i       = 0;
    void __iomem *base    = KL2_REG_GDDRCTRL_CHAN_BASE(kl2_dev->iomem_base.gddr_base, ch);
    const u32    *cfg     = NULL;
    int           cfg_num = 0;

    if (kl2_dev->ddr_conf.ecc_on && kl2_dev->ddr_conf.ddr_x8) {
        KL2_LOGW("DDR X8 with ECC is not supported for now.\n");
    } else if (kl2_dev->ddr_conf.ecc_on) {
        cfg     = gddr_ctrl_cfg_16g_ecc;
        cfg_num = GDDRCFG_NUM_16G_ECC;
    } else if (kl2_dev->ddr_conf.ddr_x8) {
        cfg     = gddr_ctrl_cfg_16g_x8;
        cfg_num = GDDRCFG_NUM_16G_X8;
    } else {
        cfg     = gddr_ctrl_cfg_16g;
        cfg_num = GDDRCFG_NUM_16G;
    }

    KL2_LOGD("init GDDR ch_%d\n", ch);

    for (i = 0; i < cfg_num; ++i)
        kl2_writel(kl2_dev, cfg[i], base + i * 4);

    kl2_writel(kl2_dev, 0xe01, base);

    // this is a temp route and could only be called during setup
    //kl2_dma_ddma_from_host(&kl2_dev->dma_engine, base, (u64)&gddr_ctrl_cfg[0], 4 * 273);
    //kl2_dma_ddma_from_host(&kl2_dev->dma_engine, base, (u64)&gddr_ctrl_cfg[273], 4);

    KL2_LOGI("finish controller cfg\n");
    return 0;
}

int kl2_gddr_interrupt_mask_init(struct kl2_device *kl2_dev)
{
    int ch;

    if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_SRIOV_OFF ||
        kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF) {
        for (ch = 0; ch < 8; ++ch) {
            void __iomem *ch_base = kl2_dev->iomem_base.gddr_base + ch * 0x10000;

            // reg:int_mask_master[0x2c4] mask all interrupts except BIT1 ecc error
            kl2_writel(kl2_dev, 0x0000fffd, ch_base + KL2_REG_GDDRCTRL_INT_MASK_MASTER);
            // reg:int_mask_ecc[0x30c] mask all interrupts except BIT0~BIT3 ecc errors
            kl2_writel(kl2_dev, 0x000000f0, ch_base + KL2_REG_GDDRCTRL_INT_MASK_ECC_LOWPOWER);
        }
    }
    return 0;
}

int kl2_gddr_init(struct kl2_device *kl2_dev)
{
    int ch, i;

    KL2_LOGI("ddr_ch = %d\n", kl2_dev->ddr_conf.nchannel);

    kl2_writel(kl2_dev, 0xc0, kl2_dev->iomem_base.syscon1_base + KL2_REG_SYSCON1_0290);

    for (ch = 0; ch < 8; ++ch) {
        if (kl2_dev->ddr_conf.nchannel == 6 && (ch == 0 || ch == 4))
            continue;

        __gddr_init_controller(kl2_dev, ch);
    }
    msleep(1000);

    for (ch = 0; ch < 8; ++ch) {
        void __iomem *ch_base = KL2_REG_GDDRCTRL_CHAN_BASE(kl2_dev->iomem_base.gddr_base, ch);

        if (kl2_dev->ddr_conf.nchannel == 6 && (ch == 0 || ch == 4))
            continue;

        for (i = 0; i < sizeof(gddr_mem_cfg) / sizeof(gddr_mem_cfg[0]); ++i) {
            if (gddr_mem_cfg[i][0] == 0x4 && kl2_dev->ddr_conf.ddr_x8) {
                __gddr_mrw(kl2_dev, ch_base, 0x4, 0x175);
                continue;
            }
            __gddr_mrw(kl2_dev, ch_base, gddr_mem_cfg[i][0], gddr_mem_cfg[i][1]);
        }

        // clear int
        kl2_writel(kl2_dev, 0xff000000, ch_base + KL2_REG_GDDRCTRL_INT_ACK_DFI);
        kl2_writel(kl2_dev, 0x000000ff, ch_base + KL2_REG_GDDRCTRL_INT_ACK_MODE);
    }
    return 0;
}

// TODO(miaotianxiang): 移到kl2_regs.h
// SDNN-CLUSTER EXCEPTION
// mask     bit       error                         desp
// 0        31        sdnn_exception                sdnn exception
// 0        30        core_trap                     core trap exception
// 0        29        reserved                      reserved for future use
// 0        28        reserved                      reserved for future use
// 0        27        lm_rdwr_conflict              lm rdwr conflict
// 0        26        sm_rdwr_conflict              sm rdwr conflict
// 1        25        sfu_floating_point_err        sfu floating point ip err
// 0        24        atom_nested_err               nested automatic operation err
// 0        23        axi1_wresp_err                axi1 wresp err
// 0        22        axi1_rresp_err                axi1 rresp err
// 0        21        axi0_wresp_err                axi0 wresp err
// 0        20        axi0_rresp_err                axi0 rresp err
// 1        19        simd_floating_point_err       simd floating point ip error, just like bit 14-8
// 0        18        simd_instr_undef              simd undefined instruction
// 0        17        simd_overflow                 simd operation exceed memory size
// 0        16        simd_unalign                  simd operation address unalign
// 0        15        hardware_fatal                hardware fatal
// 1        14        floating_point_err            floating point huge int error, when cast huge floating point num to int
// 1        13        floating_point_err            floating point inexact error
// 1        12        floating_point_err            floating point output huge, subnormal
// 1        11        floating_point_err            floating point output tiny, subnormal
// 1        10        floating_point_err            floating point op Invalid, nan as one operator
// 1        9         floating_point_err            floating point output is infinity
// 1        8         floating_point_err            floating point output is zero
// 0        7         dma_len_zero                  dma operation length equal zero
// 0        6         dma_overflow                  dma operation exceed memory size
// 0        5         ld_st_unalign                 load/store operation address unalign
// 0        4         ld_st_overflow                load/store operation exceed memory size
// 1        3         fp_div_zero                   floating point divided by zero
// 1        2         nan_err,RO,0x0                not a floating number
// 0        1         instr_undef                   undefined instruction
// 0        0         pc_overflow                   program counter exceed code length
//
// b   0000 0010 0000 1000 0111 1111 0000 1100
// 0x  0    2    0    8    7    f    0    c
int kl2_compute_unit_init(struct kl2_device *kl2_dev)
{
    int i;

    for_each_valid_cluster(kl2_dev, i) {
        kl2_cluster_set_excp_mask(kl2_dev, i, 0x02087f0c);
        KL2_LOGD("mask cluster%d fp error, v=%x\n", i,
                 kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(kl2_dev->iomem_base.cluster_base, i) +
                                            KL2_REG_CLUSTER_EXCEPTION_MASK));

        // ICACHE ERRATA: http://wiki.baidu.com/display/ISA/6.8+Case+Study
        kl2_writel(kl2_dev, 0x2,
                   KL2_REG_CLUSTER_BASE(kl2_dev->iomem_base.cluster_base, i) +
                           KL2_REG_CLUSTER_L2_REPALCE_LEN);
    }

    for_each_valid_sdnn(kl2_dev, i) {
        kl2_sdnn_set_cl_excp_mask(kl2_dev, i, 0x02087f0c);
        kl2_sdnn_set_sd_excp_mask(kl2_dev, i, 0x0);
        KL2_LOGD("mask sdnn%d fp error, v=%x\n", i,
                 kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(kl2_dev->iomem_base.sdnn_base, i) +
                                            KL2_REG_CLUSTER_EXCEPTION_MASK));

        // ICACHE ERRATA: http://wiki.baidu.com/display/ISA/6.8+Case+Study
        kl2_writel(kl2_dev, 0x2,
                   KL2_REG_SDNN_CLUSTER_BASE(kl2_dev->iomem_base.sdnn_base, i) +
                           KL2_REG_CLUSTER_L2_REPALCE_LEN);
    }

    return 0;
}

void cluster_stop_dma(struct kl2_device *kl2_dev, int idx)
{
    void __iomem *base = kl2_dev->iomem_base.cluster_base;

    kl2_writel(kl2_dev, (1u << 16), KL2_REG_CLUSTER_BASE(base, idx) + 0x88);
}

void sdnn_stop_dma(struct kl2_device *kl2_dev, int idx)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;

    kl2_writel(kl2_dev, (1u << 16),
               KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, idx) + KL2_REG_CLUSTER_DMA_CTRL);
    kl2_writel(kl2_dev, 1,
               KL2_REG_SDNN_BASE(sdnn_base, idx) +
                       KL2_REG_SDNN_DMAI0_MASK_DMA); // disable sdnn dma_i0
    kl2_writel(kl2_dev, 1,
               KL2_REG_SDNN_BASE(sdnn_base, idx) +
                       KL2_REG_SDNN_DMAI1_MASK_DMA); // disable sdnn dma_i1
    kl2_writel(kl2_dev, 1,
               KL2_REG_SDNN_BASE(sdnn_base, idx) + KL2_REG_SDNN_DMAO_BLOCK); // disable sdnn dma_o
}

void cluster_reset(struct kl2_device *kl2_dev, int idx)
{
    void __iomem *syscon0_base = kl2_dev->iomem_base.syscon0_base;

    kl2_writel(kl2_dev, (1u << idx), syscon0_base + KL2_REG_SYSCON0_CLUSTER_RST_CTRL);
    udelay(1);
    kl2_writel(kl2_dev, 0, syscon0_base + KL2_REG_SYSCON0_CLUSTER_RST_CTRL);
}

void sdnn_reset(struct kl2_device *kl2_dev, int idx)
{
    void __iomem *syscon0_base = kl2_dev->iomem_base.syscon0_base;

    kl2_writel(kl2_dev, (1u << idx), syscon0_base + KL2_REG_SYSCON0_SDNN_RST_CTRL);
    udelay(1);
    kl2_writel(kl2_dev, 0, syscon0_base + KL2_REG_SYSCON0_SDNN_RST_CTRL);
}

/*
  NOC main 0:
    [31:26] reserved
    [25:25] debug
    [24:21] sdnn0
    [20:17] sdnn1
    [16:13] sdnn2
    [12:12] sse
    [11:09] cluster0
    [08:06] cluster1
    [05:03] cluster2
    [02:00] cluster6

  NOC main 1:
    [31:24] reserved
    [23:20] sdnn3
    [19:16] sdnn4
    [15:12] sdnn5
    [11:09] cluster3
    [08:06] cluster4
    [05:03] cluster5
    [02:00] cluster7

*/
struct noc_pending_entry {
    int syscon_id;  // syscon0 or syscon1
    int bit_start;  // start bit
    int bit_length; // bit length
};

static const struct noc_pending_entry cluster_noc_entry[] = {
    { 0, 9, 3 }, // cluster 0: noc_main0_no_pending reg, bits [9 -> 11]
    { 0, 6, 3 }, { 0, 3, 3 }, { 1, 9, 3 }, { 1, 6, 3 }, { 1, 3, 3 }, { 0, 0, 3 }, { 1, 0, 3 },
};
static const struct noc_pending_entry sdnn_noc_entry[] = {
    { 0, 21, 4 }, // sdnn 0: noc_main0_no_pending reg, bits [21 -> 24]
    { 0, 17, 4 }, { 0, 13, 4 }, { 1, 20, 4 }, { 1, 16, 4 }, { 1, 12, 4 },
};

static int wait_for_noc_idle(struct kl2_device *kl2_dev, const struct noc_pending_entry *noc_entry,
                             u32 timeout_us)
{
    void __iomem *syscon0_base = kl2_dev->iomem_base.syscon0_base;
    void __iomem *syscon1_base = kl2_dev->iomem_base.syscon1_base;
    size_t        bit_idx      = 0;
    size_t        bit_len      = 0;
    u32           val          = 0;
    u32           mask         = 0;
    u32           cnt          = timeout_us;

    const void __iomem *noc_main[2]     = { NOC_MAIN0_PENDING, NOC_MAIN1_PENDING };
    const void __iomem *noc_pending_reg = noc_main[noc_entry->syscon_id];

    bit_idx = noc_entry->bit_start;
    bit_len = noc_entry->bit_length;
    mask    = ((1u << bit_len) - 1) << bit_idx;

    while (cnt-- > 0) {
        val = kl2_readl(kl2_dev, noc_pending_reg);
        if ((val & mask) == mask) {
            break;
        }
        udelay(1);
    }

    val = kl2_readl(kl2_dev, noc_pending_reg);
    if ((val & mask) == mask) {
        return 0;
    }

    return -XPUERR_TIMEOUT;
}

int wait_for_noc_cluster(struct kl2_device *kl2_dev, int idx)
{
    return wait_for_noc_idle(kl2_dev, &cluster_noc_entry[idx], WAIT_NOC_TIMEOUT);
}

int wait_for_noc_sdnn(struct kl2_device *kl2_dev, int idx)
{
    return wait_for_noc_idle(kl2_dev, &sdnn_noc_entry[idx], WAIT_NOC_TIMEOUT);
}

void cluster_clear_exception(struct kl2_device *kl2_dev, int idx, u32 mask)
{
    void __iomem *base = kl2_dev->iomem_base.cluster_base;
    if (!mask) {
        return;
    }

    kl2_writel(kl2_dev, mask, KL2_REG_CLUSTER_BASE(base, idx) + KL2_REG_CLUSTER_CLR_EXCEPTION);
}

void sdnn_cl_clear_exception(struct kl2_device *kl2_dev, int idx, u32 mask)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    if (!mask) {
        return;
    }

    kl2_writel(kl2_dev, mask,
               KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, idx) + KL2_REG_CLUSTER_CLR_EXCEPTION);
}

void sdnn_clear_exception(struct kl2_device *kl2_dev, int idx, u32 mask)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    if (!mask) {
        return;
    }

    kl2_writel(kl2_dev, mask, KL2_REG_SDNN_BASE(sdnn_base, idx) + KL2_REG_SDNN_CLR_EXCEPTION);
}

int kl2_virt_get_numvfs(struct kl2_device *kl2_dev)
{
    struct kl_device *kdev = kl2_dev->kdev;
    int               v    = kl2_readl(kl2_dev, kdev->bar[2] + KL2_REG_VAC_BAR2_BASE + 0x4);
    KL2_LOGD("VAC(0x4)= %d\n", v);
    return (v >> 29) & 0x3;
}

static int kl2_set_vf_page(struct kl2_device *kl2_dev, int mem_kind, int vf_id,
                           int start_addr_in_mb, int end_addr_in_mb)
{
    struct kl_device *kdev    = kl2_dev->kdev;
    u32               vf_page = 0;

    // VF page register: enable_flag[31] | start_addr[27:16] | end_addr[11:0]
    vf_page = 0x80000000 | ((start_addr_in_mb & 0xffff) << 16) | (end_addr_in_mb & 0xffff);
    if (mem_kind == XPU_MEM_L3) {
        kl2_writel(kl2_dev, vf_page,
                   kdev->bar[2] + KL2_REG_VMUL3_BAR2_BASE + 0x0020 + (vf_id * 0x4));
        kl2_writel(kl2_dev, vf_page,
                   kdev->bar[2] + KL2_REG_VMUL3RO_BAR2_BASE + 0x0020 + (vf_id * 0x4));
    } else {
        kl2_writel(kl2_dev, vf_page,
                   kdev->bar[2] + KL2_REG_VMUGDDR_BAR2_BASE + 0x0020 + (vf_id * 0x4));
    }

    return 0;
}

static int kl2_config_vmu(struct kl2_device *kl2_dev, int num_vfs)
{
    struct kl_device *kdev          = kl2_dev->kdev;
    u64               gddr_mem_size = 0;
    u64               gddr_mem_size_in_mb;
    u64               gddr_mem_size_per_ch_in_mb;
    u64               l3_mem_size;
    u64               l3_mem_size_in_mb;
    u64               l3_mem_size_per_ch_in_mb;
    int               i;
    u32               v;

    if (num_vfs == 0)
        v = 0;
    else
        v = ((num_vfs & 0x3) << 5) | 0x11;

    kl2_writel(kl2_dev, v, kdev->bar[2] + KL2_REG_VMUL3_BAR2_BASE);
    kl2_writel(kl2_dev, v, kdev->bar[2] + KL2_REG_VMUL3RO_BAR2_BASE);
    kl2_writel(kl2_dev, v, kdev->bar[2] + KL2_REG_VMUGDDR_BAR2_BASE);

    if (num_vfs) {
        struct kl_mm_info *mm_info = kl2_get_vf_mm_info(kl2_dev);

        gddr_mem_size = mm_info->user_dma_rr_rw_range[0].size;
        l3_mem_size   = mm_info->user_dma_rr_rw_range[1].size;

        gddr_mem_size_in_mb = gddr_mem_size >> 20;
        l3_mem_size_in_mb   = l3_mem_size >> 20;
        KL2_LOGI("set VF vmu mem size, gddr: %lldMB, l3: %lldMB", gddr_mem_size_in_mb,
                 l3_mem_size_in_mb);

        //set VF mem size in MB
        kl2_writel(kl2_dev, gddr_mem_size_in_mb - 1,
                   kdev->bar[2] + KL2_REG_VMUGDDR_BAR2_BASE + 0x0004);
        kl2_writel(kl2_dev, l3_mem_size_in_mb - 1, kdev->bar[2] + KL2_REG_VMUL3_BAR2_BASE + 0x0004);
        kl2_writel(kl2_dev, l3_mem_size_in_mb - 1,
                   kdev->bar[2] + KL2_REG_VMUL3RO_BAR2_BASE + 0x0004);

        //set VF mem page config
        gddr_mem_size_per_ch_in_mb = gddr_mem_size_in_mb / 8;
        l3_mem_size_per_ch_in_mb   = l3_mem_size_in_mb / 8;
        for (i = 0; i < num_vfs; i++) {
            kl2_set_vf_page(kl2_dev, XPU_MEM_MAIN, i, gddr_mem_size_per_ch_in_mb * i,
                            gddr_mem_size_per_ch_in_mb * (i + 1) - 1);
            kl2_set_vf_page(kl2_dev, XPU_MEM_L3, i, l3_mem_size_per_ch_in_mb * i,
                            l3_mem_size_per_ch_in_mb * (i + 1) - 1);
        }
    }

    return 0;
}

int kl2_virt_set_numvfs(struct kl2_device *kl2_dev, int num_vfs)
{
    struct kl_device *kdev = kl2_dev->kdev;
    u32               v;

    if (num_vfs == 0)
        v = 0;
    else
        v = ((num_vfs & 0x3) << 5) | 0x11;

    kl2_writel(kl2_dev, v, kdev->bar[2] + KL2_REG_VAC_BAR2_BASE);

    kl2_config_vmu(kl2_dev, num_vfs);

    kl2_writel(kl2_dev, num_vfs, kl2_dev->iomem_base.sse_base + KL2_REG_SSE_VUSERS);
    kl2_writel(kl2_dev, num_vfs, kl2_dev->iomem_base.intc_base + KL2_REG_INTC_PROC_MODE);

    return 0;
}

void kl2_reg_lock(struct kl2_device *kl2_dev)
{
    while (atomic_cmpxchg(&kl2_dev->reg_lock, 0, 1) != 0)
        ;
}

int kl2_reg_trylock(struct kl2_device *kl2_dev)
{
    u32       i     = 0;
    const u32 retry = 20;

    do {
        // CAS成功
        if (atomic_cmpxchg(&kl2_dev->reg_lock, 0, 1) == 0) {
            return 0;
        }
        cpu_relax();
    } while (i++ < retry);
    // #retry次CAS失败
    return 1;
}

void kl2_reg_unlock(struct kl2_device *kl2_dev)
{
    atomic_set(&kl2_dev->reg_lock, 0);
}

u32 kl2_intc_get_int_mask(struct kl2_device *kl2_dev, int i)
{
    u32           val;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    val = kl2_dev->reg_shadow.intc_int_mask[i];
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
    return val;
}

void kl2_intc_set_int_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    u32 offset = (i < 9) ? (KL2_REG_INTC_MASK_0 + i * 4) : (KL2_REG_INTC_MASK_9 + (i - 9) * 4);
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.intc_int_mask[i] = mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.intc_int_mask[i],
               kl2_dev->iomem_base.intc_base + offset);
    kl2_readl(kl2_dev, kl2_dev->iomem_base.intc_base + offset);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_intc_int_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    u32 offset = (i < 9) ? (KL2_REG_INTC_MASK_0 + i * 4) : (KL2_REG_INTC_MASK_9 + (i - 9) * 4);
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.intc_int_mask[i] |= mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.intc_int_mask[i],
               kl2_dev->iomem_base.intc_base + offset);
    kl2_readl(kl2_dev, kl2_dev->iomem_base.intc_base + offset);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_intc_int_unmask(struct kl2_device *kl2_dev, int i, u32 unmask)
{
    u32 offset = (i < 9) ? (KL2_REG_INTC_MASK_0 + i * 4) : (KL2_REG_INTC_MASK_9 + (i - 9) * 4);
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.intc_int_mask[i] &= ~unmask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.intc_int_mask[i],
               kl2_dev->iomem_base.intc_base + offset);
    kl2_readl(kl2_dev, kl2_dev->iomem_base.intc_base + offset);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

u32 kl2_intc_get_host_mask(struct kl2_device *kl2_dev, int i)
{
    u32           val;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    val = kl2_dev->reg_shadow.intc_host_mask[i];
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
    return val;
}

void kl2_intc_set_host_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    u32 offset =
            (i < 9) ? (KL2_REG_INTC_HOST_MASK_0 + i * 4) : (KL2_REG_INTC_HOST_MASK_9 + (i - 9) * 4);
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.intc_host_mask[i] = mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.intc_host_mask[i],
               kl2_dev->iomem_base.intc_base + offset);
    kl2_readl(kl2_dev, kl2_dev->iomem_base.intc_base + offset);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_intc_host_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    u32 offset =
            (i < 9) ? (KL2_REG_INTC_HOST_MASK_0 + i * 4) : (KL2_REG_INTC_HOST_MASK_9 + (i - 9) * 4);
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.intc_host_mask[i] |= mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.intc_host_mask[i],
               kl2_dev->iomem_base.intc_base + offset);
    kl2_readl(kl2_dev, kl2_dev->iomem_base.intc_base + offset);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_intc_host_unmask(struct kl2_device *kl2_dev, int i, u32 unmask)
{
    u32 offset =
            (i < 9) ? (KL2_REG_INTC_HOST_MASK_0 + i * 4) : (KL2_REG_INTC_HOST_MASK_9 + (i - 9) * 4);
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.intc_host_mask[i] &= ~unmask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.intc_host_mask[i],
               kl2_dev->iomem_base.intc_base + offset);
    kl2_readl(kl2_dev, kl2_dev->iomem_base.intc_base + offset);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

u32 kl2_cluster_get_excp_mask(struct kl2_device *kl2_dev, int i)
{
    u32           val;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    val = kl2_dev->reg_shadow.cl_excp_mask[i];
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
    return val;
}

void kl2_cluster_set_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    void __iomem *cluster_base = kl2_dev->iomem_base.cluster_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.cl_excp_mask[i] = mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.cl_excp_mask[i],
               KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_cluster_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    void __iomem *cluster_base = kl2_dev->iomem_base.cluster_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.cl_excp_mask[i] |= mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.cl_excp_mask[i],
               KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_cluster_excp_unmask(struct kl2_device *kl2_dev, int i, u32 unmask)
{
    void __iomem *cluster_base = kl2_dev->iomem_base.cluster_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.cl_excp_mask[i] &= ~unmask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.cl_excp_mask[i],
               KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

u32 kl2_sdnn_get_cl_excp_mask(struct kl2_device *kl2_dev, int i)
{
    u32           val;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    val = kl2_dev->reg_shadow.sdnn_cl_excp_mask[i];
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
    return val;
}

void kl2_sdnn_set_cl_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.sdnn_cl_excp_mask[i] = mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sdnn_cl_excp_mask[i],
               KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_sdnn_cl_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.sdnn_cl_excp_mask[i] |= mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sdnn_cl_excp_mask[i],
               KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_sdnn_cl_excp_unmask(struct kl2_device *kl2_dev, int i, u32 unmask)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.sdnn_cl_excp_mask[i] &= ~unmask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sdnn_cl_excp_mask[i],
               KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

u32 kl2_sdnn_get_sd_excp_mask(struct kl2_device *kl2_dev, int i)
{
    u32           val;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    val = kl2_dev->reg_shadow.sdnn_sd_excp_mask[i];
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
    return val;
}

void kl2_sdnn_set_sd_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.sdnn_sd_excp_mask[i] = mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sdnn_sd_excp_mask[i],
               KL2_REG_SDNN_BASE(sdnn_base, i) + KL2_REG_SDNN_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, i) + KL2_REG_SDNN_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_sdnn_sd_excp_mask(struct kl2_device *kl2_dev, int i, u32 mask)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.sdnn_sd_excp_mask[i] |= mask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sdnn_sd_excp_mask[i],
               KL2_REG_SDNN_BASE(sdnn_base, i) + KL2_REG_SDNN_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, i) + KL2_REG_SDNN_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_sdnn_sd_excp_unmask(struct kl2_device *kl2_dev, int i, u32 unmask)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->reg_shadow.sdnn_sd_excp_mask[i] &= ~unmask;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sdnn_sd_excp_mask[i],
               KL2_REG_SDNN_BASE(sdnn_base, i) + KL2_REG_SDNN_EXCEPTION_MASK);
    kl2_readl(kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, i) + KL2_REG_SDNN_EXCEPTION_MASK);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

u32 kl2_sse_get_cuen_cu_disable_mask(struct kl2_device *kl2_dev)
{
    u32           val;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    val = kl2_dev->cuen_cu_disable_mask;
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
    return val;
}

void kl2_sse_set_cuen_cu_disable_mask(struct kl2_device *kl2_dev, u32 mask)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->cuen_cu_disable_mask = (mask & 0x003f00ffu);
    kl2_dev->reg_shadow.sse_cu_disable_mask =
            (kl2_dev->cuen_cu_disable_mask | kl2_dev->exception_cu_disable_mask) & 0x003f00ffu;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sse_cu_disable_mask,
               sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_sse_excp_disable_clusters(struct kl2_device *kl2_dev, u32 mask)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->exception_cu_disable_mask |= (mask & 0xffu);
    kl2_dev->reg_shadow.sse_cu_disable_mask =
            (kl2_dev->cuen_cu_disable_mask | kl2_dev->exception_cu_disable_mask) & 0x003f00ffu;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sse_cu_disable_mask,
               sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_sse_excp_enable_clusters(struct kl2_device *kl2_dev, u32 mask)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->exception_cu_disable_mask &= ~(mask & 0xffu);
    kl2_dev->reg_shadow.sse_cu_disable_mask =
            (kl2_dev->cuen_cu_disable_mask | kl2_dev->exception_cu_disable_mask) & 0x003f00ffu;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sse_cu_disable_mask,
               sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_sse_excp_disable_sdnns(struct kl2_device *kl2_dev, u32 mask)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->exception_cu_disable_mask |= ((mask & 0x3fu) << 16);
    kl2_dev->reg_shadow.sse_cu_disable_mask =
            (kl2_dev->cuen_cu_disable_mask | kl2_dev->exception_cu_disable_mask) & 0x003f00ffu;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sse_cu_disable_mask,
               sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_sse_excp_enable_sdnns(struct kl2_device *kl2_dev, u32 mask)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    unsigned long flags;
    spin_lock_irqsave(&kl2_dev->sc_lock, flags);
    kl2_dev->exception_cu_disable_mask &= ~((mask & 0x3fu) << 16);
    kl2_dev->reg_shadow.sse_cu_disable_mask =
            (kl2_dev->cuen_cu_disable_mask | kl2_dev->exception_cu_disable_mask) & 0x003f00ffu;
    kl2_writel(kl2_dev, kl2_dev->reg_shadow.sse_cu_disable_mask,
               sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    spin_unlock_irqrestore(&kl2_dev->sc_lock, flags);
}

void kl2_debug_master_enable(struct kl2_device *kl2_dev, u32 time_stamp, u32 stamp_interval,
                             u32 lost_interval, u64 sgdesc_addr, u32 port)
{
    void __iomem *base = kl2_dev->iomem_base.dbgm_base;
    kl2_writel(kl2_dev, 0x0, kl2_dev->iomem_base.syscon0_base + 0x7d4);
    kl2_writel(kl2_dev, time_stamp, base + KL2_RGE_DBGM_TIME_STAMP);          // enable_global_timer
    kl2_writel(kl2_dev, stamp_interval, base + KL2_REG_DBGM_STAMP_INTERVAL);  // stamp_interval
    kl2_writel(kl2_dev, lost_interval, base + KL2_REG_DBGM_LOST_INTERVAL);    // lost trace stamp
    kl2_writel(kl2_dev, 0xf, base + KL2_REG_DBGM_AXI_LEN);                    // axilen
    kl2_writel(kl2_dev, BIT(1) | BIT(0), base + KL2_REG_DBGM_DMA_CFG);        // dma cfg
    kl2_writel(kl2_dev, low32(sgdesc_addr), base + KL2_REG_DBGM_LL_ADDR_LOW); // ll_addr_low
    kl2_writel(kl2_dev, high32(sgdesc_addr), base + KL2_REG_DBGM_LL_ADDR_HIGH); // ll_addr_high
    kl2_writel(kl2_dev, port, base + KL2_REG_DBGM_PORT);                        // port enable
    kl2_writel(kl2_dev, BIT(0), base + KL2_REG_DBGM_DMA_START);                 // dma_start
}

void kl2_debug_master_port_disable(struct kl2_device *kl2_dev)
{
    int i, hwq_id;
    // disable sse trace
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        kl2_writel(kl2_dev, 0x3f,
                   KL2_REG_SSE_QCTRL_BASE(kl2_dev->iomem_base.sse_base, hwq_id) + 0x8);
    }
    // disable cluster trace
    for_each_valid_cluster(kl2_dev, i) {
        kl2_writel(kl2_dev, 0xff, KL2_REG_CLUSTER_BASE(kl2_dev->iomem_base.cluster_base, i) + 0x90);
    }
    // disable sdnn trace
    for_each_valid_sdnn(kl2_dev, i) {
        kl2_writel(kl2_dev, 0x1ff, KL2_REG_SDNN_BASE(kl2_dev->iomem_base.sdnn_base, i) + 0x18);
        kl2_writel(kl2_dev, 0xff,
                   KL2_REG_SDNN_CLUSTER_BASE(kl2_dev->iomem_base.sdnn_base, i) + 0x90);
    }
}

int kl2_debug_master_disable(struct kl2_device *kl2_dev)
{
    void __iomem *base        = kl2_dev->iomem_base.dbgm_base;
    ktime_t       begin_ktime = ktime_get();
    u32           finished;
    int           ret = 0;

    kl2_writel(kl2_dev, 0, base + KL2_REG_DBGM_PORT);
    kl2_writel(kl2_dev, BIT(0), base + KL2_REG_DBGM_DMA_ABORT); //abort dma

    finished = kl2_readl(kl2_dev, base + KL2_REG_DBGM_INT_STAT);
    while ((finished & 0x1) == 0) {
        if (ktime_to_ms(ktime_sub(ktime_get(), begin_ktime)) > 1000 /* 1s */) {
            ret = -XPUERR_HWEXCEPTION;
            break;
        }
        msleep(100);
        finished = kl2_readl(kl2_dev, base + KL2_REG_DBGM_INT_STAT);
    }

    kl2_writel(kl2_dev, 0x7ff, base + KL2_REG_DBGM_INT_STAT_CLR); // clear intr stat

    return ret;
}
