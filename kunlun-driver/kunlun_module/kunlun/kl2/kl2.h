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

#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_H
#define BAIDU_XPU_RUNTIME_MODULE_KL2_H

#include <linux/atomic.h>
#include <linux/hrtimer.h>
#include <linux/idr.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/irqreturn.h>

#include "kl_drv.h"
#include "kl_mm.h"
#include "kl_cxpu.h"
#include "kl2/dma.h"
#include "kl2/dbgm.h"
#include "kl2/trace.h"
#include "kl2/kl2_regs.h"
#include "kl2/util.h"
#ifdef ENABLE_CODEC
#include "kl2/video_dec.h"
#include "kl2/video_cache.h"
#include "kl2/video_enc.h"
#include "kl2/img_proc.h"
#include "kl2/video_perf.h"
#endif

#define KL2_SRIOV_MAX_NUM_VFS 3
#define KL2_SRIOV_2VFS_L3_SIZE 0x2000000ull
#define KL2_SRIOV_3VFS_L3_SIZE 0x1000000ull
#define KL2_SRIOV_VF_L3_RESERVED_SIZE 0x1000ull

#define KL2_R300_CCIX_PORT_NUM 4
#define KL2_CCIX_LINK_SPEED_ESM_25_0_GTS 0xF
#define KL2_CCIX_LINK_WIDTH_MASK 0x3F
#define KL2_CCIX_LINK_WIDTH_SHIFT 20

#define KL2_MAX_MSI_VECTOR_CNT 32

// 机器负载高时，使用wait_event_xxx函数可能导致严重的xpu_wait长尾，故延用昆仑1 hybrid wait方式
#define USE_POLL_WAIT

#if defined(PLATFORM_PLD) || defined(PLATFORM_ZEBU)
#define IOCTL_WAIT_TIMEOUT (4000ull * 1000 * 1000)
#else
// 20s for KUNLUN2 board
#define IOCTL_WAIT_TIMEOUT (20ull * 1000 * 1000)
#endif

enum HW_INFO_OFFSET_IN_L3 {
    KL2_BOARD_INFO_L3_OFFSET = 0x0034,

    KL2_DDR_REG0220_L3_OFFSET = 0x0f00,
    KL2_DDR_REG4028_L3_OFFSET = 0x0f04,
    KL2_DDR_REG408C_L3_OFFSET = 0x0f08,

    KL2_TEMPERATURE_L3_OFFSET  = 0x0f0c,
    KL2_CLUSTER_FREQ_L3_OFFSET = 0x0f10,
    KL2_SDNN_FREQ_L3_OFFSET    = 0x0f14,
    KL2_POWER_L3_OFFSET        = 0x0f18,

    KL2_DECODER_FREQ_L3_OFFSET    = 0x0f1c,
    KL2_ENCODER_FREQ_L3_OFFSET    = 0x0f20,
    KL2_IMAGE_PROC_FREQ_L3_OFFSET = 0x0f24,
};

enum {
    KL2_DMA_KBUF_SIZE = 2 * 1024 * 1024,

    KL2_DMACH_CNT = 8,

    KL2_HWQ_CNT   = 12,
    KL2_HWQ_DEPTH = 32,

    KL2_ETASK_SAVE_CNT = 32,

    // 默认task超时阈值10s，可通过proc修改
    KL2_TASK_TIMEOUT_DETECT_THRESHOLD_IN_MS = 10000,
};

enum KL2_SSE_TASKTYPE {
    KL2_SSE_TASKTYPE_CL       = 0,
    KL2_SSE_TASKTYPE_SDNN     = 1,
    KL2_SSE_TASKTYPE_INTR     = 2,
    KL2_SSE_TASKTYPE_EVNTREC  = 3,
    KL2_SSE_TASKTYPE_EVNTWAIT = 4,
};

enum KL2_TASKTYPE {
    KL2_TASKTYPE_KERNEL,
    KL2_TASKTYPE_EVNTREC,
    KL2_TASKTYPE_EVNTWAIT,
};

enum KL2_TASKFLAG {
    KL2_TASKFLAG_FREE_PARAM = 1 << 0,
};

