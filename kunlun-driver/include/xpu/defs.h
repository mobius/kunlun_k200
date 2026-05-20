/// \file   defs.h
/// \brief  Shared definitions used in both Runtime and Driver.
/// \author hanjinchen@baidu.com
/// \copyright (C) 2018 Baidu, Inc
///

#ifndef BAIDU_XPU_RUNTIME_INCLUDE_XPU_DEFS_H
#define BAIDU_XPU_RUNTIME_INCLUDE_XPU_DEFS_H

#ifdef __KERNEL__
#include <linux/types.h>
static inline
const char* strerror(int e) { return "System error"; }
#else
#include <stdint.h>
#include <string.h>
#endif // __KERNEL__

#define XPU_ERRNO_BASE   700
#define XPU_ERRNO(e) (XPU_ERRNO_BASE + (e))

/// \brief  XPURT 错误码
typedef enum {
    XPU_SUCCESS = 0,

    XPUERR_INVALID_DEVICE = XPU_ERRNO(1),
    XPUERR_UNINIT         = XPU_ERRNO(2),
    XPUERR_NODEV          = XPU_ERRNO(3),
    XPUERR_VERSION        = XPU_ERRNO(4),
    XPUERR_NOMEM          = XPU_ERRNO(5),
    XPUERR_NOCPUMEM       = XPU_ERRNO(6),
    XPUERR_INVALID_PARAM  = XPU_ERRNO(7),
    XPUERR_NOXPUFUNC      = XPU_ERRNO(8),
    XPUERR_LDSO           = XPU_ERRNO(9),
    XPUERR_LDSYM          = XPU_ERRNO(10),
    XPUERR_SIMULATOR      = XPU_ERRNO(11),
    XPUERR_NOSUPPORT      = XPU_ERRNO(12),
    XPUERR_ABNORMAL       = XPU_ERRNO(13),
    XPUERR_KEXCEPTION     = XPU_ERRNO(14),
    XPUERR_TIMEOUT        = XPU_ERRNO(15),
    XPUERR_BUSY           = XPU_ERRNO(16),
    XPUERR_USEAFCLOSE     = XPU_ERRNO(17),
    XPUERR_UCECC          = XPU_ERRNO(18),
    XPUERR_OVERHEAT       = XPU_ERRNO(19),
    XPUERR_EVENTWAIT      = XPU_ERRNO(20),

    XPUERR_UNEXPECT       = XPU_ERRNO(100),

    XPUERR_DEVRESET       = XPU_ERRNO(101),
    XPUERR_HWEXCEPTION    = XPU_ERRNO(102),
    XPUERR_HBM_INIT       = XPU_ERRNO(103),
    XPUERR_DEVINIT        = XPU_ERRNO(104),
    XPUERR_PEERRESET      = XPU_ERRNO(105),

    XPUERR_MAXDEV         = XPU_ERRNO(106),
    XPUERR_NOIOC          = XPU_ERRNO(107),
    XPUERR_DMATIMEOUT     = XPU_ERRNO(108),
    XPUERR_DMAABORT       = XPU_ERRNO(109),
    XPUERR_MCUUNINIT      = XPU_ERRNO(110),
    XPUERR_OLDFW          = XPU_ERRNO(111),
    XPUERR_PCIE           = XPU_ERRNO(112),
    XPUERR_FAULT          = XPU_ERRNO(113),
    XPUERR_INTERRUPTED    = XPU_ERRNO(114),
    XPUERR_AESTIMEOUT     = XPU_ERRNO(115),
    XPUERR_HOSTMEM_ALREADY_REGISTERED = XPU_ERRNO(116),
    XPUERR_TRACER         = XPU_ERRNO(117),

} XPUError_t;

