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

/*
    { 0x025200000ull, 0x000040000ull }, // SYSCON1, 256K
    { 0x023000000ull, 0x000280000ull }, // SYSCON0+INTC+OTP+SECURITY+EMMC+VAC+IPC+SSE+PCIe+SBUS, 2M+512K
    { 0x024000000ull, 0x000800000ull }, // GDDR CTRL&PHY+VMU, 8M
    { 0x025000000ull, 0x001000000ull }, // CLUSTER+SDNN, 16M
    { 0x026000000ull, 0x000800000ull }, // VIDEO, 8M
    { 0x027000000ull, 0x000800000ull }, // CCIX+BFP, 8M
*/
#define KL2_GENERAL_REGISTER_RANGE                                                                 \
    { 0x025200000ull, 0x000040000ull }, { 0x023000000ull, 0x000280000ull },                        \
            { 0x024000000ull, 0x000800000ull }, { 0x025000000ull, 0x001000000ull },                \
            { 0x026000000ull, 0x000800000ull }, { 0x027000000ull, 0x000800000ull },

/*
    { 0x4000000000ull, 0x1000000000ull}, // CCIX0, 64G
    { 0x5000000000ull, 0x1000000000ull}, // CCIX1, 64G
    { 0x6000000000ull, 0x1000000000ull}, // CCIX2, 64G
    { 0x7000000000ull, 0x1000000000ull}, // CCIX3, 64G
*/
#define KL2_CCIX_ADDRESS_RANGE                                                                     \
    { 0x4000000000ull, 0x1000000000ull }, { 0x5000000000ull, 0x1000000000ull },                    \
            { 0x6000000000ull, 0x1000000000ull }, { 0x7000000000ull, 0x1000000000ull },

// SPEC_KL2_GENERAL_MEMORY_CONFIG
// XXX(liyunzheng): 20230424：新增跨4G边界内存分配功能，要求XPU_MEM_MAIN内存块大小（32K * 64 = 2M）对齐，且内存块间无空洞。
static struct kl_memory_info kl_mem_info_eccoff_32g[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xb00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xc00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xd00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xe00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xf00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0x0c0001000ull, 0x003fff000ull, 12, XPU_MEM_L3 },       // L3,       64M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccoff_32g[] = {
    { 0x800000000ull, 0x800000000ull }, // GDDR, 32G
    { 0x0c0000000ull, 0x004000000ull }, // L3, 64M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_memory_range kl_user_dma_rr_rw_ccix_range_eccoff_32g[] = {
    { 0x800000000ull, 0x800000000ull }, // GDDR, 32G
    { 0x0c0000000ull, 0x004000000ull }, // L3, 64M
    KL2_GENERAL_REGISTER_RANGE KL2_CCIX_ADDRESS_RANGE
};
static struct kl_mm_info kl_mm_info_eccoff_32g      = { ARRAY_SIZE(kl_mem_info_eccoff_32g),
                                                   kl_mem_info_eccoff_32g,
                                                   ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_32g),
                                                   kl_user_dma_rr_rw_range_eccoff_32g };
static struct kl_mm_info kl_mm_info_eccoff_32g_r300 = {
    ARRAY_SIZE(kl_mem_info_eccoff_32g), kl_mem_info_eccoff_32g,
    ARRAY_SIZE(kl_user_dma_rr_rw_ccix_range_eccoff_32g), kl_user_dma_rr_rw_ccix_range_eccoff_32g
};

static struct kl_memory_info kl_mem_info_eccon_32g[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xb00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xc00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xd00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xe00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0x0c0001000ull, 0x003fff000ull, 12, XPU_MEM_L3 },       // L3,       64M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccon_32g[] = {
    { 0x800000000ull, 0x700000000ull }, // GDDR, 28G
    { 0x0c0000000ull, 0x004000000ull }, // L3, 64M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_memory_range kl_user_dma_rr_rw_ccix_range_eccon_32g[] = {
    { 0x800000000ull, 0x700000000ull }, // GDDR, 28G
    { 0x0c0000000ull, 0x004000000ull }, // L3, 64M
    KL2_GENERAL_REGISTER_RANGE KL2_CCIX_ADDRESS_RANGE
};
static struct kl_mm_info kl_mm_info_eccon_32g      = { ARRAY_SIZE(kl_mem_info_eccon_32g),
                                                  kl_mem_info_eccon_32g,
                                                  ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_32g),
                                                  kl_user_dma_rr_rw_range_eccon_32g };