enum KL2_EVNT_STATE {
    KL2_EVNT_NORMAL,
    KL2_EVNT_REVOKED,
    KL2_EVNT_DESTROYED,
};

enum KL2_IRQ_PRINTK_REASON {
    KL2_IRQ_PRINTK_NA,
    KL2_IRQ_PRINTK_DOWN_UP_CLOCKING,
};

enum KL2_HWQ_STATE {
    // 用于hwq->taint_state
    KL2_HWQ_NORMAL = 0,
    KL2_HWQ_TAINT  = 1,

    // 用于hwq->regular_timer_state
    KL2_HWQ_STALL                = 2,
    KL2_HWQ_UNDERWAY_EQUALS_ZERO = 3,
};

enum KL2_SSE_HWQ_STATE {
    KL2_SSE_HWQ_IDLE,
    KL2_SSE_HWQ_BUSY,
};

enum KL2_SESS_STATE {
    KL2_SESS_NORMAL = 0,
    KL2_SESS_ERROR  = 1,
    KL2_SESS_TAINT  = 2,
};

enum KL2_UPROC_STATE {
    KL2_UPROC_NORMAL,
    KL2_UPROC_ERROR,
};

enum KL2_STATE {
    KL2_UNUSED = 0,
    // kl2_dev->state = KL2_RUNNING|KL2_ERROR
    KL2_RUNNING = 201,
    // kl2_dev->in_reset_state = 0|KL2_IN_RESET
    KL2_IN_RESET = 205,
    KL2_ERROR    = 207,
    // 不再使用，使用kl2_in_excp替代
    //KL2_IN_EXCEPTION = 208,
};

enum KL2_BOARD_ID {
    KL2_BOARD_ID_NA,
    KL2_BOARD_ID_R100,
    KL2_BOARD_ID_R200,
    KL2_BOARD_ID_R300,
    KL2_BOARD_ID_R200_8F,
    KL2_BOARD_ID_R200_8FS,
    KL2_BOARD_ID_R200_DEBUG_BOARD,
    KL2_BOARD_ID_R420,
    KL2_BOARD_ID_RG800,
    KL2_BOARD_ID_RG800_PRO,
    KL2_BOARD_ID_RM80,
    NUM_KL2_BOARD_ID,
};

enum KL2_SRIOV_CONF_ID {
    KL2_SRIOV_CONF_ID_NA,
    KL2_SRIOV_CONF_ID_SRIOV_OFF,
    KL2_SRIOV_CONF_ID_1VF,
    KL2_SRIOV_CONF_ID_2VF,
    KL2_SRIOV_CONF_ID_3VF,
    NUM_KL2_SRIOV_CONF_ID,
};

enum KL2_SRIOV_FUNC_ID {
    KL2_SRIOV_FUNC_ID_NA,
    KL2_SRIOV_FUNC_ID_SRIOV_OFF,
    KL2_SRIOV_FUNC_ID_PF,
    KL2_SRIOV_FUNC_ID_VF_0,
    KL2_SRIOV_FUNC_ID_VF_1,
    KL2_SRIOV_FUNC_ID_VF_2,
    NUM_KL2_SRIOV_FUNC_ID,
};

struct kl2_session;

union kl2_sse_task_desc {
    struct {
        u32 reserved0 : 4;
        u32 type : 4;
        u32 nclusters : 8;
        u32 ncores : 8;
        u32 reserved1 : 8;
        u32 token : 32;
        u32 codelen : 32;
        u32 param0 : 16;
        u32 param1 : 16;
        u64 code_addr : 48;
        u32 param2 : 16;
        u64 param_addr : 48;
        u32 param3 : 16;
    } kernel;

    struct {
        u32 reserved0 : 4;
        u32 type : 4;
        u32 wait_vstream_id : 4;
        u32 reserved1 : 20;
        u32 token : 32;
        u64 record_seq : 64;
        u64 reserved2 : 64;
        u64 reserved3 : 64;
    } ctrl;

    u32 r32[8];

    u64 r64[4];
};

struct kl2_event {
    int                     id;
    struct kl2_userprocess *uproc;
    struct list_head        uproc_node;
    atomic_t                state;

