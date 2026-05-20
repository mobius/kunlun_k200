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

#include "kl2/kl2.h"
#include <linux/fs.h>

static inline int check_range(unsigned long addr, unsigned long size, unsigned long range_start,
                              unsigned long range_end)
{
    if ((addr + size) < addr || PAGE_ALIGN(addr + size) < (addr + size) || addr < range_start ||
        (addr + size) > range_end)
        return 0;

    return 1;
}

// char device open handler
int kl2_open(struct inode *inode, struct file *file)
{
    struct kl_inode    *kinode  = inode_to_kinode(inode);
    struct kl2_device  *kl2_dev = (struct kl2_device *)kinode->data;
    struct kl2_session *sess    = NULL;
    int                 err;

    KL2_LOGD("open %s vf_id=%d\n", kl2_dev->kdev->name, kl2_dev->kdev->vf_id);

    err = kl2_create_session(kl2_dev, &sess);
    if (err)
        return err;

    file->private_data = sess;
    nonseekable_open(inode, file);

    return 0;
}

// char device release handler
int kl2_release(struct inode *inode, struct file *file)
{
    struct kl2_session        *sess           = (struct kl2_session *)file->private_data;
    struct kl2_device *kl2_dev __maybe_unused = sess->kl2_dev;
    int                        err;

#ifdef ENABLE_CODEC
    vdec_on_file_release(kl2_dev->video_dec, file);
    venc_on_file_release(kl2_dev->video_enc, file);
    imgproc_on_file_release(kl2_dev->image_proc, file);
#endif

    err = kl2_session_wait_until_finished(sess);
    if (err) {
        KL2_LOGW(
                "sess_%d unfinished_cnt != 0 in kl2_release, excp work stuck? unfinished_cnt= %d\n",
                sess->id, atomic_read(&sess->unfinished_cnt));
    }

    xref_put(&sess->xref, kl2_destroy_session_ref);

    file->private_data = NULL;

    return 0;
}

static void kl2_host_alloc_vm_open(struct vm_area_struct *vma)
{
}

static void kl2_host_alloc_vm_close(struct vm_area_struct *vma)
{
    unsigned long           size          = vma->vm_end - vma->vm_start;
    unsigned long           addr          = vma->vm_start;
    struct kl2_userprocess *uproc         = vma->vm_private_data;
    struct kl2_sg_minfo    *minfo         = NULL;
    int                     ret           = 0;
    bool                    release_minfo = false;

    write_lock(&uproc->sg_minfo_lock);
    ret = kl2_host_memory_is_pinned(uproc, addr, size, &minfo);
    if (ret == KL2_HOSTMEM_PINNED) {
        if (minfo->addr == addr && minfo->size == size && minfo->mmaped) {
            kl2_minfo_rb_erase(&uproc->sg_minfo_rb, minfo);
            release_minfo = true;
        }
    }
    write_unlock(&uproc->sg_minfo_lock);

    if (release_minfo) {
        xref_put(&minfo->xref, kl2_destroy_minfo_ref);
    }
}

static struct vm_operations_struct kl2_host_alloc_vm_ops = {
    .open  = kl2_host_alloc_vm_open,
    .close = kl2_host_alloc_vm_close,
};