static struct kl_mm_info kl_mm_info_eccon_32g_r300 = {
    ARRAY_SIZE(kl_mem_info_eccon_32g), kl_mem_info_eccon_32g,
    ARRAY_SIZE(kl_user_dma_rr_rw_ccix_range_eccon_32g), kl_user_dma_rr_rw_ccix_range_eccon_32g
};

static struct kl_memory_info kl_mem_info_eccoff_16g[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xb00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0x0c0001000ull, 0x003fff000ull, 12, XPU_MEM_L3 },       // L3,       64M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccoff_16g[] = {
    { 0x800000000ull, 0x400000000ull }, // GDDR, 16G
    { 0x0c0000000ull, 0x004000000ull }, // L3, 64M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccoff_16g = { ARRAY_SIZE(kl_mem_info_eccoff_16g),
                                                   kl_mem_info_eccoff_16g,
                                                   ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_16g),
                                                   kl_user_dma_rr_rw_range_eccoff_16g };

static struct kl_memory_info kl_mem_info_eccon_16g[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xb00000000ull, 0x080000000ull, 15, XPU_MEM_MAIN },     // MAIN,     2G,      32K page
    { 0x0c0001000ull, 0x003fff000ull, 12, XPU_MEM_L3 },       // L3,       64M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccon_16g[] = {
    { 0x800000000ull, 0x380000000ull }, // GDDR, 14G
    { 0x0c0000000ull, 0x004000000ull }, // L3, 64M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccon_16g = { ARRAY_SIZE(kl_mem_info_eccon_16g),
                                                  kl_mem_info_eccon_16g,
                                                  ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_16g),
                                                  kl_user_dma_rr_rw_range_eccon_16g };

static struct kl_memory_info kl_mem_info_eccoff_12g[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0x0c0001000ull, 0x003fff000ull, 12, XPU_MEM_L3 },       // L3,       64M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccoff_12g[] = {
    { 0x800000000ull, 0x300000000ull }, // GDDR, 12G
    { 0x0c0000000ull, 0x004000000ull }, // L3, 64M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccoff_12g = { ARRAY_SIZE(kl_mem_info_eccoff_12g),
                                                   kl_mem_info_eccoff_12g,
                                                   ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_12g),
                                                   kl_user_dma_rr_rw_range_eccoff_12g };

static struct kl_memory_info kl_mem_info_eccon_12g[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x0a0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     2G+512M, 32K page
    { 0x0c0001000ull, 0x003fff000ull, 12, XPU_MEM_L3 },       // L3,       64M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccon_12g[] = {
    { 0x800000000ull, 0x2a0000000ull }, // GDDR, 10G+512M
    { 0x0c0000000ull, 0x004000000ull }, // L3, 64M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccon_12g = { ARRAY_SIZE(kl_mem_info_eccon_12g),
                                                  kl_mem_info_eccon_12g,
                                                  ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_12g),
                                                  kl_user_dma_rr_rw_range_eccon_12g };

// SPEC_KL2_SRIOV_GENERAL_MEMORY_CONFIG for PF
static struct kl_memory_info kl_mem_info_sriov_pf[] = {
    // base, size, page_bits, kind
    { 0x0ull, 0x0, 15, -1 },
};
static struct kl_mm_info kl_mm_info_sriov_pf_eccoff_32g = {
    ARRAY_SIZE(kl_mem_info_sriov_pf), kl_mem_info_sriov_pf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_32g), kl_user_dma_rr_rw_range_eccoff_32g
};
static struct kl_mm_info kl_mm_info_sriov_pf_eccon_32g = {
    ARRAY_SIZE(kl_mem_info_sriov_pf), kl_mem_info_sriov_pf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_32g), kl_user_dma_rr_rw_range_eccon_32g
};
static struct kl_mm_info kl_mm_info_sriov_pf_eccoff_16g = {
    ARRAY_SIZE(kl_mem_info_sriov_pf), kl_mem_info_sriov_pf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_16g), kl_user_dma_rr_rw_range_eccoff_16g
};
static struct kl_mm_info kl_mm_info_sriov_pf_eccon_16g = {
    ARRAY_SIZE(kl_mem_info_sriov_pf), kl_mem_info_sriov_pf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_16g), kl_user_dma_rr_rw_range_eccon_16g
};