static inline
const char *xpu_strerror(int e) {
    int e_abs = (e > 0) ? e : -e;

    if (e_abs < XPU_ERRNO_BASE) {
        return strerror(e_abs);
    }

    switch (e_abs) {
    case XPU_SUCCESS:
        return "Success";
    case XPUERR_INVALID_DEVICE:
        return "Invalid XPU device";
    case XPUERR_UNINIT:
        return "XPU runtime not properly inited";
    case XPUERR_NOMEM:
        return "Device memory not enough";
    case XPUERR_NOCPUMEM:
        return "CPU memory not enough";
    case XPUERR_INVALID_PARAM:
        return "Invalid parameter";
    case XPUERR_NOXPUFUNC:
        return "Cannot get XPU func";
    case XPUERR_LDSO:
        return "Error loading dynamic library";
    case XPUERR_LDSYM:
        return "Error loading func from dynamic library";
    case XPUERR_SIMULATOR:
        return "Error from XPU Simulator";
    case XPUERR_NOSUPPORT:
        return "Operation not supported";
    case XPUERR_ABNORMAL:
        return "Device abnormal due to previous error";
    case XPUERR_KEXCEPTION:
        return "Exception in kernel execution";
    case XPUERR_TIMEOUT:
        return "Kernel execution timed out";
    case XPUERR_BUSY:
        return "Resource busy";
    case XPUERR_USEAFCLOSE:
        return "Use a stream after closed";
    case XPUERR_UCECC:
        return "Uncorrectable ECC";
    case XPUERR_OVERHEAT:
        return "Overheat";
    case XPUERR_EVENTWAIT:
        return "Exception in event record or event wait, or stream tainted by event record";

    case XPUERR_UNEXPECT:
        return "Execution error, reach unexpected control flow";

    case XPUERR_DEVRESET:
        return "Device is being reset, try again later";
    case XPUERR_HWEXCEPTION:
        return "Hardware module exception";
    case XPUERR_HBM_INIT:
        return "Error init HBM";
    case XPUERR_DEVINIT:
        return "Error init device";
    case XPUERR_PEERRESET:
        return "Device is being reset, try again later";
    case XPUERR_MAXDEV:
        return "Device count exceed limit";
    case XPUERR_NOIOC:
        return "Unknown IOCTL command";
    case XPUERR_DMATIMEOUT:
        return "DMA timed out, a reboot maybe needed";
    case XPUERR_DMAABORT:
        return "DMA aborted due to error, possibly wrong address or hardware state";
    case XPUERR_MCUUNINIT:
        return "Firmware not initialized";
    case XPUERR_OLDFW:
        return "Firmware version too old (<15), please update.";
    case XPUERR_PCIE:
        return "Error in PCIe";
    case XPUERR_FAULT:
        return "Error copy between kernel and user space";
    case XPUERR_INTERRUPTED:
        return "Execution interrupted by user";
    case XPUERR_HOSTMEM_ALREADY_REGISTERED:
        return "Host memory is already registered";
    case XPUERR_TRACER:
        return "Tracer subsystem error";
    default:
        return "Unknown error";
    }
}

typedef enum {
    // KL1
    KL1_BEGIN                  = 0,
    K200                       = 0,
    K100                       = 1,
    KL1_END                    = 1,

    // KL2
    KL2_BEGIN                  = 2,
    R200                       = 2,
    R300                       = 3,
    R200_8F                    = 4,
    R200_8FS                   = 5,
    R100                       = 6,
    R200_DEBUG_BOARD           = 7,
    R420                       = 8,
    RG800                      = 9,
    RG800_PRO                  = 10,
    RM80                       = 11,
    //R***                     = 99,

    // KL2 PF/VF
    R200_SRIOV_PF              = 200,
    R200_SRIOV_VF_ONE_OF_ONE   = 201,
    R200_SRIOV_VF_ONE_OF_TWO   = 202,
    R200_SRIOV_VF_ONE_OF_THREE = 203,

    R300_SRIOV_PF              = 204,
    R300_SRIOV_VF_ONE_OF_ONE   = 205,
    R300_SRIOV_VF_ONE_OF_TWO   = 206,
    R300_SRIOV_VF_ONE_OF_THREE = 207,

    R200_8F_SRIOV_PF              = 208,
    R200_8F_SRIOV_VF_ONE_OF_ONE   = 209,
    R200_8F_SRIOV_VF_ONE_OF_TWO   = 210,
    R200_8F_SRIOV_VF_ONE_OF_THREE = 211,

    R200_8FS_SRIOV_PF              = 212,
    R200_8FS_SRIOV_VF_ONE_OF_ONE   = 213,
    R200_8FS_SRIOV_VF_ONE_OF_TWO   = 214,
    R200_8FS_SRIOV_VF_ONE_OF_THREE = 215,

    RG800_SRIOV_PF              = 216,
    RG800_SRIOV_VF_ONE_OF_ONE   = 217,
    RG800_SRIOV_VF_ONE_OF_TWO   = 218,
    RG800_SRIOV_VF_ONE_OF_THREE = 219,

    RG800_PRO_SRIOV_PF              = 220,
    RG800_PRO_SRIOV_VF_ONE_OF_ONE   = 221,
    RG800_PRO_SRIOV_VF_ONE_OF_TWO   = 222,
    RG800_PRO_SRIOV_VF_ONE_OF_THREE = 223,

    KL2_END                    = 299,

    // KL3
    KL3_BEGIN                  = 300,
    KL3                        = 300,
    KL3_END                    = 599,
} DeviceModel;

