#ifndef BAIDU_XPU_RUNTIME_MODULE_XPU_DRIVER_IOCTL_KL1_H
#define BAIDU_XPU_RUNTIME_MODULE_XPU_DRIVER_IOCTL_KL1_H

#include "xpurt_priv/defs_private.h"
#include "xpurt_priv/defs_private_kl1.h"
#include "xpurt_priv/ioctl.h"

#define IOCTL_QUERY_IATU_REGION  _IOR(IOCTL_IOC_MAGIC, 61, struct iatu_region_info)

#endif
