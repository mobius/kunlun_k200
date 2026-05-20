#ifndef BAIDU_XPU_RUNTIME_MODULE_XPU_DRIVER_IOCTL_H
#define BAIDU_XPU_RUNTIME_MODULE_XPU_DRIVER_IOCTL_H

#include "xpu/defs.h"
#include "xpurt_priv/defs_private.h"

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdint.h>
#endif // __KERNEL__

// The version of this driver ioctl api
// Increased each time this ioctl.h changed
#define IOC_VERSION  5

/************
 * IOCTL
 ************/
#define IOCTL_IOC_MAGIC          (0x8EEDCE)

#define IOCTL_IOC_VERSION        _IOR(IOCTL_IOC_MAGIC, 1, int) //iocv1

#define IOCTL_DEV_HARD_RESET     _IO(IOCTL_IOC_MAGIC, 3)
#define IOCTL_DEV_SOFT_RESET     _IO(IOCTL_IOC_MAGIC, 4)

struct XPURegisterIoctlArgs {
    int bar;
    uint64_t addr;
    uint32_t value;
};
#define IOCTL_REG_READ           _IOWR(IOCTL_IOC_MAGIC, 5, struct XPURegisterIoctlArgs)
#define IOCTL_REG_WRITE          _IOW (IOCTL_IOC_MAGIC, 6, struct XPURegisterIoctlArgs)

struct XPUMemoryAllocIoctlArgs {
    uint64_t addr;
    uint64_t size;
    int kind;
};
#define IOCTL_MEMORY_ALLOC       _IOWR(IOCTL_IOC_MAGIC, 7, struct XPUMemoryAllocIoctlArgs)
#define IOCTL_MEMORY_FREE        _IOW (IOCTL_IOC_MAGIC, 8, void *)

struct XPUMemcpyIoctlArgs {
    uint64_t dest;
    uint64_t src;
    uint64_t size;
    int channel;
    uint64_t cycles;
    XPUMemcpyKind kind;
};
#define IOCTL_MEMCPY             _IOWR(IOCTL_IOC_MAGIC, 9, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_H2H         _IOWR(IOCTL_IOC_MAGIC, 10, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_H2D         _IOWR(IOCTL_IOC_MAGIC, 11, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_D2H         _IOWR(IOCTL_IOC_MAGIC, 12, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_D2D         _IOWR(IOCTL_IOC_MAGIC, 13, struct XPUMemcpyIoctlArgs)

struct XPUMemcpyP2PIoctlArgs {
    uint64_t dest;
    uint64_t src;
    uint64_t size;
    int dest_device;
    int src_device;
    uint64_t cycles;
    XPUMemcpyKind kind;
};
#define IOCTL_MEMCPY_P2P         _IOWR(IOCTL_IOC_MAGIC, 14, struct XPUMemcpyIoctlArgs)

struct XPUMemcpySecureIoctlArgs {
    uint64_t dest;
    uint64_t src;
    uint64_t size;
    uint64_t time_ns;
    XPUMemcpyKind kind;
    uint64_t iv01;
    uint64_t iv23;
    uint64_t reserve0;
    uint64_t reserve1;
    uint64_t reserve2;
    uint64_t reserve3;
};
#define IOCTL_MEMCPY_SECURE_H2D         _IOWR(IOCTL_IOC_MAGIC, 15, struct XPUMemcpySecureIoctlArgs)
#define IOCTL_MEMCPY_SECURE_D2H         _IOWR(IOCTL_IOC_MAGIC, 16, struct XPUMemcpySecureIoctlArgs)

struct XPUAESKeyIoctlArgs {
    unsigned char ckey[16];
    uint32_t status;
    uint32_t reserve0;
    uint64_t reserve1;
    uint64_t reserve2;
    uint64_t reserve3;
};
#define IOCTL_AES_LOCK_AND_KEY_ACQUIRE      _IOWR(IOCTL_IOC_MAGIC, 17, struct XPUAESKeyIoctlArgs)
#define IOCTL_AES_UNLOCK_AND_KEY_REGISTER   _IOWR(IOCTL_IOC_MAGIC, 18, struct XPUAESKeyIoctlArgs)

struct XPUMemcpyExIoctlArgs { // iocv4
    uint64_t dest;
    uint64_t src;
    uint64_t size;
    int channel;
    uint64_t time_ns;
    XPUMemcpyKind kind;
};
#define IOCTL_MEMCPY_H2D_EX         _IOWR(IOCTL_IOC_MAGIC, 19, struct XPUMemcpyIoctlArgs) // iocv4
#define IOCTL_MEMCPY_D2H_EX         _IOWR(IOCTL_IOC_MAGIC, 20, struct XPUMemcpyIoctlArgs) // iocv4