// SPEC_KL2_SRIOV_GENERAL_MEMORY_CONFIG for VF
static struct kl_memory_info kl_mem_info_eccoff_16g_2vf[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0x0c0001000ull, 0x001fff000ull, 12, XPU_MEM_L3 },       // L3,       32M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccoff_16g_2vf[] = {
    { 0x800000000ull, 0x200000000ull }, // GDDR, 8G
    { 0x0c0000000ull, 0x002000000ull }, // L3, 32M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccoff_16g_2vf = {
    ARRAY_SIZE(kl_mem_info_eccoff_16g_2vf), kl_mem_info_eccoff_16g_2vf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_16g_2vf), kl_user_dma_rr_rw_range_eccoff_16g_2vf
};

static struct kl_memory_info kl_mem_info_eccon_16g_2vf[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x0c0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G,      32K page
    { 0x0c0001000ull, 0x001fff000ull, 12, XPU_MEM_L3 },       // L3,       32M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccon_16g_2vf[] = {
    { 0x800000000ull, 0x1c0000000ull }, // GDDR, 7G
    { 0x0c0000000ull, 0x002000000ull }, // L3, 32M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccon_16g_2vf = {
    ARRAY_SIZE(kl_mem_info_eccon_16g_2vf), kl_mem_info_eccon_16g_2vf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_16g_2vf), kl_user_dma_rr_rw_range_eccon_16g_2vf
};

static struct kl_memory_info kl_mem_info_eccoff_16g_3vf[] = {
    // base, size, page_bits, kind
    // total 5456M(2048M / 3 * 8) available
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x055000000ull, 15, XPU_MEM_MAIN },     // MAIN,     1G+336M, 32K page
    { 0x0c0001000ull, 0x000fff000ull, 12, XPU_MEM_L3 },       // L3,       16M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccoff_16g_3vf[] = {
    { 0x800000000ull, 0x155000000ull }, // GDDR, 5G+336M
    { 0x0c0000000ull, 0x001000000ull }, // L3, 16M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccoff_16g_3vf = {
    ARRAY_SIZE(kl_mem_info_eccoff_16g_3vf), kl_mem_info_eccoff_16g_3vf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_16g_3vf), kl_user_dma_rr_rw_range_eccoff_16g_3vf
};

static struct kl_memory_info kl_mem_info_eccon_16g_3vf[] = {
    // base, size, page_bits, kind
    // total 4776M(1792M / 3 * 8) available
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x02a800000ull, 15, XPU_MEM_MAIN },     // MAIN,     680M,    32K page
    { 0x0c0001000ull, 0x000fff000ull, 12, XPU_MEM_L3 },       // L3,       16M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccon_16g_3vf[] = {
    { 0x800000000ull, 0x12a800000ull }, // GDDR, 4G+680M
    { 0x0c0000000ull, 0x001000000ull }, // L3, 16M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccon_16g_3vf = {
    ARRAY_SIZE(kl_mem_info_eccon_16g_3vf), kl_mem_info_eccon_16g_3vf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_16g_3vf), kl_user_dma_rr_rw_range_eccon_16g_3vf
};

static struct kl_mm_info *kl2_get_sriov_mm_info_16g(int sriov_conf_id, int sriov_func_id,
                                                    int ecc_on)
{
    static struct kl_mm_info *mapping_tbl_eccoff[][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = &kl_mm_info_eccoff_16g,
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_eccoff_16g,
        },
        [KL2_SRIOV_CONF_ID_1VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_16g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_16g,
        },
        [KL2_SRIOV_CONF_ID_2VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_16g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_16g_2vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccoff_16g_2vf,
        },
        [KL2_SRIOV_CONF_ID_3VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_16g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_16g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccoff_16g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_2] = &kl_mm_info_eccoff_16g_3vf,
        },
    };
    static struct kl_mm_info *mapping_tbl_eccon[][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = &kl_mm_info_eccon_16g,
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_eccon_16g,
        },
        [KL2_SRIOV_CONF_ID_1VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_16g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_16g,
        },
        [KL2_SRIOV_CONF_ID_2VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_16g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_16g_2vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccon_16g_2vf,
        },
        [KL2_SRIOV_CONF_ID_3VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_16g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_16g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccon_16g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_2] = &kl_mm_info_eccon_16g_3vf,
        },
    };

    if (!ecc_on) {
        return mapping_tbl_eccoff[sriov_conf_id][sriov_func_id];
    } else {
        return mapping_tbl_eccon[sriov_conf_id][sriov_func_id];
    }
}

