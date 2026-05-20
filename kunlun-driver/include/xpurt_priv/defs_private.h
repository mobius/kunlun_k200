/// \file   defs_private.h
/// \brief  Shared definitions used in both Runtime and Driver.
///         This is a private header file that will not be released.
/// \author hanjinchen@baidu.com
/// \copyright (C) 2018 Baidu, Inc
///
#ifndef BAIDU_XPU_RUNTIME_INCLUDE_XPU_DEFS_PRIVATE_H
#define BAIDU_XPU_RUNTIME_INCLUDE_XPU_DEFS_PRIVATE_H

#include "xpu/defs.h"

#define XPUCTL_NODE_DEVNAME "xpuctrl"

#ifndef HIGH32
#define MAKE64(hi, lo) ((((uint64_t)(hi) & 0xFFFFFFFF) << 32) | ((lo) & 0xFFFFFFFF))
#define HIGH32(v)  (uint32_t)(((v) >> 32) & 0xFFFFFFFFu)
#define LOW32(v) (uint32_t)((v) & 0xFFFFFFFFu)
#endif

#define XMP_TYPE_NRBITS     4
#define XMP_TYPE_MASK       ((1 << XMP_TYPE_NRBITS)-1)
#define XMP_TYPE_SHIFT      60
#define XMP_TYPE(off)       ((off >> XMP_TYPE_SHIFT) & XMP_TYPE_MASK)

#define XMP_ADDR_NRBITS     48
#define XMP_ADDR_MASK       ((1ull << XMP_ADDR_NRBITS)-1)
#define XMP_ADDR_SHIFT      0
#define XMP_ADDR(off)       ((off >> XMP_ADDR_SHIFT) & XMP_ADDR_MASK)

#define XMP_BARID_NRBITS    3
#define XMP_BARID_MASK      ((1 << XMP_BARID_NRBITS)-1)
#define XMP_BARID_SHIFT     (XMP_TYPE_SHIFT - XMP_BARID_NRBITS)
#define XMP_BARID(off)      ((off >> XMP_BARID_SHIFT) & XMP_BARID_MASK)

#define MMAP_TYPE_MEM      0
#define MMAP_TYPE_BAR      1
#define MMAP_TYPE_MALLOC   2
#define MMAP_TYPE_SHARE    3

#define KB 1024
#define MB (1024*KB)
#define GB (1024ull*MB)

enum {
    /// 每个设备上最多有多少个用户进程会被smi工具打印出来
    PROCESS_COMM_LEN = 16,
    DEVICE_MAX_PROCESS_PRINT_COUNT = 100,

    /// #PowerDomain one XPU contains
    XPU_PD_NUM    = 2,

    // Two minors for each XPU device, an additional one for ctrlnode
    // @deprecated
    XPU_DEVNO_NUM = MAX_DEVICE_NUM * XPU_PD_NUM + 1,

    // how many inodes could be used for kunlun device
    MAX_DEVINODE_NUM = MAX_DEVICE_NUM * XPU_PD_NUM,

    // how many inodes will kunlun driver occupy
    // additional one for control node
    INODE_NUM        = MAX_DEVINODE_NUM + 1,

    PCIE_BAR_NUM = 6,

    MAX_PARAM_DWORD_SIZE_KL1 = 29,
    MAX_PARAM_BYTE_SIZE_KL1  = MAX_PARAM_DWORD_SIZE_KL1 * 4,

    MAX_PARAM_DWORD_SIZE = 128,
    MAX_PARAM_BYTE_SIZE = MAX_PARAM_DWORD_SIZE * 4,

    XPD_CLUSTER_COUNT = 4,
    XPD_CDNN_COUNT    = 4,

    KL2_CLUSTER_MAX_COUNT = 8,
    KL2_SDNN_MAX_COUNT    = 6,
    KL2_ENCODER_MAX_COUNT = 3,
    KL2_DECODER_MAX_COUNT = 9,
    KL2_IMGPROC_MAX_COUNT = 6,
};

