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

#include "kl_drv.h"

static int kl_char_open(struct inode *inode, struct file *file)
{
    struct kl_inode *kinode = inode_to_kinode(inode);
    if (file->f_inode != inode) {
        LOGW("file has not cached inode\n");
        file->f_inode = inode;
    }

    if (kinode->fops && kinode->fops->open) {
        return kinode->fops->open(inode, file);
    }

    LOGE("[xpu_%d] inode has no open fops\n", kinode->minor);
    return -XPUERR_UNEXPECT;
}

// char device release hander
static int kl_char_release(struct inode *inode, struct file *file)
{
    struct kl_inode *kinode = inode_to_kinode(inode);
    if (kinode->fops && kinode->fops->release) {
        return kinode->fops->release(inode, file);
    }

    LOGE("[xpu_%d] inode has no release fops\n", kinode->minor);
    return -XPUERR_UNEXPECT;
}

// ioctl handler
static long kl_char_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct kl_inode *kinode = inode_to_kinode(file->f_inode);
    if (kinode->fops && kinode->fops->unlocked_ioctl) {
        return kinode->fops->unlocked_ioctl(file, cmd, arg);
    }

    LOGE("[xpu_%d] inode has no ioctl fops\n", kinode->minor);
    return -XPUERR_UNEXPECT;
}

static int kl_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct kl_inode *kinode = inode_to_kinode(file->f_inode);
    if (kinode->fops && kinode->fops->mmap) {
        return kinode->fops->mmap(file, vma);
    }

    LOGE("[xpu_%d] inode has no mmap fops\n", kinode->minor);
    return -XPUERR_UNEXPECT;
}

struct file_operations kl_fops = {
    .owner          = THIS_MODULE,
    .open           = kl_char_open,
    .release        = kl_char_release,
    .unlocked_ioctl = kl_char_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = kl_char_ioctl,
#endif
    .mmap = kl_mmap,
};
