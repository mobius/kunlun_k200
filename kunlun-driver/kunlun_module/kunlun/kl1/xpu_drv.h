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

/// \file   xpu_drv.h
/// \brief  XPU driver main header.
///         Private constants, structs and funcs used in XPU driver.
/// \author hanjinchen@baidu.com
/// \copyright (C) 2018 Baidu, Inc
///
#ifndef BAIDU_XPU_RUNTIME_MODULE_XPU_DRV_H
#define BAIDU_XPU_RUNTIME_MODULE_XPU_DRV_H

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/bitmap.h>
#include <asm/atomic.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/err.h>
#include <linux/uio.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/proc_fs.h>
#include <linux/bug.h>
#include <linux/list.h>
#include <linux/hrtimer.h>

#include "xpu/defs.h"
#include "xpurt_priv/defs_private.h"
#include "xpurt_priv/defs_private_kl1.h"
#include "../kl_drv.h"
#include "../kl_profiler.h"

enum {
    KL1_REGION_TYPE_OUTBOUND = 0,
    KL1_REGION_TYPE_INBOUND  = 1,

    KL1_MATCHMODE_MEM = 0,
    KL1_MATCHMODE_BAR = 1,

    //** Per Power Domain information **//
    KL1_PDTIMER_FREQ_MS = 10,

    KL1_HBM_PAGE_BITS = 17, // Manage Memory in 128KB page
    KL1_HBM_PAGE_SIZE = 1 << KL1_HBM_PAGE_BITS,

    KL1_L3_PAGE_BITS = 12, // Manage Memory in 4KB page
    KL1_L3_PAGE_SIZE = 1 << KL1_L3_PAGE_BITS,

    KL1_HBM_CHANNEL_NUM = 8,

    KL1_EDMA_CHANNEL_NUM        = 8,
    KL1_EDMA_CHANNEL_NUM_ONE_PD = 3,

    KL1_SSE_TQ_COUNT       = 8,
    KL1_SSE_TQ_CAPACITY    = 16,
    KL1_SSE_LPTQ_CAPACITY  = 1,
    KL1_SSE_TQ_PARAM_COUNT = 32,

    KL1_DMA_LENGTH_ALIGNMENT = 256, // Minimal DMA size
    KL1_NUM_KBUF             = 2,
    KL1_DMA_KBUF_SIZE        = 1 * 1024 * 1024,
};

/***********************
 * Driver Configurations
 ***********************/
//#define DEVICE_NAME "xpu"
//#define PROC_ROOT_DIR "xpu"

//#ifdef XPU_DRIVER_TEST
//#define DRIVER_NAME "test_xpu_driver"
//#else
//#define DRIVER_NAME "xpu_driver"
//#endif

#define MCU_STATUS_UNINIT 0x11110000u
#define MCU_STATUS_NORMAL 0x11111111u

enum {
    PDTIMER_FREQ_JIFFIES = KL1_PDTIMER_FREQ_MS * HZ / 1000,
    ECC_RETRY_COUNT      = 5,
};

typedef enum {
    XDS_UNUSED   = 0,
    XDS_ERRPROBE = 100,
    XDS_RUNNING,
    XDS_ERROR,
    XDS_PRERESET,
    XDS_POSTRESET,
} XPUDeviceState;

typedef enum {
    XPDS_UNUSED  = 0,
    XPDS_RUNNING = 201,
    XPDS_LOWPOWER,
    XPDS_PAUSING,
    XPDS_PAUSED,
    XPDS_ERROR,
} XPUPDState;

// Represent a xpu session
// a new session is established everytime when the device file opened
// an pointer of this struct is set to file->private_data when opened
enum xpu_session_state {
    XSS_NORMAL,
    XSS_ERROR,
    XSS_CLOSED,
};

enum xpu_tq_state {
    XTQS_NORMAL, // this tq works normally
    XTQS_HANGUP,
};

enum xpu_task_state {
    XTS_PENDING,  // task launched by user
    XTS_RUNNING,  // task scheduled to xpu
    XTS_FINISHED, // an interrupt goes after this task has been received
    XTS_EXCEPTION,
};