    // <sess, seq> describe a sse event, lock guarantees the atomicity of
    // updating <sess, seq>
    struct mutex    lock;
    struct kl2_hwq *rec_hwq;
    u64             rec_hwq_evnt_seq;

    atomic_t    rec_cnt;
    atomic_t    fin_cnt;
    struct xref xref;
};

struct kl2_task {
    union kl2_sse_task_desc desc;
    int                     type;
    int                     flag;

    char kernel_name[XPU_MAX_STRLEN];
    u32  params[MAX_PARAM_DWORD_SIZE];

    struct kl2_session *sess;
    struct list_head    hwq_node;
    struct kl2_event   *evnt;

    int hwq_id;
    int sess_id;
    int pid;
};

// 虚拟id和debug寄存器
struct kl2_cluster_debug_info {
    u32 cl_virt_id;
    u32 cl_debug_regs[KL2_REG_CLUSTER_DEBUG_ARRAY_SIZE];
};

struct kl2_sdnn_debug_info {
    u32 sdnn_cl_virt_id;
    u32 sdnn_cl_debug_regs[KL2_REG_CLUSTER_DEBUG_ARRAY_SIZE];
    u32 sdnn_dmai0_debug_regs[KL2_REG_SDNN_DMAI0_DEBUG_ARRAY_SIZE];
    u32 sdnn_dmai1_debug_regs[KL2_REG_SDNN_DMAI1_DEBUG_ARRAY_SIZE];
    u32 sdnn_ds0_debug_regs[KL2_REG_SDNN_DS0_DEBUG_ARRAY_SIZE];
    u32 sdnn_ds1_debug_regs[KL2_REG_SDNN_DS1_DEBUG_ARRAY_SIZE];
    u32 sdnn_mac_debug_regs[KL2_REG_SDNN_MAC_DEBUG_ARRAY_SIZE];
    u32 sdnn_ew_debug_regs[KL2_REG_SDNN_EW_DEBUG_ARRAY_SIZE];
    u32 sdnn_rs_debug_regs[KL2_REG_SDNN_RS_DEBUG_ARRAY_SIZE];
    u32 sdnn_dmao_debug_regs[KL2_REG_SDNN_DMAO_DEBUG_ARRAY_SIZE];
    u32 sdnn_sch_debug_regs[KL2_REG_SDNN_SCH_DEBUG_ARRAY_SIZE];
    u32 sdnn_dsmux_debug_regs[KL2_REG_SDNN_DSMUX_DEBUG_ARRAY_SIZE];
};

struct kl2_etask {
    struct kl2_task task;
    // 增加更多debug信息
    char                          comm[TASK_COMM_LEN];
    u32                           cl_excp_st[KL2_CLUSTER_MAX_COUNT];
    struct kl2_cluster_debug_info cl_debug_info[KL2_CLUSTER_MAX_COUNT];
    u32                           sdnn_cl_excp_st[KL2_SDNN_MAX_COUNT];
    u32                           sdnn_sd_excp_st[KL2_SDNN_MAX_COUNT];
    struct kl2_sdnn_debug_info    sdnn_debug_info[KL2_SDNN_MAX_COUNT];
};

struct kl2_hwq {
    struct kl2_device *kl2_dev;
    int                id;
    int                enable;

    // this lock is used for launching
    spinlock_t lock;
    // {
    struct list_head pt_list;
    struct list_head rt_list;
    struct list_head ft_list;
    u32              cnt_running;
    u32              cnt_all;
    // }

    // protected by kl2_dev->hwq_binding_lock
    struct list_head session_list;
    int              session_cnt;
    // 被kl2_release或excp work标记，表示hwq被taint需stall+reset
    atomic_t taint_state;
    // timer内部使用，等待hwq stall+可被reset时机
    atomic_t regular_timer_state;

    struct list_head exception_list;

    atomic64_t evnt_seq;

    struct work_struct finish_work;
};

struct kl2_p2p_info {
    u64 vaddr;
    u64 size;
    u64 pcie_addr;

    struct list_head uproc_node;
};

