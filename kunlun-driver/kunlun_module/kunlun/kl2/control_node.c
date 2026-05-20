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

static int kl2_device_model(struct kl2_device *kl2_dev)
{
    int ret = R200;

    switch (kl2_dev->dev_info.board) {
    case KL2_BOARD_ID_R100:
        ret = R100;
        break;
    case KL2_BOARD_ID_R200:
        if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF)
            ret = R200_SRIOV_PF;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_SRIOV_OFF)
            ret = R200;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_1VF)
            ret = R200_SRIOV_VF_ONE_OF_ONE;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_2VF)
            ret = R200_SRIOV_VF_ONE_OF_TWO;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_3VF)
            ret = R200_SRIOV_VF_ONE_OF_THREE;
        break;
    case KL2_BOARD_ID_R300:
        if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF)
            ret = R300_SRIOV_PF;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_SRIOV_OFF)
            ret = R300;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_1VF)
            ret = R300_SRIOV_VF_ONE_OF_ONE;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_2VF)
            ret = R300_SRIOV_VF_ONE_OF_TWO;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_3VF)
            ret = R300_SRIOV_VF_ONE_OF_THREE;
        break;
    case KL2_BOARD_ID_R200_8F:
        if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF)
            ret = R200_8F_SRIOV_PF;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_SRIOV_OFF)
            ret = R200_8F;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_1VF)
            ret = R200_8F_SRIOV_VF_ONE_OF_ONE;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_2VF)
            ret = R200_8F_SRIOV_VF_ONE_OF_TWO;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_3VF)
            ret = R200_8F_SRIOV_VF_ONE_OF_THREE;
        break;
    case KL2_BOARD_ID_R200_8FS:
        if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF)
            ret = R200_8FS_SRIOV_PF;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_SRIOV_OFF)
            ret = R200_8FS;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_1VF)
            ret = R200_8FS_SRIOV_VF_ONE_OF_ONE;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_2VF)
            ret = R200_8FS_SRIOV_VF_ONE_OF_TWO;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_3VF)
            ret = R200_8FS_SRIOV_VF_ONE_OF_THREE;
        break;
    case KL2_BOARD_ID_R200_DEBUG_BOARD:
        ret = R200_DEBUG_BOARD;
        break;
    case KL2_BOARD_ID_R420:
        ret = R420;
        break;
    case KL2_BOARD_ID_RG800:
        if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF)
            ret = RG800_SRIOV_PF;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_SRIOV_OFF)
            ret = RG800;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_1VF)
            ret = RG800_SRIOV_VF_ONE_OF_ONE;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_2VF)
            ret = RG800_SRIOV_VF_ONE_OF_TWO;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_3VF)
            ret = RG800_SRIOV_VF_ONE_OF_THREE;
        break;
    case KL2_BOARD_ID_RG800_PRO:
        if (kl2_dev->dev_info.sriov_func_id == KL2_SRIOV_FUNC_ID_PF)
            ret = RG800_PRO_SRIOV_PF;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_SRIOV_OFF)
            ret = RG800_PRO;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_1VF)
            ret = RG800_PRO_SRIOV_VF_ONE_OF_ONE;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_2VF)
            ret = RG800_PRO_SRIOV_VF_ONE_OF_TWO;
        else if (kl2_dev->dev_info.sriov_conf == KL2_SRIOV_CONF_ID_3VF)
            ret = RG800_PRO_SRIOV_VF_ONE_OF_THREE;
        break;
    case KL2_BOARD_ID_RM80:
        ret = RM80;
        break;
    default:
        // 后续增加的KL2板卡，暂识别为R200
        ret = R200;
        break;
    }

    return ret;
}

static int kl2_query_mem_info(struct kl2_device *kl2_dev, XPUMemoryKind mem_kind,
                              union xpu_device_info_v1 *i)
{
    kl_cxpu_instance_t *cxpu_instance;
    u32                 page_size = kl_mm_get_pgsz(&kl2_dev->mm, mem_kind);

    if (!page_size) {
        return -EFAULT;
    }

