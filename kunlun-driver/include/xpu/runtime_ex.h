/// \file   runtime_ex.h
/// \brief  XPU runtime extented functions
///         Functions declared in this header are supposed to be published to ISA developer only
/// \author hanjinchen@baidu.com
/// \copyright (C) 2018 Baidu, Inc

#pragma once

#include "xpu/runtime.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

// \brief   设置当前进程所在cgroup高速缓存使用量上限
// \param   内存使用上限字节数，注意由于昆仑内存使用页表管理，如果该限制不是页表大小的整数倍，
//          则实际限额会被向上取整到页表的整数倍
DLL_PUBLIC int xpu_cg_set_hs_mem_limit(uint64_t bytes);

// \brief   取消当前进程所在cgroup高速缓存使用量上限
DLL_PUBLIC int xpu_cg_unset_hs_mem_limit();

// \brief   设置当前进程所在cgroup主存使用量上限
// \param   内存使用上限字节数，注意由于昆仑内存使用页表管理，如果该限制不是页表大小的整数倍，
//          则实际限额会被向上取整到页表的整数倍
DLL_PUBLIC int xpu_cg_set_main_mem_limit(uint64_t bytes);

// \brief   取消当前进程所在cgroup主存使用量上限
DLL_PUBLIC int xpu_cg_unset_main_mem_limit();

/// \brief  设置调度模式为SINGLE。
DLL_PUBLIC int xpu_disp_set_single_mode();

/// \brief  设置调度模式为BATCH。
DLL_PUBLIC int xpu_disp_set_batch_mode();

/// \brief  设置调度模式为PROFILING。
DLL_PUBLIC int xpu_disp_set_profiling_mode();

/// \brief  恢复调度模式为默认。
DLL_PUBLIC int xpu_disp_reset();

/// \brief  返回XPU Runtime changeset
///         Changeset 是bcloud生成的，通过REPO_CHANGESET标签获得，在一个repo中，每次commit该tag会递增
DLL_PUBLIC int xpu_get_runtime_changeset(uint32_t* set);

DLL_PUBLIC int xpu_br(int bar, uint64_t reg, uint32_t* valueptr);
DLL_PUBLIC int xpu_bw(int bar, uint64_t reg, uint32_t valueptr);

/// \brief  读取指定寄存器
/// \param reg  寄存器地址
/// \param valueptr  输出寄存器读取结果
DLL_PUBLIC int xpu_rr(uint64_t reg, uint32_t* valueptr);

/// \brief  写入指定寄存器
/// \param reg  寄存器地址
/// \param value  写入值
DLL_PUBLIC int xpu_rw(uint64_t reg, uint32_t value);

DLL_PUBLIC int xpu_get_fd(
#if defined(__cplusplus)
        XPUStream stream = NULL
#else
        XPUStream stream
#endif
        );

DLL_PUBLIC int xpu_malloc_ex(void** pdevptr, uint64_t sz, int kind);

DLL_PUBLIC int xpu_memcpy_ex(void* dst, const void* src, uint64_t sz, XPUMemcpyKind kind, uint64_t *cycles);

DLL_PUBLIC int xpu_launch_param_addr_set(const void *param_addr);

/// \brief  返回当前线程算子参数的暂存地址
DLL_PUBLIC uint32_t *xpu_launch_param();

/// \brief  返回当前线程算子参数的双字大小计数
DLL_PUBLIC size_t xpu_launch_param_dword_size();

/// \biref  恢复/开始 Profiler
///         进程启动时 Profiler 默认是不工作的
DLL_PUBLIC void xpu_profiling_resume();

/// \biref  暂停 Profiler
DLL_PUBLIC void xpu_profiling_pause();

/// \brief  清空当前 Profiler 数据
///         必须先暂停 Profiler 才能清空数据
DLL_PUBLIC void xpu_profiling_clear();

/// \brief  打印 Profiler 数据
DLL_PUBLIC void xpu_print_profiling_summary();

#if defined(__cplusplus)
} // extern "C"
#endif /* __cplusplus */