typedef enum {
    MMRGN_HBM_LO,
    MMRGN_HBM_HI,
    MMRGN_L3,
    MMRGN_CODE,

    MMRGN_CNT,
} XPUMemoryRegion;

// Memory page management data
struct xpu_mem_data {
    XPUMemoryKind   kind;
    XPUMemoryRegion region;
    uint64_t        base;
    uint64_t        size;

    spinlock_t lock;

    struct xpu_pd *xpd;

    // allocator infos
    uint32_t             page_size;
    uint32_t             page_bits;
    unsigned long       *page_bitmap; // page bitmap table
    uint64_t             page_count;  // total number of pages
    uint64_t             first_free;  // current free page index
    uint64_t             page_used;   // number of used pages
    uint64_t            *free_table;  // length of each malloc
    struct xpu_session **owner_table; // owner of each malloc
};

struct xpu_wait {
    struct completion completion;
    int               errno;
    unsigned long     time;
};

enum xpu_task_type {
    XTT_CLUSTER = 0, // pure cluster kernel
    XTT_SDCDNN  = 1, // sdcdnn kernel
    XTT_INTR    = 2, // interrupt marker
    XTT_NONE    = 10000,
};

struct xpu_context {
    // info of the user process that opened this session
    int      pid;
    char     comm[TASK_COMM_LEN];
    atomic_t sess_cnt;

    atomic_t cache_mem_page_used;
    atomic_t main_page_used;

    struct list_head xpd_contexts_ent; // entry of list xpu_pd.processes
};

struct xpu_task {
    u32 token;
    int state;
    int type;
    // whether this task should be freed by tq finisher
    int free_by_tq : 1;
    // 0 for kmalloc, 1 for vmalloc
    int malloc_type : 1;

    struct xpu_session *xsess;
    struct xpu_pd      *xpd;
    struct xpu_wait    *xwait;

    u32 xsess_id;
    u32 xtq_id;

    struct xpu_kernel kernel;
    char              kernel_name[XPU_MAX_STRLEN];
    u32               params[MAX_PARAM_DWORD_SIZE_KL1];
    u32               nclusters;
    u32               ncores;

    struct list_head tq_tasks_ent;        // entry of list xpu_tq.task
    struct list_head tq_pending_ints_ent; // entry of list xpu_tq.pending_ints
    struct list_head session_tasks_ent;   // entry of list xpu_session.dispatched_tasks or
};

struct xpu_session {
    u32                    id;
    enum xpu_session_state state;
    struct mutex           lock; // must acquire this lock to modify this session
    struct file           *file;
    struct xpu_pd         *xpd;
    struct xpu_context    *xctx;
    struct xpu_tq         *xtq; // the tq which this session is currently binded to
    int                    binding_fixed;
    struct list_head       tasks; // tasks issued in this session
    u32                    tasks_cnt;

    atomic_t unfinished_cnt;

    u32 last_token;

    struct xpu_task error_xtask; // pointer to the task that caused a hw exception
    int             errno;

    struct list_head xpd_sessions_ent; // entry of list xpu_device.sessions
    //struct list_head xtq_sessions_ent; // entry of list xpu_tq.sessions
};

struct xpu_ssedma {
    struct mutex   lock;
    struct xpu_pd *xpd;
    void __iomem  *base;
    int            channel;
    unsigned long  max_cycles;
};

struct xpu_edma {
    // this lock protects dma channel from reset
    struct mutex lock;
    u8           channel;
    u8           enable;

    char      *kbuf;
    dma_addr_t dmabuf;

    struct xpu_pd *xpd;
};

// Represent a XPU compute unit
// i.e. a 16core-Cluster or an 8core-Cluster
struct xpu_cunit {
};

// Represent a XPU Task Queue
// a xpu_tq is a hardware component, which is managed by SSE
struct xpu_tq {
    u32 id;

    struct xpu_pd *xpd;
    void __iomem  *tq_base;

    int state;
    u8  enable;