struct kl2_userprocess {
    struct kl2_device *kl2_dev;
    int                pid;
    pid_t              ns_pid;
    struct pid        *task_pid;
    char               comm[TASK_COMM_LEN];
    struct xref        xref;

    spinlock_t lock;
    atomic64_t mem_used_pgcnt[XPU_MEM_COUNT];

    rwlock_t         sg_minfo_lock;
    struct rb_root   sg_minfo_rb;
    struct list_head p2p_list;

    // CXPU
    kl_cxpu_instance_t *cxpu_instance;

    struct list_head event_list;
    atomic_t         state;
};

struct kl2_session {
    struct kl2_device      *kl2_dev;
    int                     id;
    struct mutex            lock;
    struct kl2_userprocess *uproc;
    // revoke_task_from_err_session_locked前使用，表示sess中task将被清理
    atomic_t state;
    // kl2_release和timer中使用，表示sess已被TAINT，但还不必立即清理task
    atomic_t taint_state;
    int      errno;
    int      taint_errno;

    atomic_t    unfinished_cnt;
    struct xref xref;

    struct kl2_hwq  *hwq;
    struct list_head hwq_node;

    wait_queue_head_t wait_queue;

    struct kl2_debug_master *dbgm;
};

struct kl2_df_spec {
    u32 valid;
    u32 dmach_bits;
    u32 hwq_bits;
    u32 cl_bits;
    u32 sdnn_bits;
    u32 enc_bits;
    u32 dec_bits;
    u32 imgproc_bits;

    struct kl_mm_info *(*kl_mm_info_fn)(struct kl2_device *);
    struct kl_mm_info *(*kl_vf_mm_info_fn)(struct kl2_device *);
};

struct kl2_otp_info {
    u32 sdnn_avail_bits;
    u32 cl_avail_bits;
    u32 dec_avail_bits;
};

struct kl2_iomem_base {
    // BAR 0
    void __iomem *dma_base;

    // BAR 2
    void __iomem *intc_base;
    void __iomem *sse_base;
    void __iomem *aes_base;
    void __iomem *vac_base;
    void __iomem *sdnn_base;
    void __iomem *cluster_base;
    void __iomem *syscon0_base;
    void __iomem *syscon1_base;
    void __iomem *dbgm_base;
    void __iomem *video_dec_base;
    void __iomem *video_enc_base;
    void __iomem *img_proc_base;
    void __iomem *gddr_base;
    void __iomem *otp_base;
    void __iomem *ccix_base;

    // BAR 4
    void __iomem *l3_base;
};

// structure for KL2 base hardware info which stored in L3(addr: 0x0 ~ 0x1000)
typedef struct {
    volatile u32 magic;         // 0x0
    volatile u32 fw_version[3]; // 0x4
    volatile u32 SN[8];         // 0x10
    volatile u32 cpld_version;  // 0x30
    volatile u32 board_info;    // 0x34
    volatile u32 ph1;           // 0x38
    volatile u32 ph2;           // 0x3c

    volatile u32 host_cmd;        // 0x40
    volatile u32 dev_cmd;         // 0x44
    volatile u32 reserve[14];     // 0x48
    volatile u32 host_buffer[16]; // 0x80
    volatile u32 dev_buffer[16];  // 0xc0

    // used for key exchange
    volatile char data[0xe00]; // 0x100

    // 0xf00 ~ 0x1000 区间预留给驱动暂存数据
    volatile u32 ddr_reg0220_val; // 0xf00
    volatile u32 ddr_reg4028_val; // 0xf04
    volatile u32 ddr_reg408c_val; // 0xf08
    volatile u32 temperature;     // 0xf0c
    volatile u32 cluster_freq;    // 0xf10
    volatile u32 sdnn_freq;       // 0xf14
    volatile u32 power;           // 0xf18
    volatile u32 decoder_freq;    // 0xf1c
    volatile u32 encoder_freq;    // 0xf20
    volatile u32 image_proc_freq; // 0xf24
} KL2_BASE_HW_INFO;

struct kl2_device {
    struct kl_device     *kdev;
    struct kl_inode      *kinode;
    struct kl2_iomem_base iomem_base;
    int                   errno;
    struct mutex          big_global_lock;

