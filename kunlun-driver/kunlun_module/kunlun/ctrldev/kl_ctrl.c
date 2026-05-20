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

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/pid_namespace.h>

#include "kl_drv.h"
#include "kl_util.h"
#include "kl_cxpu.h"
#include "xpu/version.h"
#include "kl1/xpu_drv.h"

struct kl_ctrldev {
    unsigned int   major;
    unsigned int   minor;
    struct cdev    cdev;
    struct device *device;
};

static struct kl_ctrldev s_ctrldev;

static int ctrldev_open(struct inode *inode, struct file *file)
{
    nonseekable_open(inode, file);
    return 0;
}

static int ctrldev_release(struct inode *inode, struct file *file)
{
    return 0;
}

int ioctl_version(void __user *argp)
{
    struct XPUDriverVersionIoctlArgs args;
    args.major = XPURT_VERSION_MAJOR;
    args.minor = XPURT_VERSION_MINOR;
    strncpy(args.commit, XPURT_COMMIT, XPU_MAX_STRLEN);

    if (copy_to_user(argp, &args, sizeof(args))) {
        return -EFAULT;
    }

    return 0;
}

static int ioctl_changeset(void __user *argp)
{
    u32 changeset = 0;

#ifdef XPURT_CHANGESET
    changeset = XPURT_CHANGESET;
#endif

    if (copy_to_user(argp, &changeset, sizeof(changeset)))
        return -EFAULT;

    return 0;
}

static int ioctl_devcnt(void __user *argp)
{
    int cnt = 0;
    int i   = 0;
    int j   = 0;

    for (i = 0; i < MAX_DEVICE_NUM; ++i) {
        struct kl_device *kdev = &g_devs[i];
        if (!kdev->info)
            continue;

        if (kdev->info->kl_code == KL1) {
            struct xpu_device *xdev = kdev->data;
            for (j = 0; j < XPU_PD_NUM; ++j) {
                if (xdev->xpd[j].id == -1)
                    continue;
                ++cnt;
            }
        } else if (kdev->info->kl_code == KL2) {
            ++cnt;
        }
    }

    if (copy_to_user(argp, &cnt, sizeof(cnt))) {
        return -EFAULT;
    }

    return 0;
}

int ioctl_ioc_version(void __user *argp)
{
    int ioc_version = IOC_VERSION;
    if (copy_to_user(argp, &ioc_version, sizeof(int)))
        return -EFAULT;
    return 0;
}

static int ioctl_query_device_info_v1(void __user *argp)
{
    union xpu_device_info_v1 i;
    struct kl_device        *kdev;
    int                      devfile_id;
    int                      err;

    if (copy_from_user(&i, argp, sizeof(i)))
        return -EFAULT;

    switch (i.in.type) {
    case QDIT_PROC_INCG: {
        pid_t root_ns_pid    = task_tgid_nr(current);
        pid_t current_ns_pid = task_tgid_nr_ns(current, task_active_pid_ns(current));
        if (current_ns_pid == root_ns_pid)
            i.out.ret = 0;
        else
            i.out.ret = 1;
        i.out.v32    = *((u32 *)(&root_ns_pid));
        i.out.v64[0] = (u64)current->cgroups;
        break;
    }
    default:
        devfile_id = i.in.arg[0];
        kdev       = get_kdev_by_devfile_id(devfile_id);
        if (!kdev) {
            LOGD("kdev == NULL, invalid devfile_id? devfile_id= %d\n", devfile_id);
            return -ENODEV;
        }

        err = kdev->info->query_device_info_v1(kdev, &i);
        if (err)
            return err;
    }

    if (copy_to_user(argp, &i, sizeof(i)))
        return -EFAULT;

    return 0;
}

static int ioctl_query_device_proc_info(void __user *argp)
{
    struct ioc_qproc_info_in     i;
    struct xpu_device_processes *dp;
    struct kl_device            *kdev;
    int                          devfile_id;
    int                          err;

    if (copy_from_user(&i, argp, sizeof(i)))
        return -EFAULT;

    devfile_id = i.devfile_id;
    kdev       = get_kdev_by_devfile_id(devfile_id);
    if (!kdev) {
        LOGD("kdev == NULL, invalid devfile_id? devfile_id= %d\n", devfile_id);
        return -ENODEV;
    }

    mutex_lock(&kdev->xdprocs_lock);
    dp  = &kdev->xdprocs;
    err = kdev->info->query_device_proc_info(kdev, &i, dp);

    if (err)
        goto out;

    if (copy_to_user(argp, dp, sizeof(*dp))) {
        err = -EFAULT;
        goto out;
    }

out:
    mutex_unlock(&kdev->xdprocs_lock);
    return err;
}