// Extended memory kinds to enum XPUMemoryKind
enum {
    /// Code memory
    XPU_MEM_CODE = XPU_USER_ALLOCABLE_MEM_COUNT,
    /// Parameter memory
    XPU_MEM_PARAM,
    /// Printf memory
    XPU_MEM_PRINTF,
    /// Unused/Reserved
    XPU_MEM_RESERVED,
    /// Total Count
    XPU_MEM_COUNT,
};

enum {
    // Low priority
    XPU_PLVL0,
    // High priority
    XPU_PLVL1,
    XPU_NR_PLVL,
    XPU_PLVL_DEFAULT = XPU_PLVL1,
};

/// \brief XPU Kernel type
enum kernel_type {
    /// XPU Cluster kernel
    KT_CLUSTER = 0,
    /// XPU SDNN kernel
    KT_SDCDNN = 1,
};

/// \brief Place of XPU kernel binary
enum kernel_place {
    /// XPU kernel binary locates on CPU memory
    KP_CPU = 0,
    /// XPU kernel binary locates on XPU memory
    KP_XPU = 1,
};

/// \brief XPU Kernel
struct xpu_kernel {
    /// Combination of kernel place and type:
    /// [31:16] kernel place, KP_CPU or KP_XPU
    /// [15:0]  kernel type, KT_CLUSTER or KT_SDCDNN
    uint32_t type : 16;
    uint32_t place : 16;
    /// kernel code address on CPU Memory
    uint64_t code_addr;
    /// kernel code size in bytes
    uint32_t code_byte_size;
    /// initial program counter
    uint32_t code_pc;
    /// dword size kernel needed to transfer params
    /// essentially, this is the count of param registers needed
    uint32_t param_dword_size;
    /// kernel code hash, for cache indexing
    uint64_t hash;
    /// (maybe mangled) function name
    const char* name;
    /// private data structure used by xpu runtime
    void *rt_private;
};

/// XPU memory specification
struct mem_spec {
    uint64_t base;
    uint64_t size;
    uint64_t reserve;
    XPUMemoryKind kind;
};

struct xpu_dev_monitor {
    // temperature sensors represents:
    //     pd0:cdnn3/hbm
    //     pd0:cdnn2/l3
    //     pd1:cdnn0/hbm
    //     pd1:cdnn1/l3
    uint32_t temp[4];

    //  freqency represents:
    //     l3
    //     noc/dma/pcie/hbm
    //     cluster/sdcdnn_xpu
    //     sdcdnn
    //     hbm0
    //     hbm1
    uint32_t freq[6];
    uint32_t freq_raw[6];

    // power value
    uint32_t power;

    // hbm temperature, pd0 & pd1
    uint32_t hbm_temp[2];
};

struct xpu_mcu_info {
    uint32_t flash_version[3];
    uint32_t cpld_version;
};

struct xpu_devfile_info {
    // /dev/xpu{id}
    int id;
    // kunlun board index in the system, normally user invisible
    int board_id;
    // pd idx
    int onboard_idx;

    int domain;
    unsigned int bus;
    unsigned int slot;
    unsigned int func;
    unsigned int sn;
    uint32_t product_num;

    uint32_t l3_page_used;
    uint32_t l3_page_all;
    uint32_t l3_page_size;
    uint32_t hbm_page_used;
    uint32_t hbm_page_all;
    uint32_t hbm_page_size;
    uint32_t use_ratio_numerator;
    uint32_t use_ratio_denominator;

    int process_count;
    struct {
        int pid;
        int stream_count;
        char comm[PROCESS_COMM_LEN];
    } process[DEVICE_MAX_PROCESS_PRINT_COUNT];
};

struct xpu_dev_info {
    int id;

    int domain;
    unsigned int bus;
    unsigned int slot;
    unsigned int func;
    unsigned int sn;

