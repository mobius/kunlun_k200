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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_VIDEO_PERF_H
#define BAIDU_XPU_RUNTIME_MODULE_KL2_VIDEO_PERF_H

#define FPS_SAMPLING_COUNT (300)

typedef struct video_perf {
    atomic_t core_in_used;
    u32      sample_index;
    u32      fps_sample_index;
    u32      time_counts;
    atomic_t frames_num;
    atomic_t last_frames_num;
    u32      fps_samples[FPS_SAMPLING_COUNT];
    u8       samples[0];
} video_perf_t;

struct kl2_device;
int  kl2_video_perf_init(struct kl2_device *kl2_dev);
void kl2_video_perf_uninit(struct kl2_device *kl2_dev);
void kl2_get_video_ratio(struct kl2_device *kl2_dev, u64 *ratio);
void kl2_get_video_frame_rate(struct kl2_device *kl2_dev, u64 *frame_rate);
void kl2_get_video_clock(struct kl2_device *kl2_dev, u64 *clock_freq);

#endif
