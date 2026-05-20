#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_VENC_H_
#define BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_VENC_H_

#include <linux/ioctl.h>
#include "xpu/defs.h"

/* Use 'e' as magic number */
#define IOCTL_VENC_IOC_MAGIC  'e'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

#define IOCTL_VENC_IOCGHWOFFSET      _IOR(IOCTL_VENC_IOC_MAGIC,  3, unsigned long *)
#define IOCTL_VENC_IOCGHWIOSIZE      _IOR(IOCTL_VENC_IOC_MAGIC,  4, unsigned int *)
//#define IOCTL_VENC_IOC_CLI           _IO(IOCTL_VENC_IOC_MAGIC,  5)
//#define IOCTL_VENC_IOC_STI           _IO(IOCTL_VENC_IOC_MAGIC,  6)
//#define IOCTL_VENC_IOCXVIRT2BUS      _IOWR(IOCTL_VENC_IOC_MAGIC,  7, unsigned long *)

//#define IOCTL_VENC_IOCHARDRESET      _IO(IOCTL_VENC_IOC_MAGIC, 8)   /* debugging tool */

//#define IOCTL_VENC_IOCGSRAMOFFSET    _IOR(IOCTL_VENC_IOC_MAGIC,  9, unsigned long *)
//#define IOCTL_VENC_IOCGSRAMEIOSIZE    _IOR(IOCTL_VENC_IOC_MAGIC,  10, unsigned int *)
#define IOCTL_VENC_IOCH_ENC_RESERVE   _IOR(IOCTL_VENC_IOC_MAGIC, 11, unsigned int *)
#define IOCTL_VENC_IOCH_ENC_RELEASE   _IOR(IOCTL_VENC_IOC_MAGIC, 12, unsigned int *)
#define IOCTL_VENC_IOCG_CORE_NUM      _IOR(IOCTL_VENC_IOC_MAGIC, 13, unsigned int *)
#define IOCTL_VENC_IOCG_CORE_INFO     _IOR(IOCTL_VENC_IOC_MAGIC, 14, SUBSYS_CORE_INFO *)
#define IOCTL_VENC_IOCS_ENC_PUSH_REG  _IOW(IOCTL_VENC_IOC_MAGIC, 15, struct subsys_enc *)
#define IOCTL_VENC_IOCG_CORE_WAIT     _IOR(IOCTL_VENC_IOC_MAGIC, 19, unsigned int *)
#define IOCTL_VENC_IOC_MAXNR 19

#define GET_ENCODER_IDX(type_info)  (CORE_VC8000E)
#define CORETYPE(core)     (1 << core)

// KL2 soc has 3 hardware encoder instances
#define VENC_MAX_SUBSYS (3)

enum CORE_TYPE
{
    CORE_VC8000E  = 0,
    CORE_VC8000EJ = 1,
    CORE_CUTREE   = 2,
    CORE_DEC400   = 3,
    CORE_MAX
};

typedef struct
{
    uint32_t type_info;      // indicate which IP is contained in this subsystem
                             // and each uses one bit of this variable
    uint32_t offset[CORE_MAX];
    uint32_t regSize[CORE_MAX];
} SUBSYS_CORE_INFO;

struct subsys_enc {
    uint32_t id;    /* id of the subsys */
    uint32_t *regs; /* pointer to user registers */
    uint32_t size;  /* size of register space */
    uint32_t reg_id;
    uint32_t core_type;
};

#endif // BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_VENC_H_