#define IOCTL_DEV_SET_NUMVFS     _IOW(IOCTL_IOC_MAGIC, 21, int)

struct XPUSessionIDIoctlArgs {
    uint32_t session_id;
};
#define IOCTL_SESSION_ID         _IOR(IOCTL_IOC_MAGIC, 40, struct XPUSessionIDIoctlArgs)

#define IOCTL_LAUNCH             _IOW(IOCTL_IOC_MAGIC, 41, struct XPULaunchIoctlArgs)
#define IOCTL_WAIT               _IOW(IOCTL_IOC_MAGIC, 42, struct XPUWaitIoctlArgs)
#define IOCTL_BATCHLAUNCH        _IOW(IOCTL_IOC_MAGIC, 43, struct XPUBatchLaunchIoctlArgs)
#define IOCTL_PROFLAUNCH         _IOW(IOCTL_IOC_MAGIC, 44, struct XPUProfLaunch)
#define IOCTL_STREAM_WAIT_EVENT  _IOW(IOCTL_IOC_MAGIC, 45, int)

#define IOCTL_SESSION_BINDTQ     _IOR(IOCTL_IOC_MAGIC, 50, uint64_t)

struct XPUEventCreate_in {
    int flags;
};
struct XPUEventCreate_out {
    int handle;
};
union XPUEventCreate {
    struct XPUEventCreate_in in;
    struct XPUEventCreate_out out;
};
#define IOCTL_EVENT_CREATE       _IOR(IOCTL_IOC_MAGIC, 51, union XPUEventCreate)
#define IOCTL_EVENT_DESTROY      _IOW(IOCTL_IOC_MAGIC, 52, int)
#define IOCTL_EVENT_RECORD       _IOW(IOCTL_IOC_MAGIC, 53, int)
#define IOCTL_EVENT_WAIT         _IOW(IOCTL_IOC_MAGIC, 54, int)

#define IOCTL_QUERY_BAR          _IOR(IOCTL_IOC_MAGIC, 60, struct bar_info)
//#define IOCTL_QUERY_IATU_REGION  _IOR(IOCTL_IOC_MAGIC, 61, struct iatu_region_info)
#define IOCTL_QUERY_DEVATTR      _IOR(IOCTL_IOC_MAGIC, 62, struct xpu_devfile_info)

// struct xpu_device_info{}
// use void* in case of upgradation of this interface
#define IOCTL_QUERY_DEVINFO      _IOR(IOCTL_IOC_MAGIC, 63, void *)

enum {
    /* iocv2 */ QDIT_MODEL_SN         = 0,
    /* iocv2 */ QDIT_COORDINATE       = 1,
    /* iocv2 */ QDIT_HS_MEM           = 2,
    /* iocv2 */ QDIT_MAIN_MEM         = 3,
    /* iocv2 */ QDIT_USE_RATIO        = 4,
    /* iocv2 */ QDIT_DMA_RATIO        = 5,
    /* iocv2 */ QDIT_TEMPERATURE      = 6,
    /* iocv2 */ QDIT_STATE            = 7,
    /* iocv2 */ QDIT_FREQUENCY        = 8,
    /* iocv2 */ QDIT_POWER            = 9,
    /* iocv2 */ QDIT_FIRMWARE_VERSION = 10,
    /* iocv3 */ QDIT_PROC_USE_RATIO   = 11,
    /* iocv3 */ QDIT_CG_HS_MEM        = 12,
    /* iocv3 */ QDIT_CG_MAIN_MEM      = 13,

    // iocv5 [ctrl]
    // If current process is running inside some CGroup
    // in
    // - arg[0]: userspace pid
    // out
    // - ret: 0=no 1=yes
    // - v32: root_pid
    // - v64[0]: cgroup token
    QDIT_PROC_INCG                    = 14,

    // iocv5 [ctrl]
    // in
    // - arg[0]: device id
    // out
    // - ret: 0=no 1=yes
    // - v64[0]: cluster count
    // - v64[1]: cdnn count
    QDIT_CU = 15,

    // iocv5 [ctrl]
    // in
    // - arg[0]: device id
    // out
    // - ret: 0=no 1=yes
    // - v64[0]: decoder count
    // - v64[1]: encoder count
    // - v64[2]: image processor count
    QDIT_CODEC = 16,

