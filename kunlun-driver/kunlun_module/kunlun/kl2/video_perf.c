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

#ifdef ENABLE_CODEC
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/bitmap.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif
#include "kl2/kl2.h"

#define MS_TO_KTIME(n) (ktime_set(0, n * 1000 * 1000))
#define SAMPLING_INTERVAL (MS_TO_KTIME(10))

#define DEC_SAMPLING_COUNT (100)
#define ENC_SAMPLING_COUNT (100)
#define PROC_SAMPLING_COUNT (100)

static u8 g_dec_samples[DEC_SAMPLING_COUNT];
u32       g_dec_samples_count = DEC_SAMPLING_COUNT;
module_param_array(g_dec_samples, byte, &g_dec_samples_count, S_IRUSR);

static u8 g_enc_samples[ENC_SAMPLING_COUNT];
u32       g_enc_samples_count = ENC_SAMPLING_COUNT;
module_param_array(g_enc_samples, byte, &g_enc_samples_count, S_IRUSR);

static u8 g_proc_samples[PROC_SAMPLING_COUNT];
u32       g_proc_samples_count = PROC_SAMPLING_COUNT;
module_param_array(g_proc_samples, byte, &g_proc_samples_count, S_IRUSR);

void kl2_get_video_ratio(struct kl2_device *kl2_dev, u64 *ratio)
{
    vdecdev_t    *video_dec  = kl2_dev->video_dec;
    enc_dev_t    *video_enc  = kl2_dev->video_enc;
    imgprocdev_t *image_proc = kl2_dev->image_proc;
    u64           sum        = 0;
    int           i          = 0;

    sum = 0;
    for (i = 0; i < DEC_SAMPLING_COUNT; i++) {
        sum += video_dec->video_perf->samples[i];
    }
    ratio[0] = 100 * sum / (video_dec->cores_num * DEC_SAMPLING_COUNT);

    sum = 0;
    for (i = 0; i < ENC_SAMPLING_COUNT; i++) {
        sum += video_enc->video_perf->samples[i];
    }
    ratio[1] = 100 * sum / (video_enc->total_subsys_num * ENC_SAMPLING_COUNT);

    sum = 0;
    for (i = 0; i < PROC_SAMPLING_COUNT; i++) {
        sum += image_proc->video_perf->samples[i];
    }
    ratio[2] = 100 * sum / (image_proc->cores_num * PROC_SAMPLING_COUNT);

    return;
}

void kl2_get_video_frame_rate(struct kl2_device *kl2_dev, u64 *frame_rate)
{
    vdecdev_t    *video_dec   = kl2_dev->video_dec;
    enc_dev_t    *video_enc   = kl2_dev->video_enc;
    imgprocdev_t *image_proc  = kl2_dev->image_proc;
    int           i           = 0;
    int           time_counts = 0;
    int           total       = 0;

    // get decoding fps
    time_counts = video_dec->video_perf->time_counts;
    time_counts = time_counts > FPS_SAMPLING_COUNT ? FPS_SAMPLING_COUNT : time_counts;
    if (ktime_to_ms(SAMPLING_INTERVAL) > 0 && time_counts > 0) {
        total = 0;
        for (i = 0; i < FPS_SAMPLING_COUNT; i++) {
            total += video_dec->video_perf->fps_samples[i];
        }
        frame_rate[0] = (1000 * total / ktime_to_ms(SAMPLING_INTERVAL)) / time_counts;
    } else {
        frame_rate[0] = 0;
    }

    // get encoding fps
    time_counts = video_enc->video_perf->time_counts;
    time_counts = time_counts > FPS_SAMPLING_COUNT ? FPS_SAMPLING_COUNT : time_counts;
    if (ktime_to_ms(SAMPLING_INTERVAL) > 0 && time_counts > 0) {
        total = 0;
        for (i = 0; i < FPS_SAMPLING_COUNT; i++) {
            total += video_enc->video_perf->fps_samples[i];
        }
        frame_rate[1] = (1000 * total / ktime_to_ms(SAMPLING_INTERVAL)) / time_counts;
    } else {
        frame_rate[1] = 0;
    }

    // get image proc fps
    time_counts = image_proc->video_perf->time_counts;
    time_counts = time_counts > FPS_SAMPLING_COUNT ? FPS_SAMPLING_COUNT : time_counts;
    if (ktime_to_ms(SAMPLING_INTERVAL) > 0 && time_counts > 0) {
        total = 0;
        for (i = 0; i < FPS_SAMPLING_COUNT; i++) {
            total += image_proc->video_perf->fps_samples[i];
        }
        frame_rate[2] = (1000 * total / ktime_to_ms(SAMPLING_INTERVAL)) / time_counts;
    } else {
        frame_rate[2] = 0;
    }

    return;
}