static struct kl_memory_info kl_mem_info_eccoff_32g_2vf[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xb00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0x0c0001000ull, 0x001fff000ull, 12, XPU_MEM_L3 },       // L3,       32M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccoff_32g_2vf[] = {
    { 0x800000000ull, 0x400000000ull }, // GDDR, 16G
    { 0x0c0000000ull, 0x002000000ull }, // L3, 32M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccoff_32g_2vf = {
    ARRAY_SIZE(kl_mem_info_eccoff_32g_2vf), kl_mem_info_eccoff_32g_2vf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_32g_2vf), kl_user_dma_rr_rw_range_eccoff_32g_2vf
};

static struct kl_memory_info kl_mem_info_eccon_32g_2vf[] = {
    // base, size, page_bits, kind
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xb00000000ull, 0x080000000ull, 15, XPU_MEM_MAIN },     // MAIN,     2G,      32K page
    { 0x0c0001000ull, 0x001fff000ull, 12, XPU_MEM_L3 },       // L3,       32M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccon_32g_2vf[] = {
    { 0x800000000ull, 0x380000000ull }, // GDDR, 14G
    { 0x0c0000000ull, 0x002000000ull }, // L3, 32M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccon_32g_2vf = {
    ARRAY_SIZE(kl_mem_info_eccon_32g_2vf), kl_mem_info_eccon_32g_2vf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_32g_2vf), kl_user_dma_rr_rw_range_eccon_32g_2vf
};

static struct kl_memory_info kl_mem_info_eccoff_32g_3vf[] = {
    // base, size, page_bits, kind
    // total 10920M(4096M / 3 * 8) available
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x0aa800000ull, 15, XPU_MEM_MAIN },     // MAIN,     2G+680M, 32K page
    { 0x0c0001000ull, 0x000fff000ull, 12, XPU_MEM_L3 },       // L3,       16M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccoff_32g_3vf[] = {
    { 0x800000000ull, 0x2aa800000ull }, // GDDR, 10G+680M
    { 0x0c0000000ull, 0x001000000ull }, // L3, 16M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccoff_32g_3vf = {
    ARRAY_SIZE(kl_mem_info_eccoff_32g_3vf), kl_mem_info_eccoff_32g_3vf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccoff_32g_3vf), kl_user_dma_rr_rw_range_eccoff_32g_3vf
};

static struct kl_memory_info kl_mem_info_eccon_32g_3vf[] = {
    // base, size, page_bits, kind
    // total 9552M(3584M / 3 * 8) available
    { 0x830000000ull, 0x0d0000000ull, 15, XPU_MEM_MAIN },     // MAIN,     3G+256M, 32K page
    { 0x900000000ull, 0x100000000ull, 15, XPU_MEM_MAIN },     // MAIN,     4G,      32K page
    { 0xa00000000ull, 0x055000000ull, 15, XPU_MEM_MAIN },     // MAIN,     1G+336M, 32K page
    { 0x0c0001000ull, 0x000fff000ull, 12, XPU_MEM_L3 },       // L3,       16M-4K,  4K page
    { 0x800000000ull, 0x010000000ull, 12, XPU_MEM_CODE },     // CODE,     256M,    4K page
    { 0x810000000ull, 0x010000000ull, 10, XPU_MEM_PARAM },    // PARAM,    256M,    1K page
    { 0x820000000ull, 0x008000000ull, 10, XPU_MEM_PRINTF },   // PRINTF,   128M,    1K page
    { 0x828000000ull, 0x008000000ull, 10, XPU_MEM_RESERVED }, // RESERVED, 128M,    1K page
};
static struct kl_memory_range kl_user_dma_rr_rw_range_eccon_32g_3vf[] = {
    { 0x800000000ull, 0x255000000ull }, // GDDR, 9G+336M
    { 0x0c0000000ull, 0x001000000ull }, // L3, 16M
    KL2_GENERAL_REGISTER_RANGE
};
static struct kl_mm_info kl_mm_info_eccon_32g_3vf = {
    ARRAY_SIZE(kl_mem_info_eccon_32g_3vf), kl_mem_info_eccon_32g_3vf,
    ARRAY_SIZE(kl_user_dma_rr_rw_range_eccon_32g_3vf), kl_user_dma_rr_rw_range_eccon_32g_3vf
};

