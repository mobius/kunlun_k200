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

#include "kl2/kl2.h"
#include "kl2/kl2_regs.h"
#include "kl2/dma.h"
#include "kl2/proc.h"
#include "kl2/exception.h"
#include "kl2/mbox.h"
#include "kl2/hw.h"
#ifdef ENABLE_RB
#include "kl2_ram_blk.h"
#endif

#include <linux/bitmap.h>
#include <linux/device.h>
#include <linux/vmalloc.h>

#define CREATE_TRACE_POINTS
#include "kl2/trace.h"

// VF MSI多vector配置开关，默认打开并优先使用vector 1, 用于规避VF中断丢失问题
static bool kl2_vf_multi_msi_vector = 1;
static int  kl2_vf_main_msi_vector  = 1;
module_param(kl2_vf_multi_msi_vector, bool, 0444);
module_param(kl2_vf_main_msi_vector, int, 0444);

static bool kl2_otp_all_avail_cu = 0;
module_param(kl2_otp_all_avail_cu, bool, 0444);

static bool kl2_r300_try_soft_reset_if_ccix_not_25gt = 1;
module_param(kl2_r300_try_soft_reset_if_ccix_not_25gt, bool, 0444);

static bool kl2_r300_try_soft_reset_if_ccix_not_x8 = 1;
module_param(kl2_r300_try_soft_reset_if_ccix_not_x8, bool, 0444);

static int kl2_r300_max_soft_reset_retry_count_if_ccix_not_ok = 3;
module_param(kl2_r300_max_soft_reset_retry_count_if_ccix_not_ok, int, 0444);

// TODO(miaotianxiang): move
static inline int hwq_busy_count(struct kl2_device *kl2_dev)
{
    int i, cnt = 0;
    for (i = 0; i < KL2_HWQ_CNT; ++i)
        if (kl2_dev->hwq[i].cnt_all)
            ++cnt;
    return cnt;
}

static enum hrtimer_restart kl2_ur_update_stat_func(struct hrtimer *ur_timer)
{
    struct kl2_device *kl2_dev = container_of(ur_timer, struct kl2_device, ur_timer);

    ur_record(&kl2_dev->ur, hwq_busy_count(kl2_dev));

    hrtimer_forward_now(&kl2_dev->ur_timer, kl2_dev->ur_ktime);
    return HRTIMER_RESTART;
}