void kl2_get_video_clock(struct kl2_device *kl2_dev, u64 *clock_freq)
{
    int i;

    clock_freq[0] = kl2_dev->dev_info.decoder_freq;
    clock_freq[1] = kl2_dev->dev_info.encoder_freq;
    clock_freq[2] = kl2_dev->dev_info.image_proc_freq;

    for (i = 0; i < 3; i++) {
        if (clock_freq[i] > 0) {
            clock_freq[i] = 4000 / clock_freq[i];
        } else {
            clock_freq[i] = 0;
        }
    }

    return;
}

static int core_is_runing(video_perf_t *video_perf)
{
    int i = 0;

    for (i = 0; i < DEC_SAMPLING_COUNT; i++) {
        if (video_perf->samples[i] > 0) {
            //LOGW("samples[%d]=%u\n", i, video_perf->samples[i]);
            return 1;
        }
    }
    return 0;
}

static enum hrtimer_restart do_video_perf(struct hrtimer *htimer)
{
    struct kl2_device *kl2_dev    = container_of(htimer, struct kl2_device, video_perf_hrtimer);
    vdecdev_t         *video_dec  = kl2_dev->video_dec;
    enc_dev_t         *video_enc  = kl2_dev->video_enc;
    imgprocdev_t      *image_proc = kl2_dev->image_proc;
    video_perf_t      *video_perf = NULL;
    u32                curr       = 0;
    u32                last       = 0;

    // statistics of video decoding usage ratio
    video_perf                                    = video_dec->video_perf;
    video_perf->samples[video_perf->sample_index] = atomic_read(&video_perf->core_in_used);
    g_dec_samples[video_perf->sample_index]       = video_perf->samples[video_perf->sample_index];
    ++video_perf->sample_index;
    video_perf->sample_index %= DEC_SAMPLING_COUNT;

    //statistics of video decoding frame rate
    if (core_is_runing(video_perf) == 1) {
        video_perf->time_counts++;
    } else {
        video_perf->time_counts = 0;
        atomic_set(&video_perf->frames_num, 0);
    }
    if ((atomic_read(&video_perf->frames_num) >= (INT_MAX / (2 * 1000)) ||
         video_perf->time_counts > INT_MAX)) {
        atomic_set(&video_perf->frames_num, atomic_read(&video_perf->frames_num) / 2);
        video_perf->time_counts = video_perf->time_counts / 2;
    }
    curr = atomic_read(&video_perf->frames_num);
    last = atomic_read(&video_perf->last_frames_num);
    video_perf->fps_samples[video_perf->fps_sample_index] = curr > last ? (curr - last) : 0;
    ++video_perf->fps_sample_index;
    video_perf->fps_sample_index %= FPS_SAMPLING_COUNT;
    atomic_set(&video_perf->last_frames_num, curr);

    // statistics of video encoding usage ratio
    video_perf                                    = video_enc->video_perf;
    video_perf->samples[video_perf->sample_index] = atomic_read(&video_perf->core_in_used);
    g_enc_samples[video_perf->sample_index]       = video_perf->samples[video_perf->sample_index];
    ++video_perf->sample_index;
    video_perf->sample_index %= ENC_SAMPLING_COUNT;

    //statistics of video encoding frame rate
    if (core_is_runing(video_perf) == 1) {
        video_perf->time_counts++;
    } else {
        video_perf->time_counts = 0;
        atomic_set(&video_perf->frames_num, 0);
    }
    if ((atomic_read(&video_perf->frames_num) >= (INT_MAX / (2 * 1000)) ||
         video_perf->time_counts > INT_MAX)) {
        atomic_set(&video_perf->frames_num, atomic_read(&video_perf->frames_num) / 2);
        video_perf->time_counts = video_perf->time_counts / 2;
    }
    curr = atomic_read(&video_perf->frames_num);
    last = atomic_read(&video_perf->last_frames_num);
    video_perf->fps_samples[video_perf->fps_sample_index] = curr > last ? (curr - last) : 0;
    ++video_perf->fps_sample_index;
    video_perf->fps_sample_index %= FPS_SAMPLING_COUNT;
    atomic_set(&video_perf->last_frames_num, curr);