    struct {
        // 驱动加载时初始化
        int board;         // enum KL2_BOARD_ID_***
        int sriov_conf;    // enum KL2_SRIOV_CONF_ID_***
        int sriov_func_id; // enum KL2_SRIOV_FUNC_ID_***
        int sriov_num_vfs; // sriov vf number
        u32 sn[8];
        u32 pn[2];
        u32 fw[3];
        u32 cpld;

        // 在hrtimer中更新
        u32 temperature;
        u32 sdnn_freq;
        u32 cluster_freq;
        u32 power;
        u32 decoder_freq;
        u32 encoder_freq;
        u32 image_proc_freq;

        // 事件触发时更新
        u64 ecc_sbe_count; // ECC single-bit(correctable) count
        u64 ecc_dbe_count; // ECC double-bit(uncorrectable) count
    } dev_info;
    struct {
        u32         ddr_x8 : 1;
        u32         ecc_on : 1;
        u32         vendor : 2;
        const char *vendor_str;
        u32         max_link_speed;
        u32         nchannel;
        u32         link_speed;
    } ddr_conf;
    struct {
        u32  card_id;
        bool port_link_valid[KL2_R300_CCIX_PORT_NUM];
        u32  port_link_width[KL2_R300_CCIX_PORT_NUM];
        u32  port_link_speed[KL2_R300_CCIX_PORT_NUM];
    } ccix_info;
    struct kl2_df_spec  spec;
    struct kl_mm_info  *mm_info;
    struct kl2_otp_info otp_info;

    struct mutex uproc_session_lock;
    struct idr   uproc_idr;
    struct idr   session_idr;

    struct mutex event_idr_lock;
    struct idr   event_idr;

    struct use_ratio ur;

    // KL2_RUNNING/KL2_ERROR
    atomic_t state;
    // 0/KL2_IN_RESET，将KL2_IN_RESET单独变量表示
    atomic_t in_reset_state;
    atomic_t task_token;
    // probe后的默认cuen，包含最多可用cu
    u32 default_cuen;
    // 实际cuen，默认等同default_cuen，可经由proc文件修改
    u32 cuen;
    // 控制寄存器读写，避免与soft_reset，cu reset等构成冲突
    atomic_t reg_lock;

    // 以下寄存器可能存在并发访问(例如在sse disable了某个cluster a，另一处又enable了cluster b)，用sc_lock保证全局有序
    spinlock_t sc_lock;
    // cuen禁用的cu
    u32 cuen_cu_disable_mask;
    // 异常处理临时禁用的cu，与cuen_cu_disable_mask组合后形成最终sse_cu_disable_mask
    u32 exception_cu_disable_mask;
    struct {
        u32 cl_excp_mask[KL2_CLUSTER_MAX_COUNT];
        u32 sdnn_cl_excp_mask[KL2_SDNN_MAX_COUNT];
        u32 sdnn_sd_excp_mask[KL2_SDNN_MAX_COUNT];
        u32 intc_int_mask[15];
        u32 intc_host_mask[15];
        u32 sse_cu_disable_mask;
    } reg_shadow;

    // 记录过去触发异常或超时的task，最多KL2_ETASK_SAVE_CNT个，可通过/proc/xpu查看
    spinlock_t       etasks_lock;
    atomic_t         etasks_cur;
    struct kl2_etask etasks[KL2_ETASK_SAVE_CNT];

    // 超时检测相关
    struct {
        int detect_threshold_in_ms;
        struct {
            bool    busy;
            u32     token;
            ktime_t record_ktime;
            u32     last_virt_cl_id[KL2_CLUSTER_MAX_COUNT];
            u32     last_virt_sdnn_cl_id[KL2_SDNN_MAX_COUNT];
        } hwq[KL2_HWQ_CNT];
    } task_timeout_detect;