int kl2_device_init(struct kl2_device *kl2_dev)
{
    BUG_ON(!kl2_dev->spec.valid);

    mutex_init(&kl2_dev->big_global_lock);

    mutex_init(&kl2_dev->uproc_session_lock);
    idr_init(&kl2_dev->uproc_idr);
    idr_init(&kl2_dev->session_idr);

    mutex_init(&kl2_dev->event_idr_lock);
    idr_init(&kl2_dev->event_idr);

    ur_init(&kl2_dev->ur);

    atomic_set(&kl2_dev->task_token, 100);
    atomic_set(&kl2_dev->reg_lock, 0);
    spin_lock_init(&kl2_dev->sc_lock);

    spin_lock_init(&kl2_dev->etasks_lock);
    atomic_set(&kl2_dev->etasks_cur, 0);
    INIT_WORK(&kl2_dev->handle_exception_work, kl2_handle_excp_work_func);
    kl2_dev->task_timeout_detect.detect_threshold_in_ms = KL2_TASK_TIMEOUT_DETECT_THRESHOLD_IN_MS;

    kl2_hwq_init(kl2_dev);
    kl2_dev->hwq_wq = alloc_workqueue("kl2-hwq-jobs", WQ_UNBOUND, 0);
    if (!kl2_dev->hwq_wq) {
        KL2_LOGW("alloc_workqueue failed\n");
        return -ENOMEM;
    }

    kl2_dev->ur_ktime = ktime_set(0, 1 * 1000 * 1000); // 1ms
    hrtimer_init(&kl2_dev->ur_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    kl2_dev->ur_timer.function = kl2_ur_update_stat_func;
    kl2_dev->ur_timer_valid    = true;
    kl2_dev->regular_ktime     = ktime_set(0, 500 * 1000 * 1000); // 500ms
    hrtimer_init(&kl2_dev->regular_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    kl2_dev->regular_timer.function = kl2_regular_timer_func;
    kl2_dev->regular_timer_valid    = true;
    kl2_dev->regular_timer_toggle   = true;

    kl2_dev->proc_debug_toggle = false;

    INIT_WORK(&kl2_dev->irq_printk_work, kl2_irq_printk_work_func);
    atomic_set(&kl2_dev->irq_printk_data_used, 0);

    return 0;
}

void kl2_device_destroy(struct kl2_device *kl2_dev)
{
    flush_work(&kl2_dev->handle_exception_work);
    if (kl2_dev->regular_timer_valid) {
        hrtimer_cancel(&kl2_dev->regular_timer);
    }
    if (kl2_dev->ur_timer_valid) {
        hrtimer_cancel(&kl2_dev->ur_timer);
    }
    if (kl2_dev->hwq_wq) {
        destroy_workqueue(kl2_dev->hwq_wq);
    }
    flush_work(&kl2_dev->irq_printk_work);
    flush_work(&kl2_dev->handle_exception_work);
}

static void kl2_pcie_mmio_setup(struct kl2_device *kl2_dev)
{
    kl2_dev->iomem_base.dma_base = kl2_dev->kdev->bar[0] + 0x400;

    kl2_dev->iomem_base.intc_base    = kl2_dev->kdev->bar[2] + KL2_REG_INTC_BAR2_BASE;
    kl2_dev->iomem_base.sse_base     = kl2_dev->kdev->bar[2] + KL2_REG_SSE_BAR2_BASE;
    kl2_dev->iomem_base.vac_base     = kl2_dev->kdev->bar[2] + KL2_REG_VAC_BAR2_BASE;
    kl2_dev->iomem_base.aes_base     = kl2_dev->kdev->bar[2] + KL2_REG_AES_BAR2_BASE;
    kl2_dev->iomem_base.cluster_base = kl2_dev->kdev->bar[2] + KL2_REG_CLUSTER0_BAR2_BASE;
    kl2_dev->iomem_base.sdnn_base    = kl2_dev->kdev->bar[2] + KL2_REG_SDNN0_BAR2_BASE;
    kl2_dev->iomem_base.syscon0_base = kl2_dev->kdev->bar[2] + KL2_REG_SYSCON0_BAR2_BASE;
    kl2_dev->iomem_base.syscon1_base = kl2_dev->kdev->bar[2] + KL2_REG_SYSCON1_BAR2_BASE;
    kl2_dev->iomem_base.gddr_base    = kl2_dev->kdev->bar[2] + KL2_REG_GDDRCTRL_BAR2_BASE;
    kl2_dev->iomem_base.otp_base     = kl2_dev->kdev->bar[2] + KL2_REG_OTP_BAR2_BASE;
    kl2_dev->iomem_base.ccix_base    = kl2_dev->kdev->bar[2] + KL2_REG_CCIXCFG_BAR2_BASE;
    kl2_dev->iomem_base.dbgm_base    = kl2_dev->kdev->bar[2] + KL2_REG_DBGM_BAR2_BASE;

    kl2_dev->iomem_base.l3_base = kl2_dev->kdev->bar[4] + KL2_REG_L3_BAR4_BASE;
}

// TODO(miaotianxiang):
// 填充ddr_conf，kl2_get_df_spec中需要使用
static void kl2_fill_ddr_conf(struct kl2_device *kl2_dev)
{
    bool ddr_conf_invalid = false;
    u32  ddr_reg0220, ddr_reg0034, ddr_reg4028, ddr_reg408c;

    // 因为VF无权直接读取GDDR寄存器,这里拷贝需要GDDR寄存器信息到L3以供VF使用
    if (is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
        ddr_reg0034 = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_BOARD_INFO_L3_OFFSET);
        ddr_reg0220 = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_DDR_REG0220_L3_OFFSET);
        ddr_reg4028 = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_DDR_REG4028_L3_OFFSET);
        ddr_reg408c = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_DDR_REG408C_L3_OFFSET);
    } else {
        ddr_reg0034 = kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_BOARD_INFO_L3_OFFSET);
        //XXX(miaotianxiang): R100/R420/RM80的gddr channel 0被禁用，故统一从channel 1获取信息
        ddr_reg0220 =
                kl2_readl(kl2_dev, KL2_REG_GDDRCTRL_CHAN_BASE(kl2_dev->iomem_base.gddr_base, 1) +
                                           KL2_REG_GDDRCTRL_0220);
        ddr_reg4028 =
                kl2_readl(kl2_dev, KL2_REG_GDDRCTRL_CHAN_BASE(kl2_dev->iomem_base.gddr_base, 1) +
                                           KL2_REG_GDDRCTRL_4028);
        ddr_reg408c =
                kl2_readl(kl2_dev, KL2_REG_GDDRCTRL_CHAN_BASE(kl2_dev->iomem_base.gddr_base, 1) +
                                           KL2_REG_GDDRCTRL_408c);
        kl2_writel(kl2_dev, ddr_reg0220, kl2_dev->iomem_base.l3_base + KL2_DDR_REG0220_L3_OFFSET);
        kl2_writel(kl2_dev, ddr_reg4028, kl2_dev->iomem_base.l3_base + KL2_DDR_REG4028_L3_OFFSET);
        kl2_writel(kl2_dev, ddr_reg408c, kl2_dev->iomem_base.l3_base + KL2_DDR_REG408C_L3_OFFSET);
    }
    kl2_dev->ddr_conf.vendor = (ddr_reg0034 >> 13) & 0x3;
    if (kl2_dev->ddr_conf.vendor == 0x0) {
        kl2_dev->ddr_conf.vendor_str = "micron";
    } else if (kl2_dev->ddr_conf.vendor == 0x1) {
        kl2_dev->ddr_conf.vendor_str = "samsung";
    } else if (kl2_dev->ddr_conf.vendor == 0x2) {
        kl2_dev->ddr_conf.vendor_str = "hynix";
    } else {
        ddr_conf_invalid             = true;
        kl2_dev->ddr_conf.vendor_str = "unknown";
    }
    if (((ddr_reg0034 >> 12) & 0x1) == 0x0) {
        kl2_dev->ddr_conf.max_link_speed = 14;
    } else {
        kl2_dev->ddr_conf.max_link_speed = 16;
    }

    if (kl2_dev->dev_info.board == KL2_BOARD_ID_R300 ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_R200_8F ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_R200_8FS ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_RG800 ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_RG800_PRO) {
        kl2_dev->ddr_conf.ddr_x8 = 1;
    }
    if (((ddr_reg0220 >> 24) & 0xff) == 0x3) {
        kl2_dev->ddr_conf.ecc_on = 1;
    }
    if (kl2_dev->dev_info.board == KL2_BOARD_ID_R100 ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_R420 ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_RM80) {
        kl2_dev->ddr_conf.nchannel = 6;
    } else {
        kl2_dev->ddr_conf.nchannel = 8;
    }
    switch ((ddr_reg4028 >> 16) & 0xffff) {
    case 0x1e:
        kl2_dev->ddr_conf.link_speed = 14;
        break;
    case 0x18:
        kl2_dev->ddr_conf.link_speed = 12;
        break;
    case 0x14:
        kl2_dev->ddr_conf.link_speed = 10;
        break;
    case 0x22:
        if (ddr_reg408c == 0x13203d50) {
            kl2_dev->ddr_conf.link_speed = 8;
        } else if (ddr_reg408c == 0x13203c10) {
            kl2_dev->ddr_conf.link_speed = 16;
        } else {
            ddr_conf_invalid             = true;
            kl2_dev->ddr_conf.link_speed = 8;
        }
        break;
    default:
        ddr_conf_invalid             = true;
        kl2_dev->ddr_conf.link_speed = 8;
        break;
    }

    if (ddr_conf_invalid) {
        KL2_LOGW("invalid ddr conf from reg, default set to 8 Gbps, "
                 "ddr_reg0220= %08x, ddr_reg0034= %08x, ddr_reg4028= %08x, ddr_reg408c= %08x\n",
                 ddr_reg0220, ddr_reg0034, ddr_reg4028, ddr_reg408c);
    }
    if (kl2_dev->ddr_conf.link_speed == 16 &&
        !strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K00Y6219", 10)) {
        KL2_LOGW("invalid ddr conf from reg, link_speed= 16 Gbps ??? "
                 "maybe you should set ddr freq manually !!!");
    }
    KL2_LOGI(
            "ddr_conf= {ddr_x8= %u, ecc_on= %u, nchannel= %u, vendor= %s, max_link_speed= %u Gbps, "
            "link_speed= %u Gbps}\n",
            (unsigned int)kl2_dev->ddr_conf.ddr_x8, (unsigned int)kl2_dev->ddr_conf.ecc_on,
            kl2_dev->ddr_conf.nchannel, kl2_dev->ddr_conf.vendor_str,
            kl2_dev->ddr_conf.max_link_speed, kl2_dev->ddr_conf.link_speed);
}