    struct xpu_dev_monitor  monitor;
    struct xpu_mcu_info  mcu_info;
    struct xpu_devfile_info file[XPU_PD_NUM];
};

struct xpu_dev_info_all {
    int dev_count;
    struct xpu_dev_info devs[MAX_DEVICE_NUM];
};

// New version of device query interface
enum {
    DEVINFO_MAGIC_V0 = 6545408,  // 0x63e000
};

// * introduced in xpurt 3.0
// This is used in a new IOCTL_QUERY_DEVINFO, which is called on a device
// file like /dev/xpu0 instead of a control node, thus this struct need only
// contains info about one specific device node.
struct xpu_device_info {
    // A magic number indicate driver's query interface version
    // - DEVINFO_MAGIC_V0
    int magic_version;
    // Device board model
    int model;
    // /dev/xpu{id}
    int id;
    // Kunlun board index in the system, dertermined by probe order
    int board_idx;
    // Chip index on the board
    // Some model may contains multiple chips on a single board
    int chip_idx;
    // PCIE bus address
    int domain;
    unsigned int bus;
    unsigned int slot;
    unsigned int func;
    // Low 32-bit of Serial Number
    uint32_t sn;
    uint32_t product_num;
    // l3 memory
    uint32_t cache_mem_page_used;
    uint32_t cache_mem_page_all;
    uint32_t cache_mem_page_size;
    // memory
    uint32_t main_mem_page_used;
    uint32_t main_mem_page_all;
    uint32_t main_mem_page_size;
    // use rate
    uint32_t use_ratio_numerator;
    uint32_t use_ratio_denominator;
    // pcie_dma_byte_size bytes of data are transferred in the last
    // pcie_dma_time_us micro-seconds
    uint64_t io_rate_byte_size;
    uint32_t io_rate_time_us;
    // device configs
    uint64_t dev_configs;
    // device monitors
    // K100/K200:
    // - [0:1] PD temperature
    // - [2]   HBM temperature
    uint32_t temperature[10];
    // K100/K200
    // - [0] L3
    // - [1] noc/dma/pcie/hbm
    // - [2] cluster/cdnn_cluster
    // - [3] cdnn
    // - [4] hbm0
    // - [5] hbm1
    uint32_t frequency[10];
    uint32_t power;
    // 0:2  - flash_version
    // 3:11 - reserve
    uint32_t firmware_version[12];
    // 0    - cpld version
    // 1    - high 32-bit of SN
    // 2:11 - reserve
    uint32_t hardware_version[12];
    // process info
    int process_count;
    struct {
        int pid;
        int stream_count;

        uint32_t cache_mem_page_used;
        uint32_t cache_mem_page_all;
        uint32_t main_page_used;
        uint32_t main_mem_page_all;
        uint32_t use_ratio_numerator;
        uint32_t use_ratio_denominator;
        uint64_t pcie_dma_byte_size;
    } process[DEVICE_MAX_PROCESS_PRINT_COUNT];

    uint32_t reserve[10];
};

struct xpu_device_info_v2 {
    struct xpu_device_info v1;

    // sizeof(num_cu) = 2*16 = 32
    struct {
        uint16_t cluster;
        uint16_t sdnn;
        uint16_t dmach;
        uint16_t hwq;
        uint16_t enc;
        uint16_t dec;
        uint16_t imgproc;

        uint16_t reserved[9];
    } num_cu;

    // sizeof(reserved) = 4*1016 = 4064
    uint32_t reserved[1016];
};

// iocv5
// add process cgroup info thus xpu_smi could tell which process belongs to the
// same container where it is running
//
// iocv and proc magic
// iocv above v5 supports query device processes info using this struct,
// and proc magic version determines the meaning of fields in this struct.
enum {
    PROCINFO_MAGIC_V0 = 0x63f000,  // magic: 0x63f, version: 0x000
};