    //statistics of image proc usage ratio
    video_perf                                    = image_proc->video_perf;
    video_perf->samples[video_perf->sample_index] = atomic_read(&video_perf->core_in_used);
    g_proc_samples[video_perf->sample_index]      = video_perf->samples[video_perf->sample_index];
    ++video_perf->sample_index;
    video_perf->sample_index %= PROC_SAMPLING_COUNT;

    //statistics of image proc frame rate
    if (core_is_runing(video_perf) == 1) {
        video_perf->time_counts++;
    } else {
        video_perf->time_counts = 0;
        atomic_set(&video_perf->frames_num, 0);
    }
    if ((atomic_read(&video_perf->frames_num) >= (INT_MAX / (2 * 1000)) ||
         video_perf->time_counts > INT_MAX)) {
        atomic_set(&video_perf->frames_num, atomic_read(&video_perf->frames_num) / 2);
        video_perf->time_counts = video_perf->time_counts / 2;
    }
    curr = atomic_read(&video_perf->frames_num);
    last = atomic_read(&video_perf->last_frames_num);
    video_perf->fps_samples[video_perf->fps_sample_index] = curr > last ? (curr - last) : 0;
    ++video_perf->fps_sample_index;
    video_perf->fps_sample_index %= FPS_SAMPLING_COUNT;
    atomic_set(&video_perf->last_frames_num, curr);

    hrtimer_forward_now(htimer, SAMPLING_INTERVAL);

    return HRTIMER_RESTART;
}

int kl2_video_perf_init(struct kl2_device *kl2_dev)
{
    vdecdev_t    *video_dec  = kl2_dev->video_dec;
    enc_dev_t    *video_enc  = kl2_dev->video_enc;
    imgprocdev_t *image_proc = kl2_dev->image_proc;
    size_t        size       = 0;

    if (!video_dec || !video_enc || !image_proc) {
        LOGE("need to alloc video_dec/video_enc/image_proc memory\n");
        return -XPUERR_DEVINIT;
    }

    size = sizeof(video_perf_t) + DEC_SAMPLING_COUNT * sizeof(video_dec->video_perf->samples[0]);
    video_dec->video_perf = kzalloc(size, GFP_KERNEL);
    if (video_dec->video_perf == NULL) {
        goto dec_alloc_mem_err;
    }
    atomic_set(&video_dec->video_perf->core_in_used, 0);
    video_dec->video_perf->time_counts = 0;
    atomic_set(&video_dec->video_perf->frames_num, 0);
    atomic_set(&video_dec->video_perf->last_frames_num, 0);

    size = sizeof(video_perf_t) + ENC_SAMPLING_COUNT * sizeof(video_enc->video_perf->samples[0]);
    video_enc->video_perf = kzalloc(size, GFP_KERNEL);
    if (video_enc->video_perf == NULL) {
        goto enc_alloc_mem_err;
    }
    atomic_set(&video_enc->video_perf->core_in_used, 0);
    video_enc->video_perf->time_counts = 0;
    atomic_set(&video_enc->video_perf->frames_num, 0);
    atomic_set(&video_enc->video_perf->last_frames_num, 0);

    size = sizeof(video_perf_t) + PROC_SAMPLING_COUNT * sizeof(image_proc->video_perf->samples[0]);
    image_proc->video_perf = kzalloc(size, GFP_KERNEL);
    if (image_proc->video_perf == NULL) {
        goto proc_alloc_mem_err;
    }
    atomic_set(&image_proc->video_perf->core_in_used, 0);
    image_proc->video_perf->time_counts = 0;
    atomic_set(&image_proc->video_perf->frames_num, 0);
    atomic_set(&image_proc->video_perf->last_frames_num, 0);

    hrtimer_init(&kl2_dev->video_perf_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    kl2_dev->video_perf_hrtimer.function = do_video_perf;
    hrtimer_start(&kl2_dev->video_perf_hrtimer, SAMPLING_INTERVAL, HRTIMER_MODE_REL);

    return 0;
proc_alloc_mem_err:
    kfree(video_enc->video_perf);
enc_alloc_mem_err:
    kfree(video_dec->video_perf);
dec_alloc_mem_err:
    return -XPUERR_DEVINIT;
}

void kl2_video_perf_uninit(struct kl2_device *kl2_dev)
{
    vdecdev_t    *video_dec  = kl2_dev->video_dec;
    enc_dev_t    *video_enc  = kl2_dev->video_enc;
    imgprocdev_t *image_proc = kl2_dev->image_proc;

    hrtimer_cancel(&kl2_dev->video_perf_hrtimer);
    kfree(image_proc->video_perf);
    kfree(video_enc->video_perf);
    kfree(video_dec->video_perf);

    return;
}
#endif
