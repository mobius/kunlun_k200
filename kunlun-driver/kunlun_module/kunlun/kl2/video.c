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

#include "kl2/kl2.h"

int kl2_video_init(struct kl2_device *kl2_dev)
{
    int err = 0;

    kl2_dev->video_dec = vdec_init(kl2_dev);
    if (kl2_dev->video_dec == NULL) {
        KL2_LOGE("fail to initialize the video decoder\n");
        goto err_out;
    }

    kl2_dev->video_enc = venc_init(kl2_dev);
    if (kl2_dev->video_enc == NULL) {
        KL2_LOGE("fail to initialize the video encoder\n");
        goto err_dec;
    }

    kl2_dev->image_proc = imgproc_init(kl2_dev);
    if (kl2_dev->image_proc == NULL) {
        KL2_LOGE("fail to initialize the imgproc\n");
        goto err_enc;
    }
    err = kl2_video_perf_init(kl2_dev);
    if (err != 0) {
        LOGE("fail to initialize the video perf\n");
        goto err_perf;
    }
    return 0;
err_perf:
    imgproc_uninit(kl2_dev->image_proc);
err_enc:
    venc_uninit(kl2_dev->video_enc);
err_dec:
    vdec_uninit(kl2_dev->video_dec);
err_out:
    return -XPUERR_DEVINIT;
}

void kl2_video_destroy(struct kl2_device *kl2_dev)
{
    kl2_video_perf_uninit(kl2_dev);

    if (kl2_dev->video_dec != NULL) {
        vdec_uninit(kl2_dev->video_dec);
    }

    if (kl2_dev->video_enc != NULL) {
        venc_uninit(kl2_dev->video_enc);
    }

    if (kl2_dev->image_proc != NULL) {
        imgproc_uninit(kl2_dev->image_proc);
    }
}

#endif