static int kl2_fill_otp_info(struct kl2_device *kl2_dev)
{
    int idx;
    u32 otp_18, otp_1c, sdnn_disable_bits, cluster_disable_bits, decoder_disable_bits;

    if ((kl2_dev->dev_info.board == KL2_BOARD_ID_R100) ||
        (kl2_dev->dev_info.board == KL2_BOARD_ID_R420) ||
        (kl2_dev->dev_info.board == KL2_BOARD_ID_RM80)) {
        otp_18               = kl2_readl(kl2_dev, kl2_dev->iomem_base.otp_base + KL2_REG_OTP_018);
        otp_1c               = kl2_readl(kl2_dev, kl2_dev->iomem_base.otp_base + KL2_REG_OTP_01c);
        sdnn_disable_bits    = (otp_18 >> 12) & 0x3fu;
        cluster_disable_bits = (otp_18 >> 18) & 0xffu;
        decoder_disable_bits = ((otp_18 >> 26) & 0x3fu) | ((otp_1c & 0x7u) << 6);

        // XXX(miaotianxiang): 确保R100/R420/RM80类型板卡提供计算资源一致，屏蔽多余可用sdnn/cluster/decoder
        if ((bitcount(sdnn_disable_bits) < 2) && !kl2_otp_all_avail_cu) {
            for (idx = (KL2_SDNN_MAX_COUNT - 1); idx >= 0; --idx) {
                if (sdnn_disable_bits & (0x1u << idx))
                    continue;

                sdnn_disable_bits |= (0x1u << idx);
                if (bitcount(sdnn_disable_bits) == 2) {
                    break;
                }
            }
        } else if ((bitcount(sdnn_disable_bits) > 2) && !kl2_otp_all_avail_cu) {
            goto err_out;
        }

        if ((bitcount(cluster_disable_bits) < 2) && !kl2_otp_all_avail_cu) {
            for (idx = (KL2_CLUSTER_MAX_COUNT - 1); idx >= 0; --idx) {
                if (cluster_disable_bits & (0x1u << idx))
                    continue;

                cluster_disable_bits |= (0x1u << idx);
                if (bitcount(cluster_disable_bits) == 2) {
                    break;
                }
            }
        } else if ((bitcount(cluster_disable_bits) > 2) && !kl2_otp_all_avail_cu) {
            goto err_out;
        }

        if ((bitcount(decoder_disable_bits) < 2) && !kl2_otp_all_avail_cu) {
            for (idx = (KL2_DECODER_MAX_COUNT - 1); idx >= 0; --idx) {
                if (decoder_disable_bits & (0x1u << idx))
                    continue;

                decoder_disable_bits |= (0x1u << idx);
                if (bitcount(decoder_disable_bits) == 2) {
                    break;
                }
            }
        } else if ((bitcount(decoder_disable_bits) > 2) && !kl2_otp_all_avail_cu) {
            goto err_out;
        }

        kl2_dev->otp_info.sdnn_avail_bits = (~sdnn_disable_bits) & 0x3fu;
        kl2_dev->otp_info.cl_avail_bits   = (~cluster_disable_bits) & 0xffu;
        kl2_dev->otp_info.dec_avail_bits  = (~decoder_disable_bits) & 0x1ffu;
    } else {
        // 默认开启所有计算单元
        kl2_dev->otp_info.sdnn_avail_bits = 0x3fu;
        kl2_dev->otp_info.cl_avail_bits   = 0xffu;
        kl2_dev->otp_info.dec_avail_bits  = 0x1ffu;
    }

    KL2_LOGI("otp_info= {sdnn_avail_bits= %08x, cl_avail_bits= %08x, dec_avail_bits= %08x}\n",
             kl2_dev->otp_info.sdnn_avail_bits, kl2_dev->otp_info.cl_avail_bits,
             kl2_dev->otp_info.dec_avail_bits);
    return 0;

err_out:
    KL2_LOGW(
            "invalid conf, otp_info= {otp_18= %08x, otp_1c= %08x, sdnn_disable_bits= %08x, cluster_disable_bits= %08x, decoder_disable_bits= %08x}\n",
            otp_18, otp_1c, sdnn_disable_bits, cluster_disable_bits, decoder_disable_bits);
    return -EINVAL;
}

