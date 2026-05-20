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

#include "kl2/exception.h"
#include "kl2/hw.h"
#include "kl2/kl2_regs.h"
#include "kl2/mbox.h"

static struct kl2_excp_entry g_kl2_cluster_excp[] = {
#if defined(KL2_CE_ENTRY)
#undef KL2_CE_ENTRY
#endif

#define KL2_CE_ENTRY(i, n, l) { CE_##i, n, l },
    KL2_CLUSTER_EXCEPTIONS
};

static struct kl2_excp_entry g_kl2_sdnn_excp[] = {
#if defined(KL2_CE_ENTRY)
#undef KL2_CE_ENTRY
#endif

#define KL2_CE_ENTRY(i, n, l) { CE_##i, n, l },
    KL2_SDNN_EXCEPTIONS
};

/*
 *  GDDR exception routines
 */
static const char __maybe_unused *gddr_timeout_info[] = {
    "The MRR temperature check FM timeout has expired.", // BIT[14]
    "Reserved",
    "The DFI update FM timeout has expired.",                     // BIT[16]
    "The low power interface wakeup timeout has expired.(bit17)", // BIT[17]
    "The low power interface wakeup timeout has expired.(bit18)", // BIT[18]
    "The auto-refresh max deficit timeout has expired."           // BIT[19]
};

static const char __maybe_unused *gddr_ecc_irq_info[] = {
    "A correctable ECC event has been detected.",                // BIT[0]
    "Multiple correctable ECC events have been detected.",       // BIT[1]
    "A uncorrectable ECC event has been detected.",              // BIT[2]
    "Multiple uncorrectable ECC events have been detected.",     // BIT[3]
    "BIT[4] not ECC error",                                      // BIT[4]
    "BIT[5] not ECC error",                                      // BIT[5]
    "One or more ECC writeback commands could not be executed.", // BIT[6]
    "The scrub operation triggered by setting the ecc_scrub_start parameter has completed.", // BIT[7]
    "An ECC correctable error has been detected in a scrubbing read operation." // BIT[8]
};

static const char __maybe_unused *gddr_user_interface_irq_info[] = {
    "A memory access outside the defined PHYSICAL memory space has occurred.",         // BIT[0]
    "Multiple accesses outside the defined PHYSICAL memory space have occurred.",      // BIT[1]
    "An error occurred on the port command channel.",                                  // BIT[2]
    "BIT[3] not a user interface error",                                               // BIT[3]
    "BIT[4] not a user interface error",                                               // BIT[4]
    "BIT[5] not a user interface error",                                               // BIT[5]
    "A wrap cycle crossing a DRAM page has been detected.",                            // BIT[6]
    "The user has programmed an invalid setting associated with core words per burst." // BIT[7]
};

static const char __maybe_unused *gddr_misc_irq_info[] = {
    "BIT[0] not a misc error", // BIT[0]
    "BIT[1] not a misc error", // BIT[1]
    "BIT[2] not a misc error", // BIT[2]
    "The assertion of the inhibit_dram_cmd parameter has successfully inhibited the command queue and/or MRR traffic.", // BIT[3]
    "BIT[4] not a misc error", // BIT[4]
    "The last automatic MRR of MR4 indicated a change in the device temperature or refresh rate (TUF bit set).", // BIT[5]
    "A temperature alert condition (low or high temp) has been detected", // BIT[6]
    "The refresh operation has resulted in a status bit being set."       // BIT[7]
};

static const char __maybe_unused *gddr_dfi_irq_info[] = {
    "A DFI update error has occurred.",               // BIT[0]
    "A DFI PHY Master Interface error has occurred.", // BIT[1]
    "Error received from the PHY on the DFI bus.",    // BIT[2]
    "A state change has been detected on the dfi init complete signal after initialization.", // BIT[3]
    "The user-initiated DLL resynchronization has completed.", // BIT[4]
    "The DFI tinit-complete value has timed out.",             // BIT[5]
};

static void kl2_handle_gddr_exception_ch(struct kl2_device *kl2_dev, int channel)
{
    void __iomem *gddr_base = kl2_dev->iomem_base.gddr_base;
    void __iomem *ch_base   = KL2_REG_GDDRCTRL_CHAN_BASE(gddr_base, channel);
    u32           status_master;
    int           log_enable;
    int           idx;

    status_master = kl2_readl(kl2_dev, ch_base + KL2_REG_GDDRCTRL_INT_STATUS_MASTER);
    log_enable    = (jiffies - g_driver_load_time) > (10 * HZ) ? 1 : 0;

    if (status_master & (1u << 1)) {
        /* ecc error */
        u32 status = kl2_readl(kl2_dev, ch_base + KL2_REG_GDDRCTRL_INT_STATUS_ECC);
        if (status != 0) {
            //不打印correctable ECC event日志，避免过多的相关日志打印。
            for (idx = 2; idx < ARRAY_SIZE(gddr_ecc_irq_info); ++idx) {
                if ((status & (1u << idx)) && log_enable) {
                    KL2_LOG_XID(XPU_XID1, "gddr channel%d ECC error: %s\n", channel,
                                gddr_ecc_irq_info[idx]);
                }
            }
            if (status & (1u << 0)) {
                kl2_dev->dev_info.ecc_sbe_count++;
            }
            if (status & (1u << 2)) {
                kl2_dev->dev_info.ecc_dbe_count++;
            }
            kl2_writel(kl2_dev, status, ch_base + KL2_REG_GDDRCTRL_INT_ACK_ECC);
        }
    }

#ifdef GDDR_EXCEPTION_DEBUG
    if (status_master & (1u << 0)) {
        /* timeout error */
        u32 status = kl2_readl(kl2_dev, ch_base + KL2_REG_GDDRCTRL_INT_STATUS_TIMEOUT);
        for (idx = 0; idx < ARRAY_SIZE(gddr_timeout_info); ++idx) {
            if ((status & (1u << (idx + 14))) && log_enable) {
                KL2_LOGW_RATELIMITED("gddr channel%d timeout error: %s\n", channel,
                                     gddr_timeout_info[idx]);
            }
        }
        if (status != 0) {
            kl2_writel(kl2_dev, status, ch_base + KL2_REG_GDDRCTRL_INT_ACK_TIMEOUT);
        }
    }

    if (status_master & (1u << 6)) {
        /* user interface error */
        u32 status = kl2_readl(kl2_dev, ch_base + KL2_REG_GDDRCTRL_INT_STATUS_USERIF);
        for (idx = 0; idx < ARRAY_SIZE(gddr_user_interface_irq_info); ++idx) {
            if ((status & (1u << idx)) && log_enable) {
                KL2_LOGW_RATELIMITED("gddr channel%d user interface error: %s\n", channel,
                                     gddr_user_interface_irq_info[idx]);
            }
        }
        if (status != 0) {
            kl2_writel(kl2_dev, status, ch_base + KL2_REG_GDDRCTRL_INT_ACK_USERIF);
        }
    }
    if (status_master & (1u << 7)) {
        /* miscellaneous  */
        u32 status = kl2_readl(kl2_dev, ch_base + KL2_REG_GDDRCTRL_INT_STATUS_MISC);
        for (idx = 0; idx < ARRAY_SIZE(gddr_misc_irq_info); ++idx) {
            if ((status & (1u << idx)) && log_enable) {
                KL2_LOGW_RATELIMITED("gddr channel%d miscellaneous error: %s\n", channel,
                                     gddr_misc_irq_info[idx]);
            }
        }
        if (status != 0) {
            kl2_writel(kl2_dev, status, ch_base + KL2_REG_GDDRCTRL_INT_ACK_MISC);
        }
    }
    if (status_master & (1u << 10)) {
        /* DFI error */
        u32 status = kl2_readl(kl2_dev, ch_base + KL2_REG_GDDRCTRL_INT_STATUS_DFI);
        for (idx = 0; idx < ARRAY_SIZE(gddr_dfi_irq_info); ++idx) {
            if ((status & (1u << idx)) && log_enable) {
                KL2_LOGW_RATELIMITED("gddr channel%d DFI error: %s\n", channel,
                                     gddr_dfi_irq_info[idx]);
            }
        }
        if (status != 0) {
            kl2_writel(kl2_dev, status, ch_base + KL2_REG_GDDRCTRL_INT_ACK_DFI);
        }
    }
#endif
}

void kl2_handle_gddr_excp(struct kl2_device *kl2_dev, u32 state)
{
    int ch = 0;
    /* check 8 channels */
    for (ch = 0; ch < 8; ++ch) {
        if (state & (1u << ch))
            kl2_handle_gddr_exception_ch(kl2_dev, ch);
    }
}

/*
 *  sse exception routines
 */