    spinlock_t lock;
    u32        capacity;    // capacity of hardware xpu tq
    u32        cnt_running; // real count of tasks dispatched to hw, must smaller than capacity
    u32        cnt_all;     // count of all tasks in this xtq

    u32 cnt_successive_kernels; // count of successive non-INTR tasks
                                // if this reaches capacity, no INTR will be generated even if all
                                // tasks in this Q finish

    struct list_head  tasks;        // list of tasks currently in this tq
    struct list_head *last_running; // pointer to the last task->tq_tasks_ent which
                                    // has been dispatched to hw_tq, if not tasks or all
                                    // tasks have been dispatched, this points to tq->tasks
    //struct list_head sessions;             // list of xpu_sessions bound to this tq
    char error_xtask_name[XPU_MAX_STRLEN]; // cache task name that caused a hw exception,
                                           // used for proc display

    struct tasklet_struct tasklet_dispatch; // task queue scheduler
    atomic_t              dispatch_pending;

    struct xpu_task *nwi_for_pending;

    // statistics
    u64 st_all_task_issued;  // #tasks issued to this XPU hw tq
    u64 st_all_intr_issued;  // #intr_tasks issued to this XPU hw tq
    u64 st_all_msi_received; // #MSI_int received from this XPU hw tq
    u64 st_all_intr_handled; // #intr_tasks handled from this XPU hw tq
};

// Represents a Power Domain in an XPU device
struct xpu_pd {
    int id;         // Power Domain id
    int devfile_id; // /dev/xpu{devfile_id}

    struct xpu_device *xdev; // pointer to the XPU device this PD belongs

    unsigned int   major;
    unsigned int   minor;
    struct device *device;
    char           dev_name[XPU_MAX_STRLEN];

    XPUPDState state;
    spinlock_t state_lock;
    int        errno;

    u64           rbase;
    void __iomem *sse_base;

    spinlock_t       sessions_lock; // protect list sessions, not a single session
    struct list_head sessions;      // list of struct xpu_session
    struct list_head contexts;      // list of contexts

    // protected by sessions_lock
    atomic_t session_id_counter;
    atomic_t task_token_counter;

    //u64      except_tokens[XPD_CDNN_COUNT + XPD_CLUSTER_COUNT];
    //atomic_t except_tokens_cnt;
    u8 need_clean_etasks : 1;
    struct {
        u32 token;
        u32 cl_reason;
        u32 sd_reason;
    } cu_error[XPD_CLUSTER_COUNT + XPD_CDNN_COUNT];
    spinlock_t etasks_lock;
    // error task list
    struct list_head etasks;

    struct xpu_tq xtqs[KL1_SSE_TQ_COUNT]; // HW task queues
    atomic_t      xtqs_pending_finish_flag;
    u64           sse_errsv;

    // Memory manager
    struct xpu_mem_data mem[MMRGN_CNT];

    struct xpu_ssedma ssedma; // HW sse_dma module

    struct xpu_edma  rdch_edma[KL1_EDMA_CHANNEL_NUM_ONE_PD]; // PCIe dma module
    struct semaphore rdch_sema;
    spinlock_t       rdch_bitmap_lock; // lock to protect kbuf_bitmap
    unsigned long    rdch_bitmap;      // bitmap for kbuf
    int              rdch_enabled_cnt;

    struct xpu_edma  wrch_edma[KL1_EDMA_CHANNEL_NUM_ONE_PD]; // PCIe dma module
    struct semaphore wrch_sema;
    spinlock_t       wrch_bitmap_lock; // lock to protect kbuf_bitmap
    unsigned long    wrch_bitmap;      // bitmap for kbuf
    int              wrch_enabled_cnt;

    struct proc_dir_entry *proc_root;
    char                   proc_name[XPU_MAX_STRLEN];

    // PD regular timer
    spinlock_t timer_worklock;

    struct timer_list ktimer;
    uint64_t          last_ktimer_jiffies;

