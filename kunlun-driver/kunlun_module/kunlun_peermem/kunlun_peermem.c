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

/* SPDX-License-Identifier: Linux-OpenIB */
/*
 * Copyright (c) 2006, 2007 Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2007, 2008 Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *  - Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/hugetlb.h>
#include <linux/pci.h>

#include "nv-p2p.h"
#include "peer_mem.h"
#include "conftest.h"
#include "xpu/version.h"

#define DRV_NAME    "kunlun_peermem"
#define DRV_VERSION NV_VERSION_STRING

MODULE_AUTHOR("BAIDU-ISA");
MODULE_DESCRIPTION("kunlun GPU memory plug-in, for direct p2p GPU memory access");
MODULE_LICENSE("Linux-OpenIB");
//MODULE_VERSION(DRV_VERSION);
MODULE_VERSION(XPURT_VERSION_STR);
MODULE_VERSION(XPURT_COMMIT);

#if defined(NV_MLNX_IB_PEER_MEM_SYMBOLS_PRESENT)

#define peer_err(FMT, ARGS...) printk(KERN_ERR "kunlun_peermem:" " %s:%d " FMT, __FUNCTION__, __LINE__, ## ARGS)
#define peer_info(FMT, ARGS...) printk(KERN_INFO "kunlun_peermem:" " %s:%d " FMT, __FUNCTION__, __LINE__, ## ARGS)

#ifndef READ_ONCE
#define READ_ONCE(x) ACCESS_ONCE(x)
#endif

#ifndef WRITE_ONCE
#define WRITE_ONCE(x, val) ({ ACCESS_ONCE(x) = (val); })
#endif

#define GPU_PAGE_SHIFT   12
#define GPU_PAGE_SIZE    ((u64)1 << GPU_PAGE_SHIFT)
#define GPU_PAGE_OFFSET  (GPU_PAGE_SIZE-1)
#define GPU_PAGE_MASK    (~GPU_PAGE_OFFSET)

invalidate_peer_memory mem_invalidate_callback;
static void *reg_handle;

struct nv_mem_context {
    struct nvidia_p2p_page_table *page_table;
    struct nvidia_p2p_dma_mapping *dma_mapping;
    u64 core_context;
    u64 page_virt_start;
    u64 page_virt_end;
    size_t mapped_size;
    unsigned long npages;
    unsigned long page_size;
    struct task_struct *callback_task;
    int sg_allocated;
    struct sg_table sg_head;
};


static void nv_get_p2p_free_callback(void *data)
{
    int ret = 0;
    struct nv_mem_context *nv_mem_context = (struct nv_mem_context *)data;
    struct nvidia_p2p_page_table *page_table = NULL;
    struct nvidia_p2p_dma_mapping *dma_mapping = NULL;

    __module_get(THIS_MODULE);
    if (!nv_mem_context) {
        peer_err("nv_get_p2p_free_callback -- invalid nv_mem_context\n");
        goto out;
    }

    if (!nv_mem_context->page_table) {
        peer_err("nv_get_p2p_free_callback -- invalid page_table\n");
        goto out;
    }

    /* Save page_table locally to prevent it being freed as part of nv_mem_release
     *  in case it's called internally by that callback.
     */
    page_table = nv_mem_context->page_table;

    if (!nv_mem_context->dma_mapping) {
        peer_err("nv_get_p2p_free_callback -- invalid dma_mapping\n");
        goto out;
    }
    dma_mapping = nv_mem_context->dma_mapping;

    /* For now don't set nv_mem_context->page_table to NULL,
     * confirmed by NVIDIA that inflight put_pages with valid pointer will fail gracefully.
     */

    nv_mem_context->callback_task = current;
    (*mem_invalidate_callback) (reg_handle, nv_mem_context->core_context);
    nv_mem_context->callback_task = NULL;

    ret = kunlun_p2p_free_dma_mapping(dma_mapping);
    if (ret)
        peer_err("nv_get_p2p_free_callback -- error %d while calling kunlun_p2p_free_dma_mapping()\n", ret);

    ret = kunlun_p2p_free_page_table(page_table);
    if (ret)
        peer_err("nv_get_p2p_free_callback -- error %d while calling kunlun_p2p_free_page_table()\n", ret);

out:
    module_put(THIS_MODULE);
    return;

}

/* At that function we don't call IB core - no ticket exists */
static void nv_mem_dummy_callback(void *data)
{
    struct nv_mem_context *nv_mem_context = (struct nv_mem_context *)data;
    int ret = 0;

    __module_get(THIS_MODULE);

    ret = kunlun_p2p_free_page_table(nv_mem_context->page_table);
    if (ret)
        peer_err("nv_mem_dummy_callback -- error %d while calling kunlun_p2p_free_page_table()\n", ret);

    module_put(THIS_MODULE);
    return;
}