static int kl2_prologue_setup(struct kl2_device *kl2_dev)
{
    struct kl_device *kdev = kl2_dev->kdev;
    int               i;
    bool              sn_invalid                  = false;
    static const char r200_dbg_board_sn_prefix[6] = { 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f };

    // 检查BAR，如遇非法情况尽早报错return
    // 内核分配的'VF BAR Size'需要与'System Page Size'字节对齐，可能大于firmware定义的值
    if (kdev->bar_info.bar_en != 0x15 || kdev->bar_info.bar_size[0] < 0x4000 /* 16KB */ ||
        kdev->bar_info.bar_size[2] < 0x2000000 /* 32MB */ ||
        kdev->bar_info.bar_size[4] < 0x4000000 /* 64MB */) {
        KL2_LOGE("invalid BAR mapping or BAR size, are you sure?\n");
        return -EINVAL;
    }

    // 仅当存在sriov capability时，pdev->is_physfn|is_virtfn才会被设置，
    // kl2_plda_addr_trans_init才会被执行，这在整张R200透传给vm时会导致问题，
    // 因为sriov capability不一定被传递给了qemu ...
    //
    // 暂时将条件|kdev->pdev->is_physfn|去除，pf/vf多次配置addr trans应该不会导致副作用，
    // 后续寻找更好的方案
    //
    // /**
    //  * pci_iov_init - initialize the IOV capability
    //  * @dev: the PCI device
    //  *
    //  * Returns 0 on success, or negative on failure.
    //  */
    // int pci_iov_init(struct pci_dev *dev)
    // {
    //     int pos;
    //
    //     if (!pci_is_pcie(dev))
    //         return -ENODEV;
    //
    //     pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_SRIOV);
    //     if (pos)
    //         return sriov_init(dev, pos);
    //
    //     return -ENODEV;
    // }
    //
    //if (kdev->pdev->is_physfn)
    kl2_plda_addr_trans_init(kl2_dev, kdev->bar[0]);
    kl2_pcie_mmio_setup(kl2_dev);

    kdev->num_vfs = kl2_virt_get_numvfs(kl2_dev);
    switch (kdev->num_vfs) {
    case 0:
        kl2_dev->dev_info.sriov_conf = KL2_SRIOV_CONF_ID_SRIOV_OFF;
        break;
    case 1:
        kl2_dev->dev_info.sriov_conf = KL2_SRIOV_CONF_ID_1VF;
        break;
    case 2:
        kl2_dev->dev_info.sriov_conf = KL2_SRIOV_CONF_ID_2VF;
        break;
    case 3:
        kl2_dev->dev_info.sriov_conf = KL2_SRIOV_CONF_ID_3VF;
        break;
    default:
        KL2_LOGE("invalid kdev->num_vfs= %d\n", kdev->num_vfs);
        return -EINVAL;
    }

    if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_SRIOV_OFF) {
        kl2_dev->dev_info.sriov_func_id = KL2_SRIOV_FUNC_ID_SRIOV_OFF;
    } else {
        // 0x0834: 0/1/2 for virtfn, 4 for physfn
        kdev->vf_id = kl2_readl(kl2_dev, kdev->bar[2] + KL2_REG_SYSCON0_BAR2_BASE + 0x834);
        switch (kdev->vf_id) {
        case 0:
            kl2_dev->dev_info.sriov_func_id = KL2_SRIOV_FUNC_ID_VF_0;
            break;
        case 1:
            kl2_dev->dev_info.sriov_func_id = KL2_SRIOV_FUNC_ID_VF_1;
            break;
        case 2:
            kl2_dev->dev_info.sriov_func_id = KL2_SRIOV_FUNC_ID_VF_2;
            break;
        case 4:
            kl2_dev->dev_info.sriov_func_id = KL2_SRIOV_FUNC_ID_PF;
            break;
        default:
            KL2_LOGE("invalid kdev->vf_id= %d\n", kdev->vf_id);
            return -EINVAL;
        }
    }

    // 32 chars SN
    kl2_dev->dev_info.sn[0] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_SN0);
    kl2_dev->dev_info.sn[1] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_SN1);
    kl2_dev->dev_info.sn[2] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_SN2);
    kl2_dev->dev_info.sn[3] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_SN3);
    kl2_dev->dev_info.sn[4] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_SN4);
    kl2_dev->dev_info.sn[5] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_SN5);
    kl2_dev->dev_info.sn[6] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_SN6);
    kl2_dev->dev_info.sn[7] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_SN7);
    for (i = 0; i < 32; ++i) {
        char c = ((char *)kl2_dev->dev_info.sn)[i];
        if (!(c >= 0 && c <= 127)) {
            sn_invalid = true;
            break;
        }
    }
    if (sn_invalid) {
        KL2_LOGW("SN= 0x%08x%08x%08x%08x%08x%08x%08x%08x, invalid ASCII\n", kl2_dev->dev_info.sn[0],
                 kl2_dev->dev_info.sn[1], kl2_dev->dev_info.sn[2], kl2_dev->dev_info.sn[3],
                 kl2_dev->dev_info.sn[4], kl2_dev->dev_info.sn[5], kl2_dev->dev_info.sn[6],
                 kl2_dev->dev_info.sn[7]);
    }
    KL2_LOGI("SN= %.32s\n", (const char *)&kl2_dev->dev_info.sn[0]);

    // FW CPLD
    kl2_dev->dev_info.fw[0] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_FW0);
    kl2_dev->dev_info.fw[1] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_FW1);
    kl2_dev->dev_info.fw[2] =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_FW2);
    kl2_dev->dev_info.cpld =
            kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_BOARD_CPLD);
    KL2_LOGI("FW= %04u.%04u.%04u, CPLD= %x\n", kl2_dev->dev_info.fw[0], kl2_dev->dev_info.fw[1],
             kl2_dev->dev_info.fw[2], kl2_dev->dev_info.cpld);

    // 根据SN确定物理卡型号
    if (!strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K0A7", 6)) {
        kl2_dev->dev_info.board = KL2_BOARD_ID_R100;
    } else if (!strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K00Y", 6) ||
               !strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K037", 6) ||
               !strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K04B", 6)) {
        kl2_dev->dev_info.board = KL2_BOARD_ID_R200;
    } else if (!strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K014", 6) ||
               !strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K09R", 6)) {
        kl2_dev->dev_info.board = KL2_BOARD_ID_R300;
    } else if (!strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K06J", 6)) {
        if (!strncmp((const char *)&kl2_dev->dev_info.sn[4], "03K03M", 6)) {
            kl2_dev->dev_info.board = KL2_BOARD_ID_R200_8FS;
        } else if (!strncmp((const char *)&kl2_dev->dev_info.sn[4], "03K05P", 6)) {
            kl2_dev->dev_info.board = KL2_BOARD_ID_RG800;
        } else if (!strncmp((const char *)&kl2_dev->dev_info.sn[4], "03K072", 6)) {
            kl2_dev->dev_info.board = KL2_BOARD_ID_RG800_PRO;
        } else {
            kl2_dev->dev_info.board = KL2_BOARD_ID_R200_8F;
        }
    } else if (!memcmp((const char *)&kl2_dev->dev_info.sn[0], r200_dbg_board_sn_prefix,
                       sizeof(r200_dbg_board_sn_prefix))) {
        kl2_dev->dev_info.board = KL2_BOARD_ID_R200_DEBUG_BOARD;
    } else if (!strncmp((const char *)&kl2_dev->dev_info.sn[0], "02K0A8", 6)) {
        kl2_dev->dev_info.board = KL2_BOARD_ID_R420;
    } else if (!strncmp((const char *)&kl2_dev->dev_info.sn[0], "KLXMXM", 6)) {
        kl2_dev->dev_info.board = KL2_BOARD_ID_RM80;
    } else {
        KL2_LOGW("unclassified SN, default set to R200");
        kl2_dev->dev_info.board = KL2_BOARD_ID_R200;
    }

    // PN 由SN和板卡类型推导出
    // 参考文档：https://ku.baidu-int.com/knowledge/HFVrC7hq1Q/FTlz9IbpJx/GZfKQyv7AF/YQXkILDH_q8uzF#anchor-4fd3ba90-2081-11ee-8c45-5f20717a36b3
    snprintf((char *)&kl2_dev->dev_info.pn[0], 8, "03");
    if (kl2_dev->dev_info.board == KL2_BOARD_ID_RG800 ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_RG800_PRO) {
        memcpy((char *)&kl2_dev->dev_info.pn[0] + 2, (const char *)&kl2_dev->dev_info.sn[4], 6);
    } else {
        memcpy((char *)&kl2_dev->dev_info.pn[0] + 2, (const char *)&kl2_dev->dev_info.sn[0], 6);
    }
    KL2_LOGI("PN= %.8s\n", (const char *)&kl2_dev->dev_info.pn[0]);

    KL2_LOGI("dev_info= {board= %d, sriov_conf= %d, sriov_func_id= %d}\n", kl2_dev->dev_info.board,
             kl2_dev->dev_info.sriov_conf, kl2_dev->dev_info.sriov_func_id);

    kl2_fill_ddr_conf(kl2_dev);
    if (kl2_fill_otp_info(kl2_dev)) {
        return -EINVAL;
    }
    if (kl2_get_df_spec(kl2_dev)) {
        return -EINVAL;
    }
    kl2_dev->mm_info = kl2_get_mm_info(kl2_dev);
    if (!kl2_dev->mm_info) {
        return -EINVAL;
    }

    return 0;
}