    struct hrtimer hrtimer;
    ktime_t        kt_periode;
    u64            timer_counter;

#define USE_RATIO_PN 100
    unsigned long stat_use_ratio_bitmap[BITS_TO_LONGS(USE_RATIO_PN)];
    int           stat_use_ratio_off;

    u64 prof_cost[PROFILER_COUNT];
    u32 prof_count[PROFILER_COUNT];

    int                  profiling_enabled;
    struct profiler_stat profiler[PROFILER_COUNT];

    struct mutex           xdi_cache_lock;
    struct xpu_device_info xdi_cache;
};

// Represents a xpu device
struct xpu_device {
    struct kl_device *kdev;
    // XXX(miaotianxiang): id转移到kdev->idx
    //int id; // Device index in devices list

    // XPU device PCIe location info
    int          domain;
    unsigned int bus;
    unsigned int slot;
    unsigned int func;

    u64 sn;
    u32 product_num;

    unsigned int flash_version[3];
    unsigned int cpld_version;

    struct pci_dev *pdev;
    struct module  *owner;

    // serve as both reset state and reset lock
    // only one process could perform RESET
    atomic_t in_reset;
    u8       disabled;
    u32      reset_count;

    spinlock_t brw_lock;

    // XXX(miaotianxiang): name转移到kdev->name
    //char name[XPU_MAX_STRLEN];

    // store thec error number that changes dev state from NORMAL to ERROR
    int                errno;
    struct mutex       state_lock;
    XPUDeviceState     state;
    struct completion  irq_disable_done;
    struct completion  reset_done;
    struct work_struct reset_work;

    u64 exception_bits01;
    u64 exception_bits23;

    // XXX(miaotianxiang): bar***转移到kdev->bar***
    //int  bars;                              // Cache the BAR selection result
    //void __iomem *bar_spaces[PCIE_BAR_NUM]; // Mapped bar addresses

    //struct bar_info         bar_info;
    struct iatu_region_info iatu_inbound_info;
    struct iatu_region_info iatu_outbound_info;

    u32 irq_enabled;
    u32 msi_hi;
    u32 msi_lo;
    u16 msi_data;

    void __iomem *intc_base;
    void __iomem *syscon_base;
    void __iomem *otp_base;
    void __iomem *edma_base;
    void __iomem *iatu_base;

    // Kernel buffer used in xpuhw_edma_rrl/rwl only
    spinlock_t edma_rw_lock;
    u32       *edma_rw_kbuf;
    dma_addr_t edma_rw_dma_addr;

    spinlock_t edma_rr_lock;
    u32       *edma_rr_kbuf;
    dma_addr_t edma_rr_dma_addr;

    u32 cunit_bits;

    // get this lock before comm with firmware
    struct mutex firmware_lock;

    struct xpu_pd xpd[XPU_PD_NUM];
    int           pd_num;

    u64 st_all_msi_received; // #MSI_int received from this XPU hw tq

    struct xpu_dev_monitor monitor;

    int hbm_retrain_needed : 1;
};

// XXX(miaotianxiang): 大部分转移到kl_main.c
extern int g_kl1_config_autoreset;
//extern int                    g_errprobe_cnt;
//extern struct xpu_device      g_xpu_devs[MAX_DEVICE_NUM];
//extern struct proc_dir_entry *g_xpu_proc_root;
extern int g_kl1_config_wait_mode;
//extern u64                    g_driver_load_time;

//////////////////////////////
// Helper functions and macros
//////////////////////////////

#define assert(i) BUG_ON(!(i))
#define XPU_DMA_LEN_BOUND(_len)                                                                    \
    (((_len) + KL1_DMA_LENGTH_ALIGNMENT - 1) & (~(KL1_DMA_LENGTH_ALIGNMENT - 1)))
#define ROUND_UP(_len, _alignment) (((_len) + (_alignment)-1) & (~((_alignment)-1)))
#define __PICKVAL2__(v, m, s) (((v) >> (s)) & (m))
#define __PICKVAL__(v, m, s) (((v) & (m)) >> (s))
#define GETV(v, m, s) __PICKVAL2__(v, m, s)