void kl2_handle_sse_excp(struct kl2_device *kl2_dev, u32 state)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    int           idx;

    KL2_LOG_XID(XPU_XID3, "sse exception %08x\n", state);

    if (state & (1u << 0)) {
        u32 sse_overflow_status;

        sse_overflow_status = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_STREAM_OVERFLOW_ERR_MAP);
        for (idx = 0; idx < 12; ++idx) {
            if (sse_overflow_status & (1 << idx))
                KL2_LOGW("sse stream %d overflow error\n", idx);
        }
    }
    if (state & (1u << 1)) {
        u32 sse_desc_status;

        sse_desc_status = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_QUEUE_DSC_ERR_MAP);
        for (idx = 0; idx < 12; ++idx) {
            if (sse_desc_status & (1 << idx))
                KL2_LOGW("sse stream %d wrong desc error\n", idx);
        }
    }
    if (state & (1u << 2)) {
        u32 sse_wresp_status;

        sse_wresp_status = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_WRESP_ERR_MAP);
        if (sse_wresp_status & (1u << 0))
            KL2_LOGW("sse slave wresp error\n");
        if (sse_wresp_status & (1u << 1))
            KL2_LOGW("sse decode wresp error\n");
    }
}

/*
 *  cluster/sdnn exception routines
 */
static int __maybe_unused kl2_get_cluster_excp_level(struct kl2_device *kl2_dev, int idx)
{
    return g_kl2_cluster_excp[idx].level;
}

const char *kl2_get_cluster_excp_name(struct kl2_device *kl2_dev, int idx)
{
    return g_kl2_cluster_excp[idx].name;
}

static int __maybe_unused kl2_get_sdnn_excp_level(struct kl2_device *kl2_dev, int sdnn_id, int idx)
{
    void __iomem       *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    enum kl2_excp_level ce_level  = g_kl2_sdnn_excp[idx].level;
    void __iomem       *reg_base  = KL2_REG_SDNN_BASE(sdnn_base, sdnn_id);

    switch (idx) {
    case CE_DS_0: {
        u32 ds0_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_DS0_EXCEPTION_MAP);
        if (ds0_status & 0x6) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_DS_1: {
        u32 ds0_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_DS1_EXCEPTION_MAP);
        if (ds0_status & 0x6) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_MAC: {
        u32 mac_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_MAC_EXP_STATUS);
        if (mac_status & 0xFFEF) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_EW: {
        u32 ew_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_EW_EXCEPTION_TYPE);
        if (ew_status & 0x77FF) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_RS: {
        u32 rs_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_RS_EXP_STATUS);
        if (rs_status & 0x7FDF) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_DMA_IN0: {
        u32 dma_i0_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_DMAI0_EXCEPTION_MAP);
        if (dma_i0_status & 0x6C) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_DMA_IN1: {
        u32 dma_i0_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_DMAI1_EXCEPTION_MAP);
        if (dma_i0_status & 0x6C) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_DMA_OUT: {
        u32 dmao_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_DMAO_EXCEPTION_MAP);
        if (dmao_status & 0xB) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_SCHEDULER: {
        u32 sch_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_SCH_EXCEPTION_TYPE);
        if (sch_status & 0x4) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    case CE_DS_MUX: {
        u32 sch_status = kl2_readl(kl2_dev, reg_base + KL2_REG_SDNN_DSMUX_EXCEPTION_MAP);
        if (sch_status & 0x1) {
            ce_level = EL_RESET_UNIT;
        }
        break;
    }
    default: {
        KL2_LOGW("invalid sdnn exception:%d\n", idx);
    }
    }

    return ce_level;
}

const char *kl2_get_sdnn_excp_name(struct kl2_device *kl2_dev, int idx)
{
    return g_kl2_sdnn_excp[idx].name;
}

//static void output_ds_detail(struct kl2_device *kl2_dev, void __iomem *reg_base, int ds_base)
//{
//    u32 ds_status = kl2_readl(kl2_dev, reg_base + ds_base + 0x4);
//
//    KL2_LOGW("DS_EXCEPTION_MAP: 0x%x\n", ds_status);
//    if (ds_status & 0x8) {
//        KL2_LOGW("sram error status: 0x%x\n", ds_base + 0x98);
//    }
//
//    if (ds_status & 0x4) {
//        u32 addr = 0;
//        for (addr = ds_base + 0x48; addr <= ds_base + 0x6C; addr += 4) {
//            KL2_LOGW("fifo status[%x] = 0x%x\n", addr, kl2_readl(kl2_dev, reg_base + addr));
//        }
//    }
//
//    if (ds_status & 0x2) {
//        KL2_LOGW("param error: 0x%x\n", ds_base + 0x40);
//    }
//
//    if (ds_status & 0x1) {
//        u32 addr = 0;
//        for (addr = ds_base + 0x20; addr <= ds_base + 0x2c; addr += 4) {
//            KL2_LOGW("excep instr: 0x%x\n", kl2_readl(kl2_dev, reg_base + addr));
//        }
//    }
//}
//
//static void __maybe_unused output_sdnn_details(struct kl2_device *kl2_dev, int sdnn_idx,
//                                               int module_idx)
//{
//    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
//    void __iomem *reg_base  = SDNN_BASE(sdnn_base, sdnn_idx);
//
//    switch (module_idx) {
//    case CE_EW: {
//        u32 ew_status         = kl2_readl(kl2_dev, reg_base + 0x3000);
//        u32 ew_subtype_status = kl2_readl(kl2_dev, sdnn_base + 0x3004);
//        KL2_LOGW("ew status: 0x%x, subtype_status: 0x%x\n", ew_status, ew_subtype_status);
//        if (ew_status & 0x1ff) { //任意一个FIFO发生underflow或overflow
//            KL2_LOGW("ew exception: FIFO underflow or overflow\n");
//        } else if (ew_status & 0x200) { //收到非法指令
//            KL2_LOGW("ew exception: undefine instr\n");
//        } else if (ew_status & 0x400) { //收到非法参数
//            if (ew_subtype_status &
//                0x1) { //非法参数，activ_type与lut_mode的错误组合,不reset最终计算结果错误
//                KL2_LOGW("ew exception: invald combine of activ_type and lut_mode\n");
//            } else if (ew_subtype_status & 0x2) { //非法参数，stream_len=0,状态机会卡住
//                KL2_LOGW("ew exception: stream_len is zero\n");
//            } else if (ew_subtype_status & 0x4) { //非法参数，pool_size=0
//                KL2_LOGW("ew exception: pool_size is zero\n");
//            } else if (ew_subtype_status & 0x8) { //非法参数，vld_core_num配置有误
//                KL2_LOGW("ew exception: invalid vld_core_num\n");
//            } else if (ew_subtype_status & 0x10) { //非法参数，错误的lut_range参数
//                KL2_LOGW("ew exception: invalid lut_range\n");
//            } else if (ew_subtype_status & 0x20) { //非法参数，错误的lut_range_max参数
//                KL2_LOGW("ew exception: invalid lut_range_max\n");
//            }
//        } else if (ew_status & 0x800) {
//            //浮点数异常，input或output data存在nan，该异常默认关闭
//            KL2_LOGW("ew exception: floating error,  input or output is nan\n");
//        } else if (ew_status & 0x3000) {
//            //sram读写越界，由于现在L1E sram深度不是2的幂次方，可能会有一些超过sram真实地址范围的地址，导致读回x值等
//            KL2_LOGW("ew exception: sram read/write address beyond physical range\n");
//        } else if (ew_status & 0x4000) { //错误的打开或关闭hazard auto check功能操作
//            KL2_LOGW("ew exception: invalid hazard auto check\n");
//        }
//        break;
//    }
//    case CE_DS_0: {
//        output_ds_detail(kl2_dev, reg_base, 0x1800);
//        break;
//    }
//    case CE_DS_1: {
//        output_ds_detail(kl2_dev, reg_base, 0x2000);
//        break;
//    }
//    case CE_MAC: {
//        KL2_LOGW("mac exception status: 0x%x\n", kl2_readl(kl2_dev, reg_base + 0x2850));
//        break;
//    }
//    case CE_RS: {
//        KL2_LOGW("rs exception status: 0x%x\n", kl2_readl(kl2_dev, reg_base + 0x3808));
//        break;
//    }
//    case CE_DMA_IN0: {
//        KL2_LOGW("dmai0 exception status: 0x%x\n", kl2_readl(kl2_dev, reg_base + 0x0804));
//        break;
//    }
//    case CE_DMA_IN1: {
//        KL2_LOGW("dmai1 exception status: 0x%x\n", kl2_readl(kl2_dev, reg_base + 0x1004));
//        break;
//    }
//    case CE_DMA_OUT: {
//        KL2_LOGW("dmao exception status: 0x%x\n", kl2_readl(kl2_dev, reg_base + 0x4004));
//        break;
//    }
//    case CE_SCHEDULER: {
//        KL2_LOGW("sched exception status: 0x%x\n", kl2_readl(kl2_dev, reg_base + 0x4804));
//        break;
//    }
//    case CE_DS_MUX: {
//        KL2_LOGW("ds_mux exception status: 0x%x\n", kl2_readl(kl2_dev, reg_base + 0x5008));
//        break;
//    }
//    default:
//        break;
//    }
//}

static void kl2_dump_cluster_debug_info(struct kl2_device *kl2_dev, int cl_id,
                                        struct kl2_cluster_debug_info *cl_debug_info)
{
    void __iomem *cluster_base = kl2_dev->iomem_base.cluster_base;
    u32           i;

    cl_debug_info->cl_virt_id =
            kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, cl_id) + KL2_REG_CLUSTER_ID);
    for (i = 0; i < KL2_REG_CLUSTER_DEBUG_ARRAY_SIZE; i++) {
        cl_debug_info->cl_debug_regs[i] =
                kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, cl_id) +
                                           KL2_REG_CLUSTER_DEBUG_ARRAY[i]);
    }
}

