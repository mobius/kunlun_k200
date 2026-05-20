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

#ifdef ENABLE_RB

/* Disk on RAM Driver */
#include "kl2/ram_blk.h"
#include "kl2/dma.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/errno.h>

static struct rb_device g_rb_devices[MAX_XPU_MINOR_NUM] = { { 0 } };

int ramdevice_init(void)
{
    // placeholder
    return 0;
}

void ramdevice_cleanup(void)
{
    // placeholder
}

void ramdevice_write(struct rb_device *rb_dev, sector_t sector_off, u8 *buffer,
                     unsigned int sectors)
{
    int ret = 0;

    //printk(KERN_INFO "ramdevice_write: 0x%lx, 0x%llx, %d\n",
    //        sector_off, (u64)buffer, sectors);

    ret = kl2_dma_ddma_from_host_kernel(rb_dev->dma_engine,
                                        rb_dev->dev_addr + sector_off * RB_SECTOR_SIZE, (u64)buffer,
                                        (u64)(sectors * RB_SECTOR_SIZE));

    if (ret != 0) {
        printk(KERN_ERR "rb: ramdevice_write error!\n");
    }
}

void ramdevice_read(struct rb_device *rb_dev, sector_t sector_off, u8 *buffer, unsigned int sectors)
{
    int ret = 0;

    //printk(KERN_INFO "ramdevice_read: 0x%lx, 0x%llx, %d\n",
    //        sector_off, (u64)buffer, sectors);

    ret = kl2_dma_ddma_to_host_kernel(rb_dev->dma_engine, (u64)buffer,
                                      rb_dev->dev_addr + sector_off * RB_SECTOR_SIZE,
                                      (u64)(sectors * RB_SECTOR_SIZE));

    if (ret != 0) {
        printk(KERN_ERR "rb: ramdevice_read error!\n");
    }
}

static int rb_open(struct block_device *bdev, fmode_t mode)
{
    unsigned unit = iminor(bdev->bd_inode);

    //printk(KERN_INFO "rb: Device is opened\n");
    //printk(KERN_INFO "rb: Inode number is %d\n", unit);

    if (unit > RB_MINOR_CNT)
        return -ENODEV;
    return 0;
}

static void rb_close(struct gendisk *disk, fmode_t mode)
{
    //printk(KERN_INFO "rb: Device is closed\n");
}

/*
 * Actual Data transfer
 */
static int rb_transfer(struct request *req, struct rb_device *rb_dev)
{
    int          dir          = rq_data_dir(req);
    sector_t     start_sector = blk_rq_pos(req);
    unsigned int sector_cnt   = blk_rq_sectors(req);

    struct bio_vec     *bv;
    struct req_iterator iter;

    sector_t     sector_offset;
    unsigned int sectors;
    u8          *buffer;

    int ret = 0;

    //printk(KERN_DEBUG "rb: Dir:%d; Sec:%lld; Cnt:%d\n", dir, start_sector, sector_cnt);

    sector_offset = 0;
    rq_for_each_segment(bv, req, iter) {
        buffer = page_address(bv->bv_page) + bv->bv_offset;
        if (bv->bv_len % RB_SECTOR_SIZE != 0) {
            printk(KERN_ERR "rb: Should never happen: "
                            "bio size (%d) is not a multiple of RB_SECTOR_SIZE (%d).\n"
                            "This may lead to data truncation.\n",
                   bv->bv_len, RB_SECTOR_SIZE);
            ret = -EIO;
        }
        sectors = bv->bv_len / RB_SECTOR_SIZE;
        //printk(KERN_DEBUG "rb: Sector Offset: %lld; Buffer: %px; Length: %d sectors\n",
        //        sector_offset, buffer, sectors);
        if (dir == WRITE) /* Write to the device */
        {
            ramdevice_write(rb_dev, start_sector + sector_offset, buffer, sectors);
        } else /* Read from the device */
        {
            ramdevice_read(rb_dev, start_sector + sector_offset, buffer, sectors);
        }
        sector_offset += sectors;
    }
    if (sector_offset != sector_cnt) {
        printk(KERN_ERR "rb: bio info doesn't match with the request info");
        ret = -EIO;
    }

    return ret;
}

/*
 * Represents a block I/O request for us to execute
 */