    // iocv5 [ctrl]
    // in
    // - arg[0]: device id
    // out
    // - ret: 0=no 1=yes
    // - v32: node type [0=PF 1=VF]
    // - v64[0]: max VF count (only available for PF)
    // - v64[1]: enabled VF num (only available for PF)
    // - v64[2]: VF id (only available for VF)
    QDIT_SRIOV_INFO = 17,
    QDIT_VIDEO_RATIO = 18,
    QDIT_VIDEO_FRAME_RATE = 19,
    QDIT_VIDEO_CLOCK = 20,

    // iocv5 [ctrl]
    // in
    // - arg[0]: device id
    // out
    // - ret: 0=no 1=yes
    // - v64[0]: current ECC mode
    // - v64[1]: pending(target) ECC mode
    QDIT_ECC_MODE = 21,

    // iocv5 [ctrl]
    // in
    // - arg[0]: device id
    // - arg[1]: memory error type
    // - arg[2]: ECC error counter type(reserved)
    // out
    // - ret: 0=no 1=yes
    // - v64[0]: ECC error count
    QDIT_ECC_ERROR_COUNT = 22,

    // iocv5 [ctrl]
    // in
    // - arg[0]: device id
    // out
    // - ret: 0=no 1=yes
    // - v64[0]: PN code
     QDIT_PN = 23,
};

struct xpu_device_info_v1_in { // iocv2
    int type;
    int arg[7];
};
struct xpu_device_info_v1_out { // iocv2
    int ret;
    uint32_t v32;
    uint64_t v64[3];
};
union xpu_device_info_v1 { // iocv2
    struct xpu_device_info_v1_in in;
    struct xpu_device_info_v1_out out;
};
// iocv2 [dev]
// iocv5 [ctrl]
#define IOCTL_QUERY_DEVINFO1     _IOR(IOCTL_IOC_MAGIC, 64, union xpu_device_info_v1)

// used to query info of all processes on given device
struct ioc_qproc_info_in { // iocv5
    int qmv; // query magic version
    int devfile_id;
};
union ioc_qproc_info { // iocv5
    struct ioc_qproc_info_in in;
    struct xpu_device_processes out;
};
// iocv5
#define IOCTL_QUERY_PROCINFO     _IO(IOCTL_IOC_MAGIC, 65)

//////////////////////////
// MPW Specific
//////////////////////////
#define IOCTL_MPW_SD_LAUNCH      _IOW(IOCTL_IOC_MAGIC, 80, struct XPULaunchIoctlArgs)
#define IOCTL_MPW_CLUSTER_LAUNCH _IOW(IOCTL_IOC_MAGIC, 81, struct XPULaunchIoctlArgs)
#define IOCTL_MPW_SE_LAUNCH      _IOW(IOCTL_IOC_MAGIC, 82, struct XPULaunchIoctlArgs)

//////////////////////////
// Zebu Fullmask Specific
//////////////////////////
#define IOCTL_SD_LAUNCH          _IOW(IOCTL_IOC_MAGIC, 90, struct XPULaunchIoctlArgs)
#define IOCTL_CLUSTER_LAUNCH     _IOW(IOCTL_IOC_MAGIC, 91, struct XPULaunchIoctlArgs)

struct XPUStartCunits {
    size_t count;
    int cunits[4];
};
#define IOCTL_START_CUNITS       _IO (IOCTL_IOC_MAGIC, 99)

//////////////////////////
// Device Debug use IOCTL
//////////////////////////

// no specific usage, use this ioctl for any test
#define IOCTL_PROFCLR            _IO (IOCTL_IOC_MAGIC, 100)
#define IOCTL_TEST               _IO (IOCTL_IOC_MAGIC, 101)

// SSE launch and sync
struct XPUSSELaunchIoctlArgs {
    struct xpu_kernel kernel;  // kernel to launch
    uint64_t params;           // pointer to XPU func call parameters in user-space memory
    uint32_t tq_id;            // task queue id
    uint32_t nclusters;        // how many clusters this func call is going to need
    uint32_t ncores;           // how many cores each cluster will need
};
#define IOCTL_SSE_LAUNCH         _IOW(IOCTL_IOC_MAGIC, 141, struct XPULaunchIoctlArgs)

#define MAX_SSE_WAIT_COUNT 8
struct XPUSSEWaitIoctlArgs {
    uint32_t wait_count;
    uint32_t tq_ids[MAX_SSE_WAIT_COUNT];
};
#define IOCTL_SSE_WAIT           _IOW(IOCTL_IOC_MAGIC, 142, struct XPUSSEWaitIoctlArgs)