    i->out.v32 = page_size;
    cxpu_instance =
            kl_cxpu_get_instance_by_token_locked(&kl2_dev->kdev->cxpu, (u64)current->cgroups);
    if (kl_cxpu_instance_is_mem_limit_on(cxpu_instance, mem_kind)) {
        i->out.v64[0] = kl_cxpu_instance_get_mem_used(cxpu_instance, mem_kind);
        i->out.v64[1] = kl_cxpu_instance_get_mem_limit(cxpu_instance, mem_kind);
        i->out.v64[2] = kl_mm_get_bytes_all(&kl2_dev->mm, mem_kind) / page_size;
    } else {
        if (mem_kind == XPU_MEM_MAIN) {
            u64 main_mem_bytes_all, main_mem_bytes_used;
            main_mem_bytes_used = kl_mm_get_bytes_used(&kl2_dev->mm, XPU_MEM_MAIN) +
                                  kl_mm_get_bytes_used(&kl2_dev->mm, XPU_MEM_CODE) +
                                  kl_mm_get_bytes_used(&kl2_dev->mm, XPU_MEM_PARAM) +
                                  kl_mm_get_bytes_used(&kl2_dev->mm, XPU_MEM_PRINTF) +
                                  kl_mm_get_bytes_used(&kl2_dev->mm, XPU_MEM_RESERVED);
            main_mem_bytes_all = kl_mm_get_bytes_all(&kl2_dev->mm, XPU_MEM_MAIN) +
                                 kl_mm_get_bytes_all(&kl2_dev->mm, XPU_MEM_CODE) +
                                 kl_mm_get_bytes_all(&kl2_dev->mm, XPU_MEM_PARAM) +
                                 kl_mm_get_bytes_all(&kl2_dev->mm, XPU_MEM_PRINTF) +
                                 kl_mm_get_bytes_all(&kl2_dev->mm, XPU_MEM_RESERVED);
            // 如gddr开启inline ecc，还原至颗粒容量，xpu_smi代码无需修改
            if (kl2_dev->ddr_conf.ecc_on) {
                main_mem_bytes_used = main_mem_bytes_used / 7 * 8;
                main_mem_bytes_all  = main_mem_bytes_all / 7 * 8;
            }
            i->out.v64[0] = main_mem_bytes_used / i->out.v32;
            i->out.v64[1] = main_mem_bytes_all / i->out.v32;
        } else if (mem_kind == XPU_MEM_L3) {
            i->out.v64[0] = kl_mm_get_bytes_used(&kl2_dev->mm, mem_kind) / i->out.v32;
            i->out.v64[1] = kl_mm_get_bytes_all(&kl2_dev->mm, mem_kind) / i->out.v32;
        } else {
            KL2_LOGW("invalid mem_kind= %d\n", mem_kind);
        }
    }
    kl_cxpu_put_instance_locked(&kl2_dev->kdev->cxpu, cxpu_instance);

    return 0;
}

int kl2_query_device_info_v1(struct kl_device *kdev, union xpu_device_info_v1 *i)
{
    struct kl2_device *kl2_dev = kdev->data;

    switch (i->in.type) {
    case QDIT_MODEL_SN: {
        i->out.ret = kl2_device_model(kl2_dev);
        // 16 chars SN
        i->out.v64[0] = makeu64(kl2_dev->dev_info.sn[1], kl2_dev->dev_info.sn[0]);
        i->out.v64[1] = makeu64(kl2_dev->dev_info.sn[3], kl2_dev->dev_info.sn[2]);
        break;
    }
    case QDIT_PN: {
        // 8 chars PN
        i->out.v64[0] = makeu64(kl2_dev->dev_info.pn[1], kl2_dev->dev_info.pn[0]);
        break;
    }
    case QDIT_COORDINATE: {
        i->out.v32    = i->in.arg[0];
        i->out.v64[0] = makeu64(kdev->idx, 0);
        i->out.v64[1] = makeu64(kdev->domain, kdev->bus);
        i->out.v64[2] = makeu64(kdev->slot, kdev->func);
        break;
    }
    case QDIT_HS_MEM: {
        kl2_query_mem_info(kl2_dev, XPU_MEM_L3, i);
        break;
    }
    case QDIT_MAIN_MEM: {
        kl2_query_mem_info(kl2_dev, XPU_MEM_MAIN, i);
        break;
    }
    case QDIT_USE_RATIO: {
        i->out.v32    = ur_weight(&kl2_dev->ur);
        i->out.v64[0] = USE_RATIO_PN;
        break;
    }
    case QDIT_DMA_RATIO: {
        i->out.v32    = 0; // time_us
        i->out.v64[0] = 0; // read bytes
        i->out.v64[1] = 0; // write bytes
        i->out.v64[2] = 0;
        break;
    }
    case QDIT_TEMPERATURE: {
        // decoding required: -0.23751*$temp+356.0
        i->out.v32    = kl2_dev->dev_info.temperature;
        i->out.v64[0] = makeu64(0, 0);
        break;
    }
    case QDIT_STATE: {
        i->out.ret = kl2_get_state(kl2_dev);
        break;
    }
    case QDIT_FREQUENCY: {
        // decoding required:
        //
        //if (sdnn_freq != 0) {
        //sdnn_freq = 5200 / sdnn_freq;
        //}
        //if (cluster_freq != 0) {
        //cluster_freq = 4800 / cluster_freq;
        //}
        i->out.v64[0] = makeu64(kl2_dev->dev_info.sdnn_freq, kl2_dev->dev_info.cluster_freq);
        break;
    }
    case QDIT_POWER: {
        i->out.v32 = kl2_dev->dev_info.power;
        break;
    }
    case QDIT_FIRMWARE_VERSION: {
        // fw version x.x.x
        i->out.v32    = kl2_dev->dev_info.fw[0];
        i->out.v64[0] = makeu64(kl2_dev->dev_info.fw[1], kl2_dev->dev_info.fw[2]);
        // cpld version
        i->out.v64[1] = kl2_dev->dev_info.cpld;
        break;
    }
    case QDIT_CG_HS_MEM: {
        kl2_query_mem_info(kl2_dev, XPU_MEM_L3, i);
        break;
    }
    case QDIT_CG_MAIN_MEM: {
        kl2_query_mem_info(kl2_dev, XPU_MEM_MAIN, i);
        break;
    }
    case QDIT_CU: {
        i->out.v64[0] = bitcount(kl2_dev->spec.cl_bits);
        i->out.v64[1] = bitcount(kl2_dev->spec.sdnn_bits);
        i->out.v64[2] = bitcount(kl2_dev->spec.dmach_bits);
        break;
    }
    case QDIT_CODEC: {
        i->out.v64[0] = bitcount(kl2_dev->spec.dec_bits);
        i->out.v64[1] = bitcount(kl2_dev->spec.enc_bits);
        i->out.v64[2] = bitcount(kl2_dev->spec.imgproc_bits);
        break;
    }
    case QDIT_SRIOV_INFO: {
        if (is_vf_id(kl2_dev->dev_info.sriov_func_id)) {
            i->out.v32    = 1;
            i->out.v64[0] = 0;
            i->out.v64[1] = 0;
            i->out.v64[2] = kl2_dev->dev_info.sriov_func_id - KL2_SRIOV_FUNC_ID_VF_0;
        } else {
            i->out.v32    = 0;
            i->out.v64[0] = KL2_SRIOV_MAX_NUM_VFS;
            i->out.v64[1] = kl2_dev->dev_info.sriov_num_vfs;
            i->out.v64[2] = 0;
        }
        break;
    }
#ifdef ENABLE_CODEC
    case QDIT_VIDEO_RATIO: {
        kl2_get_video_ratio(kl2_dev, i->out.v64);
        break;
    }
    case QDIT_VIDEO_FRAME_RATE: {
        kl2_get_video_frame_rate(kl2_dev, i->out.v64);
        break;
    }
    case QDIT_VIDEO_CLOCK: {
        kl2_get_video_clock(kl2_dev, i->out.v64);
        break;
    }
#endif
    case QDIT_ECC_MODE: {
        i->out.v64[0] = kl2_dev->ddr_conf.ecc_on;
        i->out.v64[1] = kl2_dev->ddr_conf.ecc_on;
        break;
    }
    case QDIT_ECC_ERROR_COUNT: {
        if (i->in.arg[1] == XPU_ECC_SBE) {
            i->out.v64[0] = kl2_dev->dev_info.ecc_sbe_count;
        } else if (i->in.arg[1] == XPU_ECC_DBE) {
            i->out.v64[0] = kl2_dev->dev_info.ecc_dbe_count;
        }
        break;
    }
    default:
        return -XPUERR_NOSUPPORT;
    }

    return 0;
}

