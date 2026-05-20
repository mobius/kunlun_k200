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

// Copyright 2018 Baidu Inc. All Rights Reserved.
// authors: Han Jinchen hanjinche@baidu.com
//
// kl_util.h - Kunlun driver utilities
//
#ifndef BAIDU_XPU_RUNTIME_MODULE_KL_UTIL_H
#define BAIDU_XPU_RUNTIME_MODULE_KL_UTIL_H

#include <linux/bitops.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/delay.h>

static inline u32 low32(u64 v)
{
    return (v & 0xffffffff);
}
static inline u32 high32(u64 v)
{
    return ((v >> 32) & 0xffffffff);
}
static inline u64 makeu64(u32 hi, u32 lo)
{
    return (((u64)hi) << 32) | lo;
}

static inline int bitcount(u32 v)
{
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

#include <linux/printk.h>

// 解注释下一行，打开DEBUG打印
//#define DEBUG

#define KL_CUT_HERE "------------[ cut here  ]------------\n"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define LOGE(fmt, arg...)                                                                          \
    do {                                                                                           \
        pr_err("[ERR ] " fmt, ##arg);                                                              \
    } while (0)

#define LOGW(fmt, arg...)                                                                          \
    do {                                                                                           \
        pr_warn("[WARN] " fmt, ##arg);                                                             \
    } while (0)

#define LOGI(fmt, arg...)                                                                          \
    do {                                                                                           \
        pr_info("[INFO] " fmt, ##arg);                                                             \
    } while (0)

#ifdef DEBUG
#define LOGD(fmt, arg...)                                                                          \
    do {                                                                                           \
        pr_info("[DBG ] " fmt, ##arg);                                                             \
    } while (0)
#else
#define LOGD(fmt, arg...)                                                                          \
    do {                                                                                           \
    } while (0)
#endif

#define LOG_ONCE(fmt, arg...)                                                                      \
    do {                                                                                           \
        pr_info_once("[ONCE] " fmt, ##arg);                                                        \
    } while (0)

#define KL2_LOGE(fmt, arg...)                                                                      \
    do {                                                                                           \
        pr_err("%s: [ERR ] " fmt, kl2_dev->kdev->name, ##arg);                                     \
    } while (0)

#define KL2_LOGW(fmt, arg...)                                                                      \
    do {                                                                                           \
        pr_warn("%s: [WARN] " fmt, kl2_dev->kdev->name, ##arg);                                    \
    } while (0)

#define KL2_LOGI(fmt, arg...)                                                                      \
    do {                                                                                           \
        pr_info("%s: [INFO] " fmt, kl2_dev->kdev->name, ##arg);                                    \
    } while (0)

#ifdef DEBUG
#define KL2_LOGD(fmt, arg...)                                                                      \
    do {                                                                                           \
        pr_info("%s: [DBG ] " fmt, kl2_dev->kdev->name, ##arg);                                    \
    } while (0)
#else
#define KL2_LOGD(fmt, arg...)                                                                      \
    do {                                                                                           \
    } while (0)
#endif

#define KL2_LOGE_RATELIMITED(fmt, arg...)                                                          \
    do {                                                                                           \
        pr_err_ratelimited("%s: [ERR ] " fmt, kl2_dev->kdev->name, ##arg);                         \
    } while (0)

#define KL2_LOGW_RATELIMITED(fmt, arg...)                                                          \
    do {                                                                                           \
        pr_warn_ratelimited("%s: [WARN] " fmt, kl2_dev->kdev->name, ##arg);                        \
    } while (0)

#define KL2_LOGI_RATELIMITED(fmt, arg...)                                                          \
    do {                                                                                           \
        pr_info_ratelimited("%s: [INFO] " fmt, kl2_dev->kdev->name, ##arg);                        \
    } while (0)

#define KL2_LOG_ONCE(fmt, arg...)                                                                  \
    do {                                                                                           \
        pr_info_once("%s: [ONCE] " fmt, kl2_dev->kdev->name, ##arg);                               \
    } while (0)

#define KL2_LOG_XID(Xid, fmt, arg...)                                                              \
    do {                                                                                           \
        pr_warn("%s: [Xid %d] " fmt, kl2_dev->kdev->name, Xid, ##arg);                             \
    } while (0)

// TODO(miaotianxiang): deprecated
#ifdef DEBUG_XEVENT
#define LOG_EVENT(fmt, arg...)                                                                     \
    do {                                                                                           \
        pr_info("[DBG][EVENT] " fmt, ##arg);                                                       \
    } while (0)
#else
#define LOG_EVENT(fmt, arg...)                                                                     \
    do {                                                                                           \
    } while (0)
#endif

#ifdef DEBUG_XCG
#define LOG_XCG(fmt, arg...)                                                                       \
    do {                                                                                           \
        pr_info("[DBG][XCG] " fmt, ##arg);                                                         \
    } while (0)
#else
#define LOG_XCG(fmt, arg...)                                                                       \
    do {                                                                                           \
    } while (0)
#endif

#define LOG_LEVEL_DRIVER 1
#define LOG_LEVEL_DEVICE 2
#define LOG_LEVEL_SESS 3
#define LOG_LEVEL_KERNEL 4
#define LOG_LEVEL_REG 5
#define LOG_LEVEL_PERF 6

#define LOGL(level, fmt, arg...)                                                                   \
    do {                                                                                           \
        pr_info("[DBGL" #level "] " fmt, ##arg);                                                   \
    } while (0)

#define LOGL1(fmt, arg...)
#define LOGL2(fmt, arg...)
#define LOGL3(fmt, arg...)
#define LOGL4(fmt, arg...)
#define LOGL5(fmt, arg...)
#define LOGL6(fmt, arg...)

#if defined(LOG_LEVEL)

#if (1 <= LOG_LEVEL)
#undef LOGL1
#define LOGL1(fmt, arg...) LOGL(1, fmt, ##arg)
#endif

#if (2 <= LOG_LEVEL)
#undef LOGL2
#define LOGL2(fmt, arg...) LOGL(2, fmt, ##arg)
#endif

#if (3 <= LOG_LEVEL)
#undef LOGL3
#define LOGL3(fmt, arg...) LOGL(3, fmt, ##arg)
#endif

#if (4 <= LOG_LEVEL)
#undef LOGL4
#define LOGL4(fmt, arg...) LOGL(4, fmt, ##arg)
#endif

#if (5 <= LOG_LEVEL)
#undef LOGL5
#define LOGL5(fmt, arg...) LOGL(5, fmt, ##arg)
#endif

#if (6 <= LOG_LEVEL)
#undef LOGL6
#define LOGL6(fmt, arg...) LOGL(6, fmt, ##arg)
#endif

#endif
// TODO(miaotianxiang): end deprecated

#define CHECK_NULL_RETURN(ptr)                                                                     \
    do {                                                                                           \
        if (unlikely((ptr) == NULL)) {                                                             \
            LOGE("Null ptr\n");                                                                    \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#define CHECK_NULL_RET_VAL(ptr, retVal)                                                            \
    do {                                                                                           \
        if (unlikely((ptr) == NULL)) {                                                             \
            LOGE("Null ptr\n");                                                                    \
            return retVal;                                                                         \
        }                                                                                          \
    } while (0)

#define ASSERT_RET_VAL(b, retVal)                                                                  \
    do {                                                                                           \
        if (unlikely(!(b))) {                                                                      \
            LOGE("Assert fail\n");                                                                 \
            return retVal;                                                                         \
        }                                                                                          \
    } while (0)

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

// TODO(miaotianxiang): use min/max/min_t/max_t with type check
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

struct xref {
    struct kref kref;
    atomic_t    refcount;
};

static inline void xref_init(struct xref *xref)
{
    kref_init(&xref->kref);
    atomic_set(&xref->refcount, 1);
}

/*
 we could tolerate some inconsistency with kref, in case kref_read is not implemented
*/
static inline unsigned int xref_read(const struct xref *xref)
{
    return atomic_read(&xref->refcount);
}

static inline void xref_get(struct xref *xref)
{
    kref_get(&xref->kref);
    atomic_add(1, &xref->refcount);
}

static inline int xref_put(struct xref *xref, void (*release)(struct kref *kref))
{
    atomic_sub(1, &xref->refcount);
    return kref_put(&xref->kref, release);
}

/**
 * Modified from kernel function
 *
 * list_bulk_move_tail - move a subsection of a list to its tail
 * @head: the head that will follow our entry
 * @first: first entry to move
 * @last: last entry to move, can be the same as first
 *
 * Move all entries between @first and including @last before @head.
 * All three entries must belong to the same linked list.
 */
static inline void x_list_move_all_tail(struct list_head *head, struct list_head *src_list)
{
    struct list_head *first = src_list->next;
    struct list_head *last  = src_list->prev;

    if (src_list->next == src_list)
        return;

    first->prev->next = last->next;
    last->next->prev  = first->prev;

    head->prev->next = first;
    first->prev      = head->prev;

    last->next = head;
    head->prev = last;
}

#define USE_RATIO_PN 100

struct use_ratio {
    unsigned long bitmap[BITS_TO_LONGS(USE_RATIO_PN)];
    int           pos;
    int           weight;
};

static inline int ur_init(struct use_ratio *ur)
{
    bitmap_zero(ur->bitmap, USE_RATIO_PN);
    ur->pos    = 0;
    ur->weight = 0;

    return 0;
}

static inline int ur_record(struct use_ratio *ur, int is_set)
{
    int test;

    if (is_set)
        test = test_and_set_bit(ur->pos, ur->bitmap);
    else
        test = test_and_clear_bit(ur->pos, ur->bitmap);

    ur->pos = (ur->pos + 1) % USE_RATIO_PN;

    // 0 -> 1
    if (!test && is_set)
        return ++ur->weight;
    // 1 -> 0
    if (test && !is_set)
        return --ur->weight;
    // 0 -> 0 or 1 -> 1
    return ur->weight;
}

static inline int ur_weight(struct use_ratio *ur)
{
    return ur->weight;
};

#ifndef list_entry_is_head
#define list_entry_is_head(pos, head, member) (&pos->member == (head))
#endif

#ifndef list_next_entry
/**
 * list_next_entry - get the next element in list
 * @pos:    the type * to cursor
 * @member: the name of the list_head within the struct.
 */
#define list_next_entry(pos, member) list_entry((pos)->member.next, typeof(*(pos)), member)
#endif

#define kl_poll_cond_timeout(cond, sleep_us, timeout_us)                                           \
    ({                                                                                             \
        ktime_t __timeout;                                                                         \
        __timeout = ktime_add_us(ktime_get(), timeout_us);                                         \
        might_sleep_if(sleep_us);                                                                  \
        for (;;) {                                                                                 \
            if (cond)                                                                              \
                break;                                                                             \
            if (timeout_us && ktime_compare(ktime_get(), __timeout) > 0) {                         \
                break;                                                                             \
            }                                                                                      \
            if (sleep_us)                                                                          \
                usleep_range((sleep_us >> 2) + 1, sleep_us);                                       \
        }                                                                                          \
        (cond) ? 0 : -ETIMEDOUT;                                                                   \
    })

#endif /*BAIDU_XPU_RUNTIME_MODULE_KL_UTIL_H*/