static int ioctl_cxpu_get_memory_info(kl_cxpu_t *cxpu, XPUMemoryKind kind,
                                      struct XPUCxpuIoctlArgs *cmd)
{
    kl_cxpu_mem_info_t mem_info = { 0 };
    int                err;

    if (!cxpu || !cmd)
        return -EINVAL;

    err = kl_cxpu_get_memory_info(cxpu, kind, &mem_info);
    if (!err) {
        cmd->reserved[0] = mem_info.page_size;
        cmd->reserved[1] = mem_info.page_used;
        cmd->reserved[2] = mem_info.page_cnt;
    }

    return err;
}

static int ioctl_cxpu_get_instance_memory_info(kl_cxpu_t *cxpu, char *instance_id,
                                               XPUMemoryKind kind, struct XPUCxpuIoctlArgs *cmd)
{
    kl_cxpu_mem_info_t mem_info = { 0 };
    int                err;

    if (!cxpu || !instance_id || !cmd)
        return -EINVAL;

    err = kl_cxpu_get_instance_memory_info(cxpu, instance_id, kind, &mem_info);
    if (!err) {
        cmd->reserved[0] = mem_info.page_size;
        cmd->reserved[1] = mem_info.page_used;
        cmd->reserved[2] = mem_info.page_cnt;
    }

    return err;
}

static int ioctl_cxpu_config(void __user *argp)
{
    struct kl_device       *kdev;
    int                     devfile_id;
    struct XPUCxpuIoctlArgs cmd;
    int                     err = 0;
    char                   *instance_id;

    if (copy_from_user(&cmd, argp, sizeof(cmd)))
        return -EFAULT;

    devfile_id = cmd.devfile_id;
    kdev       = get_kdev_by_devfile_id(devfile_id);
    if (!kdev) {
        LOGD("kdev == NULL, invalid devfile_id? devfile_id= %d\n", devfile_id);
        return -ENODEV;
    }

    instance_id = (char *)&cmd.instance_id[0];

    switch (cmd.type) {
    case CXPU_GET_INSTANCE_ID:
        err = kl_cxpu_get_instance_id(&kdev->cxpu, cmd.value, instance_id);
        break;
    case CXPU_GET_INSTANCE_COUNT:
        cmd.value = kl_cxpu_get_instance_count(&kdev->cxpu);
        break;
    case CXPU_GET_MAX_INSTANCE_COUNT:
        cmd.value = kl_cxpu_get_max_instance_count(&kdev->cxpu);
        break;
    case CXPU_GET_HSMEM_INFO:
        err = ioctl_cxpu_get_memory_info(&kdev->cxpu, XPU_MEM_L3, &cmd);
        break;
    case CXPU_GET_MAINMEM_INFO:
        err = ioctl_cxpu_get_memory_info(&kdev->cxpu, XPU_MEM_MAIN, &cmd);
        break;
    case CXPU_GET_INSTANCE_HSMEM_INFO:
        err = ioctl_cxpu_get_instance_memory_info(&kdev->cxpu, instance_id, XPU_MEM_L3, &cmd);
        break;
    case CXPU_GET_INSTANCE_MAINMEM_INFO:
        err = ioctl_cxpu_get_instance_memory_info(&kdev->cxpu, instance_id, XPU_MEM_MAIN, &cmd);
        break;
    case CXPU_CREATE_INSTANCE:
        err = kl_cxpu_create_instance(&kdev->cxpu, instance_id);
        break;
    case CXPU_DESTROY_INSTANCE:
        err = kl_cxpu_destroy_instance(&kdev->cxpu, instance_id);
        break;
    case CXPU_SET_INSTANCE_HSMEM_LIMIT:
        err = kl_cxpu_set_instance_mem_limit(&kdev->cxpu, instance_id, XPU_MEM_L3, cmd.value);
        break;
    case CXPU_SET_INSTANCE_MAINMEM_LIMIT:
        err = kl_cxpu_set_instance_mem_limit(&kdev->cxpu, instance_id, XPU_MEM_MAIN, cmd.value);
        break;
    default:
        LOGW("unknown cXPU ioctl cmd type: %x\n", cmd.type);
        err = -EINVAL;
        break;
    }

    if (err)
        return err;

    if (copy_to_user(argp, &cmd, sizeof(cmd)))
        return -EFAULT;

    return 0;
}