/* acquire return code: 1 mine, 0 - not mine */
static int nv_mem_acquire(unsigned long addr, size_t size, void *peer_mem_private_data,
                          char *peer_mem_name, void **client_context)
{

    int ret = 0;
    struct nv_mem_context *nv_mem_context;

    nv_mem_context = kzalloc(sizeof *nv_mem_context, GFP_KERNEL);
    if (!nv_mem_context)
        /* Error case handled as not mine */
        return 0;

    nv_mem_context->page_virt_start = addr & GPU_PAGE_MASK;
    nv_mem_context->page_virt_end   = (addr + size + GPU_PAGE_SIZE - 1) & GPU_PAGE_MASK;
    nv_mem_context->mapped_size  = nv_mem_context->page_virt_end - nv_mem_context->page_virt_start;

    ret = kunlun_p2p_get_pages(0, 0, nv_mem_context->page_virt_start, nv_mem_context->mapped_size,
                               &nv_mem_context->page_table, nv_mem_dummy_callback, nv_mem_context);

    if (ret < 0)
        goto err;

    ret = kunlun_p2p_put_pages(0, 0, nv_mem_context->page_virt_start,
                               nv_mem_context->page_table);
    if (ret < 0) {
        /* Not expected, however in case callback was called on that buffer just before
            put pages we'll expect to fail gracefully (confirmed by NVIDIA) and return an error.
        */
        peer_err("nv_mem_acquire -- error %d while calling kunlun_p2p_put_pages()\n", ret);
        goto err;
    }

    /* 1 means mine */
    *client_context = nv_mem_context;
    __module_get(THIS_MODULE);
    return 1;

err:
    kfree(nv_mem_context);

    /* Error case handled as not mine */
    return 0;
}

static int nv_dma_map(struct sg_table *sg_head, void *context,
                      struct device *dma_device, int dmasync,
                      int *nmap)
{
    int i, ret;
    struct scatterlist *sg;
    struct nv_mem_context *nv_mem_context =
        (struct nv_mem_context *) context;
    struct nvidia_p2p_page_table *page_table = nv_mem_context->page_table;
    struct nvidia_p2p_dma_mapping *dma_mapping;
    struct pci_dev *pdev = to_pci_dev(dma_device);

    if (page_table->page_size != NVIDIA_P2P_PAGE_SIZE_4KB) {
        peer_err("nv_dma_map -- assumption of 4KB pages failed size_id=%u\n",
                    nv_mem_context->page_table->page_size);
        return -EINVAL;
    }

    if (!pdev) {
        peer_err("nv_dma_map -- invalid pci_dev\n");
        return -EINVAL;
    }

    ret = kunlun_p2p_dma_map_pages(pdev, page_table, &dma_mapping);
    if (ret) {
        peer_err("nv_dma_map -- error %d while calling kunlun_p2p_dma_map_pages()\n", ret);
        return ret;
    }

    if (!NVIDIA_P2P_DMA_MAPPING_VERSION_COMPATIBLE(dma_mapping)) {
        peer_err("error, incompatible dma mapping version 0x%08x\n",
                 dma_mapping->version);
        kunlun_p2p_dma_unmap_pages(pdev, page_table, dma_mapping);
        return -EINVAL;
    }

    nv_mem_context->npages = dma_mapping->entries;

    ret = sg_alloc_table(sg_head, dma_mapping->entries, GFP_KERNEL);
    if (ret) {
        kunlun_p2p_dma_unmap_pages(pdev, page_table, dma_mapping);
        return ret;
    }

    nv_mem_context->dma_mapping = dma_mapping;
    nv_mem_context->sg_allocated = 1;
    for_each_sg(sg_head->sgl, sg, nv_mem_context->npages, i) {
        sg_set_page(sg, NULL, nv_mem_context->page_size, 0);
        sg->dma_address = dma_mapping->dma_addresses[i];
        sg->dma_length = nv_mem_context->page_size;
    }
    nv_mem_context->sg_head = *sg_head;
    *nmap = nv_mem_context->npages;

    return 0;
}

static int nv_dma_unmap(struct sg_table *sg_head, void *context,
               struct device  *dma_device)
{
    struct pci_dev *pdev = to_pci_dev(dma_device);
    struct nv_mem_context *nv_mem_context =
        (struct nv_mem_context *)context;

    if (!nv_mem_context) {
        peer_err("nv_dma_unmap -- invalid nv_mem_context\n");
        return -EINVAL;
    }

    if (WARN_ON(0 != memcmp(sg_head, &nv_mem_context->sg_head, sizeof(*sg_head))))
        return -EINVAL;

    if (nv_mem_context->callback_task == current)
        goto out;

    if (nv_mem_context->dma_mapping)
        kunlun_p2p_dma_unmap_pages(pdev, nv_mem_context->page_table,
                                   nv_mem_context->dma_mapping);

out:
    return 0;
}