static void kl2_dump_sdnn_cl_debug_info(struct kl2_device *kl2_dev, int sdnn_id,
                                        struct kl2_sdnn_debug_info *sdnn_debug_info)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    u32           i;

    sdnn_debug_info->sdnn_cl_virt_id =
            kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, sdnn_id) + KL2_REG_CLUSTER_ID);
    // sdnn cluster has the same debug regs with cluster
    for (i = 0; i < KL2_REG_CLUSTER_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_cl_debug_regs[i] =
                kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, sdnn_id) +
                                           KL2_REG_CLUSTER_DEBUG_ARRAY[i]);
    }
}

static void kl2_dump_sdnn_sd_debug_info(struct kl2_device *kl2_dev, int sdnn_id,
                                        struct kl2_sdnn_debug_info *sdnn_debug_info)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    u32           i;

    for (i = 0; i < KL2_REG_SDNN_DMAI0_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_dmai0_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_DMAI0_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_DMAI1_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_dmai1_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_DMAI1_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_DS0_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_ds0_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_DS0_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_DS1_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_ds1_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_DS1_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_MAC_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_mac_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_MAC_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_EW_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_ew_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_EW_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_RS_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_rs_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_RS_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_DMAO_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_dmao_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_DMAO_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_SCH_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_sch_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_SCH_DEBUG_ARRAY[i]);
    }
    for (i = 0; i < KL2_REG_SDNN_DSMUX_DEBUG_ARRAY_SIZE; i++) {
        sdnn_debug_info->sdnn_dsmux_debug_regs[i] = kl2_readl(
                kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, sdnn_id) + KL2_REG_SDNN_DSMUX_DEBUG_ARRAY[i]);
    }
}

// 从hwq rt_list/pt_list移除全部异常sess包含的task到exception_list
// XXX(miaotianxiang): 可能存在正常中断响应慢，rt_list中正常结束的task未被移除的情况？
static void revoke_task_from_err_session_locked(struct kl2_hwq *hwq)
{
    struct kl2_device *kl2_dev __maybe_unused = hwq->kl2_dev;
    struct kl2_task           *task, *safe;

    list_for_each_entry_safe(task, safe, &hwq->rt_list, hwq_node) {
        if (atomic_read(&task->sess->state) == KL2_SESS_NORMAL)
            continue;

        if (task->type == KL2_TASKTYPE_EVNTREC)
            atomic_set(&task->evnt->state, KL2_EVNT_REVOKED);

        KL2_LOGD("revoke task(sess= %d, tk= %08x(%u)) from hwq_%d rt_list\n", task->sess->id,
                 task->desc.kernel.token, task->desc.kernel.token, hwq->id);
        list_move_tail(&task->hwq_node, &hwq->exception_list);
        hwq->cnt_all -= 1;
        hwq->cnt_running -= 1;
    }

    list_for_each_entry_safe(task, safe, &hwq->pt_list, hwq_node) {
        if (atomic_read(&task->sess->state) == KL2_SESS_NORMAL)
            continue;

        if (task->type == KL2_TASKTYPE_EVNTREC)
            atomic_set(&task->evnt->state, KL2_EVNT_REVOKED);

        KL2_LOGD("revoke task(sess= %d, tk= %08x(%u)) from hwq_%d pt_list\n", task->sess->id,
                 task->desc.kernel.token, task->desc.kernel.token, hwq->id);
        list_move_tail(&task->hwq_node, &hwq->exception_list);
        hwq->cnt_all -= 1;
    }
}

// TODO(miaotianxiang): 移到hwq.c
static void redispatch_running_task_locked(struct kl2_hwq *hwq)
{
    struct kl2_device *kl2_dev __maybe_unused = hwq->kl2_dev;
    struct kl2_task           *task;

    if (hwq->cnt_running > KL2_HWQ_DEPTH) {
        BUG();
    }

    list_for_each_entry(task, &hwq->rt_list, hwq_node) {
        KL2_LOGD("rewrite task(sess= %d, tk= %08x(%u)) to hwq_%d\n", task->sess->id,
                 task->desc.kernel.token, task->desc.kernel.token, hwq->id);
        kl2_sse_write_desc_locked(kl2_dev, kl2_dev->iomem_base.sse_base, &task->desc, hwq->id);
    }
}

static u32 kl2_save_etask(struct kl2_device *kl2_dev, struct kl2_task *etask, int hwq_id)
{
    u32               cur;
    struct kl2_etask *to_save;

    spin_lock(&kl2_dev->etasks_lock);
    // etasks_cur++
    cur     = atomic_inc_return(&kl2_dev->etasks_cur) - 1;
    to_save = &kl2_dev->etasks[cur % KL2_ETASK_SAVE_CNT];
    memcpy(&to_save->task, etask, sizeof(*etask));

    memcpy(&to_save->comm, &etask->sess->uproc->comm, TASK_COMM_LEN);
    memcpy(&to_save->cl_excp_st, &kl2_dev->exception_board_hwq[hwq_id].cl_excp_st,
           sizeof(to_save->cl_excp_st));
    memcpy(&to_save->cl_debug_info, &kl2_dev->exception_board_hwq[hwq_id].cl_debug_info,
           sizeof(to_save->cl_debug_info));
    memcpy(&to_save->sdnn_cl_excp_st, &kl2_dev->exception_board_hwq[hwq_id].sdnn_cl_excp_st,
           sizeof(to_save->sdnn_cl_excp_st));
    memcpy(&to_save->sdnn_sd_excp_st, &kl2_dev->exception_board_hwq[hwq_id].sdnn_sd_excp_st,
           sizeof(to_save->sdnn_sd_excp_st));
    memcpy(&to_save->sdnn_debug_info, &kl2_dev->exception_board_hwq[hwq_id].sdnn_debug_info,
           sizeof(to_save->sdnn_debug_info));
    spin_unlock(&kl2_dev->etasks_lock);

    return cur % KL2_ETASK_SAVE_CNT;
}

void kl2_cluster_disable_and_record_excp(struct kl2_device *kl2_dev, u32 st)
{
    void __iomem *cluster_base = kl2_dev->iomem_base.cluster_base;
    int           i, j;

    KL2_LOGD("int_stat_1= %08x\n", st);
    {
        u32 cl_disable_mask = 0;
        for_each_valid_cluster(kl2_dev, i) {
            if (st & (0x1u << (i * 2 + 1))) {
                cl_disable_mask |= (0x1u << i);
            }
        }
        // sse不再调度子任务到该cluster
        kl2_sse_excp_disable_clusters(kl2_dev, cl_disable_mask);
    }

    for_each_valid_cluster(kl2_dev, i) {
        if (st & (0x1u << (i * 2 + 1))) {
            u32           token;
            u32           cl_excp_st;
            u32           cl_excp_mask;
            unsigned long cl_excp_st_ul;
            int           hwq_id;

            token        = kl2_readl(kl2_dev,
                                     KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_TOKEN);
            cl_excp_st   = kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) +
                                                      KL2_REG_CLUSTER_EXCEPTION_STATE);
            cl_excp_mask = kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, i) +
                                                      KL2_REG_CLUSTER_EXCEPTION_MASK);
            kl2_dump_cluster_debug_info(kl2_dev, i,
                                        &kl2_dev->exception_board_cluster[i].cl_debug_info);

            if (is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
                /* use cu id instead of real hwq id to store hwq
                 * exception info and fix before recovering hwq later.
                 */
                hwq_id = -1;
            } else {
                hwq_id = kl2_sse_cluster_map_hwq(kl2_dev, 0, i);
                // TODO(miaotianxiang): 谨防hwq_id非法

                kl2_dev->exception_board_hwq[hwq_id].token         = token;
                kl2_dev->exception_board_hwq[hwq_id].cl_excp_st[i] = cl_excp_st & (~cl_excp_mask);
                memcpy(&kl2_dev->exception_board_hwq[hwq_id].cl_debug_info[i],
                       &kl2_dev->exception_board_cluster[i].cl_debug_info,
                       sizeof(kl2_dev->exception_board_cluster[i].cl_debug_info));
                // Refer to https://elixir.bootlin.com/linux/v5.15.2/source/Documentation/atomic_t.txt#L169
                //
                // ORDERING  (go read memory-barriers.txt first)
                // --------
                //
                // The rule of thumb:
                //
                //  - non-RMW operations are unordered;
                //
                //  - RMW operations that have no return value are unordered;
                //
                //  - RMW operations that have a return value are fully ordered;
                //
                //  - RMW operations that are conditional are unordered on FAILURE,
                //    otherwise the above rules apply.
                //
                // Except of course when an operation has an explicit ordering like:
                //
                //  {}_relaxed: unordered
                //  {}_acquire: the R of the RMW (or atomic_read) is an ACQUIRE
                //  {}_release: the W of the RMW (or atomic_set)  is a  RELEASE
                //
                // Where 'unordered' is against other memory locations. Address dependencies are
                // not defeated.
                //
                // Fully ordered primitives are ordered against everything prior and everything
                // subsequent. Therefore a fully ordered primitive is like having an smp_mb()
                // before and an smp_mb() after the primitive.
                //
                // wq延迟处理时，如读到excp_cnt非0，则token和excp_st等信息一定合法
                atomic_inc_return(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
            }
            kl2_dev->exception_board_cluster[i].token        = token;
            kl2_dev->exception_board_cluster[i].cl_excp_st   = cl_excp_st;
            kl2_dev->exception_board_cluster[i].cl_excp_mask = cl_excp_mask;
            atomic_inc_return(&kl2_dev->exception_board_cluster[i].excp_cnt);

            KL2_LOGD(
                    "cluster[p%d]: cl_excp_st= %08x, cl_excp_mask= %08x, cl_excp_st&(~cl_excp_mask)= %08x, "
                    "tk= %08x(%u), hwq_id= %d\n",
                    i, cl_excp_st, cl_excp_mask, cl_excp_st & (~cl_excp_mask), token, token,
                    hwq_id);
            cl_excp_st_ul = cl_excp_st & (~cl_excp_mask);
            for_each_set_bit(j, &cl_excp_st_ul, 32) {
                KL2_LOGD("cluster[p%d]: ..reason[%d] %s\n", i, j,
                         kl2_get_cluster_excp_name(kl2_dev, j));
            }

            // 停止中断
            // XXX(miaotianxiang): clr excp行为不明确，无法判断是否已给sse发done，
            // 改用host mask+reset+force done+host unmask方案
            // 参考http://wiki.baidu.com/pages/viewpage.action?pageId=902809712
            //kl2_writel(kl2_dev, cl_excp_st, KL2_REG_CLUSTER_BASE(cluster_base, i) + KL2_REG_CLUSTER_CLR_EXCEPTION);
        }
    }
}