// TODO(miaotianxiang): 逐步移除
void kl2_misc_setup(struct kl2_device *kl2_dev)
{
#ifndef PLATFORM_KUNLUN
    // only needed on PLD/ZEBU
    // 0x252003e8
    // 0:8channel
    // 1:4channel
    // 2:2channel
    // 3:6channel
#if (defined(DDRCH) && DDRCH == 6)
    kl2_writel(kl2_dev, 3, kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_NOC_GDDR_CTRL);
    kl2_writel(kl2_dev, 0x11, kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_GDDR_CLK_GATE0);
#else
    kl2_writel(kl2_dev, 0, kl2_dev->iomem_base.syscon0_base + KL2_REG_SYSCON0_NOC_GDDR_CTRL);
#endif

    if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_SRIOV_OFF) {
        kl2_gddr_init(kl2_dev);
    }
#endif
}

static int kl2_epilogue_setup(struct kl2_device *kl2_dev)
{
    //int               i;
    //struct kl_mm     *mm = &kl2_dev->mm;
    //struct kl_memory *mem;
    int err;

    // XXX(miaotianxiang):
    // 20220418 清零专用于printf的gm
    // 20221117 xpu2-elfconv实际生成的调用顺序为
    //              xpu_kernel_debug_reset()
    //              xpu_launch_async()
    //              xpu_kernel_debug()
    //          保证每次使用kernel printf前相关gm区域均已清零，故epilogue中不再需要kl2_dma_ddma_zero_gm
    //
    //          R300 fw初始化gddr有概率失败，此处访问gddr可能超时，进而总线hang，造成CmpltTO和uce等严重后果
    //for (i = 0; i < mm->mem_count; ++i) {
    //    mem = &mm->mem[i];
    //    if (mem->kind == XPU_MEM_PRINTF) {
    //        kl2_dma_ddma_zero_gm(&kl2_dev->dma_engine, mem->base, mem->size);
    //    }
    //}

    // XXX(miaotianxiang):
    // 20230214 Hot reset发生后，PCIe IP无法自动全部恢复PCIe配置空间。
    //          此处通知fw及时保存PCIe配置空间伺机恢复，以通过厂商ltloop测试。
    if (kl2_dev->dev_info.fw[2] >= 27 && !is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
        kl2_writel(kl2_dev, 0x800000, kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD);
        kl2_writel(kl2_dev, BIT(0), kl2_dev->iomem_base.intc_base + KL2_REG_INTC_SET_7);
        err = kl_poll_cond_timeout(
                (kl2_readl(kl2_dev, kl2_dev->iomem_base.l3_base + KL2_REG_L3_HOST_CMD) == 0), 10000,
                10000000 /* 10s */);
        if (err) {
            KL2_LOGW("stash_for_ltloop timeout ...\n");
        } else {
            KL2_LOGI("stash_for_ltloop done ...\n");
        }
    }

    return 0;
}

int kl2_probe(struct kl_device *kdev)
{
    struct kl2_device *kl2_dev = NULL;
    struct kl_inode   *kinode  = NULL;
    int                minor, err;

    kl2_dev = vzalloc(sizeof(*kl2_dev));
    if (!kl2_dev) {
        err = -ENOMEM;
        goto err_out;
    }

    kl2_dev->kdev = kdev;
    kdev->data    = kl2_dev;

    // |kl2_dev->spec| 在 kl2_prologue_setup 中设置
    err = kl2_prologue_setup(kl2_dev);
    if (err) {
        goto err_free_kl2_dev;
    }

    err = kl2_device_init(kl2_dev);
    if (err) {
        goto err_free_kl2_dev;
    }

    // MSI is enabled by default
    kl2_intc_disable_msi(kl2_dev);
    kl2_misc_setup(kl2_dev);
    kl2_gddr_interrupt_mask_init(kl2_dev);
    kl2_sse_init(kl2_dev);
    kl2_compute_unit_init(kl2_dev);
    kl2_sriov_mbox_init(kl2_dev);
    kl2_dbgm_init(kl2_dev);

    err = kl2_dma_init(kl2_dev, KL2_DMACH_CNT, kl2_dev->spec.dmach_bits, &kl2_dma_ops);
    if (err)
        goto err_destroy_kl2_dev;
    kl2_plda_dma_init(kl2_dev, kl2_dev->spec.dmach_bits);

    err = kl_mm_init(kdev, &kl2_dev->mm, kl2_dev->mm_info);
    if (err)
        goto err_release_dma;

#ifdef ENABLE_CODEC
    err = kl2_video_init(kl2_dev);
    if (err)
        goto err_mm_uninit;
#endif

    if (is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
        kl2_dev->multi_msi_vector = kl2_vf_multi_msi_vector;
        kl2_dev->main_msi_vector  = kl2_vf_main_msi_vector;
    }
#ifdef ENABLE_MSIX
    err = kl2_msix_register(kl2_dev);
    if (err < 0) {
        KL2_LOGE("error enable msix, err= %d\n", err);
        goto err_destroy_video;
    }
#else
    err = kl2_msi_register(kl2_dev);
    if (err < 0) {
        KL2_LOGE("error enable msi, err= %d\n", err);
        goto err_destroy_video;
    }
#endif
    //if (kdev->pdev->is_physfn)
    kl2_intc_setup(kl2_dev);

#ifdef ENABLE_RB
    // @gddr 0x810000000
    // total device size = 1024*1024 * 512 bytes = 512 MB
    if (kl_rb_init(&kl2_dev->dma_engine, minor, 0x810000000, 0x100000) != 0) {
        KL2_LOGW("kl_rb_init error!\n");
    }
#endif

    hrtimer_start(&kl2_dev->ur_timer, kl2_dev->ur_ktime, HRTIMER_MODE_REL);
    hrtimer_start(&kl2_dev->regular_timer, kl2_dev->regular_ktime, HRTIMER_MODE_REL);

    kl2_epilogue_setup(kl2_dev);

    //cxpu init
    kdev->cxpu.mm     = &kl2_dev->mm;
    kdev->cxpu.ecc_on = kl2_dev->ddr_conf.ecc_on;
    kl_cxpu_init(&kdev->cxpu);

    minor           = kdev->idx * XPU_PD_NUM;
    kinode          = &g_devinodes[minor];
    kl2_dev->kinode = kinode;

    minor = kl_create_device(&kdev->pdev->dev, kdev->idx * XPU_PD_NUM, 1, &kl2_fops,
                             (void *)kl2_dev, kdev);
    if (minor < 0) {
        err = -XPUERR_DEVINIT;
        goto err_unsetup_intc;
    }

    err = kl2_device_proc_create(g_proc_root, kl2_dev);
    if (err)
        goto err_destroy_device;

    kl2_set_state(kl2_dev, KL2_RUNNING);

    return 0;

err_destroy_device:
    kl_destroy_device(minor, 1);

err_unsetup_intc:
    kl2_intc_unsetup(kl2_dev);
#ifdef ENABLE_MSIX
    kl2_msix_unregister(kl2_dev);
#else
    kl2_msi_unregister(kl2_dev);
#endif

err_destroy_video:
#ifdef ENABLE_CODEC
    kl2_video_destroy(kl2_dev);

err_mm_uninit:
#endif
    kl_mm_uninit(&kl2_dev->mm);

err_release_dma:
    kl2_plda_dma_uninit(kl2_dev);
    kl2_dma_destroy(kl2_dev);

err_destroy_kl2_dev:
    kl2_device_destroy(kl2_dev);

err_free_kl2_dev:
    vfree(kl2_dev);

err_out:
    return err;
}