static struct kl_mm_info *kl2_get_sriov_mm_info_32g(int sriov_conf_id, int sriov_func_id,
                                                    int ecc_on)
{
    static struct kl_mm_info *mapping_tbl_eccoff[][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = &kl_mm_info_eccoff_32g,
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_eccoff_32g,
        },
        [KL2_SRIOV_CONF_ID_1VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_32g,
        },
        [KL2_SRIOV_CONF_ID_2VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_32g_2vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccoff_32g_2vf,
        },
        [KL2_SRIOV_CONF_ID_3VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_32g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccoff_32g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_2] = &kl_mm_info_eccoff_32g_3vf,
        },
    };
    static struct kl_mm_info *mapping_tbl_eccon[][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = &kl_mm_info_eccon_32g,
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_eccon_32g,
        },
        [KL2_SRIOV_CONF_ID_1VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_32g,
        },
        [KL2_SRIOV_CONF_ID_2VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_32g_2vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccon_32g_2vf,
        },
        [KL2_SRIOV_CONF_ID_3VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_32g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccon_32g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_2] = &kl_mm_info_eccon_32g_3vf,
        },
    };

    if (!ecc_on) {
        return mapping_tbl_eccoff[sriov_conf_id][sriov_func_id];
    } else {
        return mapping_tbl_eccon[sriov_conf_id][sriov_func_id];
    }
}

static struct kl_mm_info *kl2_get_sriov_mm_info_32g_r300(int sriov_conf_id, int sriov_func_id,
                                                         int ecc_on)
{
    static struct kl_mm_info *mapping_tbl_eccoff[][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = &kl_mm_info_eccoff_32g_r300,
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_eccoff_32g_r300,
        },
        [KL2_SRIOV_CONF_ID_1VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_32g,
        },
        [KL2_SRIOV_CONF_ID_2VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_32g_2vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccoff_32g_2vf,
        },
        [KL2_SRIOV_CONF_ID_3VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccoff_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccoff_32g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccoff_32g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_2] = &kl_mm_info_eccoff_32g_3vf,
        },
    };
    static struct kl_mm_info *mapping_tbl_eccon[][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = &kl_mm_info_eccon_32g_r300,
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_eccon_32g_r300,
        },
        [KL2_SRIOV_CONF_ID_1VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_32g,
        },
        [KL2_SRIOV_CONF_ID_2VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_32g_2vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccon_32g_2vf,
        },
        [KL2_SRIOV_CONF_ID_3VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = &kl_mm_info_sriov_pf_eccon_32g,
            [KL2_SRIOV_FUNC_ID_VF_0] = &kl_mm_info_eccon_32g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_1] = &kl_mm_info_eccon_32g_3vf,
            [KL2_SRIOV_FUNC_ID_VF_2] = &kl_mm_info_eccon_32g_3vf,
        },
    };

    if (!ecc_on) {
        return mapping_tbl_eccoff[sriov_conf_id][sriov_func_id];
    } else {
        return mapping_tbl_eccon[sriov_conf_id][sriov_func_id];
    }
}

static struct kl_mm_info *kl2_get_sriov_mm_info_12g(int sriov_conf_id, int sriov_func_id,
                                                    int ecc_on)
{
    static struct kl_mm_info *mapping_tbl_eccoff[][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = &kl_mm_info_eccoff_12g,
        },
    };
    static struct kl_mm_info *mapping_tbl_eccon[][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = &kl_mm_info_eccon_12g,
        },
    };

    if (!ecc_on) {
        return mapping_tbl_eccoff[sriov_conf_id][sriov_func_id];
    } else {
        return mapping_tbl_eccon[sriov_conf_id][sriov_func_id];
    }
}

static struct kl_mm_info *kl_mm_info_general_fn(struct kl2_device *kl2_dev)
{
    int board = kl2_dev->dev_info.board;

