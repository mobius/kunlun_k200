#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_H
#ifdef __XPU4__
#include "xpu/kernel/cluster4.h"
#else
#ifdef __XPU3__
#include "xpu/kernel/cluster3.h"
#else
#include "xpu/kernel/cluster2.h"
#endif
#endif
#endif