    // 异常处理相关
    struct work_struct handle_exception_work;
    // 均在timer或wq中临时使用，无并发访问，无需保护
    struct {
        // 在wq的hwq->lock critical section中复制etask，避免spinlock嵌套
        struct kl2_task saved_etask;
        u32             excp_work_seq;
        u32             excp_work_running;
        // 在hrtimer中使用，避免占用过多内核栈空间
        u32           hwq_timeout_token[KL2_HWQ_CNT];
        unsigned long hwq_timeout_cl_sd_st[KL2_HWQ_CNT];
        u32           cl_timeout_token[KL2_CLUSTER_MAX_COUNT];
        u32           sd_timeout_token[KL2_SDNN_MAX_COUNT];
        u32           regular_timer_seq;
    } exception_stash;
    struct {
        atomic_t excp_cnt;
        atomic_t taint_2_reset_cnt;
        atomic_t taint_cnt;
        u32      token;
        // TODO(miaotianxiang):
        // 记录下的异常信息有可能被后续异常覆盖，如sse调度两个子任务到同一个cluster，而两个子任务都发生了异常
        u32                           cl_excp_st[KL2_CLUSTER_MAX_COUNT];
        struct kl2_cluster_debug_info cl_debug_info[KL2_CLUSTER_MAX_COUNT];
        u32                           sdnn_cl_excp_st[KL2_SDNN_MAX_COUNT];
        u32                           sdnn_sd_excp_st[KL2_SDNN_MAX_COUNT];
        struct kl2_sdnn_debug_info    sdnn_debug_info[KL2_SDNN_MAX_COUNT];
    } exception_board_hwq[KL2_HWQ_CNT];
    struct {
        atomic_t                      excp_cnt;
        atomic_t                      reset_cnt;
        u32                           token;
        atomic_t                      timeout_cnt;
        atomic_t                      timeout_reset_cnt;
        u32                           timeout_token;
        u32                           cl_excp_st;
        u32                           cl_excp_mask;
        struct kl2_cluster_debug_info cl_debug_info;
    } exception_board_cluster[KL2_CLUSTER_MAX_COUNT];
    struct {
        atomic_t                   excp_cnt;
        atomic_t                   reset_cnt;
        u32                        token;
        atomic_t                   timeout_cnt;
        atomic_t                   timeout_reset_cnt;
        u32                        timeout_token;
        u32                        sdnn_cl_excp_st;
        u32                        sdnn_sd_excp_st;
        u32                        sdnn_cl_excp_mask;
        u32                        sdnn_sd_excp_mask;
        struct kl2_sdnn_debug_info sdnn_debug_info;
    } exception_board_sdnn[KL2_SDNN_MAX_COUNT];

    struct mutex             hwq_binding_lock;
    unsigned long            hwq_bitmap;
    struct kl2_hwq           hwq[KL2_HWQ_CNT];
    struct workqueue_struct *hwq_wq;

    ktime_t             ur_ktime;
    struct hrtimer      ur_timer;
    bool                ur_timer_valid;
    ktime_t             regular_ktime;
    struct hrtimer      regular_timer;
    bool                regular_timer_valid;
    bool                regular_timer_toggle;
    spinlock_t          sriov_mbox_lock;
    struct task_struct *sriov_mbox_kthread;

    struct proc_dir_entry *proc_root;
    char                   proc_name[XPU_MAX_STRLEN];
    bool                   proc_debug_toggle;

    struct dma_engine dma_engine;
    struct kl_mm      mm;
    struct {
        bool valid;
        u64  peer_bar4_iova[MAX_DEVICE_NUM];
        struct {
            bool valid;
            int  devfile_id;
            u64  start;
            u64  end;
            u64  iova;
        } peer_iova[MAX_DEVINODE_NUM][2];
    } p2p;

    struct msix_entry msix_entries[32];
    int               msix_nvec;
    int               msi_en : 1;
    int               msi_nvec;
    bool              multi_msi_vector;
    int               main_msi_vector;

    struct work_struct irq_printk_work;
    atomic_t           irq_printk_data_used;
    int                irq_printk_reason;
    u32                irq_printk_u32_data[8];

#ifdef ENABLE_CODEC
    pvdec_device_t    video_dec;
    pvenc_device_t    video_enc;
    pimgproc_device_t image_proc;
    struct hrtimer    video_perf_hrtimer;
#endif

    struct kl2_debug_master dbgm; // 一个设备一个
};