// infomation about a single user process
struct xpu_proc {
    // pid in the running namespace
    int pid;
    // pid in the root namespace
    int root_ns_pid;
    // stream count in this process
    int stream_count;
    // process command line
    char comm[PROCESS_COMM_LEN];
    // token to identify the cgroup this process in
    uint64_t cgtoken;
    // l3 usage
    uint32_t hs_mem_pgused;
    // l3 limit of this process ('s cg)
    uint32_t hs_mem_pgall;
    // memory usage
    uint32_t main_mem_pgused;
    // memory limit of this process ('s cg)
    uint32_t main_mem_pgall;
    // use dma_time_us
    uint64_t dma_read_byte_size;
    uint64_t dma_write_byte_size;
    // usage weight
    int ur_weight;
    // process running level
    int level;
    // for future use
    uint32_t reserve0[32];
};

// container of all processes on given device file
struct xpu_device_processes {
    int magic_version;
    int process_count;
    int devfile_id;
    uint32_t dma_time_us;
    struct xpu_proc proc[DEVICE_MAX_PROCESS_PRINT_COUNT];
};

struct bar_info {
    int      bar_en; // bit_X indicates whether BAR_X is enabled
    uint64_t pcie_addr[PCIE_BAR_NUM];
    uint64_t bar_size[PCIE_BAR_NUM];
    uint64_t bus_addr[PCIE_BAR_NUM];
    uint64_t real_bus_addr[PCIE_BAR_NUM];
};

struct XPULaunchIoctlArgs {
    struct xpu_kernel kernel;
    char              name[XPU_MAX_STRLEN];
    uint32_t          params[MAX_PARAM_DWORD_SIZE_KL1];
    uint64_t          param_addr;
    uint32_t          nclusters;
    uint32_t          ncores;
    unsigned long     kernel_enter_cycle;
    unsigned long     kernel_exit_cycle;
};

#define MAX_WAIT_COUNT 32
struct XPUWaitIoctlArgs {
    uint32_t wait_count;
    uint32_t session_ids[MAX_WAIT_COUNT];
};

#define MAX_LAUNCH_BATCH_COUNT 10000
struct XPUBatchLaunchIoctlArgs {
    uint32_t                   cnt;
    struct XPULaunchIoctlArgs *cmds;
    unsigned long              kernel_enter_cycle;
    unsigned long              kernel_exit_cycle;
};

struct XPUProfLaunch {
    struct XPULaunchIoctlArgs launch;
    uint64_t cycles;
};

#define AES_STATUS_UNINIT                  (0x0)
#define AES_STATUS_DISABLED                (0x1)
#define AES_STATUS_CONFIGURED              (0x2)

#define MCU_BOOT_OPTION_ENABLE_SB          (0x1 << 0)
#define MCU_BOOT_OPTION_DISABLE_JTAG       (0x1 << 1)
#define MCU_BOOT_OPTION_DISABLE_BURN       (0x1 << 2)
#define MCU_BOOT_OPTION_ENABLE_AES         (0x1 << 3)

enum dbgm_flags {
    DBGM_FLAGS_NONBLOCK = 0,
    DBGM_FLAGS_BLOCK = 1,
};

enum dbgm_mesg {
    DBGM_MESG_KLPROF_START = 100,
    DBGM_MESG_KLPROF_STOP  = 200,
    DBGM_MESG_KLPROF_START_ACK   = 300,
    DBGM_MESG_KLPROF_STOP_ACK   = 400,
};

struct XPUDebugMasterConfig {
    uint32_t time_stamp;
    uint32_t stamp_interval;
    uint32_t lost_interval;
    uint32_t port;
    void* pinned_ptr;
    uint64_t size;
    uint32_t is_devmem;
    uint32_t enable_mbox;
    uint32_t reserve[5];
};

struct XPUDebugMasterMbox {
    uint32_t write;
    int message;
    int success;
    uint32_t reserve[6];
};

#endif // include guard
