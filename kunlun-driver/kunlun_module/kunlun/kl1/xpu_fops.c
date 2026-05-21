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
// xpu_fops.c - XPU device file operations
//
#define __FILENAME__ "xpu_fops.c"

#include <linux/errno.h>
#include <linux/version.h>

#include "xpurt_priv/ioctl.h"
#include "xpurt_priv/ioctl_kl1.h"
#include "xpu_drv.h"

// XXX(miaotianxiang): 向下兼容，老接口在/dev/xpu***继续支持
int ioctl_version(void __user *argp);
int ioctl_ioc_version(void __user *argp);

static inline struct xpu_pd *inode_to_xpd(struct inode *inode)
{
    int minor = MINOR(inode->i_rdev);
    return get_xpd_by_minor(minor);
}

// char device open hander
int xpu_char_open(struct inode *inode, struct file *file)
{
    struct xpu_pd      *xpd  = inode_to_xpd(inode);
    struct xpu_session *sess = session_create(xpd, file);

    if (sess == NULL) {
        return -XPUERR_NOCPUMEM;
    }

    file->private_data = sess;

    LOGL1("[xpu_%d] new session id=%d pid=%d comm=%s\n", xpd->devfile_id, sess->id, sess->xctx->pid,
          sess->xctx->comm);

    return 0;
}

// char device release hander
int xpu_char_release(struct inode *inode, struct file *file)
{
    struct xpu_pd      *xpd   = inode_to_xpd(inode);
    struct xpu_session *xsess = (struct xpu_session *)file->private_data;

    // destroy xpu_session
    session_destroy(xpd, xsess);
    file->private_data = NULL;

    return 0;
}

static int check_xsess_error(struct xpu_session *xsess, unsigned int cmd)
{
    if (xsess->state == XSS_NORMAL)
        return 0;

    if (xsess->state == XSS_CLOSED)
        return -XPUERR_INVALID_PARAM;

    switch (cmd) {
    case IOCTL_QUERY_DEVINFO:
    case IOCTL_QUERY_DEVATTR:
        return 0;
    default:
        return -xsess->errno;
    }
}

static int check_xpd_error(struct xpu_pd *xpd, unsigned int cmd)
{
    switch (cmd) {
    case XPDS_RUNNING:
    case XPDS_LOWPOWER:
    case XPDS_PAUSING:
    case XPDS_PAUSED:
        return 0;
    case XPDS_ERROR:
        return -xpd->errno;
    default:
        return 0;
    }

    return 0;
}

