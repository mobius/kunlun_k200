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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_EXCEPTION_H
#define BAIDU_XPU_RUNTIME_MODULE_KL2_EXCEPTION_H

#include "kl2/kl2.h"

enum kl2_excp_level {
    EL_CONTINUE = 1,
    EL_RESET_UNIT,
    EL_RESET_CHIP,
};

struct kl2_excp_entry {
    int         id;
    const char *name;
    int         level;
};

// 借用BIT(28)表示task timeout
//KL2_CE_ENTRY(RESERVED28, "Reserved for future use", EL_RESET_UNIT)
#define KL2_CLUSTER_EXCEPTIONS                                                                     \
    KL2_CE_ENTRY(PC_OVERFLOW, "program counter exceed code length", EL_RESET_UNIT)                 \
    KL2_CE_ENTRY(INSTR_UNDEF, "undefined instruction", EL_RESET_UNIT)                              \
    KL2_CE_ENTRY(NAN_ERR, "not a floating point number", EL_CONTINUE)                              \
    KL2_CE_ENTRY(INT_DIV0, "integer divided by zero", EL_CONTINUE)                                 \
    KL2_CE_ENTRY(LD_ST_OVERFLOW, "load/store exceed memory size", EL_RESET_UNIT)                   \
    KL2_CE_ENTRY(LD_ST_UNALIGN, "load/store address unaligned", EL_RESET_UNIT)                     \
    KL2_CE_ENTRY(DMA_OVERFLOW, "dma operation exceed memory size", EL_RESET_UNIT)                  \
    KL2_CE_ENTRY(DMA_LEN_ZERO, "dma operation length equal zero", EL_RESET_UNIT)                   \
    KL2_CE_ENTRY(FP_ERROR_ZERO, "floating-point output is zero", EL_CONTINUE)                      \
    KL2_CE_ENTRY(FP_ERROR_INFINITY, "output is infinity", EL_CONTINUE)                             \
    KL2_CE_ENTRY(FP_ERROR_INVALID, "floating point op Invalid", EL_CONTINUE)                       \
    KL2_CE_ENTRY(FP_ERROR_TINY, "floating point output tiny", EL_CONTINUE)                         \
    KL2_CE_ENTRY(FP_ERROR_HUGE, "floating point output huge", EL_CONTINUE)                         \
    KL2_CE_ENTRY(FP_ERROR_INEXACT, "floating point inexact error", EL_CONTINUE)                    \
    KL2_CE_ENTRY(FP_ERROR_HUGEINT, "floating point huge int error ", EL_CONTINUE)                  \
    KL2_CE_ENTRY(HARDWARE_FATAL, "hardware fatal", EL_RESET_CHIP)                                  \
    KL2_CE_ENTRY(SIMD_VSTORE_OVER, "simd operation vstore overflow", EL_RESET_UNIT)                \
    KL2_CE_ENTRY(SIMD_VLOAD_OVER, "simd operation vload overflow", EL_RESET_UNIT)                  \
    KL2_CE_ENTRY(SIMD_INST_UNDEF, "simd instruction undefined", EL_RESET_UNIT)                     \
    KL2_CE_ENTRY(SIMD_FP_ERROR, "simd floating point ip error", EL_CONTINUE)                       \
    KL2_CE_ENTRY(AXI0_RRESP_ERROR, "axi0 rresp error", EL_RESET_CHIP)                              \
    KL2_CE_ENTRY(AXI0_WRESP_ERROR, "axi0 wresp error", EL_RESET_CHIP)                              \
    KL2_CE_ENTRY(AXI1_RRESP_ERROR, "axi1 rresp error", EL_RESET_CHIP)                              \
    KL2_CE_ENTRY(AXI1_WRESP_ERROR, "axi1 wresp error", EL_RESET_CHIP)                              \
    KL2_CE_ENTRY(ATOM_NESTED_ERROR, "nested automatic operation error", EL_RESET_UNIT)             \
    KL2_CE_ENTRY(SFU_FP_ERROR, "sfu floating point ip error", EL_CONTINUE)                         \
    KL2_CE_ENTRY(SM_RDWR_CONFLICT, "sm rdwr conflict", EL_RESET_UNIT)                              \
    KL2_CE_ENTRY(LM_RDWR_CONFLICT, "lm rdwr conflict", EL_RESET_UNIT)                              \
    KL2_CE_ENTRY(TASK_TIMEOUT, "task timeout", EL_RESET_UNIT)                                      \
    KL2_CE_ENTRY(RESERVED29, "Reserved for future use", EL_RESET_UNIT)                             \
    KL2_CE_ENTRY(CORE_TRAP, "trap exception", EL_CONTINUE)                                         \
    KL2_CE_ENTRY(SDNN_EXCEPTION, "sdnn exception", EL_RESET_UNIT)