    if (board == KL2_BOARD_ID_R200) {
        if (!kl2_dev->ddr_conf.ddr_x8) {
            if (kl2_dev->ddr_conf.nchannel == 6) {
                return kl2_get_sriov_mm_info_12g(kl2_dev->dev_info.sriov_conf,
                                                 kl2_dev->dev_info.sriov_func_id,
                                                 kl2_dev->ddr_conf.ecc_on);
            } else if (kl2_dev->ddr_conf.nchannel == 8) {
                // support SR-IOV only when nchannel is 8
                return kl2_get_sriov_mm_info_16g(kl2_dev->dev_info.sriov_conf,
                                                 kl2_dev->dev_info.sriov_func_id,
                                                 kl2_dev->ddr_conf.ecc_on);
            } else {
                KL2_LOGW("invalid ddr nchannel\n");
            }
        }
    } else if (board == KL2_BOARD_ID_R300) {
        return kl2_get_sriov_mm_info_32g_r300(kl2_dev->dev_info.sriov_conf,
                                              kl2_dev->dev_info.sriov_func_id,
                                              kl2_dev->ddr_conf.ecc_on);
    } else if (board == KL2_BOARD_ID_R200_8F || board == KL2_BOARD_ID_R200_8FS ||
               board == KL2_BOARD_ID_RG800 || board == KL2_BOARD_ID_RG800_PRO) {
        return kl2_get_sriov_mm_info_32g(kl2_dev->dev_info.sriov_conf,
                                         kl2_dev->dev_info.sriov_func_id, kl2_dev->ddr_conf.ecc_on);
    } else if (board == KL2_BOARD_ID_R200_DEBUG_BOARD) {
        return kl2_get_sriov_mm_info_32g(kl2_dev->dev_info.sriov_conf,
                                         kl2_dev->dev_info.sriov_func_id, kl2_dev->ddr_conf.ecc_on);
    } else if ((board == KL2_BOARD_ID_R100) || (board == KL2_BOARD_ID_R420) ||
               (board == KL2_BOARD_ID_RM80)) {
        return kl2_get_sriov_mm_info_12g(kl2_dev->dev_info.sriov_conf,
                                         kl2_dev->dev_info.sriov_func_id, kl2_dev->ddr_conf.ecc_on);
    } else {
        KL2_LOGW("invalid board= %d, use default config\n", board);
        return kl2_get_sriov_mm_info_16g(kl2_dev->dev_info.sriov_conf,
                                         kl2_dev->dev_info.sriov_func_id, kl2_dev->ddr_conf.ecc_on);
    }
    return NULL;
}

static struct kl_mm_info *kl_vf_mm_info_general_fn(struct kl2_device *kl2_dev)
{
    struct kl_device *kdev          = kl2_dev->kdev;
    int               sriov_conf_id = KL2_SRIOV_CONF_ID_1VF + kdev->num_vfs - 1;
    // use mem config of vf0, because all vfs have the save mem config now
    int sriov_func_id = KL2_SRIOV_FUNC_ID_VF_0;
    int board         = kl2_dev->dev_info.board;

    if (kl2_dev->ddr_conf.nchannel != 8) {
        return NULL;
    }

    if (board == KL2_BOARD_ID_R200) {
        return kl2_get_sriov_mm_info_16g(sriov_conf_id, sriov_func_id, kl2_dev->ddr_conf.ecc_on);
    } else if (board == KL2_BOARD_ID_R300) {
        return kl2_get_sriov_mm_info_32g_r300(sriov_conf_id, sriov_func_id,
                                              kl2_dev->ddr_conf.ecc_on);
    } else if (board == KL2_BOARD_ID_R200_8F || board == KL2_BOARD_ID_R200_8FS ||
               board == KL2_BOARD_ID_RG800 || board == KL2_BOARD_ID_RG800_PRO) {
        return kl2_get_sriov_mm_info_32g(sriov_conf_id, sriov_func_id, kl2_dev->ddr_conf.ecc_on);
    } else {
        KL2_LOGW("invalid board= %d, use default config\n", board);
        return kl2_get_sriov_mm_info_16g(sriov_conf_id, sriov_func_id, kl2_dev->ddr_conf.ecc_on);
    }
}

enum SPEC_ID_KL2 {
    SPEC_ID_KL2_NA,
    SPEC_ID_KL2_GENERAL,
    SPEC_ID_KL2_GENERAL_SRIOV_PF,
    SPEC_ID_KL2_GENERAL_SRIOV_1VF_0,
    SPEC_ID_KL2_GENERAL_SRIOV_2VF_0,
    SPEC_ID_KL2_GENERAL_SRIOV_2VF_1,
    SPEC_ID_KL2_GENERAL_SRIOV_3VF_0,
    SPEC_ID_KL2_GENERAL_SRIOV_3VF_1,
    SPEC_ID_KL2_GENERAL_SRIOV_3VF_2,
    NUM_SPEC_ID_KL2,
};

static struct kl2_df_spec kl2_df_spec[] = {
    [SPEC_ID_KL2_GENERAL] = {
        .valid = 1,
        .dmach_bits   = 0xff,
        .hwq_bits     = 0xfff,
        .cl_bits      = 0xff,
        .sdnn_bits    = 0x3f,
        .enc_bits     = 0x7,
        .dec_bits     = 0x1ff,
        .imgproc_bits = 0x3f,
        .kl_mm_info_fn = kl_mm_info_general_fn,
        .kl_vf_mm_info_fn = kl_vf_mm_info_general_fn,
    },