#define bit_field_value(regname, bfname, valname)                                                  \
    (((unsigned long)(R##regname##_##bfname##_##valname)) << (R##regname##_##bfname##_SHIFT))

#define extract_bit_field(value, regname, bfname)                                                  \
    ((value >> (R##regname##_##bfname##_SHIFT)) & (R##regname##_##bfname##_MASK))

// 10s
#define XPU_MAX_TIMEOUT_US (10 * 1000 * 1000)
#define xpu_poll_reg_timeout(reg, val, cond, sleep_us, timeout_us)                                 \
    ({                                                                                             \
        ktime_t __timeout;                                                                         \
        __timeout = ktime_add_us(ktime_get(), timeout_us);                                         \
        might_sleep_if(sleep_us);                                                                  \
        for (;;) {                                                                                 \
            (val) = readl(reg);                                                                    \
            if (cond)                                                                              \
                break;                                                                             \
            if (timeout_us && ktime_compare(ktime_get(), __timeout) > 0) {                         \
                (val) = readl(reg);                                                                \
                break;                                                                             \
            }                                                                                      \
            if (sleep_us)                                                                          \
                usleep_range((sleep_us >> 2) + 1, sleep_us);                                       \
        }                                                                                          \
        (cond) ? 0 : -ETIMEDOUT;                                                                   \
    })

#define xpu_poll_cond_timeout(cond, sleep_us, timeout_us)                                          \
    ({                                                                                             \
        ktime_t __timeout;                                                                         \
        __timeout = ktime_add_us(ktime_get(), timeout_us);                                         \
        might_sleep_if(sleep_us);                                                                  \
        for (;;) {                                                                                 \
            if (cond)                                                                              \
                break;                                                                             \
            if (timeout_us && ktime_compare(ktime_get(), __timeout) > 0) {                         \
                break;                                                                             \
            }                                                                                      \
            if (sleep_us)                                                                          \
                usleep_range((sleep_us >> 2) + 1, sleep_us);                                       \
        }                                                                                          \
        (cond) ? 0 : -ETIMEDOUT;                                                                   \
    })

#define xpu_poll_cond_timeout_spinlocked(cond, sleep_us, timeout_us, lock)                         \
    ({                                                                                             \
        ktime_t __timeout;                                                                         \
        __timeout = ktime_add_us(ktime_get(), timeout_us);                                         \
        might_sleep_if(sleep_us);                                                                  \
        for (;;) {                                                                                 \
            if (cond)                                                                              \
                break;                                                                             \
            if (timeout_us && ktime_compare(ktime_get(), __timeout) > 0)                           \
                break;                                                                             \
            spin_unlock(lock);                                                                     \
            if (sleep_us)                                                                          \
                usleep_range((sleep_us >> 2) + 1, sleep_us);                                       \
            spin_lock(lock);                                                                       \
        }                                                                                          \
        (cond) ? 0 : -ETIMEDOUT;                                                                   \
    })

#define xpu_poll_cond_timeout_spinlocked_irqsave(cond, sleep_us, timeout_us, lock, flags)          \
    ({                                                                                             \
        ktime_t __timeout;                                                                         \
        __timeout = ktime_add_us(ktime_get(), timeout_us);                                         \
        might_sleep_if(sleep_us);                                                                  \
        for (;;) {                                                                                 \
            if (cond)                                                                              \
                break;                                                                             \
            if ((timeout_us) && (ktime_compare(ktime_get(), (__timeout)) > 0))                     \
                break;                                                                             \
            spin_unlock_irqrestore(lock, flags);                                                   \
            if (sleep_us)                                                                          \
                usleep_range(((sleep_us) >> 2) + 1, (sleep_us));                                   \
            spin_lock_irqsave(lock, flags);                                                        \
        }                                                                                          \
        (cond) ? 0 : -ETIMEDOUT;                                                                   \
    })

// PCIE device 64-bits Register read
#define reg_readq(addr)                                                                            \
    ({                                                                                             \
        uint64_t val = 0;                                                                          \
        LOGL1("R %llx\n", addr);                                                                   \
        val = readq(addr);                                                                         \
        smp_rmb();                                                                                 \
        LOGL1("- %llx= 0x%llx\n", addr, val);                                                      \
        val;                                                                                       \
    })

// PCIE device 64-bits Register write
#define reg_writeq(addr, val)                                                                      \
    ({                                                                                             \
        LOGL1("W %llx= 0x%llx\n", addr, val);                                                      \
        writeq(val, addr);                                                                         \
        smp_wmb();                                                                                 \
    })

// PCIE device 32-bits Register read
#define reg_readl(addr)                                                                            \
    ({                                                                                             \
        uint32_t val = 0;                                                                          \
        LOGL1("R %llx\n", addr);                                                                   \
        val = readl(addr);                                                                         \
        smp_rmb();                                                                                 \
        LOGL1("- %llx= 0x%x\n", addr, val);                                                        \
        val;                                                                                       \
    })

// PCIE device 32-bits Register write
#define reg_writel(addr, val)                                                                      \
    ({                                                                                             \
        LOGL1("W %llx= 0x%x\n", addr, val);                                                        \
        writel(val, addr);                                                                         \
        smp_wmb();                                                                                 \
    })

#if (defined(LOG_LEVEL) && (1 >= LOG_LEVEL)) || defined(LOG_REG)
#define xreadl(base, reg)                                                                          \
    ({                                                                                             \
        u32 __tmpv;                                                                                \
        LOGL1("R " #reg " 0x%llx\n", (base) + (reg));                                              \
        __tmpv = readl((base) + (reg));                                                            \
        LOGL1("- " #reg " 0x%llx = 0x%x\n", (base) + (reg), __tmpv);                               \
        __tmpv;                                                                                    \
    })
#define xreadq(base, reg)                                                                          \
    ({                                                                                             \
        u64 __tmpv;                                                                                \
        LOGL1("R " #reg " 0x%llx\n", (base) + (reg));                                              \
        __tmpv = readq((base) + (reg));                                                            \
        LOGL1("- " #reg " 0x%llx = 0x%llx\n", (base) + (reg), __tmpv);                             \
        __tmpv;                                                                                    \
    })
#define xwritel(val, base, reg)                                                                    \
    do {                                                                                           \
        LOGL1("W " #reg " 0x%llx = 0x%x\n", (base) + (reg), (u32)(val));                           \
        writel((val), (base) + (reg));                                                             \
    } while (0)
#define xwriteq(val, base, reg)                                                                    \
    do {                                                                                           \
        LOGL1("W " #reg " 0x%llx = 0x%llx\n", (base) + (reg), (u64)(val));                         \
        writeq((val), (base) + (reg));                                                             \
    } while (0)
#else
#define xreadl(base, reg) readl((base) + (reg))
#define xreadq(base, reg) readq((base) + (reg))
#define xwritel(val, base, reg) writel((val), (base) + (reg))
#define xwriteq(val, base, reg) writeq((val), (base) + (reg))
#endif

static inline pid_t get_current_tgid(void)
{
    return current->tgid;
}

static inline void register_timer(struct timer_list *timer, void (*handler)(unsigned long),
                                  unsigned long interval, unsigned long data)
{
    if (timer->function) {
        del_timer(timer);
    }
    //timer->data     = data;
    //// interval should be ms, so interval/1000 is s
    //timer->expires  = jiffies + interval * HZ / 1000;
    //timer->function = handler;
    //add_timer(timer);
}

// XXX(miaotianxiang): 转移到kl_util.h
//static inline u32 low32(u64 v)
//{
//    return (v & 0xffffffff);
//}
//
//static inline u32 high32(u64 v)
//{
//    return ((v >> 32) & 0xffffffff);
//}
//
//static inline u64 makeu64(u32 hi, u32 lo)
//{
//    return (((u64)hi) << 32) | lo;
//}

#ifndef CONFIG_ARCH_HAS_ATOMIC_OR
static inline void __atomic_or(int i, atomic_t *v)
{
    int old;
    int new;

    do {
        old = atomic_read(v);
        new = old | i;
    } while (atomic_cmpxchg(v, old, new) != old);
}
#else
static inline void __atomic_or(int i, atomic_t *v)
{
    atomic_or(i, v);
}
#endif

// Control node handler
//int  controller_init(unsigned int major, unsigned int minor);
//void controller_del(void);

// DMA interface
int  dma_setup(struct xpu_pd *xpd);
void dma_unsetup(struct xpu_pd *xpd);
int  dma_device_to_host(struct xpu_pd *, u64 dst, u64 src, u64 sz, u64 *cycles);
int  dma_host_to_device(struct xpu_pd *, u64 dst, u64 src, u64 sz, u64 *cycles);
int  dma_device_to_device(struct xpu_pd *, u64 dst, u64 src, u64 sz, u64 *cycles);
int  dma_device_to_device_p2p(struct xpu_pd *, u64 dst, u64 src, u64 sz, u64 *cycles);

// Get xpd by xxx
struct xpu_pd *get_xpd_by_devfile_id(int dst_dev);
struct xpu_pd *get_xpd_by_minor(int minor);

// Memory management
int      xpu_mem_setup(struct xpu_pd *);
void     xpu_mem_unsetup(struct xpu_pd *);
uint64_t xpu_mem_alloc(struct xpu_session *, u64, int);
void     xpu_mem_free(struct xpu_session *, u64);
void     xpu_release_mem(struct xpu_session *);

// Interrupt handler
int  xpu_msi_register(struct xpu_device *dev);
void xpu_msi_unregister(struct xpu_device *dev);

// ioctl handlers
//int ioctl_ioc_version(void __user *argp);
int ioctl_reg_read(struct xpu_pd *, void __user *argp);
int ioctl_reg_write(struct xpu_pd *, void __user *argp);
int ioctl_memory_alloc(struct file *file, struct xpu_pd *, void __user *argp);
int ioctl_memory_free(struct file *file, struct xpu_pd *, void __user *argp);
int ioctl_memcpy_h2d(struct xpu_pd *, void __user *);
int ioctl_memcpy_d2h(struct xpu_pd *, void __user *);
int ioctl_memcpy_d2d(struct xpu_pd *, void __user *);
int ioctl_memcpy(struct xpu_pd *, void __user *);
int ioctl_host_register_kl1(struct xpu_pd *, void __user *);
int ioctl_host_unregister_kl1(struct xpu_pd *, void __user *);
int ioctl_memcpy_p2p_kl1(struct xpu_pd *, void __user *);
int ioctl_dev_hard_reset(struct xpu_pd *);
int ioctl_dev_soft_reset(struct xpu_pd *);

int ioctl_launch(struct file *file, struct xpu_pd *, void __user *argp);
int ioctl_wait(struct file *file, struct xpu_pd *, void __user *argp);
int ioctl_batchlaunch(struct file *file, struct xpu_pd *, void __user *argp);
int ioctl_proflaunch(struct file *file, struct xpu_pd *xpd, void __user *argp);
int ioctl_start_cunits(struct file *file, struct xpu_pd *xpd, void __user *argp);
int ioctl_session_bindtq(struct file *file, struct xpu_pd *xpd, void __user *argp);

int ioctl_test(struct file *file, struct xpu_device *, void __user *argp);
int ioctl_sse_launch(struct file *file, struct xpu_pd *, void __user *argp);
int ioctl_sse_wait(struct file *file, struct xpu_pd *, void __user *argp);
int ioctl_sd_launch(struct file *, struct xpu_pd *, void __user *);

int ioctl_query_bar(struct file *, struct xpu_device *, void __user *);
int ioctl_query_iatu_region(struct file *, struct xpu_device *, void __user *);
int ioctl_query_device_attr(struct file *, struct xpu_device *, void __user *);
int ioctl_query_device_info(struct file *, void __user *);

int ioctl_prof_clear(struct file *file, struct xpu_pd *xpd, void __user *argp);
//int ioctl_version(void __user *argp);
//int ioctl_changeset(void __user *argp);

// Proc entry creation and destruction
int  xpu_proc_entries_create(struct xpu_pd *xpd, struct proc_dir_entry *xpu_proc_root);
void xpu_proc_entries_destroy(struct xpu_pd *xpd, struct proc_dir_entry *xpu_proc_root);
int  xpu_drvproc_entries_create(void);
void xpu_drvproc_entries_destroy(void);

// Monitor related
int xpu_read_temp_sensor(struct xpu_device *);
int xpu_read_frequency(struct xpu_device *);
int xpu_read_power(struct xpu_device *);
int xpu_switch_i3e(struct xpu_device *);
int xpu_read_hbm_temp(struct xpu_device *);
int xpu_read_mcu_version(struct xpu_device *);
int static_pll_set(struct xpu_device *xdev, u32 pll_index);
int dynamic_pll_set(struct xpu_device *xdev, u32 is_add);

char *xpd_state_str(XPUPDState state);
int   xpd_state_to_running(struct xpu_pd *xpd);
int   xpd_state_to_lowpower(struct xpu_pd *xpd);
int   xpd_state_to_pausing(struct xpu_pd *xpd);
int   xpd_state_to_paused(struct xpu_pd *xpd);
int   xpd_state_to_error(struct xpu_pd *xpd, int errno);

//// Session Management ////
void                session_init(struct xpu_pd *);
struct xpu_session *session_create(struct xpu_pd *, struct file *);
void                session_destroy(struct xpu_pd *, struct xpu_session *);
int                 session_launch_tasks(struct xpu_session *, struct xpu_task **, int);
int                 sessions_prepare_reset(struct xpu_device *xdev);

// A device level timer
enum hrtimer_restart xpd_timer_handler(struct hrtimer *hrtimer);
void                 xpd_init_ktimer(struct xpu_pd *xpd);
void                 xpd_del_ktimer(struct xpu_pd *xpd);
int                  xpd_busy_tq_count(struct xpu_pd *);
void                 xpd_clean_etasks_ifneed(struct xpu_pd *xpd);

//// XTQ Management ////

// Init Xpu Task Queue manager
void xtq_init(struct xpu_pd *);
int  xtq_add_tasks(struct xpu_tq *, struct xpu_task **, int);
// The number of available slots in the task queue
int xtq_free_count(struct xpu_tq *);
// Handle the interrupt signal on the specific task queue
void xtq_intr_handler(struct xpu_device *dev, struct xpu_tq *tq);

void tasklet_xtq_dispatch(unsigned long data);

int  xtq_save_etask_locked(struct xpu_tq *xtq, struct xpu_task *xtask);
void ISR_xtq_on_finish(struct xpu_tq *xtq, int);
void ISR_xpd_on_exception(struct xpu_pd *xpd);
void isr_xpd_on_sse_exception(struct xpu_pd *xpd);

//// Scheduler ////
void xpu_sched_bind_sess_to_tq(struct xpu_session *xsess, u32 tq_id);
void xpu_sched_bind_sess(struct xpu_session *xsess);

//// Device control ////
#define PRERESET_USRINTR_IDX 11

int  xpu_reinit_after_reset(struct xpu_device *xdev);
int  xpu_device_reset(struct xpu_device *xdev);
bool xpu_device_disabled_or_in_reset(struct xpu_device *xdev);
void xpu_device_reset_work(struct work_struct *work);

//// Exception Info ////
struct exception_info {
    // 该异常位于哪个INTC状态寄存器
    int status_idx;
    // 状态寄存器哪个bit标识该异常
    int bit_idx;
    // 异常名称
    char *name;
    // 异常原因表
    char **reason_table;
};
extern struct exception_info g_clstr_dbgs[12];

/// Ctrl Node ///
int kl1_query_device_info_v1(struct kl_device *kdev, union xpu_device_info_v1 *i);
int kl1_query_device_proc_info(struct kl_device *kdev, struct ioc_qproc_info_in *i,
                               struct xpu_device_processes *dp);

#endif /* INCLUDE GUARD */