// ioctl handler
long xpu_char_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user        *argp  = (void __user *)arg;
    struct xpu_session *xsess = (struct xpu_session *)file->private_data;
    struct xpu_pd      *xpd;
    int                 ret = 0;

    if (xsess == NULL) {
        LOGE("xpd is NULL\n");
        return -XPUERR_UNEXPECT;
    }

    xpd = xsess->xpd;

    ret = check_xsess_error(xsess, cmd);
    if (ret)
        return ret;

    ret = check_xpd_error(xpd, cmd);
    if (ret)
        return ret;

    /* Intercept P2P ioctls by command number (size may differ from header) */
    if (_IOC_TYPE(cmd) == _IOC_TYPE(IOCTL_IOC_MAGIC)) {
        if (_IOC_NR(cmd) == _IOC_NR(IOCTL_MEMCPY_P2P_DIRECT) ||
            _IOC_NR(cmd) == _IOC_NR(IOCTL_MEMCPY_P2P)) {
            return ioctl_memcpy_p2p_kl1(xpd, argp);
        }
        if (_IOC_NR(cmd) == _IOC_NR(IOCTL_HOST_REGISTER)) {
            return ioctl_host_register_kl1(xpd, argp);
        }
        if (_IOC_NR(cmd) == _IOC_NR(IOCTL_HOST_UNREGISTER)) {
            return ioctl_host_unregister_kl1(xpd, argp);
        }
    }

    switch (cmd) {
    case IOCTL_LAUNCH:
    case IOCTL_WAIT:
    case IOCTL_SSE_LAUNCH:
    case IOCTL_SSE_WAIT:
        if (xpd->state == XPDS_ERROR)
            return -XPUERR_ABNORMAL;
        break;
    default:
        break;
    }

    switch (cmd) {
    case IOCTL_DEV_HARD_RESET:
        ret = ioctl_dev_hard_reset(xpd);
        break;
    case IOCTL_DEV_SOFT_RESET:
        ret = ioctl_dev_soft_reset(xpd);
        break;
    case IOCTL_REG_READ:
        ret = ioctl_reg_read(xpd, argp);
        break;
    case IOCTL_REG_WRITE:
        ret = ioctl_reg_write(xpd, argp);
        break;
    case IOCTL_MEMORY_ALLOC:
        ret = ioctl_memory_alloc(file, xpd, argp);
        break;
    case IOCTL_MEMORY_FREE:
        ret = ioctl_memory_free(file, xpd, argp);
        break;
    case IOCTL_MEMCPY_H2D:
    case IOCTL_MEMCPY_H2D_EX:
        ret = ioctl_memcpy_h2d(xpd, argp);
        break;
    case IOCTL_MEMCPY_D2H:
    case IOCTL_MEMCPY_D2H_EX:
        ret = ioctl_memcpy_d2h(xpd, argp);
        break;
    case IOCTL_MEMCPY_D2D:
        ret = ioctl_memcpy_d2d(xpd, argp);
        break;
    case IOCTL_MEMCPY:
        LOGW("IOCTL_MEMCPY is deprecated, please recompile with latest RT.\n");
        ret = ioctl_memcpy(xpd, argp);
        break;
    case IOCTL_LAUNCH:
        ret = ioctl_launch(file, xpd, argp);
        break;
    case IOCTL_WAIT:
        ret = ioctl_wait(file, xpd, argp);
        break;
    case IOCTL_BATCHLAUNCH:
        ret = ioctl_batchlaunch(file, xpd, argp);
        break;
    case IOCTL_PROFLAUNCH:
        ret = ioctl_proflaunch(file, xpd, argp);
        break;
    case IOCTL_SESSION_BINDTQ:
        break;
    case IOCTL_TEST:
        ret = ioctl_test(file, xpd->xdev, argp);
        break;
    case IOCTL_SSE_LAUNCH:
        ret = ioctl_sse_launch(file, xpd, argp);
        break;
    case IOCTL_SSE_WAIT:
        ret = ioctl_sse_wait(file, xpd, argp);
        break;
    case IOCTL_SD_LAUNCH:
        LOGW("not implemented\n");
        break;
    case IOCTL_CLUSTER_LAUNCH:
        LOGW("not implemented\n");
        break;
    case IOCTL_START_CUNITS:
        ret = ioctl_start_cunits(file, xpd, argp);
        break;
    case IOCTL_QUERY_BAR:
        ret = ioctl_query_bar(file, xpd->xdev, argp);
        break;
    case IOCTL_QUERY_IATU_REGION:
        ret = ioctl_query_iatu_region(file, xpd->xdev, argp);
        break;
    case IOCTL_QUERY_DEVATTR:
        ret = ioctl_query_device_attr(file, xpd->xdev, argp);
        break;
    case IOCTL_QUERY_DEVINFO:
        ret = ioctl_query_device_info(file, argp);
        break;
    case IOCTL_PROFCLR:
        ret = ioctl_prof_clear(file, xpd, argp);
        break;

    // XXX(miaotianxiang): 向下兼容，老接口在/dev/xpu***继续支持
    case XPUCTL_VERSION:
        ret = ioctl_version(argp);
        break;
    case IOCTL_IOC_VERSION:
        ret = ioctl_ioc_version(argp);
        break;

    case IOCTL_HOST_REGISTER:
        ret = ioctl_host_register_kl1(xpd, argp);
        break;
    case IOCTL_HOST_UNREGISTER:
        ret = ioctl_host_unregister_kl1(xpd, argp);
        break;
    case IOCTL_MEMCPY_P2P:
    case IOCTL_MEMCPY_P2P_DIRECT:
        ret = ioctl_memcpy_p2p_kl1(xpd, argp);
        break;

    default:
        ret = -XPUERR_NOIOC;
        break;
    }

    return ret;
}

// mmap handler
int xpu_char_mmap(struct file *file, struct vm_area_struct *vma)
{
    return 0;
}

struct file_operations xpu_fops = {
    .owner          = THIS_MODULE,
    .open           = xpu_char_open,
    .release        = xpu_char_release,
    .unlocked_ioctl = xpu_char_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = xpu_char_ioctl,
#endif
};