    [SPEC_ID_KL2_GENERAL_SRIOV_PF] = {
        .valid = 1,
        .dmach_bits   = 0x3,
        .hwq_bits     = 0x0,
        .cl_bits      = 0x0,
        .sdnn_bits    = 0x0,
        .enc_bits     = 0x0,
        .dec_bits     = 0x0,
        .imgproc_bits = 0x0,
        .kl_mm_info_fn = kl_mm_info_general_fn,
        .kl_vf_mm_info_fn = kl_vf_mm_info_general_fn,
    },

    [SPEC_ID_KL2_GENERAL_SRIOV_1VF_0] = {
        .valid = 1,
        .dmach_bits   = 0xfc,
        .hwq_bits     = 0xfff,
        .cl_bits      = 0xff,
        .sdnn_bits    = 0x3f,
        .enc_bits     = 0x7,
        .dec_bits     = 0x1ff,
        .imgproc_bits = 0x3f,
        .kl_mm_info_fn = kl_mm_info_general_fn,
    },

    [SPEC_ID_KL2_GENERAL_SRIOV_2VF_0] = {
        .valid = 1,
        .dmach_bits   = 0x1c,
        .hwq_bits     = 0x3f,
        .cl_bits      = 0x7,
        .sdnn_bits    = 0x7,
        .enc_bits     = 0x1,
        .dec_bits     = 0xf,
        .imgproc_bits = 0x7,
        .kl_mm_info_fn = kl_mm_info_general_fn,
    },
    [SPEC_ID_KL2_GENERAL_SRIOV_2VF_1] = {
        .valid = 1,
        .dmach_bits   = 0xe0,
        .hwq_bits     = 0x3f,
        .cl_bits      = 0x7,
        .sdnn_bits    = 0x7,
        .enc_bits     = 0x1,
        .dec_bits     = 0xf,
        .imgproc_bits = 0x7,
        .kl_mm_info_fn = kl_mm_info_general_fn,
    },

    [SPEC_ID_KL2_GENERAL_SRIOV_3VF_0] = {
        .valid = 1,
        .dmach_bits   = 0xc,
        .hwq_bits     = 0xf,
        .cl_bits      = 0x3,
        .sdnn_bits    = 0x3,
        .enc_bits     = 0x1,
        .dec_bits     = 0x7,
        .imgproc_bits = 0x3,
        .kl_mm_info_fn = kl_mm_info_general_fn,
    },
    [SPEC_ID_KL2_GENERAL_SRIOV_3VF_1] = {
        .valid = 1,
        .dmach_bits   = 0x30,
        .hwq_bits     = 0xf,
        .cl_bits      = 0x3,
        .sdnn_bits    = 0x3,
        .enc_bits     = 0x1,
        .dec_bits     = 0x7,
        .imgproc_bits = 0x3,
        .kl_mm_info_fn = kl_mm_info_general_fn,
    },
    [SPEC_ID_KL2_GENERAL_SRIOV_3VF_2] = {
        .valid = 1,
        .dmach_bits   = 0xc0,
        .hwq_bits     = 0xf,
        .cl_bits      = 0x3,
        .sdnn_bits    = 0x3,
        .enc_bits     = 0x1,
        .dec_bits     = 0x7,
        .imgproc_bits = 0x3,
        .kl_mm_info_fn = kl_mm_info_general_fn,
    },
};

// 根据C语言手册，未显式初始化的元素将被初始化为0
static int kl2_df_spec_mapping[NUM_KL2_SRIOV_CONF_ID][NUM_KL2_SRIOV_FUNC_ID] = {
        [KL2_SRIOV_CONF_ID_SRIOV_OFF] = {
            [KL2_SRIOV_FUNC_ID_SRIOV_OFF] = SPEC_ID_KL2_GENERAL,
        },
        [KL2_SRIOV_CONF_ID_1VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = SPEC_ID_KL2_GENERAL_SRIOV_PF,
            [KL2_SRIOV_FUNC_ID_VF_0] = SPEC_ID_KL2_GENERAL_SRIOV_1VF_0,
        },
        [KL2_SRIOV_CONF_ID_2VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = SPEC_ID_KL2_GENERAL_SRIOV_PF,
            [KL2_SRIOV_FUNC_ID_VF_0] = SPEC_ID_KL2_GENERAL_SRIOV_2VF_0,
            [KL2_SRIOV_FUNC_ID_VF_1] = SPEC_ID_KL2_GENERAL_SRIOV_2VF_1,
        },
        [KL2_SRIOV_CONF_ID_3VF] = {
            [KL2_SRIOV_FUNC_ID_PF] = SPEC_ID_KL2_GENERAL_SRIOV_PF,
            [KL2_SRIOV_FUNC_ID_VF_0] = SPEC_ID_KL2_GENERAL_SRIOV_3VF_0,
            [KL2_SRIOV_FUNC_ID_VF_1] = SPEC_ID_KL2_GENERAL_SRIOV_3VF_1,
            [KL2_SRIOV_FUNC_ID_VF_2] = SPEC_ID_KL2_GENERAL_SRIOV_3VF_2,
        },
};