void kl2_sdnn_disable_and_record_excp(struct kl2_device *kl2_dev, u32 st)
{
    void __iomem *sdnn_base = kl2_dev->iomem_base.sdnn_base;
    int           i, j;

    KL2_LOGD("int_stat_0= %08x\n", st);
    {
        u32 sdnn_disable_mask = 0;
        for_each_valid_sdnn(kl2_dev, i) {
            if (st & (0x1u << (i * 2 + 1))) {
                sdnn_disable_mask |= (0x1u << i);
            }
        }
        // sse不再调度子任务到该sdnn
        kl2_sse_excp_disable_sdnns(kl2_dev, sdnn_disable_mask);
    }

    for_each_valid_sdnn(kl2_dev, i) {
        if (st & (0x1u << (i * 2 + 1))) {
            u32           token;
            u32           sdnn_cl_excp_st;
            u32           sdnn_sd_excp_st;
            u32           sdnn_cl_excp_mask;
            u32           sdnn_sd_excp_mask;
            unsigned long sdnn_cl_excp_st_ul;
            unsigned long sdnn_sd_excp_st_ul;
            int           hwq_id;

            token             = kl2_readl(kl2_dev,
                                          KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_TOKEN);
            sdnn_cl_excp_st   = kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) +
                                                           KL2_REG_CLUSTER_EXCEPTION_STATE);
            sdnn_sd_excp_st   = kl2_readl(kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, i) +
                                                           KL2_REG_SDNN_EXCEPTION_STATE);
            sdnn_cl_excp_mask = kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) +
                                                           KL2_REG_CLUSTER_EXCEPTION_MASK);
            sdnn_sd_excp_mask = kl2_readl(kl2_dev, KL2_REG_SDNN_BASE(sdnn_base, i) +
                                                           KL2_REG_SDNN_EXCEPTION_MASK);
            kl2_dump_sdnn_cl_debug_info(kl2_dev, i,
                                        &kl2_dev->exception_board_sdnn[i].sdnn_debug_info);
            kl2_dump_sdnn_sd_debug_info(kl2_dev, i,
                                        &kl2_dev->exception_board_sdnn[i].sdnn_debug_info);

            if (is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
                /* use cu id instead of real hwq id to store hwq
                 * exception info and fix before recovering hwq later.
                 */
                hwq_id = -1;
            } else {
                hwq_id = kl2_sse_sdnn_map_hwq(kl2_dev, 0, i);
                // TODO(miaotianxiang): 谨防hwq_id非法

                kl2_dev->exception_board_hwq[hwq_id].token = token;
                kl2_dev->exception_board_hwq[hwq_id].sdnn_cl_excp_st[i] =
                        sdnn_cl_excp_st & (~sdnn_cl_excp_mask);
                kl2_dev->exception_board_hwq[hwq_id].sdnn_sd_excp_st[i] =
                        sdnn_sd_excp_st & (~sdnn_sd_excp_mask);
                memcpy(&kl2_dev->exception_board_hwq[hwq_id].sdnn_debug_info[i],
                       &kl2_dev->exception_board_sdnn[i].sdnn_debug_info,
                       sizeof(kl2_dev->exception_board_sdnn[i].sdnn_debug_info));
                atomic_inc_return(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
            }
            kl2_dev->exception_board_sdnn[i].token             = token;
            kl2_dev->exception_board_sdnn[i].sdnn_cl_excp_st   = sdnn_cl_excp_st;
            kl2_dev->exception_board_sdnn[i].sdnn_sd_excp_st   = sdnn_sd_excp_st;
            kl2_dev->exception_board_sdnn[i].sdnn_cl_excp_mask = sdnn_cl_excp_mask;
            kl2_dev->exception_board_sdnn[i].sdnn_sd_excp_mask = sdnn_sd_excp_mask;
            atomic_inc_return(&kl2_dev->exception_board_sdnn[i].excp_cnt);

            KL2_LOGD(
                    "sdnn[p%d]: sdnn_cl_excp_st= %08x, sdnn_cl_excp_mask= %08x, sdnn_cl_excp_st&(~sdnn_cl_excp_mask)= %08x, "
                    "sdnn_sd_excp_st= %08x, sdnn_sd_excp_mask= %08x, sdnn_sd_excp_st&(~sdnn_sd_excp_mask)= %08x, "
                    "tk= %08x(%u), hwq_id= %d\n",
                    i, sdnn_cl_excp_st, sdnn_cl_excp_mask, sdnn_cl_excp_st & (~sdnn_cl_excp_mask),
                    sdnn_sd_excp_st, sdnn_sd_excp_mask, sdnn_sd_excp_st & (~sdnn_sd_excp_mask),
                    token, token, hwq_id);
            sdnn_cl_excp_st_ul = sdnn_cl_excp_st & (~sdnn_cl_excp_mask);
            for_each_set_bit(j, &sdnn_cl_excp_st_ul, 32) {
                KL2_LOGD("sdnn(cl)[p%d]: ..reason[%d] %s\n", i, j,
                         kl2_get_cluster_excp_name(kl2_dev, j));
            }
            if (sdnn_cl_excp_st_ul & (0x1u << CE_SDNN_EXCEPTION)) {
                sdnn_sd_excp_st_ul = sdnn_sd_excp_st & (~sdnn_sd_excp_mask);
                for_each_set_bit(j, &sdnn_sd_excp_st_ul, 32) {
                    KL2_LOGD("sdnn(sd)[p%d]: ..reason[%d] %s\n", i, j,
                             kl2_get_sdnn_excp_name(kl2_dev, j));
                }
            }

            // 停止中断
            // XXX(miaotianxiang): clr excp行为不明确，无法判断是否已给sse发done，
            // 改用host mask+reset+force done+host unmask方案
            // 参考http://wiki.baidu.com/pages/viewpage.action?pageId=902809712
            //kl2_writel(kl2_dev, sdnn_cl_excp_st, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, i) + KL2_REG_CLUSTER_CLR_EXCEPTION);
        }
    }
}

void kl2_cluster_disable_and_record_timeout(struct kl2_device *kl2_dev, u32 st,
                                            u32 *cl_timeout_token)
{
    int i, j;

    KL2_LOGD("cl_timeout_st= %08x\n", st);
    {
        u32 cl_disable_mask = 0;
        for_each_valid_cluster(kl2_dev, i) {
            if (st & (0x1u << (i * 2 + 1))) {
                cl_disable_mask |= (0x1u << i);
            }
        }
        // sse不再调度子任务到该cluster
        kl2_sse_excp_disable_clusters(kl2_dev, cl_disable_mask);
    }

    for_each_valid_cluster(kl2_dev, i) {
        if (st & (0x1u << (i * 2 + 1))) {
            // TODO(miaotianxiang): 增加超时信息打印
            u32           cl_excp_st    = 0x1u << CE_TASK_TIMEOUT;
            u32           cl_excp_mask  = 0;
            unsigned long cl_excp_st_ul = cl_excp_st;

            kl2_dump_cluster_debug_info(kl2_dev, i,
                                        &kl2_dev->exception_board_cluster[i].cl_debug_info);

            kl2_dev->exception_board_cluster[i].cl_excp_st    = cl_excp_st;
            kl2_dev->exception_board_cluster[i].cl_excp_mask  = cl_excp_mask;
            kl2_dev->exception_board_cluster[i].timeout_token = cl_timeout_token[i];
            atomic_inc_return(&kl2_dev->exception_board_cluster[i].timeout_cnt);

            KL2_LOGD(
                    "cluster[p%d]: cl_excp_st= %08x, cl_excp_mask= %08x, cl_excp_st&(~cl_excp_mask)= %08x, "
                    "tk= %08x(%u)" /*, hwq_id= %d*/ "\n",
                    i, cl_excp_st, cl_excp_mask, cl_excp_st & (~cl_excp_mask), cl_timeout_token[i],
                    cl_timeout_token[i] /*, hwq_id*/);
            cl_excp_st_ul &= (~cl_excp_mask);
            for_each_set_bit(j, &cl_excp_st_ul, 32) {
                KL2_LOGD("cluster[p%d]: ..reason[%d] %s\n", i, j,
                         kl2_get_cluster_excp_name(kl2_dev, j));
            }
        }
    }
}

