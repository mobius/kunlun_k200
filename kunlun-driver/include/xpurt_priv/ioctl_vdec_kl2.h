#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_VDEC_H_
#define BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_VDEC_H_

#include <linux/ioctl.h>
#include "xpu/defs.h"

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

/*
 * Decoder part
 */
#define IOCTL_VDEC_IOC_MAGIC    'k'

struct vdec_core_desc {
    uint32_t id;      /* id of the core */
    uint32_t *regs;   /* pointer to user registers */
    uint32_t size;    /* size of register space */
    uint32_t reg_id;
};

#define IOCTL_VDEC_PP_INSTANCE        _IO(IOCTL_VDEC_IOC_MAGIC,   1)
#define IOCTL_VDEC_HW_PERFORMANCE     _IO(IOCTL_VDEC_IOC_MAGIC,   2)
#define IOCTL_VDEC_IOCGHWOFFSET       _IOR(IOCTL_VDEC_IOC_MAGIC,  3, unsigned long *)
#define IOCTL_VDEC_IOCGHWIOSIZE       _IOR(IOCTL_VDEC_IOC_MAGIC,  4, unsigned int *)
#define IOCTL_VDEC_IOC_CLI            _IO(IOCTL_VDEC_IOC_MAGIC,   5)
#define IOCTL_VDEC_IOC_STI            _IO(IOCTL_VDEC_IOC_MAGIC,   6)
#define IOCTL_VDEC_IOC_MC_OFFSETS     _IOR(IOCTL_VDEC_IOC_MAGIC,  7, unsigned long *)
#define IOCTL_VDEC_IOC_MC_CORES       _IOR(IOCTL_VDEC_IOC_MAGIC,  8, unsigned int *)
#define IOCTL_VDEC_IOCS_DEC_PUSH_REG  _IOW(IOCTL_VDEC_IOC_MAGIC,  9, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCS_PP_PUSH_REG   _IOW(IOCTL_VDEC_IOC_MAGIC,  10, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCH_DEC_RESERVE   _IO(IOCTL_VDEC_IOC_MAGIC,   11)
#define IOCTL_VDEC_IOCT_DEC_RELEASE   _IO(IOCTL_VDEC_IOC_MAGIC,   12)
#define IOCTL_VDEC_IOCQ_PP_RESERVE    _IO(IOCTL_VDEC_IOC_MAGIC,   13)
#define IOCTL_VDEC_IOCT_PP_RELEASE    _IO(IOCTL_VDEC_IOC_MAGIC,   14)
#define IOCTL_VDEC_IOCX_DEC_WAIT      _IOWR(IOCTL_VDEC_IOC_MAGIC, 15, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCX_PP_WAIT       _IOWR(IOCTL_VDEC_IOC_MAGIC, 16, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCS_DEC_PULL_REG  _IOWR(IOCTL_VDEC_IOC_MAGIC, 17, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCS_PP_PULL_REG   _IOWR(IOCTL_VDEC_IOC_MAGIC, 18, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCG_CORE_WAIT     _IOR(IOCTL_VDEC_IOC_MAGIC,  19, int *)
#define IOCTL_VDEC_IOX_ASIC_ID        _IOWR(IOCTL_VDEC_IOC_MAGIC, 20, unsigned long *)
#define IOCTL_VDEC_IOCG_CORE_ID       _IOR(IOCTL_VDEC_IOC_MAGIC,  21, unsigned long)
#define IOCTL_VDEC_IOCS_DEC_WRITE_REG _IOW(IOCTL_VDEC_IOC_MAGIC,  22, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCS_DEC_READ_REG  _IOWR(IOCTL_VDEC_IOC_MAGIC, 23, struct vdec_core_desc *)
#define IOCTL_VDEC_IOX_ASIC_BUILD_ID  _IOWR(IOCTL_VDEC_IOC_MAGIC, 24, unsigned long *)
#define IOCTL_VDEC_DEBUG_STATUS       _IO(IOCTL_VDEC_IOC_MAGIC,   25)

#define IOCTL_VDEC_CACHE_IOCGHWOFFSET      _IOR(IOCTL_VDEC_IOC_MAGIC, 26, unsigned long *)
#define IOCTL_VDEC_CACHE_IOCGHWIOSIZE      _IOR(IOCTL_VDEC_IOC_MAGIC, 27, unsigned int *)
#define IOCTL_VDEC_CACHE_IOCHARDRESET      _IO(IOCTL_VDEC_IOC_MAGIC,  28)
#define IOCTL_VDEC_CACHE_IOCH_HW_RESERVE   _IOR(IOCTL_VDEC_IOC_MAGIC, 29, unsigned int *)
#define IOCTL_VDEC_CACHE_IOCH_HW_RELEASE   _IOR(IOCTL_VDEC_IOC_MAGIC, 30, unsigned int *)
#define IOCTL_VDEC_CACHE_IOCG_CORE_NUM     _IOR(IOCTL_VDEC_IOC_MAGIC, 31, unsigned int *)
#define IOCTL_VDEC_CACHE_IOCG_ABORT_WAIT   _IOR(IOCTL_VDEC_IOC_MAGIC, 32, unsigned int *)

#define IOCTL_VDEC_IOC_MAXNR 32

#endif /* !BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_VDEC_H_ */
