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

#define __FILENAME__ "device_state.c"
#include <linux/spinlock.h>
#include "xpu_drv.h"

inline char *xpd_state_str(XPUPDState state)
{
    switch (state) {
    case XPDS_RUNNING:
        return "RUNNING";
    case XPDS_LOWPOWER:
        return "LOWPOWER";
    case XPDS_PAUSING:
        return "PAUSING";
    case XPDS_PAUSED:
        return "PAUSED";
    case XPDS_ERROR:
        return "ERROR";
    default:
        return "illegal";
    }
}

inline int xpd_state_to_running(struct xpu_pd *xpd)
{
    int           ret   = 0;
    unsigned long flags = 0;

    spin_lock_irqsave(&xpd->state_lock, flags);
    switch (xpd->state) {
    case XPDS_RUNNING:
    case XPDS_LOWPOWER:
    case XPDS_PAUSED:
    case XPDS_ERROR:
        LOGI("xpu%d: start running\n", xpd->devfile_id);
        xpd->state = XPDS_RUNNING;
        break;
    default:
        LOGE("xpu%d: try to enter RUNNING state from %s\n", xpd->devfile_id,
             xpd_state_str(xpd->state));
        ret = -EINVAL;
        break;
    }
    spin_unlock_irqrestore(&xpd->state_lock, flags);
    return ret;
}

inline int xpd_state_to_lowpower(struct xpu_pd *xpd)
{
    int           ret   = 0;
    unsigned long flags = 0;
    spin_lock_irqsave(&xpd->state_lock, flags);
    switch (xpd->state) {
    case XPDS_RUNNING:
        xpd->state = XPDS_LOWPOWER;
        break;
    default:
        LOGE("xpu%d: try to enter LOWPOWER state from %s\n", xpd->devfile_id,
             xpd_state_str(xpd->state));
        ret = -EINVAL;
        break;
    }
    spin_unlock_irqrestore(&xpd->state_lock, flags);
    return ret;
}

// xpd error priority, high prio could mask low prio
static inline int __eprio(int errno)
{
    switch (errno) {
    case XPUERR_UCECC:
        return 6;
    case XPUERR_OVERHEAT:
        return 5;
    case XPUERR_HWEXCEPTION:
        return 4;
    case XPUERR_KEXCEPTION:
        return 3;
    case XPUERR_TIMEOUT:
        return 2;
    case XPUERR_DMATIMEOUT:
        return 1;
    default:
        return -1;
    }
}

inline int xpd_state_to_error(struct xpu_pd *xpd, int errno)
{
    int           ret   = 0;
    unsigned long flags = 0;
    spin_lock_irqsave(&xpd->state_lock, flags);
    switch (xpd->state) {
    case XPDS_RUNNING:
    case XPDS_PAUSING:
    case XPDS_ERROR:
        xpd->state = XPDS_ERROR;
        break;
    default:
        LOGE("xpu%d: try to enter ERROR state from %s\n", xpd->devfile_id,
             xpd_state_str(xpd->state));
        ret = -EINVAL;
        break;
    }
    if (__eprio(errno) > __eprio(xpd->errno))
        xpd->errno = errno;
    spin_unlock_irqrestore(&xpd->state_lock, flags);
    return ret;
}

inline int xpd_state_to_pausing(struct xpu_pd *xpd)
{
    int           ret   = 0;
    unsigned long flags = 0;
    spin_lock_irqsave(&xpd->state_lock, flags);
    LOGI("xpu%d: %s -> PAUSING\n", xpd->devfile_id, xpd_state_str(xpd->state));
    switch (xpd->state) {
    case XPDS_RUNNING:
    case XPDS_PAUSING:
        xpd->state = XPDS_PAUSING;
        break;
    default:
        LOGE("xpu%d: cannot do that\n", xpd->devfile_id);
        ret = -EINVAL;
        break;
    }
    spin_unlock_irqrestore(&xpd->state_lock, flags);
    return ret;
}

inline int xpd_state_to_paused(struct xpu_pd *xpd)
{
    int           ret   = 0;
    unsigned long flags = 0;
    spin_lock_irqsave(&xpd->state_lock, flags);
    switch (xpd->state) {
    case XPDS_PAUSING:
        LOGI("xpu%d: %s -> PAUSED\n", xpd->devfile_id, xpd_state_str(xpd->state));
        xpd->state = XPDS_PAUSED;
        break;
    default:
        LOGE("xpu%d: try to enter PAUSED state from %s\n", xpd->devfile_id,
             xpd_state_str(xpd->state));
        ret = -EINVAL;
        break;
    }
    spin_unlock_irqrestore(&xpd->state_lock, flags);
    return ret;
}
