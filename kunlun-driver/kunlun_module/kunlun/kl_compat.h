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

#ifndef BAIDU_XPU_RUNTIME_MODULE_COMPAT_T
#define BAIDU_XPU_RUNTIME_MODULE_COMPAT_T

#include <linux/pci.h>

void compat_pcibios_resource_to_bus(struct pci_bus *bus, struct pci_bus_region *region,
                                    struct resource *res);

void compat_pcibios_bus_to_resource(struct pci_bus *bus, struct resource *res,
                                    struct pci_bus_region *region);

#include <linux/list.h>

/**
 * list_is_first -- tests whether @list is the first entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int compat_list_is_first(const struct list_head *list, const struct list_head *head)
{
    return list->prev == head;
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int compat_list_is_last(const struct list_head *list, const struct list_head *head)
{
    return list->next == head;
}

#include <uapi/linux/types.h>
#include <linux/compiler.h>

static __always_inline void compat_data_access_exceeds_word_size(void)
#ifdef __compiletime_warning
        __compiletime_warning("data access exceeds word size and won't be atomic")
#endif
                ;

static __always_inline void compat_data_access_exceeds_word_size(void)
{
}

static __always_inline void __compat_read_once_size(const volatile void *p, void *res, int size)
{
    switch (size) {
    case 1:
        *(__u8 *)res = *(volatile __u8 *)p;
        break;
    case 2:
        *(__u16 *)res = *(volatile __u16 *)p;
        break;
    case 4:
        *(__u32 *)res = *(volatile __u32 *)p;
        break;
#ifdef CONFIG_64BIT
    case 8:
        *(__u64 *)res = *(volatile __u64 *)p;
        break;
#endif
    default:
        barrier();
        __builtin_memcpy((void *)res, (const void *)p, size);
        compat_data_access_exceeds_word_size();
        barrier();
    }
}

static __always_inline void __compat_write_once_size(volatile void *p, void *res, int size)
{
    switch (size) {
    case 1:
        *(volatile __u8 *)p = *(__u8 *)res;
        break;
    case 2:
        *(volatile __u16 *)p = *(__u16 *)res;
        break;
    case 4:
        *(volatile __u32 *)p = *(__u32 *)res;
        break;
#ifdef CONFIG_64BIT
    case 8:
        *(volatile __u64 *)p = *(__u64 *)res;
        break;
#endif
    default:
        barrier();
        __builtin_memcpy((void *)p, (const void *)res, size);
        compat_data_access_exceeds_word_size();
        barrier();
    }
}

/*
 * Prevent the compiler from merging or refetching reads or writes. The
 * compiler is also forbidden from reordering successive instances of
 * READ_ONCE, WRITE_ONCE and ACCESS_ONCE (see below), but only when the
 * compiler is aware of some particular ordering.  One way to make the
 * compiler aware of ordering is to put the two invocations of READ_ONCE,
 * WRITE_ONCE or ACCESS_ONCE() in different C statements.
 *
 * In contrast to ACCESS_ONCE these two macros will also work on aggregate
 * data types like structs or unions. If the size of the accessed data
 * type exceeds the word size of the machine (e.g., 32 bits or 64 bits)
 * READ_ONCE() and WRITE_ONCE()  will fall back to memcpy and print a
 * compile-time warning.
 *
 * Their two major use cases are: (1) Mediating communication between
 * process-level code and irq/NMI handlers, all running on the same CPU,
 * and (2) Ensuring that the compiler does not  fold, spindle, or otherwise
 * mutilate accesses that either do not require ordering or that interact
 * with an explicit memory barrier or atomic instruction that provides the
 * required ordering.
 */

#define COMPAT_READ_ONCE(x)                                                                        \
    ({                                                                                             \
        union {                                                                                    \
            typeof(x) __val;                                                                       \
            char      __c[1];                                                                      \
        } __u;                                                                                     \
        __compat_read_once_size(&(x), __u.__c, sizeof(x));                                         \
        __u.__val;                                                                                 \
    })

#define COMPAT_WRITE_ONCE(x, val)                                                                  \
    ({                                                                                             \
        typeof(x) __val = (val);                                                                   \
        __compat_write_once_size(&(x), &__val, sizeof(__val));                                     \
        __val;                                                                                     \
    })

// 从NV驱动移植，以解决不同内核版本间的编译差异，后期完成NV字符串替换
#include "port/nv-linux.h"
#include "port/nv-procfs.h"

#endif