struct XPUHostRegisterIoctlArgs {
    uint64_t ptr;
    uint64_t size;
    uint32_t flags;
    uint64_t reserved[5];
};
#define IOCTL_HOST_REGISTER       _IOWR(IOCTL_IOC_MAGIC, 143, struct XPUHostRegisterIoctlArgs)
#define IOCTL_HOST_UNREGISTER     _IOWR(IOCTL_IOC_MAGIC, 144, struct XPUHostRegisterIoctlArgs)

enum {
    /* iocv5 */ DBGM_IOC_ENABLE      = 1,
    /* iocv5 */ DBGM_IOC_DISABLE     = 2,
    /* iocv5 */ DBGM_IOC_START       = 101,
    /* iocv5 */ DBGM_IOC_STOP        = 102,
    /* iocv5 */ DBGM_IOC_MBOX_WRITE  = 201,
    /* iocv5 */ DBGM_IOC_MBOX_READ   = 202,
    /* iocv5 */ DBGM_IOC_RELAX       = 301,
    /* iocv5 */ DBGM_IOC_INUSE       = 302,
};
struct XPUDBGMIoctlArgs {
    uint32_t type;
    void *args;
    uint64_t sz;
};
#define IOCTL_DBGM     _IOWR(IOCTL_IOC_MAGIC, 145, struct XPUDBGMIoctlArgs)

#define IOCTL_MEMCPY_P2P_DIRECT   _IOWR(IOCTL_IOC_MAGIC, 146, struct XPUMemcpyIoctlArgs)

/////////////////////
// Control Node IOCTL
/////////////////////
#define XPUCTL_MAGIC      (0x8EEDCF)

struct XPUDriverVersionIoctlArgs {
    uint32_t major;
    uint32_t minor;
    char commit[XPU_MAX_STRLEN];
};
#define XPUCTL_VERSION     _IOR (XPUCTL_MAGIC, 1, struct XPUDriverVersionIoctlArgs)
// 获取XPU设备文件数量，注意真实设备数量应当是该ioctl返回值的一半
#define XPUCTL_DEVCNT      _IOR (XPUCTL_MAGIC, 2, int *)

struct XPUCtrlDevInfoIoctlArgs {
    int dev_id;
};
#define XPUCTL_DEVINFO     _IOR (XPUCTL_MAGIC, 3, struct xpu_dev_info)
// 获取全部XPU设备文件信息
#define XPUCTL_DEVINFOALL  _IOR (XPUCTL_MAGIC, 4, struct xpu_dev_info_all *)

#define XPUCTL_CHANGESET   _IOR (XPUCTL_MAGIC, 5, uint32_t *)

struct XPUSriovIoctlArgs {
    uint32_t devfile_id;
    uint32_t num_vfs;
    uint32_t reserve[6];
};
#define XPUCTL_SET_NUMVFS  _IOW (XPUCTL_MAGIC, 6, struct XPUSriovIoctlArgs)

enum {
    /* iocv5 */ CXPU_CREATE_INSTANCE,
    /* iocv5 */ CXPU_DESTROY_INSTANCE,
    /* iocv5 */ CXPU_SET_INSTANCE_HSMEM_LIMIT,
    /* iocv5 */ CXPU_SET_INSTANCE_MAINMEM_LIMIT,
    /* iocv5 */ CXPU_SET_INSTANCE_CORE_LIMIT,
    /* iocv5 */ CXPU_GET_INSTANCE_COUNT,
    /* iocv5 */ CXPU_GET_MAX_INSTANCE_COUNT,
    /* iocv5 */ CXPU_GET_INSTANCE_ID,
    /* iocv5 */ CXPU_GET_HSMEM_INFO,
    /* iocv5 */ CXPU_GET_MAINMEM_INFO,
    /* iocv5 */ CXPU_GET_CORE_INFO,
    /* iocv5 */ CXPU_GET_INSTANCE_HSMEM_INFO,
    /* iocv5 */ CXPU_GET_INSTANCE_MAINMEM_INFO,
    /* iocv5 */ CXPU_GET_INSTANCE_CORE_INFO,
};

#define CXPU_INSTANCE_ID_LEN (12)
struct XPUCxpuIoctlArgs{
    uint32_t devfile_id;
    uint64_t instance_id[2];
    uint32_t type;
    uint64_t value;
    uint64_t reserved[3];
};
#define XPUCTL_CXPU_CONFIG _IOWR (XPUCTL_MAGIC, 7, struct XPUCxpuIoctlArgs)

#endif // include guard