static void nv_mem_put_pages(struct sg_table *sg_head, void *context)
{
    int ret = 0;
    struct nv_mem_context *nv_mem_context =
        (struct nv_mem_context *) context;

    if (!nv_mem_context) {
        peer_err("nv_mem_put_pages -- invalid nv_mem_context\n");
        return;
    }

    if (WARN_ON(0 != memcmp(sg_head, &nv_mem_context->sg_head, sizeof(*sg_head))))
        return;

    if (nv_mem_context->callback_task == current)
        return;

    ret = kunlun_p2p_put_pages(0, 0, nv_mem_context->page_virt_start,
                               nv_mem_context->page_table);

#ifdef _DEBUG_ONLY_
    /* Here we expect an error in real life cases that should be ignored - not printed.
      * (e.g. concurrent callback with that call)
    */
    if (ret < 0) {
        printk(KERN_ERR "error %d while calling kunlun_p2p_put_pages, page_table=%p \n",
               ret,  nv_mem_context->page_table);
    }
#endif

    return;
}

static void nv_mem_release(void *context)
{
    struct nv_mem_context *nv_mem_context =
        (struct nv_mem_context *) context;
    if (nv_mem_context->sg_allocated) {
        sg_free_table(&nv_mem_context->sg_head);
        nv_mem_context->sg_allocated = 0;
    }
    kfree(nv_mem_context);
    module_put(THIS_MODULE);
    return;
}

static int nv_mem_get_pages(unsigned long addr,
                            size_t size, int write, int force,
                            struct sg_table *sg_head,
                            void *client_context,
                            u64 core_context)
{
    int ret;
    struct nv_mem_context *nv_mem_context;

    nv_mem_context = (struct nv_mem_context *)client_context;
    if (!nv_mem_context)
        return -EINVAL;

    nv_mem_context->core_context = core_context;
    nv_mem_context->page_size = GPU_PAGE_SIZE;

    ret = kunlun_p2p_get_pages(0, 0, nv_mem_context->page_virt_start, nv_mem_context->mapped_size,
                               &nv_mem_context->page_table, nv_get_p2p_free_callback, nv_mem_context);
    if (ret < 0) {
        peer_err("nv_mem_get_pages -- error %d while calling kunlun_p2p_get_pages()\n", ret);
        return ret;
    }

    /* No extra access to nv_mem_context->page_table here as we are
        called not under a lock and may race with inflight invalidate callback on that buffer.
        Extra handling was delayed to be done under nv_dma_map.
     */
    return 0;
}


static unsigned long nv_mem_get_page_size(void *context)
{
    struct nv_mem_context *nv_mem_context =
                (struct nv_mem_context *)context;

    return nv_mem_context->page_size;
}


static struct peer_memory_client_ex nv_mem_client_ex = { .client = {
    .acquire        = nv_mem_acquire,
    .get_pages  = nv_mem_get_pages,
    .dma_map    = nv_dma_map,
    .dma_unmap  = nv_dma_unmap,
    .put_pages  = nv_mem_put_pages,
    .get_page_size  = nv_mem_get_page_size,
    .release        = nv_mem_release,
}};

#endif /* NV_MLNX_IB_PEER_MEM_SYMBOLS_PRESENT */

static int __init kunlun_mem_client_init(void)
{
#if defined (NV_MLNX_IB_PEER_MEM_SYMBOLS_PRESENT)
    // off by one, to leave space for the trailing '1' which is flagging
    // the new client type
    BUG_ON(strlen(DRV_NAME) > IB_PEER_MEMORY_NAME_MAX-1);
    strcpy(nv_mem_client_ex.client.name, DRV_NAME);

    // [VER_MAX-1]=1 <-- last byte is used as flag
    // [VER_MAX-2]=0 <-- version string terminator
    BUG_ON(strlen(DRV_VERSION) > IB_PEER_MEMORY_VER_MAX-2);
    strcpy(nv_mem_client_ex.client.version, DRV_VERSION);

    // Register as new-style client
    // Needs updated peer_mem patch, but is harmless otherwise
    nv_mem_client_ex.client.version[IB_PEER_MEMORY_VER_MAX-1] = 1;
    nv_mem_client_ex.ex_size = sizeof(struct peer_memory_client_ex);

    // PEER_MEM_INVALIDATE_UNMAPS allow clients to opt out of
    // unmap/put_pages during invalidation, i.e. the client tells the
    // infiniband layer that it does not need to call
    // unmap/put_pages in the invalidation callback
    nv_mem_client_ex.flags = PEER_MEM_INVALIDATE_UNMAPS;

    reg_handle = ib_register_peer_memory_client(&nv_mem_client_ex.client,
                         &mem_invalidate_callback);
    if (!reg_handle) {
        peer_err("kunlun_mem_client_init -- reg_handle %px while calling ib_register_peer_memory_client()\n", reg_handle);
        return -EINVAL;
    }

    peer_info("module load success\n");
    return 0;
#else
    return -EINVAL;
#endif
}

static void __exit kunlun_mem_client_cleanup(void)
{
#if defined (NV_MLNX_IB_PEER_MEM_SYMBOLS_PRESENT)
    ib_unregister_peer_memory_client(reg_handle);
    peer_info("module unload success\n");
#endif
}

module_init(kunlun_mem_client_init);
module_exit(kunlun_mem_client_cleanup);