int kl2_get_df_spec(struct kl2_device *kl2_dev)
{
    int sriov_conf    = kl2_dev->dev_info.sriov_conf;
    int sriov_func_id = kl2_dev->dev_info.sriov_func_id;
    int mapping       = SPEC_ID_KL2_NA;

    if ((sriov_conf >= 0 && sriov_conf < NUM_KL2_SRIOV_CONF_ID) &&
        (sriov_func_id >= 0 && sriov_func_id < NUM_KL2_SRIOV_FUNC_ID)) {
        mapping = kl2_df_spec_mapping[sriov_conf][sriov_func_id];
    }
    if (mapping > SPEC_ID_KL2_NA && mapping < NUM_SPEC_ID_KL2 && kl2_df_spec[mapping].valid) {
        memcpy(&kl2_dev->spec, &kl2_df_spec[mapping], sizeof(struct kl2_df_spec));

        if ((kl2_dev->dev_info.board == KL2_BOARD_ID_R100) ||
            (kl2_dev->dev_info.board == KL2_BOARD_ID_R420) ||
            (kl2_dev->dev_info.board == KL2_BOARD_ID_RM80)) {
            kl2_dev->spec.cl_bits   = kl2_dev->otp_info.cl_avail_bits;
            kl2_dev->spec.sdnn_bits = kl2_dev->otp_info.sdnn_avail_bits;
            kl2_dev->spec.dec_bits  = kl2_dev->otp_info.dec_avail_bits;
        }

        return 0;
    }

    KL2_LOGW("invalid conf, ddr_conf= {ddr_x8= %u, ecc_on= %u, nchannel= %u}, "
             "dev_info= {board= %d, sriov_conf= %d, sriov_func_id= %d}\n",
             (unsigned int)kl2_dev->ddr_conf.ddr_x8, (unsigned int)kl2_dev->ddr_conf.ecc_on,
             kl2_dev->ddr_conf.nchannel, kl2_dev->dev_info.board, kl2_dev->dev_info.sriov_conf,
             kl2_dev->dev_info.sriov_func_id);
    kl2_dev->spec.valid = 0;
    return -EINVAL;
}

struct kl_mm_info *kl2_get_mm_info(struct kl2_device *kl2_dev)
{
    struct kl_mm_info *mm_info;
    BUG_ON(!kl2_dev->spec.valid);
    BUG_ON(!kl2_dev->spec.kl_mm_info_fn);

    mm_info = kl2_dev->spec.kl_mm_info_fn(kl2_dev);
    if (!mm_info) {
        KL2_LOGW("mm_info not found, ddr_conf= {ddr_x8= %u, ecc_on= %u, nchannel= %u}, "
                 "dev_info= {board= %d, sriov_conf= %d, sriov_func_id= %d}\n",
                 (unsigned int)kl2_dev->ddr_conf.ddr_x8, (unsigned int)kl2_dev->ddr_conf.ecc_on,
                 kl2_dev->ddr_conf.nchannel, kl2_dev->dev_info.board, kl2_dev->dev_info.sriov_conf,
                 kl2_dev->dev_info.sriov_func_id);
    }
    return mm_info;
}

struct kl_mm_info *kl2_get_vf_mm_info(struct kl2_device *kl2_dev)
{
    struct kl_mm_info *mm_info;
    BUG_ON(!kl2_dev->spec.valid);
    BUG_ON(!kl2_dev->spec.kl_vf_mm_info_fn);

    mm_info = kl2_dev->spec.kl_vf_mm_info_fn(kl2_dev);
    if (!mm_info) {
        KL2_LOGW("vf_mm_info not found, dev_info= {board= %d}\n", kl2_dev->dev_info.board);
    }
    return mm_info;
}
