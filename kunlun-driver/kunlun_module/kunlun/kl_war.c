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

#include <linux/string.h>
#include <kl_drv.h>

#if defined(CONFIG_X86) || defined(CONFIG_X86_64)

static inline void kl_native_cpuid(unsigned int *eax, unsigned int *ebx, unsigned int *ecx,
                                   unsigned int *edx)
{
    /* ecx is often an input as well as an output. */
    asm volatile("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "0"(*eax), "2"(*ecx)
                 : "memory");
}

#define kl__cpuid kl_native_cpuid

/*
 * Generic CPUID function
 * clear %ecx since some cpus (Cyrix MII) do not set or clear %ecx
 * resulting in stale register contents being returned.
 */
static inline void kl_cpuid(unsigned int op, unsigned int *eax, unsigned int *ebx,
                            unsigned int *ecx, unsigned int *edx)
{
    *eax = op;
    *ecx = 0;
    kl__cpuid(eax, ebx, ecx, edx);
}

/* Some CPUID calls want 'count' to be placed in ecx */
static inline void kl_cpuid_count(unsigned int op, int count, unsigned int *eax, unsigned int *ebx,
                                  unsigned int *ecx, unsigned int *edx)
{
    *eax = op;
    *ecx = count;
    kl__cpuid(eax, ebx, ecx, edx);
}

/*
 * CPUID functions returning a single datum
 */
static inline unsigned int kl_cpuid_eax(unsigned int op)
{
    unsigned int eax, ebx, ecx, edx;

    kl_cpuid(op, &eax, &ebx, &ecx, &edx);

    return eax;
}

static inline unsigned int kl_cpuid_ebx(unsigned int op)
{
    unsigned int eax, ebx, ecx, edx;

    kl_cpuid(op, &eax, &ebx, &ecx, &edx);

    return ebx;
}

static inline unsigned int kl_cpuid_ecx(unsigned int op)
{
    unsigned int eax, ebx, ecx, edx;

    kl_cpuid(op, &eax, &ebx, &ecx, &edx);

    return ecx;
}

static inline unsigned int kl_cpuid_edx(unsigned int op)
{
    unsigned int eax, ebx, ecx, edx;

    kl_cpuid(op, &eax, &ebx, &ecx, &edx);

    return edx;
}

// 参考https://elixir.bootlin.com/linux/v6.0-rc7/source/arch/x86/include/asm/processor.h#L216
int _kl_is_hygon_x86(void)
{
    unsigned int cpuid_level;
    char         x86_vendor_id[16] = { 0 };

    /* Get vendor name */
    kl_cpuid(0x00000000, (unsigned int *)&cpuid_level, (unsigned int *)&x86_vendor_id[0],
             (unsigned int *)&x86_vendor_id[8], (unsigned int *)&x86_vendor_id[4]);

    LOGD("x86_vendor_id= %.12s\n", x86_vendor_id);
    if (strncmp(x86_vendor_id, "HygonGenuine", 12) == 0) {
        return 1;
    }
    return 0;
}

#define kl_is_hygon_x86() _kl_is_hygon_x86()

#elif defined(CONFIG_ARM64)

// Main ID Register
#define CP_MIDR "midr_el1"
#define CP_MIDR_IMPLEMENTER_PHYTIUM 0x70

#define CP_READ_REGISTER(reg)                                                                      \
    ({                                                                                             \
        u32 __res;                                                                                 \
                                                                                                   \
        asm volatile("mrs %0, " reg "\r\t" : "=r"(__res));                                         \
                                                                                                   \
        __res;                                                                                     \
    })

#define CP_READ_MIDR_REGISTER() CP_READ_REGISTER(CP_MIDR)

// 参考https://github.com/NVIDIA/open-gpu-kernel-modules/blob/4397463e738d2d90aa1164cc5948e723701f7b53/src/nvidia/src/kernel/platform/cpu.c
int _kl_is_phytium_arm64(void)
{
    u32 val;
    u32 impl;
    u32 part;

    // Retrieve Main ID register
    val  = CP_READ_MIDR_REGISTER();
    impl = (val >> 24) & 0xff;
    part = (val >> 4) & 0xfff;

    if (impl == CP_MIDR_IMPLEMENTER_PHYTIUM) {
        return 1;
    }
    return 0;
}

#define kl_is_phytium_arm64() _kl_is_phytium_arm64()

#endif

#ifndef kl_is_hygon_x86
#define kl_is_hygon_x86() 0
#endif

#ifndef kl_is_phytium_arm64
#define kl_is_phytium_arm64() 0
#endif

int kl_add_tiny_mem_read_after_d2h_war(void)
{
    return kl_is_hygon_x86() || kl_is_phytium_arm64();
}

int kl_init_war(void)
{
    g_kl_wars.add_tiny_mem_read_after_d2h_war = kl_add_tiny_mem_read_after_d2h_war();
    if (g_kl_wars.add_tiny_mem_read_after_d2h_war) {
        LOGI("add_tiny_mem_read_after_d2h_war= %d\n", g_kl_wars.add_tiny_mem_read_after_d2h_war);
    }

    return 0;
}
