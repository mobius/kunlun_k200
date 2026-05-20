#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_H

// avoid re-include xpu/kernel/cluster2.h
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_H

#include "xpu/kernel/xtdk.h"
#include "xpu/kernel/cluster_math.h"
#include "xpu/kernel/cluster3_type.h"
#include "xpu/kernel/cluster_partition.h"
#include "xpu/kernel/cluster_primitive.h"
#include "xpu/kernel/cluster3_simd.h"

// xpu3 namespace
#define KERNEL_NAMESPACE_BEGIN namespace xpu3 {
#define KERNEL_NAMESPACE_END }

#endif // BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_H