extern struct file_operations kl2_fops;

static inline bool is_vf_id(enum KL2_SRIOV_FUNC_ID sriov_func_id)
{
    if (sriov_func_id >= KL2_SRIOV_FUNC_ID_VF_0 && sriov_func_id <= KL2_SRIOV_FUNC_ID_VF_2) {
        return true;
    } else {
        return false;
    }
}

static inline bool is_pf_id(enum KL2_SRIOV_FUNC_ID sriov_func_id)
{
    return (sriov_func_id == KL2_SRIOV_FUNC_ID_PF);
}

/// SPEC ///
int                kl2_get_df_spec(struct kl2_device *kl2_dev);
struct kl_mm_info *kl2_get_mm_info(struct kl2_device *kl2_dev);
struct kl_mm_info *kl2_get_vf_mm_info(struct kl2_device *kl2_dev);

/// IRQ ///
irqreturn_t kl2_handle_irq(int irq, void *instance);
int         kl2_msix_register(struct kl2_device *kl2dev);
void        kl2_msix_unregister(struct kl2_device *kl2dev);
int         kl2_msi_register(struct kl2_device *kl2_dev);
void        kl2_msi_unregister(struct kl2_device *kl2_dev);
void        kl2_irq_printk_work_func(struct work_struct *work);

/// HW ///
int  kl2_plda_addr_trans_init(struct kl2_device *kl2_dev, void __iomem *pcie_base);
int  kl2_plda_dma_init(struct kl2_device *kl2_dev, int chbits);
int  kl2_plda_dma_uninit(struct kl2_device *kl2_dev);
int  kl2_gddr_init(struct kl2_device *kl2_dev);
int  kl2_gddr_interrupt_mask_init(struct kl2_device *kl2_dev);
int  kl2_compute_unit_init(struct kl2_device *kl2_dev);
int  kl2_virt_get_numvfs(struct kl2_device *kl2_dev);
int  kl2_virt_set_numvfs(struct kl2_device *kl2_dev, int num_vfs);
void kl2_reg_lock(struct kl2_device *kl2_dev);
int  kl2_reg_trylock(struct kl2_device *kl2_dev);
void kl2_reg_unlock(struct kl2_device *kl2_dev);

/// SSE ///
int  kl2_sse_init(struct kl2_device *kl2_dev);
void kl2_sse_cuen_update(struct kl2_device *kl2_dev, u32 new_cuen);
void kl2_sse_write_desc_locked(struct kl2_device *kl2_dev, void __iomem *sse_base,
                               union kl2_sse_task_desc *desc, int hwq_id);
int  kl2_sse_hwq_intr_count(struct kl2_hwq *);
u32  kl2_sse_hwq_last_cycles(struct kl2_hwq *hwq);
u32  kl2_sse_hwq_underway(struct kl2_hwq *hwq);
void kl2_sse_hwq_reset(struct kl2_device *kl2_dev, int hwq_id);
void kl2_sse_hwq_stall(struct kl2_device *kl2_dev, int hwq_id);
void kl2_sse_hwq_stall_all(struct kl2_device *kl2_dev);
void kl2_sse_hwq_unstall(struct kl2_device *kl2_dev, int hwq_id);
void kl2_sse_hwq_unstall_all(struct kl2_device *kl2_dev);
void kl2_sse_cluster_force_done(struct kl2_device *kl2_dev, int cl_id);
void kl2_sse_sdnn_force_done(struct kl2_device *kl2_dev, int sdnn_id);
u32  kl2_sse_cluster_map_hwq(struct kl2_device *kl2_dev, u32 user, u32 cl_id);
u32  kl2_sse_sdnn_map_hwq(struct kl2_device *kl2_dev, u32 user, u32 sdnn_id);
u32  kl2_sse_get_cluster_number(struct kl2_device *kl2_dev, u32 user);
u32  kl2_sse_get_sdnn_number(struct kl2_device *kl2_dev, u32 user);
u32  kl2_sse_get_hwq_number(struct kl2_device *kl2_dev, u32 user);