static int kl2_mmap_host_alloc(struct kl2_session *sess, struct vm_area_struct *vma)
{
    struct kl2_device      *kl2_dev = sess->kl2_dev;
    struct kl2_userprocess *uproc   = sess->uproc;
    struct kl2_sg_minfo    *minfo   = NULL;
    unsigned long           addr    = vma->vm_start;
    unsigned long           size    = vma->vm_end - vma->vm_start;
    unsigned long           offset  = vma->vm_pgoff;
    int                     ret     = 0;
    int                     i;

    if (offset != 0) {
        ret = -EINVAL;
        goto err_out;
    }

    minfo = kzalloc(sizeof(*minfo), GFP_KERNEL);
    if (!minfo) {
        ret = -ENOMEM;
        goto err_out;
    }

    ret = kl2_host_alloc_hugepages(uproc, addr, size, minfo);
    if (ret) {
        goto err_free_minfo;
    }

    for (i = 0; i < minfo->page_count; i++) {
        ret = vm_insert_page(vma, addr, minfo->user_pages[i]);
        if (ret) {
            // XXX(miaotianxiang):
            //
            // 20220919: 失败后调用zap_page_range，清理已映射的pages?
            // 20220920: mmap_region在调用call_mmap失败后，将自动清理partial
            // mapping部分，递减page->_refcount，最终释放page，参考下列dump_stack内容
            //
            // <...>-3094982 [004] ...1 77212.233931: mm_page_free: page=0000000061b2fc33 pfn=1844202 order=0
            // <...>-3094982 [004] ...1 77212.233934: <stack trace>
            //   => trace_event_raw_event_mm_page_free
            //   => free_pcp_prepare
            //   => free_unref_page_list
            //   => release_pages
            //   => tlb_flush_mmu
            //   => tlb_finish_mmu
            //   => unmap_region
            //   => mmap_region
            //   => do_mmap
            //   => vm_mmap_pgoff
            //   => ksys_mmap_pgoff
            //   => do_syscall_64
            //   => entry_SYSCALL_64_after_hwframe
            //
            KL2_LOGW("vm_insert_page failed, ret= %d\n", ret);
            goto err_mmap;
        }
        addr += PAGE_SIZE;
    }

    write_lock(&uproc->sg_minfo_lock);
    ret = kl2_minfo_rb_insert(&uproc->sg_minfo_rb, minfo);
    write_unlock(&uproc->sg_minfo_lock);
    if (ret) {
        goto err_mmap;
    }

    /* VM_DONTCOPY: Do not copy this vma on fork */
    /* VM_DONTEXPAND: Cannot expand with mremap() */
    nv_vm_flags_set(vma, VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP | VM_LOCKED);
    vma->vm_private_data = uproc;
    vma->vm_ops          = &kl2_host_alloc_vm_ops;
    vma->vm_ops->open(vma);

    return 0;

err_mmap:
    kl2_host_free_hugepages(minfo);

err_free_minfo:
    kfree(minfo);

err_out:
    return ret;
}

static struct kl2_mappable_mem {
    u64 start;
    u64 end;
    int bar_idx;
    u64 bar_off;
} mappable_mem[2] = {
    /* L3 */
    {
            .start   = 0xc0000000ull,
            .end     = 0xc4000000ull,
            .bar_idx = 4,
            .bar_off = 0,
    },
    /* GDDR */
    {
            .start   = 0x800000000ull,
            .end     = 0x1000000000ull,
            .bar_idx = 4,
            .bar_off = 0x800000000ull,
    },
};

static void kl2_mmap_vm_open(struct vm_area_struct *vma)
{
}

static void kl2_mmap_vm_close(struct vm_area_struct *vma)
{
    unsigned long           size  = vma->vm_end - vma->vm_start;
    unsigned long           addr  = vma->vm_start;
    struct kl2_userprocess *uproc = vma->vm_private_data;
    struct kl2_p2p_info    *p2p_info, *safe;

    write_lock(&uproc->sg_minfo_lock);
    list_for_each_entry_safe(p2p_info, safe, &uproc->p2p_list, uproc_node) {
        if (p2p_info->vaddr == addr && p2p_info->size == size) {
            list_del(&p2p_info->uproc_node);
            kfree(p2p_info);
            break;
        }
    }
    write_unlock(&uproc->sg_minfo_lock);
}

static struct vm_operations_struct kl2_mmap_vm_ops = {
    .open  = kl2_mmap_vm_open,
    .close = kl2_mmap_vm_close,
};

static int kl2_uproc_add_p2p_list(struct kl2_userprocess *uproc, u64 vaddr, u64 size, u64 pcie_addr)
{
    struct kl2_p2p_info *p2p_info = NULL;

    p2p_info = kzalloc(sizeof(*p2p_info), GFP_KERNEL);
    if (p2p_info == NULL) {
        return -ENOMEM;
    }
    p2p_info->vaddr     = vaddr;
    p2p_info->size      = size;
    p2p_info->pcie_addr = pcie_addr;

    write_lock(&uproc->sg_minfo_lock);
    list_add_tail(&p2p_info->uproc_node, &uproc->p2p_list);
    write_unlock(&uproc->sg_minfo_lock);
    return 0;
}

