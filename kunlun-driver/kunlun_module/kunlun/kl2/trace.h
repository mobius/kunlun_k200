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

// clang-format off
#if !defined(_KL2_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _KL2_TRACE_H_

#undef TRACE_SYSTEM
#define TRACE_SYSTEM kunlun2

#include <linux/tracepoint.h>
#include "xpurt_priv/defs_private.h"

struct kl_device;
struct kl2_device;
struct kl2_session;
struct kl2_event;

DECLARE_EVENT_CLASS(clock_sync_template,

     TP_PROTO(int bid, u64 dev_ts),

     TP_ARGS(bid, dev_ts),

     TP_STRUCT__entry(
             __field(    int,          bid                                 )
             __field(    u64,          dev_ts                              )
             __field(    u64,          sys_ts                              )
     ),

     TP_fast_assign(
             __entry->bid             = bid;
             __entry->dev_ts          = dev_ts;
             __entry->sys_ts          = ktime_to_ns(ktime_get());
     ),

     TP_printk("%d %llu %llu", __entry->bid, __entry->dev_ts, __entry->sys_ts)
);

DECLARE_EVENT_CLASS(stream_template,

     TP_PROTO(struct kl2_session *sess),

     TP_ARGS(sess),

     TP_STRUCT__entry(
             __field(    pid_t,        pid                                 )
             __field(    pid_t,        tid                                 )
             __field(    int,          bid                                 )
             __field(    int,          channel_id                          )
             __field(    int,          stream_id                           )
     ),

     TP_fast_assign(
             __entry->pid             = task_tgid_nr(current);
             __entry->tid             = task_pid_nr(current);
             __entry->bid             = sess->kl2_dev->kdev->idx;
             __entry->channel_id      = (sess->hwq) ? sess->hwq->id : -1;
             __entry->stream_id       = sess->id;
     ),

     TP_printk("%d %d %d %d %d",
               __entry->pid, __entry->tid, __entry->bid, __entry->channel_id, __entry->stream_id)
);

DECLARE_EVENT_CLASS(memcpy_template,
     TP_PROTO(struct XPUMemcpyExIoctlArgs *args, int bid, int dbid, u32 dir),
     TP_ARGS(args, bid, dbid, dir),

     TP_STRUCT__entry(
             __field(    pid_t,        pid                                 )
             __field(    pid_t,        tid                                 )
             __field(    int,          bid                                 )
             __field(    int,          dbid                                )
             __field(    u64,          dest                                )
             __field(    u64,          src                                 )
             __field(    u64,          size                                )
             __field(    u32,          dir                                 )
     ),

     TP_fast_assign(
             __entry->pid             = task_tgid_nr(current);
             __entry->tid             = task_pid_nr(current);
             __entry->bid             = bid;
             __entry->dbid            = dbid;
             __entry->dest            = args->dest;
             __entry->src             = args->src;
             __entry->size            = args->size;
             __entry->dir             = dir;
     ),

     TP_printk("%d %d %d %d %llu %llu %llu %u",
               __entry->pid, __entry->tid, __entry->bid, __entry->dbid,
               __entry->dest, __entry->src, __entry->size, __entry->dir)
);