/// INTC ///
int  kl2_intc_setup(struct kl2_device *kl2_dev);
void kl2_intc_unsetup(struct kl2_device *kl2_dev);
void kl2_intc_disable_msi(struct kl2_device *kl2_dev);
void kl2_intc_enable_msi(struct kl2_device *kl2_dev);
void kl2_intc_toggle_msi(struct kl2_device *kl2_dev, int irq);
void kl2_intc_toggle_msix(struct kl2_device *kl2_dev, int irq);
u32  kl2_intc_status(struct kl2_device *kl2_dev, int idx);

/// SESSION ///
void kl2_mm_malloc_update_stat_cb(u64 addr, u64 size, int kind, void *owner, struct kl_memory *mem);
void kl2_mm_free_update_stat_cb(u64 addr, u64 size, int kind, void *owner, struct kl_memory *mem);
int  kl2_session_bind_hwq(struct kl2_session *sess);
int  kl2_create_session(struct kl2_device *kl2_dev, struct kl2_session **sess_ptr);
void kl2_destroy_session_ref(struct kref *kref);
int  kl2_session_add_task(struct kl2_session *sess, struct kl2_task *task);
void kl2_session_free_task(struct kl2_task *task);
void kl2_session_mark_error(struct kl2_session *sess, int errno, int taint);
void kl2_session_taint(struct kl2_session *sess, int errno);
bool kl2_session_state_normal(struct kl2_session *sess);
int  kl2_session_wait_until_finished(struct kl2_session *sess);

/// HWQ ///
void                 kl2_hwq_init(struct kl2_device *kl2_dev);
void                 __kl2_hwq_dispatch_locked_nocheck(struct kl2_hwq *hwq);
void                 kl2_hwq_dispatch_locked(struct kl2_hwq *hwq);
void                 kl2_handle_hwq_intr(struct kl2_hwq *hwq, int cnt);
enum hrtimer_restart kl2_regular_timer_func(struct hrtimer *hrtimer);

/// EVENT ///
void kl2_destroy_event_locked_ref(struct kref *kref);
void kl2_destroy_event_ref(struct kref *kref);
int  ioctl_event_create(struct kl2_session *sess, void __user *argp);
int  ioctl_event_destroy(struct kl2_session *sess, void __user *argp);
int  ioctl_event_record(struct kl2_session *sess, void __user *argp);
int  ioctl_event_stream_wait(struct kl2_session *sess, void __user *argp);
int  ioctl_event_wait(struct kl2_session *sess, void __user *argp);

/// DEVICE STATE ///
const char *kl2_device_state_str(int state);
int         kl2_get_state(struct kl2_device *kl2_dev);
int         kl2_get_in_reset_state(struct kl2_device *kl2_dev);
void        kl2_set_state(struct kl2_device *kl2_dev, int state);
int         kl2_dev_soft_reset(struct kl2_device *kl2_dev, int mode, int fast_reset);
int         kl2_dev_reinit_gddr(struct kl2_device *kl2_dev);
int         kl2_dev_set_numvfs(struct kl2_device *kl2_dev, int num_vfs);
int         kl2_ccix_link_speed_check(struct kl2_device *kl2_dev, u32 r300_bitmap);
int         kl2_ccix_link_width_check(struct kl2_device *kl2_dev, u32 r300_bitmap);
int         kl2_ccix_link_port_mode_switch_if_necessary(struct kl2_device *kl2_dev, u32 r300_bitmap,
                                                        struct kl2_device **r300_devs, int r300_count);

/// Control Node ///
int kl2_query_device_info_v1(struct kl_device *kdev, union xpu_device_info_v1 *i);
int kl2_query_device_proc_info(struct kl_device *kdev, struct ioc_qproc_info_in *i,
                               struct xpu_device_processes *dp);

/// IOCTL ///
int  kl2_map_memcpy_p2p_direct(struct kl2_device *kl2_dev);
int  kl2_unmap_memcpy_p2p_direct(struct kl2_device *kl2_dev);
long kl2_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#ifdef ENABLE_CODEC
/// Video ///
int  kl2_video_init(struct kl2_device *kl2_dev);
void kl2_video_destroy(struct kl2_device *kl2_dev);
#endif

#endif