static int kl2_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct kl2_session *sess            = (struct kl2_session *)file->private_data;
    struct kl2_device  *kl2_dev         = sess->kl2_dev;
    unsigned long       address         = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long       size            = vma->vm_end - vma->vm_start;
    unsigned long       pcie_start_addr = 0;
    unsigned long       pcie_end_addr   = 0;
    int                 bar_idx         = 0;
    int                 ret             = -EINVAL;
    int                 i               = 0;

    // XXX(miaotianxiang): 暂时从LOGD改为LOGI，用于debug
    KL2_LOGD("request to map phy addr = 0x%lx, size = %ld to usr addr = 0x%lx\n", address, size,
             vma->vm_start);

    // XXX(miaotianxiang): 如果vm_pgoff为0，则认为是xpu_host_alloc
    if (address == 0) {
        ret = kl2_mmap_host_alloc(sess, vma);
        goto check_ret;
    }

    // 以noc地址作为offset调用mmap()
    for (i = 0; i < ARRAY_SIZE(mappable_mem); ++i) {
        struct kl2_mappable_mem *entry = &mappable_mem[i];
        u64                      pcie_addr;

        if (!check_range(address, size, entry->start, entry->end)) {
            continue;
        }

        pcie_addr = kl2_dev->kdev->bar_info.pcie_addr[entry->bar_idx] + entry->bar_off +
                    (address - entry->start);
        // 检查是否超过BAR size，不同设备/fw版本BAR size可能不一致
        if (!check_range(pcie_addr, size, kl2_dev->kdev->bar_info.pcie_addr[entry->bar_idx],
                         kl2_dev->kdev->bar_info.pcie_addr[entry->bar_idx] +
                                 kl2_dev->kdev->bar_info.bar_size[entry->bar_idx])) {
            // 如超过，直接返回-EINVAL
            goto check_ret;
        }

        nv_vm_flags_set(vma, VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_NORESERVE | VM_DONTDUMP |
                                     VM_PFNMAP | VM_LOCKED);
        vma->vm_page_prot    = pgprot_noncached(vma->vm_page_prot);
        vma->vm_private_data = sess->uproc;
        vma->vm_ops          = &kl2_mmap_vm_ops;
        vma->vm_ops->open(vma);

        ret = io_remap_pfn_range(vma, vma->vm_start, pcie_addr >> PAGE_SHIFT, size,
                                 vma->vm_page_prot);

        kl2_uproc_add_p2p_list(sess->uproc, vma->vm_start, size, pcie_addr);

        goto check_ret;
    }

    // 以BAR物理地址作为offset调用mmap()
    for (bar_idx = 0; bar_idx < PCIE_BAR_NUM; bar_idx++) {
        pcie_start_addr = kl2_dev->kdev->bar_info.pcie_addr[bar_idx];
        pcie_end_addr   = pcie_start_addr + kl2_dev->kdev->bar_info.bar_size[bar_idx];

        if (!check_range(address, size, pcie_start_addr, pcie_end_addr)) {
            continue;
        }

        nv_vm_flags_set(vma, VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_NORESERVE | VM_DONTDUMP |
                                     VM_PFNMAP | VM_LOCKED);
        vma->vm_page_prot    = pgprot_noncached(vma->vm_page_prot);
        vma->vm_private_data = sess->uproc;
        vma->vm_ops          = &kl2_mmap_vm_ops;
        vma->vm_ops->open(vma);

        ret = io_remap_pfn_range(vma, vma->vm_start, address >> PAGE_SHIFT, size,
                                 vma->vm_page_prot);

        kl2_uproc_add_p2p_list(sess->uproc, vma->vm_start, size, address);

        goto check_ret;
    }

check_ret:
    return ret;
}

struct file_operations kl2_fops = {
    .owner          = THIS_MODULE,
    .open           = kl2_open,
    .release        = kl2_release,
    .unlocked_ioctl = kl2_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = kl2_ioctl,
#endif
    .mmap = kl2_mmap,
};
