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

#ifndef BAIDU_XPU_RUNTIME_MODULE_PROFILER_H
#define BAIDU_XPU_RUNTIME_MODULE_PROFILER_H

#include <linux/timex.h>

enum ProfilerType {
    PROF_launch,
    PROF_launch_usercpy,
    PROF_launch_param_alloc,
    PROF_launch_param_dma,
    PROF_launch_param_dma_pure,
    PROF_launch_preparextask,
    PROF_launch_sesslaunch,
    PROF_launch_addcost,

    PROF_batchlaunch,
    PROF_batchlaunch_prepare,

    PROF_wait,
    PROF_wait_pure,
    PROF_wait_complete_delay,
    PROF_wait_sse_counter,

    PROF_sess_launch_task,
    PROF_sess_launch_task_enqueue,
    PROF_sess_launch_task_schedule,

    PROF_xtq_add_pending_task,

    PROF_sched_bind_sess,

    PROF_xtq_dispatch,
    PROF_sse_enqueue_task_locked,

    PROF_xtq_finish,

    PROF_isr,

    PROFILER_COUNT // keep this the last one
};

struct profiler_stat {
    char *name;
    u64   cost;
    u32   count;
};

#define DECLARE_PROFILER(key)                                                                      \
    unsigned long __xpurt_prflr_##key##_t0__ = 0;                                                  \
    unsigned long __xpurt_prflr_##key##_t1__ = 0

#define START_PROFILING(key) __xpurt_prflr_##key##_t0__ = get_cycles()

#define END_PROFILING(key, kdev)                                                                   \
    __xpurt_prflr_##key##_t1__ = get_cycles();                                                     \
    if ((kdev)->profiling_enabled) {                                                               \
        (kdev)->profiler[key].cost +=                                                              \
                ((__xpurt_prflr_##key##_t1__) - (__xpurt_prflr_##key##_t0__));                     \
        (kdev)->profiler[key].count += 1;                                                          \
        if ((kdev)->profiler[key].name == NULL)                                                    \
            (kdev)->profiler[key].name = #key;                                                     \
    }                                                                                              \
    ((void)0)

#define PROF_ADD_DATA(key, kdev, cycle)                                                            \
    if ((kdev)->profiling_enabled) {                                                               \
        (kdev)->profiler[key].cost += (cycle);                                                     \
        (kdev)->profiler[key].count += 1;                                                          \
        if ((kdev)->profiler[key].name == NULL)                                                    \
            (kdev)->profiler[key].name = #key;                                                     \
    }                                                                                              \
    ((void)0)

#endif