static int ioctl_set_numvfs(void __user *argp)
{
    struct kl_device        *kdev;
    int                      devfile_id;
    int                      err;
    int                      num_vfs;
    struct XPUSriovIoctlArgs cmd;

    if (copy_from_user(&cmd, argp, sizeof(cmd)))
        return -EFAULT;

    devfile_id = cmd.devfile_id;
    num_vfs    = cmd.num_vfs;

    kdev = get_kdev_by_devfile_id(devfile_id);
    if (!kdev) {
        LOGD("kdev == NULL, invalid devfile_id? devfile_id= %d\n", devfile_id);
        return -ENODEV;
    }

    if (!kdev->info->sriov_configure)
        return -EFAULT;

    if (kdev->num_vfs == num_vfs) {
        LOGI("vf number is already %d", num_vfs);
        return 0;
    }

    if (num_vfs && kdev->num_vfs) {
        err = kdev->info->sriov_configure(kdev, 0);
        if (err < 0) {
            return err;
        }
    }

    return kdev->info->sriov_configure(kdev, num_vfs);
}

static long ctrldev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    if (_IOC_NR(cmd) == 14 || _IOC_NR(cmd) == 146) {
        LOGI("[ctrl] ioctl nr=%d type=0x%x size=%d\n",
             _IOC_NR(cmd), _IOC_TYPE(cmd), _IOC_SIZE(cmd));
    }
    void __user *argp = (void __user *)arg;
    int          ret  = -XPUERR_DEVINIT;

    switch (cmd) {
    case XPUCTL_CHANGESET:
        ret = ioctl_changeset(argp);
        break;
    case XPUCTL_DEVCNT:
        ret = ioctl_devcnt(argp);
        break;
    case XPUCTL_VERSION:
        ret = ioctl_version(argp);
        break;
    case XPUCTL_SET_NUMVFS:
        ret = ioctl_set_numvfs(argp);
        break;
    case XPUCTL_CXPU_CONFIG:
        ret = ioctl_cxpu_config(argp);
        break;

    case IOCTL_QUERY_DEVINFO1:
        ret = ioctl_query_device_info_v1(argp);
        break;
    case IOCTL_QUERY_PROCINFO: {
        ret = ioctl_query_device_proc_info(argp);
        break;
    }
    case IOCTL_IOC_VERSION:
        ret = ioctl_ioc_version(argp);
        break;

    // 废弃的ioctl cmd不要删除，统一放到这里
    case XPUCTL_DEVINFOALL:
        LOGW("obsolete ioctl cmd %08x, maybe you're using an old version of XRE (libxpurt.so/xpu_smi/...)\n",
             cmd);
        ret = -EINVAL;
        break;

    default:
        LOGW("unknown ioctl cmd %08x\n", cmd);
        ret = -XPUERR_NOIOC;
        break;
    }
    return ret;
}

static struct file_operations ctrldev_ops = {
    .owner          = THIS_MODULE,
    .open           = ctrldev_open,
    .release        = ctrldev_release,
    .unlocked_ioctl = ctrldev_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = ctrldev_ioctl,
#endif
};

int ctrldev_create(unsigned int major, unsigned int minor)
{
    dev_t devno;
    int   rc;

    s_ctrldev.major = major;
    s_ctrldev.minor = minor;

    devno = MKDEV(major, minor);

    cdev_init(&s_ctrldev.cdev, &ctrldev_ops);
    s_ctrldev.cdev.owner = THIS_MODULE;

    rc = cdev_add(&s_ctrldev.cdev, devno, 1);
    if (rc != 0) {
        LOGI("[CTRLNODE] cdev_add failed, ERR= %d\n", rc);
        rc = -XPUERR_DEVINIT;
        goto err_out;
    }

    s_ctrldev.device = device_create(g_class, NULL, devno, &s_ctrldev, XPUCTL_NODE_DEVNAME);

    if (IS_ERR_OR_NULL(s_ctrldev.device)) {
        LOGE("[CTRLNODE] device_create fail\n");
        rc = -XPUERR_DEVINIT;
        goto err_cdev_del;
    }

    return 0;

err_cdev_del:
    cdev_del(&s_ctrldev.cdev);
err_out:
    return rc;
}

void ctrldev_destroy(void)
{
    if (!IS_ERR_OR_NULL(s_ctrldev.device)) {
        device_destroy(g_class, MKDEV(s_ctrldev.major, s_ctrldev.minor));
    }

    cdev_del(&s_ctrldev.cdev);
}