static inline const char *xpu_device_model_str(int model) {
    switch (model) {
        case K200: return "K200";
        case K100: return "K100";

        case R200: return "R200";
        case R300: return "R300";
        case R200_8F: return "R200-8F";
        case R200_8FS: return "R200-8FS";
        case R100: return "R100";
        case R200_DEBUG_BOARD: return "R200_DEBUG_BOARD";
        case R420: return "R420";
        case RG800: return "RG800";
        case RG800_PRO: return "RG800-PRO";
        case RM80: return "RM80";

        case R200_SRIOV_PF: return "R200-PF";
        case R200_SRIOV_VF_ONE_OF_ONE: return "R200-VF-16G";
        case R200_SRIOV_VF_ONE_OF_TWO: return "R200-VF-8G";
        case R200_SRIOV_VF_ONE_OF_THREE: return "R200-VF-5G";

        case R300_SRIOV_PF: return "R300-PF";
        case R300_SRIOV_VF_ONE_OF_ONE: return "R300-VF-32G";
        case R300_SRIOV_VF_ONE_OF_TWO: return "R300-VF-16G";
        case R300_SRIOV_VF_ONE_OF_THREE: return "R300-VF-10G";

        case R200_8F_SRIOV_PF: return "R200-8F-PF";
        case R200_8F_SRIOV_VF_ONE_OF_ONE: return "R200-8F-VF-32G";
        case R200_8F_SRIOV_VF_ONE_OF_TWO: return "R200-8F-VF-16G";
        case R200_8F_SRIOV_VF_ONE_OF_THREE: return "R200-8F-VF-10G";

        case R200_8FS_SRIOV_PF: return "R200-8FS-PF";
        case R200_8FS_SRIOV_VF_ONE_OF_ONE: return "R200-8FS-VF-32G";
        case R200_8FS_SRIOV_VF_ONE_OF_TWO: return "R200-8FS-VF-16G";
        case R200_8FS_SRIOV_VF_ONE_OF_THREE: return "R200-8FS-VF-10G";

        case RG800_SRIOV_PF: return "RG800-PF";
        case RG800_SRIOV_VF_ONE_OF_ONE: return "RG800-VF-32G";
        case RG800_SRIOV_VF_ONE_OF_TWO: return "RG800-VF-16G";
        case RG800_SRIOV_VF_ONE_OF_THREE: return "RG800-VF-10G";

        case RG800_PRO_SRIOV_PF: return "RG800-PRO-PF";
        case RG800_PRO_SRIOV_VF_ONE_OF_ONE: return "RG800-PRO-VF-32G";
        case RG800_PRO_SRIOV_VF_ONE_OF_TWO: return "RG800-PRO-VF-16G";
        case RG800_PRO_SRIOV_VF_ONE_OF_THREE: return "RG800-PRO-VF-10G";

        case KL3: return "KL3";
        default: return "Unknown model";
    }
}

/// \brief Indicates the memory type
typedef enum {
    /// HBM memory
    XPU_MEM_HBM = 0,
    /// Main memory (HBM or GDDR)
    XPU_MEM_MAIN = 0,
    /// L3 memory
    XPU_MEM_L3 = 1,
    /// Total User Allocable Count
    XPU_USER_ALLOCABLE_MEM_COUNT,

    XPU_MEM_MMAPPABLE = 16,
    /// KL2 huge memory (可能跨越4G边界) 仅用于特殊场景，分配开销较大，不通用
    XPU_MEM_MAIN_KL2_HUGE = 17,
} XPUMemoryKind;

/// \brief Indicates the copy direction
typedef enum {
    /// Copy from Device to CPU
    XPU_DEVICE_TO_HOST = 0,
    /// Copy from CPU to Device
    XPU_HOST_TO_DEVICE = 1,
    /// Copy from Device to Device
    XPU_DEVICE_TO_DEVICE = 2,
} XPUMemcpyKind;

enum {
    /// Max device number
    MAX_DEVICE_NUM = 64,

    /// Max string length of all XPU infomation
    XPU_MAX_STRLEN = 64,
};

enum {
    /// kernel异常，cluster/sdnn运行异常
    XPU_XID0 = 0,
    /// hbm/gddr异常，ecc uncorrected error
    XPU_XID1 = 1,
    /// 温度功耗异常
    XPU_XID2 = 2,
    /// sse异常
    XPU_XID3 = 3,
};

enum {
    /// ECC single-bit error
    XPU_ECC_SBE = 0,
    /// ECC double-bit error
    XPU_ECC_DBE = 1,
};

#endif