int kl2_remove(struct kl_device *kdev)
{
    struct kl2_device *kl2_dev = (struct kl2_device *)kdev->data;
    struct kl_inode   *kinode;

    if (!kl2_dev) {
        return 0;
    }
    kinode = kl2_dev->kinode;

    kl2_unmap_memcpy_p2p_direct(kl2_dev);

    kl2_device_proc_destroy(g_proc_root, kl2_dev);
    kl_destroy_device(kl2_dev->kinode->minor, 1);

#ifdef ENABLE_RB
    kl_rb_cleanup(kl2_dev->kinode->minor);
#endif

    kl2_intc_unsetup(kl2_dev);
#ifdef ENABLE_MSIX
    kl2_msix_unregister(kl2_dev);
#else
    kl2_msi_unregister(kl2_dev);
#endif

#ifdef ENABLE_CODEC
    kl2_video_destroy(kl2_dev);
#endif

    kl_mm_uninit(&kl2_dev->mm);

    kl2_plda_dma_uninit(kl2_dev);
    kl2_dma_destroy(kl2_dev);

    if (is_pf_id(kl2_dev->dev_info.sriov_func_id)) {
        // need to disable sriov before remove pf
        kl2_sriov_mbox_disable_pf(kl2_dev);
        pci_disable_sriov(kdev->pdev);
        kl2_virt_set_numvfs(kl2_dev, 0);
    }

    kl_cxpu_uninit(&kl2_dev->kdev->cxpu);

    kl2_device_destroy(kl2_dev);
    vfree(kl2_dev);

    return 0;
}

static int kl2_clone_hw_info(struct kl2_device *kl2_dev, int num_vfs)
{
    KL2_BASE_HW_INFO *base_hw_info = NULL;

    base_hw_info = vmalloc(sizeof(KL2_BASE_HW_INFO));
    if (base_hw_info == NULL) {
        return 1;
    }

    // clone base hw info to vf1 & vf2
    memcpy_fromio(base_hw_info, kl2_dev->iomem_base.l3_base, sizeof(KL2_BASE_HW_INFO));
    if (num_vfs == 2) {
        memcpy_toio(kl2_dev->iomem_base.l3_base + KL2_SRIOV_2VFS_L3_SIZE, base_hw_info,
                    sizeof(KL2_BASE_HW_INFO));
    } else if (num_vfs == 3) {
        memcpy_toio(kl2_dev->iomem_base.l3_base + KL2_SRIOV_3VFS_L3_SIZE, base_hw_info,
                    sizeof(KL2_BASE_HW_INFO));
        memcpy_toio(kl2_dev->iomem_base.l3_base + KL2_SRIOV_3VFS_L3_SIZE * 2, base_hw_info,
                    sizeof(KL2_BASE_HW_INFO));
    }

    vfree(base_hw_info);

    return 0;
}

static int kl2_sriov_configure(struct kl_device *kdev, int num_vfs)
{
    struct kl2_device *kl2_dev = (struct kl2_device *)kdev->data;
    int                err     = 0;

    if (num_vfs < 0 || num_vfs > KL2_SRIOV_MAX_NUM_VFS)
        return -EINVAL;

    if (kl2_dev->ddr_conf.nchannel != 8) {
        KL2_LOGI("Cannot enable SRIOV when nchannel= %d\n", kl2_dev->ddr_conf.nchannel);
        return -EINVAL;
    }

    kl_mm_uninit(&kl2_dev->mm);
    err = kl_mm_init(kdev, &kl2_dev->mm, kl2_get_mm_info(kl2_dev));
    if (err) {
        KL2_LOGW("Re-init Memory Management failed, a reload of driver is needed.\n");
        return err;
    }

    //kl2_dev->spec = &kl2_df_specs[spec_mapping[num_vfs][0]];

    if (num_vfs == 0)
        bitmap_clear(&kl2_dev->hwq_bitmap, 0, KL2_HWQ_CNT);
    else
        bitmap_set(&kl2_dev->hwq_bitmap, 0, KL2_HWQ_CNT);
    KL2_LOGI("update hwq bitmap(%px)=%lx\n", &kl2_dev->hwq_bitmap, kl2_dev->hwq_bitmap);

    kl2_clone_hw_info(kl2_dev, num_vfs);

    if (!err)
        kl2_virt_set_numvfs(kl2_dev, num_vfs);

    if (num_vfs == 0) {
        kl2_sriov_mbox_disable_pf(kl2_dev);
        kl2_dev->dev_info.sriov_func_id = KL2_SRIOV_FUNC_ID_SRIOV_OFF;
        kl2_sse_init(kl2_dev);
        kl2_compute_unit_init(kl2_dev);
    } else {
        kl2_dev->dev_info.sriov_func_id = KL2_SRIOV_FUNC_ID_PF;

        // 用户曾禁用某些cu，开启SR-IOV需先恢复默认
        if (kl2_dev->cuen != kl2_dev->default_cuen) {
            kl2_sse_cuen_update(kl2_dev, kl2_dev->default_cuen);
        }

        kl2_sriov_mbox_enable_pf(kl2_dev);
    }

    kl2_dev->dev_info.sriov_num_vfs = num_vfs;

    return err;
}