// 借用BIT(10)表示task timeout，软件超时检测使用。
#define KL2_SDNN_EXCEPTIONS                                                                        \
    KL2_CE_ENTRY(DS_0, "ds_0 exception", EL_CONTINUE)                                              \
    KL2_CE_ENTRY(DS_1, "ds_1 exception", EL_CONTINUE)                                              \
    KL2_CE_ENTRY(MAC, "mac exception", EL_CONTINUE)                                                \
    KL2_CE_ENTRY(EW, "ew exception", EL_CONTINUE)                                                  \
    KL2_CE_ENTRY(RS, "rs exception", EL_CONTINUE)                                                  \
    KL2_CE_ENTRY(DMA_IN0, "dma_in0 exception", EL_CONTINUE)                                        \
    KL2_CE_ENTRY(DMA_IN1, "dma_in1 exception", EL_CONTINUE)                                        \
    KL2_CE_ENTRY(DMA_OUT, "dma_out exception", EL_CONTINUE)                                        \
    KL2_CE_ENTRY(SCHEDULER, "scheduler exception", EL_CONTINUE)                                    \
    KL2_CE_ENTRY(DS_MUX, "ds_mux exception", EL_CONTINUE)                                          \
    KL2_CE_ENTRY(SDNN_TASK_TIMEOUT, "task timeout", EL_RESET_UNIT)                                 \
    KL2_CE_ENTRY(SDNN_MAX, "reserved", EL_CONTINUE)

enum kl2_cluster_excp_enum {
#if defined(KL2_CE_ENTRY)
#undef KL2_CE_ENTRY
#endif

#define KL2_CE_ENTRY(i, n, l) CE_##i,
    KL2_CLUSTER_EXCEPTIONS
};

enum kl2_sdnn_excp_enum {
#if defined(KL2_CE_ENTRY)
#undef KL2_CE_ENTRY
#endif

#define KL2_CE_ENTRY(i, n, l) CE_##i,
    KL2_SDNN_EXCEPTIONS
};

void kl2_handle_gddr_excp(struct kl2_device *kl2_dev, u32 state);
void kl2_handle_sse_excp(struct kl2_device *kl2_dev, u32 state);

const char *kl2_get_cluster_excp_name(struct kl2_device *kl2_dev, int idx);
const char *kl2_get_sdnn_excp_name(struct kl2_device *kl2_dev, int idx);

void kl2_cluster_disable_and_record_excp(struct kl2_device *kl2_dev, u32 st);
void kl2_sdnn_disable_and_record_excp(struct kl2_device *kl2_dev, u32 st);
void kl2_cluster_disable_and_record_timeout(struct kl2_device *kl2_dev, u32 st,
                                            u32 *cl_timeout_token);
void kl2_sdnn_disable_and_record_timeout(struct kl2_device *kl2_dev, u32 st, u32 *sd_timeout_token);

void kl2_handle_excp_work_func(struct work_struct *work);

#define kl2_print_cluster_debug_info(kl2_dev, cl_id, cl_debug_info)                                \
    {                                                                                              \
        u32 i1;                                                                                    \
                                                                                                   \
        for (i1 = 0; i1 < KL2_REG_CLUSTER_DEBUG_ARRAY_SIZE; i1++) {                                \
            KL2_PRINT_FMT_STR("cluster[p%d, v%d]: [%04x]= %08x\n", (cl_id),                        \
                              (cl_debug_info)->cl_virt_id, KL2_REG_CLUSTER_DEBUG_ARRAY[i1],        \
                              (cl_debug_info)->cl_debug_regs[i1]);                                 \
        }                                                                                          \
    }