int kl2_query_device_proc_info(struct kl_device *kdev, struct ioc_qproc_info_in *i,
                               struct xpu_device_processes *dp)
{
    struct kl2_device      *kl2_dev = kdev->data;
    struct kl2_userprocess *uproc;
    int                     pid;

    if (i == NULL || dp == NULL)
        return -EINVAL;

    dp->magic_version = PROCINFO_MAGIC_V0;
    dp->process_count = 0;
    dp->devfile_id    = i->devfile_id;
    dp->dma_time_us   = 0;

    mutex_lock(&kl2_dev->uproc_session_lock);
    idr_for_each_entry(&kl2_dev->uproc_idr, uproc, pid) {
        struct xpu_proc *proc = &dp->proc[dp->process_count];

        proc->pid          = uproc->ns_pid;
        proc->root_ns_pid  = uproc->pid;
        proc->stream_count = xref_read(&uproc->xref);

        memcpy(proc->comm, uproc->comm, PROCESS_COMM_LEN);

        // pgused会被截断，但一般无问题
        proc->cgtoken             = kl_cxpu_instance_get_token(uproc->cxpu_instance);
        proc->hs_mem_pgused       = (u32)atomic64_read(&uproc->mem_used_pgcnt[XPU_MEM_L3]);
        proc->hs_mem_pgall        = 0;
        proc->main_mem_pgused     = (u32)atomic64_read(&uproc->mem_used_pgcnt[XPU_MEM_MAIN]);
        proc->main_mem_pgall      = 0;
        proc->dma_read_byte_size  = 0;
        proc->dma_write_byte_size = 0;
        proc->ur_weight           = 0; // ur_weight(&uproc->ur);
        proc->level               = 0; // uproc->plvl->lvl;
        // 如gddr开启inline ecc，还原至颗粒容量，xpu_smi代码无需修改
        if (kl2_dev->ddr_conf.ecc_on) {
            proc->main_mem_pgused = proc->main_mem_pgused / 7 * 8;
        }

        ++dp->process_count;
        if (dp->process_count >= DEVICE_MAX_PROCESS_PRINT_COUNT)
            break;
    }
    mutex_unlock(&kl2_dev->uproc_session_lock);

    return 0;
}
