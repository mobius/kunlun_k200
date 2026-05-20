#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_SIMD_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_SIMD_H
#if (defined(__XPU3__) || defined(__XPU4__))
#include "xpu/kernel/cluster3_simd.h"
#else
#include "xpu/kernel/cluster2_simd.h"
#endif
#endif