void kl2_sdnn_disable_and_record_timeout(struct kl2_device *kl2_dev, u32 st, u32 *sd_timeout_token)
{
    int i, j;

    KL2_LOGD("sd_timeout_st= %08x\n", st);
    {
        u32 sdnn_disable_mask = 0;
        for_each_valid_sdnn(kl2_dev, i) {
            if (st & (0x1u << (i * 2 + 1))) {
                sdnn_disable_mask |= (0x1u << i);
            }
        }
        // sse不再调度子任务到该sdnn
        kl2_sse_excp_disable_sdnns(kl2_dev, sdnn_disable_mask);
    }

    for_each_valid_sdnn(kl2_dev, i) {
        if (st & (0x1u << (i * 2 + 1))) {
            u32           sdnn_cl_excp_st    = 0x1u << CE_TASK_TIMEOUT;
            u32           sdnn_sd_excp_st    = 0x1u << CE_SDNN_TASK_TIMEOUT;
            u32           sdnn_cl_excp_mask  = 0;
            u32           sdnn_sd_excp_mask  = 0;
            unsigned long sdnn_cl_excp_st_ul = sdnn_cl_excp_st;
            unsigned long sdnn_sd_excp_st_ul = sdnn_sd_excp_st;

            kl2_dump_sdnn_cl_debug_info(kl2_dev, i,
                                        &kl2_dev->exception_board_sdnn[i].sdnn_debug_info);
            kl2_dump_sdnn_sd_debug_info(kl2_dev, i,
                                        &kl2_dev->exception_board_sdnn[i].sdnn_debug_info);

            kl2_dev->exception_board_sdnn[i].sdnn_cl_excp_st   = sdnn_cl_excp_st;
            kl2_dev->exception_board_sdnn[i].sdnn_sd_excp_st   = sdnn_sd_excp_st;
            kl2_dev->exception_board_sdnn[i].sdnn_cl_excp_mask = sdnn_cl_excp_mask;
            kl2_dev->exception_board_sdnn[i].sdnn_sd_excp_mask = sdnn_sd_excp_mask;
            kl2_dev->exception_board_sdnn[i].timeout_token     = sd_timeout_token[i];
            atomic_inc_return(&kl2_dev->exception_board_sdnn[i].timeout_cnt);

            KL2_LOGD(
                    "sdnn[p%d]: sdnn_cl_excp_st= %08x, sdnn_cl_excp_mask= %08x, sdnn_cl_excp_st&(~sdnn_cl_excp_mask)= %08x, "
                    "sdnn_sd_excp_st= %08x, sdnn_sd_excp_mask= %08x, sdnn_sd_excp_st&(~sdnn_sd_excp_mask)= %08x, "
                    "tk= %08x(%u)" /*, hwq_id= %d*/ "\n",
                    i, sdnn_cl_excp_st, sdnn_cl_excp_mask, sdnn_cl_excp_st & (~sdnn_cl_excp_mask),
                    sdnn_sd_excp_st, sdnn_sd_excp_mask, sdnn_sd_excp_st & (~sdnn_sd_excp_mask),
                    sd_timeout_token[i], sd_timeout_token[i] /*, hwq_id*/);
            sdnn_cl_excp_st_ul &= (~sdnn_cl_excp_mask);
            for_each_set_bit(j, &sdnn_cl_excp_st_ul, 32) {
                KL2_LOGD("sdnn(cl)[p%d]: ..reason[%d] %s\n", i, j,
                         kl2_get_cluster_excp_name(kl2_dev, j));
            }
            if (sdnn_cl_excp_st_ul & (0x1u << CE_SDNN_EXCEPTION)) {
                for_each_set_bit(j, &sdnn_sd_excp_st_ul, 32) {
                    KL2_LOGD("sdnn(sd)[p%d]: ..reason[%d] %s\n", i, j,
                             kl2_get_sdnn_excp_name(kl2_dev, j));
                    //output_sdnn_details(kl2_dev, i, j);
                }
            }
        }
    }
}

//static bool is_cur_token_timeout(struct kl2_device *kl2_dev, u32 cur_token)
//{
//    int hwq_id;
//    for_each_valid_sse_queue(kl2_dev, hwq_id)
//    {
//        if (atomic_read(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt) &&
//            cur_token == kl2_dev->exception_board_hwq[hwq_id].token)
//            return true;
//    }
//    return false;
//}

static void excp_reset_cluster(struct kl2_device *kl2_dev, int cl_id)
{
    int sriov_func_id = kl2_dev->dev_info.sriov_func_id;
    int user_id       = sriov_func_id - KL2_SRIOV_FUNC_ID_VF_0;

    // 1. stop dma
    cluster_stop_dma(kl2_dev, cl_id);

    // 2. wait noc idle and reset cu
    if (is_vf_id(sriov_func_id)) {
        // if is vf, notify pf to do physical noc wait and cu reset.
        kl2_sriov_mbox_reset_cluster_vf(kl2_dev, user_id, cl_id, 200);
    } else {
        wait_for_noc_cluster(kl2_dev, cl_id);
        udelay(10);
        cluster_reset(kl2_dev, cl_id);
    }

    // 3. wait stable
    udelay(10);
}

static void excp_reset_sdnn(struct kl2_device *kl2_dev, int sdnn_id)
{
    int sriov_func_id = kl2_dev->dev_info.sriov_func_id;
    int user_id       = sriov_func_id - KL2_SRIOV_FUNC_ID_VF_0;

    // 1. stop dma
    sdnn_stop_dma(kl2_dev, sdnn_id);

    // 2. wait noc idle and reset cu
    if (is_vf_id(sriov_func_id)) {
        // if is vf, notify pf to do physical noc wait and cu reset.
        kl2_sriov_mbox_reset_sdnn_vf(kl2_dev, user_id, sdnn_id, 200);
    } else {
        wait_for_noc_sdnn(kl2_dev, sdnn_id);
        udelay(10);
        sdnn_reset(kl2_dev, sdnn_id);
    }

    // 3. wait stable
    udelay(10);
}

static int excp_get_sdnn_hwqid(struct kl2_device *kl2_dev, int sdnn_id)
{
    int sriov_func_id = kl2_dev->dev_info.sriov_func_id;
    int user_id       = sriov_func_id - KL2_SRIOV_FUNC_ID_VF_0;
    int hwq_id        = -1;

    if (is_vf_id(sriov_func_id)) {
        hwq_id = kl2_sriov_mbox_request_sdnn_hwqid_vf(kl2_dev, user_id, sdnn_id, 200);
    }

    return hwq_id;
}

static int excp_get_cluster_hwqid(struct kl2_device *kl2_dev, int cl_id)
{
    int sriov_func_id = kl2_dev->dev_info.sriov_func_id;
    int user_id       = sriov_func_id - KL2_SRIOV_FUNC_ID_VF_0;
    int hwq_id        = -1;

    if (is_vf_id(sriov_func_id)) {
        hwq_id = kl2_sriov_mbox_request_cluster_hwqid_vf(kl2_dev, user_id, cl_id, 200);
    }

    return hwq_id;
}

static void kl2_check_sse_cu_disable_mask(struct kl2_device *kl2_dev)
{
    void __iomem *sse_base = kl2_dev->iomem_base.sse_base;
    u32           val, cuen_cu_disable_mask;

    if (is_pf_id(kl2_dev->dev_info.sriov_func_id))
        return;

    // 避免与proc文件cuen更新产生冲突
    mutex_lock(&kl2_dev->big_global_lock);
    // 关闭sriov时和vf0/1/2 均为 KL2_REG_SSE_USER0_XPU_DISABLE 有效
    val                  = kl2_readl(kl2_dev, sse_base + KL2_REG_SSE_USER0_XPU_DISABLE);
    cuen_cu_disable_mask = kl2_sse_get_cuen_cu_disable_mask(kl2_dev);
    mutex_unlock(&kl2_dev->big_global_lock);
    if (val != cuen_cu_disable_mask)
        KL2_LOG_ONCE("weird sse_cu_disable_mask, val= %08x, cuen_cu_disable_mask= %08x\n", val,
                     cuen_cu_disable_mask);
}

/*
 * 根据标记的token查找异常task，继续遍历寻找后面可能存在的evnt rec，返回其所属的sess，
 * 异常task一定在rt_list，evnt rec可能在pt_list。
 */