DECLARE_EVENT_CLASS(async_template,

     TP_PROTO(struct kl2_session *sess,
              struct XPULaunchIoctlArgs *args,
              u32 token),

     TP_ARGS(sess, args, token),

     TP_STRUCT__entry(
             __field(    int,        type                             )
             __field(    pid_t,      pid                              )
             __field(    pid_t,      tid                              )
             __field(    int,        bid                              )
             __field(    int,        channel_id                       )
             __field(    int,        stream_id                        )
             __field(    u32,        token                            )
             __field(    u32,        nclusters                        )
             __field(    u32,        ncores                           )
             __field(    u64,        param_addr                       )
             __field(    u64,        code_addr                        )
             __array(    char,       name,              XPU_MAX_STRLEN)
     ),

     TP_fast_assign(
             __entry->type            = args->kernel.type;
             __entry->pid             = task_tgid_nr(current);
             __entry->tid             = task_pid_nr(current);
             __entry->bid             = sess->kl2_dev->kdev->idx;
             __entry->channel_id      = (sess->hwq) ? sess->hwq->id : -1;
             __entry->stream_id       = sess->id;
             __entry->token           = token;
             __entry->nclusters       = args->nclusters;
             __entry->ncores          = args->ncores;
             __entry->param_addr      = args->param_addr;
             __entry->code_addr       = args->kernel.code_addr;
             strncpy(__entry->name, args->name, XPU_MAX_STRLEN);
             __entry->name[XPU_MAX_STRLEN - 1] = '\0';
     ),

     TP_printk("%d %d %d %d %d %d %u %u %u %llu %llu %s",
               __entry->type, __entry->pid, __entry->tid, __entry->bid, __entry->channel_id,
               __entry->stream_id, __entry->token, __entry->nclusters, __entry->ncores,
               __entry->param_addr, __entry->code_addr, __entry->name)
);

DECLARE_EVENT_CLASS(event_async_template,

     TP_PROTO(struct kl2_session *sess,
              struct kl2_event *evnt,
              u32 token),

     TP_ARGS(sess, evnt, token),

     TP_STRUCT__entry(
             __field(    pid_t,      pid                              )
             __field(    pid_t,      tid                              )
             __field(    int,        bid                              )
             __field(    int,        channel_id                       )
             __field(    int,        stream_id                        )
             __field(    u32,        token                            )
             __field(    int,        event_id                         )
             __field(    u64,        event_rec_seq                    )
     ),

     TP_fast_assign(
             __entry->pid             = task_tgid_nr(current);
             __entry->tid             = task_pid_nr(current);
             __entry->bid             = sess->kl2_dev->kdev->idx;
             __entry->channel_id      = (sess->hwq) ? sess->hwq->id : -1;
             __entry->stream_id       = sess->id;
             __entry->token           = token;
             __entry->event_id        = evnt->id;
             __entry->event_rec_seq   = evnt->rec_hwq_evnt_seq;
     ),

     TP_printk("%d %d %d %d %d %u %d %llu",
               __entry->pid, __entry->tid, __entry->bid, __entry->channel_id,
               __entry->stream_id, __entry->token, __entry->event_id, __entry->event_rec_seq)
);

DEFINE_EVENT(async_template, kernel_launch,
             TP_PROTO(struct kl2_session *sess, struct XPULaunchIoctlArgs *args, u32 token),
             TP_ARGS(sess, args, token));

DEFINE_EVENT(event_async_template, event_record,
             TP_PROTO(struct kl2_session *sess, struct kl2_event *evnt, u32 token),
             TP_ARGS(sess, evnt, token));

DEFINE_EVENT(event_async_template, event_stream_wait,
             TP_PROTO(struct kl2_session *sess, struct kl2_event *evnt, u32 token),
             TP_ARGS(sess, evnt, token));

DEFINE_EVENT(event_async_template, event_wait,
             TP_PROTO(struct kl2_session *sess, struct kl2_event *evnt, u32 token),
             TP_ARGS(sess, evnt, token));

DEFINE_EVENT(stream_template, xpu_wait,
             TP_PROTO(struct kl2_session *sess),
             TP_ARGS(sess));

DEFINE_EVENT(memcpy_template, xpu_memcpy,
             TP_PROTO(struct XPUMemcpyExIoctlArgs *args, int bid, int dbid, u32 dir),
             TP_ARGS(args, bid, dbid, dir));

DEFINE_EVENT(clock_sync_template, xpu_clock_sync,
             TP_PROTO(int bid, u64 dev_ts),
             TP_ARGS(bid, dev_ts));


#endif /* _KL2_TRACE_H_ */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH kl2
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace
#include <trace/define_trace.h>
// clang-format on
