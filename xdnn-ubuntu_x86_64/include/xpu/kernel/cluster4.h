#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER4_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER4_H

// avoid re-include xpu/kernel/cluster2,3.h
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_H
// avoid re-include xpu/kernel/cluster2,3.h
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_H

#include "xpu/kernel/xtdk.h"
#include "xpu/kernel/cluster_math.h"
#include "xpu/kernel/cluster3_type.h"
#include "xpu/kernel/cluster_partition.h"
#ifndef __XPU_KERNEL_SDNN__
#include "xpu/kernel/cluster_primitive.h"
#include "xpu/kernel/cluster3_simd.h"
#endif // __XPU_KERNEL_SDNN__

// xpu4 namespace
#define KERNEL_NAMESPACE_BEGIN namespace xpu4 {
#define KERNEL_NAMESPACE_END }

#endif // BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_H