static struct kl2_session *kl2_excp_find_sess_tainting_from(struct kl2_device *kl2_dev,
                                                            struct kl2_hwq    *hwq)
{
    unsigned long       flags;
    struct kl2_session *esess = NULL;
    struct kl2_task    *task, *safe;
    u32                 etoken = kl2_dev->exception_board_hwq[hwq->id].token;

    spin_lock_irqsave(&hwq->lock, flags);
    list_for_each_entry_safe(task, safe, &hwq->rt_list, hwq_node) {
        if (task->desc.kernel.token == etoken) {
            esess = task->sess;
            break;
        }
    }
    if (unlikely(!esess)) {
        goto out;
    }

    list_for_each_entry_safe_continue(task, safe, &hwq->rt_list, hwq_node) {
        if (task->type == KL2_TASKTYPE_EVNTREC && task->sess == esess) {
            goto out;
        }
    }
    list_for_each_entry_safe(task, safe, &hwq->pt_list, hwq_node) {
        if (task->type == KL2_TASKTYPE_EVNTREC && task->sess == esess) {
            goto out;
        }
    }
    // rt_list/pt_list均未找到evnt rec，异常task无需扩散污染同uproc sess
    esess = NULL;

out:
    spin_unlock_irqrestore(&hwq->lock, flags);
    return esess;
}

static void kl2_excp_taint_hwq(struct kl2_device *kl2_dev, struct kl2_session *esess)
{
    struct kl2_hwq *hwq;

    // 确保sess->hwq合法
    kl2_session_bind_hwq(esess);
    hwq = esess->hwq;
    // 增加hwq->taint_cnt而非hwq->excp_cnt，excp_cnt可视作KL2_TASKTYPE_KERNEL类型task触发的异常计数，
    // kl2_excp_taint_all根据excp_cnt判断是否需要扩散污染，taint_cnt则为该hwq被扩散污染的计数，
    // kl2_handle_excp_work_func同时根据以上两个计数确定是否reset hwq。
    atomic_inc(&kl2_dev->exception_board_hwq[hwq->id].taint_cnt);
}

static void kl2_excp_taint_uproc_sess(struct kl2_device *kl2_dev, struct kl2_session *esess)
{
    struct kl2_session *sess;
    int                 sess_id;

    // 同uproc多个sess同时需要扩散污染，仅需执行一次遍历
    if (atomic_cmpxchg(&(esess->uproc->state), KL2_UPROC_NORMAL, KL2_UPROC_ERROR) !=
        KL2_UPROC_NORMAL)
        return;

    mutex_lock(&kl2_dev->uproc_session_lock);
    idr_for_each_entry(&kl2_dev->session_idr, sess, sess_id) {
        if (sess->uproc == esess->uproc && sess != esess) {
            KL2_LOGI("mark sess error(errno= XPUERR_EVENTWAIT), sess= %d, pid= %d, comm= %s ...\n",
                     sess_id, sess->uproc->pid, sess->uproc->comm);
            kl2_session_mark_error(sess, XPUERR_EVENTWAIT, 1);
            kl2_excp_taint_hwq(kl2_dev, sess);
        }
    }
    mutex_unlock(&kl2_dev->uproc_session_lock);
}

/*
 * 此函数原旨在将hwq之间的evnt依赖关系检测出来并且处理掉异常的sess/hwq/task，主要方法为：
 * （1）根据异常token找到异常的sess，标记err，
 * （2）根据sess将整个uproc以及uproc中的所有sess标记err，
 * （3）所有的异常sess的task全部销毁（在revoke_task_from_err_session_locked实现），
 */
static void kl2_excp_taint_all(struct kl2_device *kl2_dev)
{
    int                 hwq_id;
    struct kl2_session *esess;

    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        if (atomic_read(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt) > 0) {
            esess = kl2_excp_find_sess_tainting_from(kl2_dev, &kl2_dev->hwq[hwq_id]);
            // 异常无需扩散污染同uproc sess
            if (!esess)
                continue;

            KL2_LOGW(
                    "(excp + evnt rec) found, taint all sess in uproc, hwq= %d, sess= %d, pid= %d, comm= %s\n",
                    hwq_id, esess->id, esess->uproc->pid, esess->uproc->comm);
            kl2_session_mark_error(esess, XPUERR_KEXCEPTION, 0);
            kl2_excp_taint_uproc_sess(kl2_dev, esess);
        }
    }
}

static bool kl2_in_excp(struct kl2_device *kl2_dev)
{
    int i, hwq_id;

    for_each_valid_cluster(kl2_dev, i) {
        u32 excp_cnt          = atomic_read(&kl2_dev->exception_board_cluster[i].excp_cnt);
        u32 reset_cnt         = atomic_read(&kl2_dev->exception_board_cluster[i].reset_cnt);
        u32 timeout_cnt       = atomic_read(&kl2_dev->exception_board_cluster[i].timeout_cnt);
        u32 timeout_reset_cnt = atomic_read(&kl2_dev->exception_board_cluster[i].timeout_reset_cnt);
        if ((excp_cnt > reset_cnt) || (timeout_cnt > timeout_reset_cnt))
            return true;
    }
    for_each_valid_sdnn(kl2_dev, i) {
        u32 excp_cnt          = atomic_read(&kl2_dev->exception_board_sdnn[i].excp_cnt);
        u32 reset_cnt         = atomic_read(&kl2_dev->exception_board_sdnn[i].reset_cnt);
        u32 timeout_cnt       = atomic_read(&kl2_dev->exception_board_sdnn[i].timeout_cnt);
        u32 timeout_reset_cnt = atomic_read(&kl2_dev->exception_board_sdnn[i].timeout_reset_cnt);
        if ((excp_cnt > reset_cnt) || (timeout_cnt > timeout_reset_cnt))
            return true;
    }
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        u32 excp_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
        u32 taint_2_reset_cnt =
                atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_2_reset_cnt);
        u32 taint_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_cnt);
        if (excp_cnt || taint_2_reset_cnt || taint_cnt)
            return true;
    }

    return false;
}