static void rb_request(struct request_queue *q)
{
    struct request *req;
    int             ret;

    /* Gets the current request from the dispatch queue */
    while ((req = blk_fetch_request(q)) != NULL) {
        ret = rb_transfer(req, (struct rb_device *)(q->queuedata));
        __blk_end_request_all(req, ret);
    }
}

/*
 * These are the file operations that performed on the ram block device
 */
static struct block_device_operations rb_fops = {
    .owner   = THIS_MODULE,
    .open    = rb_open,
    .release = rb_close,
};

/*
 * This is the registration and initialization section of the ram block device
 * driver
 */
int kl_rb_init(struct dma_engine *dma_engine, int xpu_minor, u64 rb_dev_addr, u64 rb_size)
{
    int               ret;
    char              rb_name[16];
    struct rb_device *rb_dev = &(g_rb_devices[xpu_minor]);

    snprintf(rb_name, 16, "xpu%d_rb", xpu_minor);
    printk(KERN_INFO "kl_rb_init: %s\n", rb_name);

    rb_dev->dma_engine = dma_engine;
    rb_dev->xpu_minor  = xpu_minor;

    /* Set up our RAM Device */
    if ((ret = ramdevice_init()) < 0) {
        return ret;
    }

    rb_dev->size     = rb_size;
    rb_dev->dev_addr = rb_dev_addr;

    /* Get Registered */
    rb_dev->major = register_blkdev(0, rb_name);
    if (rb_dev->major <= 0) {
        printk(KERN_ERR "rb: Unable to get Major Number\n");
        ret = -EBUSY;
        goto err_rd_cleanup;
    }

    /* Get a request queue (here queue is created) */
    spin_lock_init(&rb_dev->lock);
    rb_dev->rb_queue = blk_init_queue(rb_request, &rb_dev->lock);
    if (rb_dev->rb_queue == NULL) {
        printk(KERN_ERR "rb: blk_init_queue failure\n");
        ret = -ENOMEM;
        goto err_unreg;
    }

    rb_dev->rb_queue->queuedata = (void *)rb_dev;

    /*
     * Add the gendisk structure
     * By using this memory allocation is involved,
     * the minor number we need to pass bcz the device
     * will support this much partitions
     */
    rb_dev->rb_disk = alloc_disk(RB_MINOR_CNT);
    if (!rb_dev->rb_disk) {
        printk(KERN_ERR "rb: alloc_disk failure\n");
        ret = -ENOMEM;
        goto err_blk_cleanup;
    }

    rb_dev->rb_disk->major        = rb_dev->major;
    rb_dev->rb_disk->first_minor  = RB_FIRST_MINOR;
    rb_dev->rb_disk->fops         = &rb_fops;
    rb_dev->rb_disk->private_data = (void *)rb_dev;
    rb_dev->rb_disk->queue        = rb_dev->rb_queue;
    sprintf(rb_dev->rb_disk->disk_name, rb_name);
    set_capacity(rb_dev->rb_disk, rb_dev->size);

    add_disk(rb_dev->rb_disk);
    printk(KERN_INFO "rb: Ram Block driver initialised (%lld sectors; %lld bytes)\n", rb_dev->size,
           rb_dev->size * RB_SECTOR_SIZE);

    return 0;

err_blk_cleanup:
    blk_cleanup_queue(rb_dev->rb_queue);
err_unreg:
    unregister_blkdev(rb_dev->major, rb_name);
err_rd_cleanup:
    ramdevice_cleanup();

    return ret;
}

/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
void kl_rb_cleanup(int xpu_minor)
{
    char              rb_name[16];
    struct rb_device *rb_dev = &(g_rb_devices[xpu_minor]);

    snprintf(rb_name, 16, "xpu%d_rb", xpu_minor);
    printk(KERN_INFO "kl_rb_cleanup: %s\n", rb_name);

    /* clean all up if initialization is completed */
    if (rb_dev->rb_disk) {
        del_gendisk(rb_dev->rb_disk);
        put_disk(rb_dev->rb_disk);
        blk_cleanup_queue(rb_dev->rb_queue);
        unregister_blkdev(rb_dev->major, rb_name);
        ramdevice_cleanup();
    }
}

#endif // ENABLE_RB