static bool kl2_device_in_used(struct kl_device *kdev)
{
    struct kl2_device      *kl2_dev = kdev->data;
    struct kl2_userprocess *uproc;
    int                     pid;

    if (is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
        idr_for_each_entry(&kl2_dev->uproc_idr, uproc, pid) {
            // idr is not empty
            return true;
        }
    } else {
        idr_for_each_entry(&kl2_dev->uproc_idr, uproc, pid) {
            if (pid != current->tgid)
                return true;
        }
    }

    return false;
}

static int kl2_pci_sriov_configure(struct kl_device *kdev, int num_vfs)
{
    u32                domain = kdev->domain;
    u32                bus    = kdev->bus;
    u32                slot   = kdev->slot;
    u32                func   = kdev->func;
    int                err, num_vfs_sv;
    struct kl2_device *kl2_dev = kdev->data;
    int                i;

    if (kl2_dev->dev_info.board == KL2_BOARD_ID_R200 ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_R300 ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_R200_8F ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_R200_8FS ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_RG800 ||
        kl2_dev->dev_info.board == KL2_BOARD_ID_RG800_PRO) {
        if (!kdev->pdev->is_physfn) {
            KL2_LOGW("The device is not a PF device or has no SR-IOV capabilities\n");
            return -ENOSYS;
        }

        KL2_LOGI("[%04x:%02x:%02x.%x] change num_vfs to %d\n", domain, bus, slot, func, num_vfs);

        // check if current device(PF) or any of its VFs are in used
        for (i = 0; i < MAX_DEVICE_NUM; i++) {
            struct kl_device *kdev_to_check = get_kdev_by_id(i);
            if (kdev_to_check != NULL && kdev_to_check->pf == kdev &&
                kl2_device_in_used(kdev_to_check)) {
                KL2_LOGW("device is in use, can not change it's SR-IOV mode\n");
                return -EBUSY;
            }
        }

        num_vfs_sv    = kdev->num_vfs;
        kdev->num_vfs = num_vfs;

        err = kl2_sriov_configure(kdev, num_vfs);
        if (err)
            goto err_out;

        if (num_vfs > 0)
            err = pci_enable_sriov(kdev->pdev, num_vfs);
        else
            pci_disable_sriov(kdev->pdev);

        if (err) {
            // recovery kl2 SR-IOV config if pci_enable_sriov failed.
            kl2_sriov_configure(kdev, num_vfs_sv);
            goto err_out;
        }

        return num_vfs;
    } else {
        KL2_LOGW("The board type %d has no SR-IOV capabilities\n", kl2_dev->dev_info.board);
        return -ENOSYS;
    }

err_out:
    kdev->num_vfs = num_vfs_sv;
    return err;
}

int kl2_dev_set_numvfs(struct kl2_device *kl2_dev, int num_vfs)
{
    int ret;

    if (kl2_dev->dev_info.sriov_num_vfs == num_vfs) {
        KL2_LOGI("vf number is already %d", num_vfs);
        return 0;
    }

    if (num_vfs && kl2_dev->dev_info.sriov_num_vfs) {
        ret = kl2_pci_sriov_configure(kl2_dev->kdev, 0);
        if (ret < 0) {
            return ret;
        }
    }

    return kl2_pci_sriov_configure(kl2_dev->kdev, num_vfs);
}

const struct kl_info kl2_info = {
    .kl_code                = KL2,
    .canonical_name         = "Kunlun2",
    .device_name            = "kl2_dev",
    .probe                  = kl2_probe,
    .remove                 = kl2_remove,
    .sriov_configure        = kl2_pci_sriov_configure,
    .query_device_info_v1   = kl2_query_device_info_v1,
    .query_device_proc_info = kl2_query_device_proc_info,
};