static void kl2_excp_reset_excp_timeout_cu(struct kl2_device *kl2_dev)
{
    void __iomem *cluster_base = kl2_dev->iomem_base.cluster_base;
    void __iomem *sdnn_base    = kl2_dev->iomem_base.sdnn_base;
    int           j;
    int           sriov_func_id = kl2_dev->dev_info.sriov_func_id;

    for_each_valid_cluster(kl2_dev, j) {
        u32 excp_cnt          = atomic_read(&kl2_dev->exception_board_cluster[j].excp_cnt);
        u32 reset_cnt         = atomic_read(&kl2_dev->exception_board_cluster[j].reset_cnt);
        u32 timeout_cnt       = atomic_read(&kl2_dev->exception_board_cluster[j].timeout_cnt);
        u32 timeout_reset_cnt = atomic_read(&kl2_dev->exception_board_cluster[j].timeout_reset_cnt);
        u32 timeout_token     = kl2_dev->exception_board_cluster[j].timeout_token;
        u32 busy_token =
                kl2_readl(kl2_dev, KL2_REG_CLUSTER_BASE(cluster_base, j) + KL2_REG_CLUSTER_TOKEN);
        //KL2_LOGD("cl%d busy_token= %08x(%u)\n", j, busy_token, busy_token);
        if (excp_cnt > reset_cnt) {
            if (is_vf_id(sriov_func_id)) {
                int hwq_id = excp_get_cluster_hwqid(kl2_dev, j);
                kl2_dev->exception_board_hwq[hwq_id].token =
                        kl2_dev->exception_board_cluster[j].token;
                kl2_dev->exception_board_hwq[hwq_id].cl_excp_st[j] =
                        kl2_dev->exception_board_cluster[j].cl_excp_st &
                        (~kl2_dev->exception_board_cluster[j].cl_excp_mask);
                memcpy(&kl2_dev->exception_board_hwq[hwq_id].cl_debug_info[j],
                       &kl2_dev->exception_board_cluster[j].cl_debug_info,
                       sizeof(kl2_dev->exception_board_cluster[j].cl_debug_info));
                atomic_inc_return(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
                KL2_LOGD("j(vf_cl_id)= %d, hwq_id= %d, excp_cnt= %d, reset_cnt= %d\n", j, hwq_id,
                         excp_cnt, reset_cnt);
            }

            // reset cu期间禁止操作当前cu
            kl2_reg_lock(kl2_dev);
            excp_reset_cluster(kl2_dev, j);
            kl2_reg_unlock(kl2_dev);
            // reset后需恢复excp mask
            kl2_cluster_set_excp_mask(kl2_dev, j, kl2_cluster_get_excp_mask(kl2_dev, j));
            // 解除该cluster异常中断屏蔽
            kl2_intc_int_unmask(kl2_dev, 1, 0x1u << (2 * j + 1));
            atomic_inc_return(&kl2_dev->exception_board_cluster[j].reset_cnt);
            memset(&kl2_dev->exception_board_cluster[j], 0,
                   sizeof(kl2_dev->exception_board_cluster[j]));

            kl2_sse_cluster_force_done(kl2_dev, j);
            // enable后，sse将可能调度新的子任务到该cluster，可能导致新的异常
            kl2_sse_excp_enable_clusters(kl2_dev, 0x1u << j);
            KL2_LOGD("reset and enable cl%d\n", j);
        }
        if (timeout_cnt > timeout_reset_cnt) {
            if (busy_token != timeout_token) {
                // 之前记录的timeout task最终执行完了。。。
                atomic_inc_return(&kl2_dev->exception_board_cluster[j].timeout_reset_cnt);
                memset(&kl2_dev->exception_board_cluster[j], 0,
                       sizeof(kl2_dev->exception_board_cluster[j]));
                kl2_sse_excp_enable_clusters(kl2_dev, 0x1u << j);
                KL2_LOGD("no reset and enable cl%d\n", j);
            } else {
                kl2_reg_lock(kl2_dev);
                excp_reset_cluster(kl2_dev, j);
                kl2_reg_unlock(kl2_dev);
                // reset后需恢复excp mask
                kl2_cluster_set_excp_mask(kl2_dev, j, kl2_cluster_get_excp_mask(kl2_dev, j));
                // 解除该cluster异常中断屏蔽
                kl2_intc_int_unmask(kl2_dev, 1, 0x1u << (2 * j + 1));
                atomic_inc_return(&kl2_dev->exception_board_cluster[j].timeout_reset_cnt);
                memset(&kl2_dev->exception_board_cluster[j], 0,
                       sizeof(kl2_dev->exception_board_cluster[j]));

                kl2_sse_cluster_force_done(kl2_dev, j);
                // enable后，sse将可能调度新的子任务到该cluster，可能导致新的异常
                kl2_sse_excp_enable_clusters(kl2_dev, 0x1u << j);
                KL2_LOGD("reset and enable cl%d\n", j);
            }
        }
    }
    for_each_valid_sdnn(kl2_dev, j) {
        u32 excp_cnt          = atomic_read(&kl2_dev->exception_board_sdnn[j].excp_cnt);
        u32 reset_cnt         = atomic_read(&kl2_dev->exception_board_sdnn[j].reset_cnt);
        u32 timeout_cnt       = atomic_read(&kl2_dev->exception_board_sdnn[j].timeout_cnt);
        u32 timeout_reset_cnt = atomic_read(&kl2_dev->exception_board_sdnn[j].timeout_reset_cnt);
        u32 timeout_token     = kl2_dev->exception_board_sdnn[j].timeout_token;
        u32 busy_token =
                kl2_readl(kl2_dev, KL2_REG_SDNN_CLUSTER_BASE(sdnn_base, j) + KL2_REG_CLUSTER_TOKEN);
        //KL2_LOGD("sd%d busy_token= %08x(%u)\n", j, busy_token, busy_token);
        if (excp_cnt > reset_cnt) {
            if (is_vf_id(sriov_func_id)) {
                int hwq_id                                 = excp_get_sdnn_hwqid(kl2_dev, j);
                kl2_dev->exception_board_hwq[hwq_id].token = kl2_dev->exception_board_sdnn[j].token;
                kl2_dev->exception_board_hwq[hwq_id].sdnn_cl_excp_st[j] =
                        kl2_dev->exception_board_sdnn[j].sdnn_cl_excp_st &
                        (~kl2_dev->exception_board_sdnn[j].sdnn_cl_excp_mask);
                kl2_dev->exception_board_hwq[hwq_id].sdnn_sd_excp_st[j] =
                        kl2_dev->exception_board_sdnn[j].sdnn_sd_excp_st &
                        (~kl2_dev->exception_board_sdnn[j].sdnn_sd_excp_mask);
                memcpy(&kl2_dev->exception_board_hwq[hwq_id].sdnn_debug_info[j],
                       &kl2_dev->exception_board_sdnn[j].sdnn_debug_info,
                       sizeof(kl2_dev->exception_board_sdnn[j].sdnn_debug_info));
                atomic_inc_return(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
                KL2_LOGD("j(vf_sd_id)= %d, hwq_id= %d, excp_cnt= %d, reset_cnt= %d\n", j, hwq_id,
                         excp_cnt, reset_cnt);
            }

            // reset cu期间禁止操作当前cu
            kl2_reg_lock(kl2_dev);
            excp_reset_sdnn(kl2_dev, j);
            kl2_reg_unlock(kl2_dev);
            // reset后需恢复excp mask
            kl2_sdnn_set_cl_excp_mask(kl2_dev, j, kl2_sdnn_get_cl_excp_mask(kl2_dev, j));
            kl2_sdnn_set_sd_excp_mask(kl2_dev, j, kl2_sdnn_get_sd_excp_mask(kl2_dev, j));
            // 解除该sdnn异常中断屏蔽
            kl2_intc_int_unmask(kl2_dev, 0, 0x1u << (2 * j + 1));
            atomic_inc_return(&kl2_dev->exception_board_sdnn[j].reset_cnt);
            memset(&kl2_dev->exception_board_sdnn[j], 0, sizeof(kl2_dev->exception_board_sdnn[j]));

            kl2_sse_sdnn_force_done(kl2_dev, j);
            // enable后，sse将可能调度新的子任务到该sdnn，可能导致新的异常
            kl2_sse_excp_enable_sdnns(kl2_dev, 0x1u << j);
            KL2_LOGD("reset and enable sd%d\n", j);
        }
        if (timeout_cnt > timeout_reset_cnt) {
            if (busy_token != timeout_token) {
                // 之前记录的timeout task最终执行完了。。。
                atomic_inc_return(&kl2_dev->exception_board_sdnn[j].timeout_reset_cnt);
                memset(&kl2_dev->exception_board_sdnn[j], 0,
                       sizeof(kl2_dev->exception_board_sdnn[j]));
                kl2_sse_excp_enable_sdnns(kl2_dev, 0x1u << j);
                KL2_LOGD("no reset and enable sd%d\n", j);
            } else {
                kl2_reg_lock(kl2_dev);
                excp_reset_sdnn(kl2_dev, j);
                kl2_reg_unlock(kl2_dev);
                // reset后需恢复excp mask
                kl2_sdnn_set_cl_excp_mask(kl2_dev, j, kl2_sdnn_get_cl_excp_mask(kl2_dev, j));
                kl2_sdnn_set_sd_excp_mask(kl2_dev, j, kl2_sdnn_get_sd_excp_mask(kl2_dev, j));
                // 解除该sdnn异常中断屏蔽
                kl2_intc_int_unmask(kl2_dev, 0, 0x1u << (2 * j + 1));
                atomic_inc_return(&kl2_dev->exception_board_sdnn[j].timeout_reset_cnt);
                memset(&kl2_dev->exception_board_sdnn[j], 0,
                       sizeof(kl2_dev->exception_board_sdnn[j]));

                kl2_sse_sdnn_force_done(kl2_dev, j);
                // enable后，sse将可能调度新的子任务到该sdnn，可能导致新的异常
                kl2_sse_excp_enable_sdnns(kl2_dev, 0x1u << j);
                KL2_LOGD("reset and enable sd%d\n", j);
            }
        }
    }
}

void kl2_handle_excp_work_func(struct work_struct *work)
{
    struct kl2_device *kl2_dev = container_of(work, struct kl2_device, handle_exception_work);
    u32                underway;
    int                hwq_id;
    unsigned long      flags;
    ktime_t            begin_ktime                           = ktime_get();
    bool               hwq_underway_equals_zero_timeout      = false;
    char               hwq_underway_equals_zero[KL2_HWQ_CNT] = { 0 };

    // 前一次kl2_handle_excp_work_func中，又触发了新的异常，于是多次queue_work，work_queue调度
    // kl2_handle_excp_work_func再次执行时实际上异常已全部处理完，可直接返回
    if (!kl2_in_excp(kl2_dev)) {
        KL2_LOGD("!kl2_in_excp, kl2_state= %d, early return\n", kl2_get_state(kl2_dev));
        return;
    }
    // timer位于中断上下文优先级高，excp work最后unstall可能会覆盖timer中的stall，
    // 造成在unstall时reset hwq，可能导致奇怪的pending= 65和溢出异常
    kl2_sse_hwq_stall_all(kl2_dev);

    // 单个kernel可能触发多次异常中断，尽可能等待收到全部异常中断，一次性处理完成
    msleep(100);
    KL2_LOGD("exception handle begin\n");
    // 通知timer采取必要措施，比如恢复超时检测时间阈值，以使excp work尽快结束
    kl2_dev->exception_stash.excp_work_running = 1;

    // TODO(miaotianxiang): 换成更好的解决方案，类似synchronize_rcu
    // 等待所有正在访问hwq的操作结束，如kl2_hwq_dispatch_locked
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        struct kl2_hwq *hwq = &kl2_dev->hwq[hwq_id];
        spin_lock_irqsave(&hwq->lock, flags);
        // do nothing
        spin_unlock_irqrestore(&hwq->lock, flags);
    }

    // 循环reset cu，使所有hwq underway尽快归零
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        //u32 excp_cnt  = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
        //u32 taint_2_reset_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_2_reset_cnt);
        //u32 taint_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_cnt);
        // TODO(miaotianxiang): 不关心当前hwq，无需等待underway变为0 ？？？还有很多问题，暂不开启
        //if (!excp_cnt && !taint_2_reset_cnt && !taint_cnt)
        //    continue;

        struct kl2_hwq *hwq = &kl2_dev->hwq[hwq_id];
        while (1) {
            kl2_excp_reset_excp_timeout_cu(kl2_dev);

            // 此处无需kl2_reg_lock，因为不会与下方kl2_sse_hwq_reset冲突
            underway = kl2_sse_hwq_underway(hwq);
            // 尽早退出循环，节省时间
            if (!underway) {
                hwq_underway_equals_zero[hwq_id] = 1;
                break;
            }

            // while循环退出条件，避免soft lockup
            if (ktime_to_ms(ktime_sub(ktime_get(), begin_ktime)) > 60000 /* 60s */) {
                KL2_LOGW("hwq_%d hwq_underway_equals_zero_timeout\n", hwq_id);
                hwq_underway_equals_zero_timeout = true;
                break;
            }
            udelay(100);
            cond_resched();
        }
    }
    if (hwq_underway_equals_zero_timeout) {
        KL2_LOGW("hwq_underway_equals_zero_timeout, goto err ...\n");
        goto err;
    }

    // 等待全部hwq underway变为0期间可能有kernel完成中断，需要等待中断发送和处理完成
    synchronize_irq(kl2_dev->kdev->pdev->irq);
    msleep(100);

    // 前置条件：1、所有的hwq都stall, 2、所有的可执行的task都finish；
    // 当前状态：所有hwq中的rt list中的task都未被执行
    // 后续操作：如果excp hwq后续有evnt rec则整个uproc的sess/hwq销毁，否则do nothing
    //
    // XXX(miaotianxiang): 如果synchronize_irq + msleep不能
    // 使前置条件成立，则revoke_task_from_err_session_locked将导致严重逻辑错误...
    kl2_excp_taint_all(kl2_dev);

    // 将sess->taint_state安全转化到sess->state，在revoke_task_from_err_session_locked中回收task
    {
        struct kl2_session *sess;
        int                 sess_id;

        mutex_lock(&kl2_dev->uproc_session_lock);
        idr_for_each_entry(&kl2_dev->session_idr, sess, sess_id) {
            if (atomic_read(&sess->taint_state) != KL2_SESS_NORMAL) {
                kl2_session_mark_error(sess, sess->taint_errno, 1);
            }
        }
        mutex_unlock(&kl2_dev->uproc_session_lock);
    }

    // 为缩短excp work时间，上面第一个大循环加入了短路分支，如果某hwq无任何异常事件（excp_cnt == 0...），
    // 则可跳过第一个大循环中检查。但kl2_excp_taint_all()可能因为EVNTREC增加这些hwq的taint_cnt，
    // 导致hwq将在下一个大循环中被reset。这些hwq上可能仍有健康task正在运行，会导致严重BUG。所以将
    // taint_cnt转化为state状态KL2_HWQ_TAINT，在timer中继续等待underway变为0后再reset。
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        u32 excp_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
        u32 taint_2_reset_cnt =
                atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_2_reset_cnt);
        u32 taint_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_cnt);
        if (!excp_cnt && !taint_2_reset_cnt && taint_cnt) {
            // hwq已确认underway归零，可以安全reset
            if (hwq_underway_equals_zero[hwq_id]) {
                atomic_inc(&kl2_dev->exception_board_hwq[hwq_id].taint_2_reset_cnt);
            } else {
                atomic_set(&kl2_dev->hwq[hwq_id].taint_state, KL2_HWQ_TAINT);
            }
            atomic_set(&kl2_dev->exception_board_hwq[hwq_id].taint_cnt, 0);
        }
    }

    // 最后reset hwq
    for_each_valid_sse_queue(kl2_dev, hwq_id) {
        // {}_acquire: the R of the RMW (or atomic_read) is an ACQUIRE
        u32 excp_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].excp_cnt);
        u32 taint_2_reset_cnt =
                atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_2_reset_cnt);
        u32 taint_cnt = atomic_read(&kl2_dev->exception_board_hwq[hwq_id].taint_cnt);

        // hwq未确认underway归零，延后reset
        if (!hwq_underway_equals_zero[hwq_id]) {
            KL2_LOGW(
                    "hwq_%d !hwq_underway_equals_zero, excp_cnt= %d, taint_2_reset_cnt= %d, taint_cnt= %d, delay hwq reset ...\n",
                    hwq_id, excp_cnt, taint_2_reset_cnt, taint_cnt);
            continue;
        }

        if (excp_cnt || taint_2_reset_cnt) {
            // excp_cnt非0表示该队列有task触发了异常，
            // taint_2_reset_cnt非0表示该队列被其他队列异常扩散污染或有任务需要尽快清理
            struct kl2_hwq     *hwq = &kl2_dev->hwq[hwq_id];
            struct kl2_task    *task, *etask = NULL, *first_task = NULL;
            struct kl2_task    *saved_etask = &kl2_dev->exception_stash.saved_etask;
            struct kl2_session *esess       = NULL;
            u32                 etask_cur;

            spin_lock_irqsave(&hwq->lock, flags);
            // 仅当excp_cnt非0时需遍历rt_list寻找etask，taint_cnt非0则该队列所有被污染task->sess已标记
            if (excp_cnt) {
                list_for_each_entry(task, &hwq->rt_list, hwq_node) {
                    if (task->desc.kernel.token == kl2_dev->exception_board_hwq[hwq_id].token) {
                        etask = task;
                        break;
                    }
                }
                first_task = list_first_entry_or_null(&hwq->rt_list, struct kl2_task, hwq_node);
            }

            if (etask) {
                esess = etask->sess;
                // 保证esess在spinlock critical section外仍然有效
                //
                // 20230417: kl2_session_mark_error上移后，esess只在spinlock critical section内访问，
                //           但仍保留已有代码，以备后续critical section外展示更多esess信息
                xref_get(&esess->xref);
                // 保证etask信息在spinlock critical section外仍然有效
                memcpy(saved_etask, etask, sizeof(*etask));

                // kl2_session_mark_error需放在revoke_task_from_err_session_locked前，
                // 并且kl2_session_add_task需持有spinlock检查sess->state，
                // 从而保证sess被标记为ERROR且revoke后不会添加新task待执行
                // TODO(miaotianxiang): 对于超时任务能否提前？？
                kl2_session_mark_error(esess, XPUERR_KEXCEPTION, 0);
            }

            // excp_cnt非0或taint_2_reset_cnt非0均需revoke+reset+redispatch
            // TODO(miaotianxiang): 仅当rt_list包含revoke task时，reset hwq
            revoke_task_from_err_session_locked(hwq);
            // reset hwq期间禁止操作当前hwq
            kl2_reg_lock(kl2_dev);
            kl2_sse_hwq_reset(kl2_dev, hwq_id);
            kl2_reg_unlock(kl2_dev);
            redispatch_running_task_locked(hwq);
            __kl2_hwq_dispatch_locked_nocheck(hwq);
            spin_unlock_irqrestore(&hwq->lock, flags);

            // timeout判断无法做到精确（存在task在hrtimer中被判定为timeout但实际能够执行完的情形）
            if (excp_cnt && !etask)
                KL2_LOG_ONCE("excp_cnt && !etask, excp_cnt= %x, etask= %px\n", excp_cnt, etask);
            // 可能存在正常中断响应慢，rt_list中正常结束的task未被移除的情况？
            if (etask && first_task && etask != first_task)
                KL2_LOG_ONCE("etask != first_task, etask= %px, first_task= %px\n", etask,
                             first_task);

            if (etask) {
                // 在hwq spinlock critical section外save etask，避免spinlock嵌套
                etask_cur = kl2_save_etask(kl2_dev, saved_etask, hwq_id);
                // esess使命结束！
                xref_put(&esess->xref, kl2_destroy_session_ref);

                KL2_LOG_XID(XPU_XID0, "xpu kernel exception\n");
#define KL2_PRINT_FMT_STR(fmt_str, ...) KL2_LOGW(fmt_str, ##__VA_ARGS__)
                kl2_print_etask(kl2_dev, &kl2_dev->etasks[etask_cur], /* debug_info= */ 0);
#undef KL2_PRINT_FMT_STR
            }

            queue_work(kl2_dev->hwq_wq, &hwq->finish_work);
            // TODO(miaotianxiang): hwq已被reset，excp board可安全清零，但有可能小概率造成多一次hwq
            // reset？？？（memset后timer又增加excp_cnt或taint_2_reset_cnt）
            memset(&kl2_dev->exception_board_hwq[hwq_id], 0,
                   sizeof(kl2_dev->exception_board_hwq[hwq_id]));
        }
    }

    // 验证无多余cluster或sdnn被错误disable
    kl2_check_sse_cu_disable_mask(kl2_dev);
    if (!kl2_get_in_reset_state(kl2_dev)) {
        kl2_set_state(kl2_dev, KL2_RUNNING);
        kl2_sse_hwq_unstall_all(kl2_dev);
    }
    kl2_dev->exception_stash.excp_work_seq++;
    kl2_dev->exception_stash.excp_work_running = 0;
    KL2_LOGD("exception handle end\n");
    return;

err:
    kl2_dev->errno = XPUERR_KEXCEPTION;
    kl2_set_state(kl2_dev, KL2_ERROR);
    kl2_dev->exception_stash.excp_work_running = 0;
    KL2_LOGE("something unexpected happened, hardware stuck now, "
             "please check/reset manually !!!\n");
}