#define kl2_print_sdnn_cl_debug_info(kl2_dev, sdnn_id, sdnn_debug_info)                            \
    {                                                                                              \
        u32 i2;                                                                                    \
                                                                                                   \
        for (i2 = 0; i2 < KL2_REG_CLUSTER_DEBUG_ARRAY_SIZE; i2++) {                                \
            KL2_PRINT_FMT_STR("sdnn(cl)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                     \
                              (sdnn_debug_info)->sdnn_cl_virt_id, KL2_REG_CLUSTER_DEBUG_ARRAY[i2], \
                              (sdnn_debug_info)->sdnn_cl_debug_regs[i2]);                          \
        }                                                                                          \
    }

#define kl2_print_sdnn_sd_debug_info(kl2_dev, sdnn_id, sdnn_debug_info, ce)                        \
    {                                                                                              \
        u32 i3;                                                                                    \
                                                                                                   \
        switch (ce) {                                                                              \
        case CE_DMA_IN0:                                                                           \
            for (i3 = 0; i3 < KL2_REG_SDNN_DMAI0_DEBUG_ARRAY_SIZE; i3++) {                         \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_DMAI0_DEBUG_ARRAY[i3],                              \
                                  (sdnn_debug_info)->sdnn_dmai0_debug_regs[i3]);                   \
            }                                                                                      \
            break;                                                                                 \
        case CE_DMA_IN1:                                                                           \
            for (i3 = 0; i3 < KL2_REG_SDNN_DMAI1_DEBUG_ARRAY_SIZE; i3++) {                         \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_DMAI1_DEBUG_ARRAY[i3],                              \
                                  (sdnn_debug_info)->sdnn_dmai1_debug_regs[i3]);                   \
            }                                                                                      \
            break;                                                                                 \
        case CE_DS_0:                                                                              \
            for (i3 = 0; i3 < KL2_REG_SDNN_DS0_DEBUG_ARRAY_SIZE; i3++) {                           \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_DS0_DEBUG_ARRAY[i3],                                \
                                  (sdnn_debug_info)->sdnn_ds0_debug_regs[i3]);                     \
            }                                                                                      \
            break;                                                                                 \
        case CE_DS_1:                                                                              \
            for (i3 = 0; i3 < KL2_REG_SDNN_DS1_DEBUG_ARRAY_SIZE; i3++) {                           \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_DS1_DEBUG_ARRAY[i3],                                \
                                  (sdnn_debug_info)->sdnn_ds1_debug_regs[i3]);                     \
            }                                                                                      \
            break;                                                                                 \
        case CE_MAC:                                                                               \
            for (i3 = 0; i3 < KL2_REG_SDNN_MAC_DEBUG_ARRAY_SIZE; i3++) {                           \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_MAC_DEBUG_ARRAY[i3],                                \
                                  (sdnn_debug_info)->sdnn_mac_debug_regs[i3]);                     \
            }                                                                                      \
            break;                                                                                 \
        case CE_EW:                                                                                \
            for (i3 = 0; i3 < KL2_REG_SDNN_EW_DEBUG_ARRAY_SIZE; i3++) {                            \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_EW_DEBUG_ARRAY[i3],                                 \
                                  (sdnn_debug_info)->sdnn_ew_debug_regs[i3]);                      \
            }                                                                                      \
            break;                                                                                 \
        case CE_RS:                                                                                \
            for (i3 = 0; i3 < KL2_REG_SDNN_RS_DEBUG_ARRAY_SIZE; i3++) {                            \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_RS_DEBUG_ARRAY[i3],                                 \
                                  (sdnn_debug_info)->sdnn_rs_debug_regs[i3]);                      \
            }                                                                                      \
            break;                                                                                 \
        case CE_DMA_OUT:                                                                           \
            for (i3 = 0; i3 < KL2_REG_SDNN_DMAO_DEBUG_ARRAY_SIZE; i3++) {                          \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_DMAO_DEBUG_ARRAY[i3],                               \
                                  (sdnn_debug_info)->sdnn_dmao_debug_regs[i3]);                    \
            }                                                                                      \
            break;                                                                                 \
        case CE_SCHEDULER:                                                                         \
            for (i3 = 0; i3 < KL2_REG_SDNN_SCH_DEBUG_ARRAY_SIZE; i3++) {                           \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_SCH_DEBUG_ARRAY[i3],                                \
                                  (sdnn_debug_info)->sdnn_sch_debug_regs[i3]);                     \
            }                                                                                      \
            break;                                                                                 \
        case CE_DS_MUX:                                                                            \
            for (i3 = 0; i3 < KL2_REG_SDNN_DSMUX_DEBUG_ARRAY_SIZE; i3++) {                         \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: [%04x]= %08x\n", (sdnn_id),                 \
                                  (sdnn_debug_info)->sdnn_cl_virt_id,                              \
                                  KL2_REG_SDNN_DSMUX_DEBUG_ARRAY[i3],                              \
                                  (sdnn_debug_info)->sdnn_dsmux_debug_regs[i3]);                   \
            }                                                                                      \
            break;                                                                                 \
        default:                                                                                   \
            break;                                                                                 \
        }                                                                                          \
    }

#define kl2_print_all_sdnn_sd_debug_info(kl2_dev, sdnn_id, sdnn_debug_info)                        \
    {                                                                                              \
        int ce;                                                                                    \
                                                                                                   \
        for (ce = CE_DS_0; ce < CE_SDNN_MAX; ce++) {                                               \
            kl2_print_sdnn_sd_debug_info(kl2_dev, (sdnn_id), (sdnn_debug_info), ce);               \
        }                                                                                          \
    }

#define kl2_print_etask(kl2_dev, etask, debug_info)                                                 \
    {                                                                                               \
        struct kl2_task *task = &(etask)->task;                                                     \
        int              i4, j4;                                                                    \
                                                                                                    \
        KL2_PRINT_FMT_STR(KL_CUT_HERE);                                                             \
        KL2_PRINT_FMT_STR(                                                                          \
                "err task, pid=%d, comm=%s, sess=%d, hwq=%d, "                                      \
                ".tk=%u .name=%s .ty=%u .ncl=%u .nco=%u .kaddr=%llx .ksz=%x .paddr=%llx .psz=%x\n", \
                task->pid, (etask)->comm, task->sess_id, task->hwq_id, task->desc.kernel.token,     \
                task->kernel_name, task->desc.kernel.type, task->desc.kernel.nclusters,             \
                task->desc.kernel.ncores, (u64)task->desc.kernel.code_addr,                         \
                task->desc.kernel.codelen, (u64)task->desc.kernel.param_addr,                       \
                task->desc.kernel.param0);                                                          \
        for (i4 = 0; i4 < task->desc.kernel.param0 / sizeof(u32); ++i4) {                           \
            KL2_PRINT_FMT_STR("..param[%d]= %08x\n", i4, task->params[i4]);                         \
        }                                                                                           \
                                                                                                    \
        for (i4 = 0; i4 < KL2_CLUSTER_MAX_COUNT; ++i4) {                                            \
            u32           cl_excp_st    = (etask)->cl_excp_st[i4];                                  \
            unsigned long cl_excp_st_ul = cl_excp_st;                                               \
            u32           cl_virt_id    = (etask)->cl_debug_info[i4].cl_virt_id;                    \
            if (cl_excp_st_ul) {                                                                    \
                KL2_PRINT_FMT_STR("cluster[p%d, v%d]: cl_excp_st= %08x\n", i4, cl_virt_id,          \
                                  cl_excp_st);                                                      \
                for_each_set_bit(j4, &cl_excp_st_ul, 32) {                                          \
                    KL2_PRINT_FMT_STR("cluster[p%d, v%d]: ..reason[%d] %s\n", i4, cl_virt_id, j4,   \
                                      kl2_get_cluster_excp_name(kl2_dev, j4));                      \
                }                                                                                   \
                if ((debug_info)) {                                                                 \
                    kl2_print_cluster_debug_info(kl2_dev, i4, &(etask)->cl_debug_info[i4]);         \
                }                                                                                   \
            }                                                                                       \
        }                                                                                           \
        for (i4 = 0; i4 < KL2_SDNN_MAX_COUNT; ++i4) {                                               \
            u32           sdnn_cl_excp_st    = (etask)->sdnn_cl_excp_st[i4];                        \
            u32           sdnn_sd_excp_st    = (etask)->sdnn_sd_excp_st[i4];                        \
            unsigned long sdnn_cl_excp_st_ul = sdnn_cl_excp_st;                                     \
            unsigned long sdnn_sd_excp_st_ul = sdnn_sd_excp_st;                                     \
            u32           sdnn_cl_virt_id    = (etask)->sdnn_debug_info[i4].sdnn_cl_virt_id;        \
            if (sdnn_cl_excp_st_ul) {                                                               \
                KL2_PRINT_FMT_STR("sdnn(cl)[p%d, v%d]: sdnn_cl_excp_st= %08x\n", i4,                \
                                  sdnn_cl_virt_id, sdnn_cl_excp_st);                                \
                for_each_set_bit(j4, &sdnn_cl_excp_st_ul, 32) {                                     \
                    KL2_PRINT_FMT_STR("sdnn(cl)[p%d, v%d]: ..reason[%d] %s\n", i4,                  \
                                      sdnn_cl_virt_id, j4,                                          \
                                      kl2_get_cluster_excp_name(kl2_dev, j4));                      \
                }                                                                                   \
                if ((debug_info)) {                                                                 \
                    kl2_print_sdnn_cl_debug_info(kl2_dev, i4, &(etask)->sdnn_debug_info[i4]);       \
                }                                                                                   \
            }                                                                                       \
            if (sdnn_sd_excp_st_ul) {                                                               \
                KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: sdnn_sd_excp_st= %08x\n", i4,                \
                                  sdnn_cl_virt_id, sdnn_sd_excp_st);                                \
                for_each_set_bit(j4, &sdnn_sd_excp_st_ul, 32) {                                     \
                    KL2_PRINT_FMT_STR("sdnn(sd)[p%d, v%d]: ..reason[%d] %s\n", i4,                  \
                                      sdnn_cl_virt_id, j4, kl2_get_sdnn_excp_name(kl2_dev, j4));    \
                }                                                                                   \
                if ((debug_info)) {                                                                 \
                    kl2_print_all_sdnn_sd_debug_info(kl2_dev, i4, &(etask)->sdnn_debug_info[i4]);   \
                }                                                                                   \
            }                                                                                       \
        }                                                                                           \
        KL2_PRINT_FMT_STR(KL_CUT_HERE);                                                             \
    }

#endif