// XXX(miaotianxiang):
// 20220218: r300 ccix link speed may not be 25GT/s after power up, workaround here
// 20230912: 添加 ccix link width != x8 情况下的workaround
int __init kl2_module_post(void)
{
    struct kl2_device **r300_devs   = NULL;
    struct kl2_device  *kl2_dev     = NULL;
    u32                 reset_count = 0;
    u32                 r300_count  = 0;
    u32                 r300_bitmap = 0;
    u32                 card_id;
    u32                 some_r300_firmware_lt65 = 0;
    int                 ret                     = 0;
    int                 ccix_link_speed_status = 0, reset_by_ccix_link_speed = 0;
    int                 ccix_link_width_status = 0, reset_by_ccix_link_width = 0;
    int                 i;
    bool                regular_timer_toggle[8];

    if (g_devs_count == 0) {
        return 0;
    }

    r300_devs = vmalloc(sizeof(*r300_devs) * g_devs_count);
    if (!r300_devs) {
        LOGW("vmalloc r300_devs failed\n");
        return -ENOMEM;
    }

    // find r300 board
    for (i = 0; i < g_devs_count; i++) {
        if (g_devs[i].info->kl_code == KL2) {
            kl2_dev = (struct kl2_device *)(g_devs[i].data);
            if (kl2_dev->dev_info.board == KL2_BOARD_ID_R300) {
                r300_devs[r300_count++] = kl2_dev;
            }
        }
    }
    if (!r300_count) {
        ret = 0;
        goto err_out;
    }
    if (r300_count && !kl2_r300_try_soft_reset_if_ccix_not_25gt &&
        !kl2_r300_try_soft_reset_if_ccix_not_x8) {
        LOGI("!kl2_r300_try_soft_reset_if_ccix_not_***, skip ccix link speed/width check/reinit ...\n");
        ret = 0;
        goto err_out;
    }

    // r300 topo check
    for (i = 0; i < r300_count; i++) {
        card_id = kl2_readl(r300_devs[i], r300_devs[i]->iomem_base.syscon1_base +
                                                  KL2_REG_SYSCON1_GLOBAL_CARD_NUM);
        if (card_id >= 8) {
            LOGW("%s invalid r300 card id %u ...\n", r300_devs[i]->kdev->name, card_id);
            ret = -EINVAL;
            goto err_out;
        }
        r300_bitmap = r300_bitmap | BIT(card_id) | /* port 0 */
                      BIT(card_id + 8) |           /* port 1 */
                      BIT(card_id + 16) |          /* port 2 */
                      BIT(card_id + 24);           /* port 3 */
        r300_devs[i]->ccix_info.card_id = card_id;
    }

    // 检查固件版本，< *.*.65 时不支持port mode switch，因此不进行link width check
    for (i = 0; i < r300_count; i++) {
        if (r300_devs[i]->dev_info.fw[2] < 65) {
            some_r300_firmware_lt65 = 1;
            LOGI("%s FW should be x.x.65 or later, now it's %04u.%04u.%04u, skip ccix link width check/reinit ...\n",
                 r300_devs[i]->kdev->name, r300_devs[i]->dev_info.fw[0],
                 r300_devs[i]->dev_info.fw[1], r300_devs[i]->dev_info.fw[2]);
        }
    }

    do {
        ccix_link_speed_status = 0;
        ccix_link_width_status = 0;
        for (i = 0; i < r300_count; i++) {
            ccix_link_speed_status += (kl2_ccix_link_speed_check(r300_devs[i], r300_bitmap) != 0);
            ccix_link_width_status += (kl2_ccix_link_width_check(r300_devs[i], r300_bitmap) != 0);
        }
        // 逐卡打印link speed/width信息
        for (i = 0; i < r300_count; i++) {
            LOGI("%s SN= %.32s, card_id= %u, reset_count= %u, port[0,1,2,3].speed= %u,%u,%u,%u, port[0,1,2,3].width= %u,%u,%u,%u\n",
                 r300_devs[i]->kdev->name, (const char *)&r300_devs[i]->dev_info.sn[0],
                 r300_devs[i]->ccix_info.card_id, reset_count,
                 r300_devs[i]->ccix_info.port_link_speed[0],
                 r300_devs[i]->ccix_info.port_link_speed[1],
                 r300_devs[i]->ccix_info.port_link_speed[2],
                 r300_devs[i]->ccix_info.port_link_speed[3],
                 r300_devs[i]->ccix_info.port_link_width[0],
                 r300_devs[i]->ccix_info.port_link_width[1],
                 r300_devs[i]->ccix_info.port_link_width[2],
                 r300_devs[i]->ccix_info.port_link_width[3]);
        }
        reset_by_ccix_link_speed =
                (kl2_r300_try_soft_reset_if_ccix_not_25gt && ccix_link_speed_status);
        reset_by_ccix_link_width = (kl2_r300_try_soft_reset_if_ccix_not_x8 &&
                                    ccix_link_width_status && !some_r300_firmware_lt65);
        // do while循环退出条件
        if (!reset_by_ccix_link_speed && !reset_by_ccix_link_width) {
            break;
        }
        // reset_count >= 20作为循环退出条件，避免用户设置超大reset次数
        if (reset_count >= kl2_r300_max_soft_reset_retry_count_if_ccix_not_ok ||
            reset_count >= 20) {
            break;
        }

        LOGI("ccix link reinit start, reset_by_ccix_link_speed= %d, reset_by_ccix_link_width= %d ...\n",
             reset_by_ccix_link_speed, reset_by_ccix_link_width);
        // soft reset first
        for (i = 0; i < r300_count; i++) {
            r300_devs[i]->errno = XPUERR_DEVINIT;
            kl2_set_state(r300_devs[i], KL2_ERROR);
            ret = kl2_dev_soft_reset(r300_devs[i], 1, 1);
            if (ret) {
                goto err_out;
            }
            mutex_lock(&r300_devs[i]->big_global_lock);
            regular_timer_toggle[i] = r300_devs[i]->regular_timer_toggle;
            mutex_unlock(&r300_devs[i]->big_global_lock);
            // 关闭regular timer, 包含温度功耗读取/pt_list重试下发/超时检测 ...
            if (regular_timer_toggle[i]) {
                hrtimer_cancel(&r300_devs[i]->regular_timer);
            }
        }

        // 遍历所有卡link width，对 link_width != x8 的port做模式切换 RC <--> EP
        for (i = 0; i < r300_count; i++) {
            if (reset_by_ccix_link_width) {
                ret = kl2_ccix_link_port_mode_switch_if_necessary(r300_devs[i], r300_bitmap,
                                                                  r300_devs, r300_count);
                if (ret) {
                    goto err_out;
                }
            }
        }

        // write ccix init parameters
        for (i = 0; i < r300_count; i++) {
            kl2_writel(r300_devs[i], 16 /* speed_esm0 */,
                       r300_devs[i]->iomem_base.l3_base + KL2_REG_L3_HOST_BUFFER0);
            kl2_writel(r300_devs[i], 25 /* speed-esm1 */,
                       r300_devs[i]->iomem_base.l3_base + KL2_REG_L3_HOST_BUFFER1);
            kl2_writel(r300_devs[i], 10000 /* timeout_ms */,
                       r300_devs[i]->iomem_base.l3_base + KL2_REG_L3_HOST_BUFFER2);
            kl2_writel(r300_devs[i], BIT(21),
                       r300_devs[i]->iomem_base.l3_base + KL2_REG_L3_HOST_CMD);
        }
        // reinit start
        LOGI("ccix link reinit start, send UserInt_0 to m3 ...\n");
        for (i = 0; i < r300_count; i++) {
            kl2_writel(r300_devs[i], BIT(0),
                       r300_devs[i]->iomem_base.intc_base + KL2_REG_INTC_SET_7);
        }
        // poll
        LOGI("wait for ccix reinit done(at least 30s) ...\n");
        msleep(3000);
        for (i = 0; i < r300_count; i++) {
            ret = kl_poll_cond_timeout(((kl2_readl(r300_devs[i], r300_devs[i]->iomem_base.l3_base +
                                                                         KL2_REG_L3_HOST_CMD) &
                                         BIT(21)) == 0),
                                       10000, 30000000 /* 30s */);
            // 恢复regular timer, 包含温度功耗读取/pt_list重试下发/超时检测 ...
            if (regular_timer_toggle[i]) {
                hrtimer_start(&r300_devs[i]->regular_timer, r300_devs[i]->regular_ktime,
                              HRTIMER_MODE_REL);
            }

            if (ret) {
                // ccix reinit timeout can be fixed by softreset
                LOGW("%s kl_poll_cond_timeout() failed, ret= %d\n", r300_devs[i]->kdev->name, ret);
                goto err_out;
            }
        }

        ++reset_count;
    } while (1);

    if (ccix_link_speed_status || ccix_link_width_status) {
        LOGW("ccix link reinit failed, reset_count= %d ...\n", reset_count);
    } else if (reset_count) {
        LOGI("ccix link reinit done, reset_count= %d ...\n", reset_count);
    } else {
        LOGI("ccix link check ok ...\n");
    }

err_out:
    vfree(r300_devs);
    return ret;
}
