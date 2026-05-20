/// \file   runtime.h
/// \brief  XPU Runtime API
/// \author hanjinchen@baidu.com
/// \copyright (C) 2018 Baidu, Inc

#pragma once

#if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define DLL_PUBLIC
#define DLL_LOCAL
#endif

#include <stdint.h>
#include <stdlib.h>

#include "xpu/defs.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/// \brief  指向一个 XPU Stream
typedef void *XPUStream;

/// \brief  指向一个 XPU Function
typedef void *XPUFunc;

/// \brief  指向一个 XPU Event
typedef void *XPUEvent;

/// \brief  定义可查询的XPU设备信息
typedef enum {
    /// 设备的 PCI 地址
    XPUATTR_PCI_ADDRESS       = 0,
    XPUATTR_DEVID             = 1,
    XPUATTR_BOARDID           = 2,
    XPUATTR_ONBOARDID         = 3,
    XPUATTR_MODEL             = 4,
    XPUATTR_MEM_MAIN_CAPACITY = 5,
    XPUATTR_MEM_L3_CAPACITY   = 6,
    XPUATTR_NUM_CLUSTER       = 7,
    XPUATTR_NUM_SDNN          = 8,
    XPUATTR_NUM_DMACH         = 9,
    XPUATTR_NUM_HWQ           = 10,
    XPUATTR_NUM_ENC           = 11,
    XPUATTR_NUM_DEC           = 12,
    XPUATTR_NUM_IMGPROC       = 13,
} XPUDeviceAttr;

/// \brief  将设备ID转为模拟器设备ID。
///
///         在调用 \ref xpu_set_device 的时候，使用该函数将XPU设备号转为模拟器设备号，
///         从而显式的设定模拟器计算模式。
/// \param devid  XPU设备号
DLL_PUBLIC int DEVICE_SIMULATOR(int devid);

/// \brief  返回驱动版本号。
/// \param[out] major  返回主版本号
/// \param[out] minor  返回副版本号
DLL_PUBLIC int xpu_get_driver_version(uint32_t* major, uint32_t* minor);

/// \brief  返回当前驱动的GIT提交哈希值。
/// \param[out] commit  返回GIT提交哈希值字符串
DLL_PUBLIC int xpu_get_driver_commit(char* commit);

/// \brief  返回XPU Runtime版本号。
/// \param[out] major  返回主版本号
/// \param[out] minor  返回副版本号
DLL_PUBLIC int xpu_get_runtime_version(uint32_t* major, uint32_t* minor);

/// \brief  返回当前版本XPU Runtime的GIT提交哈希值。
/// \param[out] commit  返回GIT提交哈希值字符串
DLL_PUBLIC int xpu_get_runtime_commit(char* commit);

/// \brief  设定当前工作设备。
///
///         当前工作设备是一个线程级的状态，设定当前工作设备之后，后续所有的内存分配、搬运、
///         Stream管理等函数调用都作用于该设备。如果用户希望在一个线程内同时使用两个设备，
///         需要确保利用该函数进行设备切换。
///
///         如果该函数从未被调用，默认使用0号设备。
/// \param[in] devid  目标设备号
DLL_PUBLIC int xpu_set_device(int devid);

/// \brief  获取当前工作设备。
/// \param[out] devid  返回当前工作设备设备号
/// \return \ref XPU_SUCCESS, \ref XPUERR_INVALID_PARAM
DLL_PUBLIC int xpu_current_device(int* devid);

/// \brief  获取当前系统中XPU设备的个数。
/// \param[out] dev_count  返回XPU设备个数
DLL_PUBLIC int xpu_device_count(int* dev_count);

/// \brief  获取当前系统中XPU设备的设备号列表。
///         Caller负责分配并传入dev_list数组，其大小为xpu_device_count()个int，
///         Runtime通过该数组返回当前进程可见的XPU设备号。
///         注意Caller需要正确的分配dev_list数组，Runtime无法对该数组进行检查，如果Caller
///         传入了非法地址，可能会导致内存访问错误。
/// \param[out] dev_list  XPU设备号列表
DLL_PUBLIC int xpu_device_list(int* dev_list);

/// \brief  获取当前工作设备的信息。
///
///         在 \p v 中返回 \p attr 所指定的设备信息， \p v 本身类型为 uint64_t，根据
///         所查询信息的不同，返回值的有效 bit 位数也不尽相同。当前支持查询的设备信息为：
///
///         - \ref XPUATTR_PCI_ADDRESS ：设备的PCI地址，其中比特域[31:16][15:8][7:3][2:0]
///           分别表示 Domain、Bus、Slot（Device）、Function
///         - \ref XPUATTR_MODEL：板卡产品编号。返回值包括K200、K100、R200
///
/// \param[out] v  返回所要查询的设备信息值
/// \param[in] attr  指定所要查询的设备信息
/// \param[in] devid  目标设备号
/// \return \ref XPU_SUCCESS, \ref XPUERR_INVALID_PARAM
DLL_PUBLIC int xpu_device_get_attr(uint64_t* v, XPUDeviceAttr attr, int devid);

/// \brief  在当前工作设备上分配内存。
/// \param[out] pdevptr  返回分配的XPU设备地址
/// \param[in]  sz       所需分配的内存大小，单位为字节
/// \param[in]  kind     分配的内存种类，默认值为 \ref XPU_MEM_MAIN ，如果希望在L3内存上分配，
///                      使用 \ref XPU_MEM_L3
DLL_PUBLIC int xpu_malloc(
        void** pdevptr,
        uint64_t sz,
#if defined(__cplusplus)
        XPUMemoryKind kind = XPU_MEM_MAIN
#else
        XPUMemoryKind kind
#endif
        );

/// \brief  释放当前工作设备上的内存。
/// \param[in] devptr  XPU Pointer to be freed
DLL_PUBLIC int xpu_free(void* devptr);

/// \brief  锁定 [ptr, ptr + sz) 范围内的内存页，这段内存可以用于加速 xpu_memcpy。
/// \param[in] ptr   目标地址
/// \param[in] sz    内存字节数
/// \param[in] flags placeholder, 必须赋值为0，暂时没有作用
DLL_PUBLIC int xpu_host_register(void* ptr, uint64_t sz, unsigned int flags);

/// \brief  解除被 xpu_host_register 锁定的内存页。
/// \param[in] ptr   目标地址
DLL_PUBLIC int xpu_host_unregister(void* ptr);

/// \brief  分配并锁定大小为 sz 且尽量物理连续的主机内存页，这段内存可以用于加速 xpu_memcpy。
/// \param[out] ptr  返回分配的主机内存页地址
/// \param[in] sz    内存字节数
/// \param[in] flags placeholder, 必须赋值为0，暂时没有作用
DLL_PUBLIC int xpu_host_alloc(void** ptr, uint64_t sz, unsigned int flags);

/// \brief  释放由 xpu_host_alloc 分配的主机内存页。
/// \param[in] ptr   目标地址
DLL_PUBLIC int xpu_host_free(void* ptr);

/// \brief  在CPU和XPU间进行内存拷贝。
/// \param[in] dst   目标地址
/// \param[in] src   源地址
/// \param[in] sz    拷贝字节数
/// \param[in] kind  内存拷贝种类, 可以是 \ref XPU_DEVICE_TO_HOST ，
///                  \ref XPU_HOST_TO_DEVICE 或 \ref XPU_DEVICE_TO_DEVICE
DLL_PUBLIC int xpu_memcpy(void* dst, const void* src, uint64_t sz, XPUMemcpyKind kind);

/// \brief  在不同XPU设备上进行内存拷贝
/// \param[in] dst_device   目标设备
/// \param[in] dst_addr     目标地址
/// \param[in] src_device   源设备
/// \param[in] src_addr     源地址
/// \param[in] sz           拷贝字节数
DLL_PUBLIC int xpu_memcpy_peer(int dst_device, void* dst_addr,
                           int src_device, const void* src_addr, uint64_t sz);

/// \brief  在指定Stream上创建一个Event，如未指定Stream，则在默认Stream上创建。
///
///         调用该函数在Stream上创建一个Event，当指定Stream位于该Event之前的所有Kernel都
///         执行完成后，该Event会被「触发」。
///
/// \param[out] pevent  指向待创建的Event
DLL_PUBLIC int xpu_event_create(XPUEvent *pevent);

/// \param[in]  stream  在该Stream上创建Event
DLL_PUBLIC int xpu_event_record(
        XPUEvent event,
#if defined(__cplusplus)
        XPUStream stream = NULL
#else
        XPUStream stream
#endif
        );

/// \brief  销毁一个Event
DLL_PUBLIC int xpu_event_destroy(XPUEvent event);

/// \brief  等待Event触发
DLL_PUBLIC int xpu_event_wait(XPUEvent event);

/// \brief  在当前设备上创建一个Stream。
/// \param[out] pstream  指向待创建的Stream
DLL_PUBLIC int xpu_stream_create(XPUStream* pstream);

/// \brief  销毁一个Stream。
/// \param[in] stream  待销毁的Stream。
DLL_PUBLIC int xpu_stream_destroy(XPUStream stream);

/// \brief  令Stream等待指定Event触发
///
///         在`stream`上插入一个等待任务，该等待任务会Block住stream上后续Kernel的
///         执行，直到指定的Event触发后才会继续。
///         注意该接口本身并不会等待Event的触发。
///
/// \param[in] stream 插入等待任务的队列
/// \param[in] event  被等待的事件
DLL_PUBLIC int xpu_stream_wait_event(XPUStream stream, XPUEvent event);

/// \brief  创建一个SD-CDNN类型的XPU函数。
/// \note   该接口仅高级开发者可能会用到
/// \param[out] pfunc             指向待创建的XPUFunc
/// \param[in]  code_addr         XPU代码的地址
/// \param[in]  code_byte_size    XPU代码的字节数
/// \param[in]  code_pc           XPU代码的初始偏移
/// \param[in]  param_dword_size  XPU函数参数需要多少个4字节
/// \param[in]  hash              XPU代码的哈希值
/// \param[in]  name              XPU函数的Mangled name
/// \param[in]  on_xpu            XPU代码地址是否在XPU主存上
DLL_PUBLIC
int xpu_create_sd_func(
        XPUFunc *pfunc,
        uint64_t code_addr,
        uint32_t code_byte_size,
        uint32_t code_pc,
        uint32_t param_dword_size,
        uint64_t hash,
#if defined(__cplusplus)
        const char* name = NULL,
#else
        const char* name,
#endif
#if defined(__cplusplus)
        bool on_xpu = false
#else
        bool on_xpu
#endif
        );

/// \brief  创建一个CLUSTER类型的XPU函数。
/// \note   该接口仅高级开发者可能会用到
/// \param[out] pfunc             指向待创建的XPUFunc
/// \param[in]  code_addr         XPU代码的地址
/// \param[in]  code_byte_size    XPU代码的字节数
/// \param[in]  code_pc           XPU代码的初始偏移
/// \param[in]  param_dword_size  XPU函数参数需要多少个4字节
/// \param[in]  hash              XPU代码的哈希值
/// \param[in]  name              XPU函数的Mangled name
/// \param[in]  on_xpu            XPU代码地址是否在XPU主存上
DLL_PUBLIC
int xpu_create_cl_func(
        XPUFunc *pfunc,
        uint64_t code_addr,
        uint32_t code_byte_size,
        uint32_t code_pc,
        uint32_t param_dword_size,
        uint64_t hash,
#if defined(__cplusplus)
        const char* name = NULL,
#else
        const char* name,
#endif
#if defined(__cplusplus)
        bool on_xpu = false
#else
        bool on_xpu
#endif
        );

/// \brief  设定函数调用配置。
///
///         调用配置是传给XPURT的参数，运行时会解析配置信息，并根据配置进行函数调用。
/// \note   调用 \ref xpu_launch_async 之前需要先调用本函数
/// \param[in] nclusters  Number of Clusters needed to compute this kernel
/// \param[in] ncores     Number of Cores needed in each Cluster
/// \param[in] stream     Stream to launch this kernel on
DLL_PUBLIC int xpu_launch_config(
        int nclusters,
        int ncores,
#if defined(__cplusplus)
        XPUStream stream = NULL
#else
        XPUStream stream
#endif
        );

/// \brief  设定下一次函数调用的参数。
///
///         XPURT不会解析函数参数，而是直接将其透传给被调用的XPU函数。本函数会将 \p arg
///         指向的内容拷贝到参数 \p offset 偏移的位置，拷贝大小为 \p size ，这里
///         \p offset 要求4字节对齐，\p size 无此限制，但是该函数会将 \p size 向上
///         对齐到4，并将参数对齐部分补0。
/// \note   调用 \ref xpu_launch_async 之前需要先调用本函数
/// \param[in] arg     存储参数的CPU内存地址
/// \param[in] size    需要拷贝的参数字节数
/// \param[in] offset  目标参数的偏移字节数
DLL_PUBLIC int xpu_launch_argument_set(const void *arg, size_t size, size_t offset);

/// \brief  调用一个XPU函数。
///
///         该函数为异步函数，如果调用配置中指定了Stream，该函数会在指定Stream上调用，否则使用
///         默认Stream进行调用。指定Stream的情况下，所指定的Stream必须是在当前工作设备上创建的。
///
///         确保在调用XPU函数之前使用 \ref xpu_launch_config 和 \ref xpu_launch_argument_set
///         配置了调用配置和调用参数。
/// \param[in] func  待调用的XPU函数
DLL_PUBLIC int xpu_launch_async(XPUFunc func);

/// \brief  等待目标Stream上所有的函数调用完成。
///
///         如果不指定Stream，则等待默认Stream上所有的函数调用完成。
/// \param[in] stream  待同步的XPU Stream
DLL_PUBLIC int xpu_wait(
#if defined(__cplusplus)
        XPUStream stream = NULL
#else
        XPUStream stream
#endif
        );

/// \brief  获取上一次调用的函数的执行时间（ns）。
/// \note  该函数进可用于模拟器模式与PROFILING模式
/// \param[out] value  返回函数的执行时间（ns）
DLL_PUBLIC int xpu_last_kernel_exec_time(uint64_t *value);

/// \brief  在有 klprof 处于 daemon 运行的前提下，
///
///         通过这两个函数控制 klprof 起始和结束时间
DLL_PUBLIC int xpu_profiler_start();
DLL_PUBLIC int xpu_profiler_stop();


DLL_PUBLIC int xpu_mmap(void **pcpuptr, void *devptr, size_t sz);

/// \brief  获取与目标设备通过指定ccix端口互接的对应设备编号
///         若对应连接不存在，则返回负值
/// \note   ccix端口号(逻辑)范围为0-3，对应实际物理编号的4-7
/// \param[in]  dev_id      目标设备的逻辑编号
/// \param[in]  port        ccix端口的逻辑编号
/// \param[out] peer_dev_id 返回互接设备编号
DLL_PUBLIC int xpu_get_ccix_peer_id(int dev_id, int port, int* peer_dev_id);

/// \brief  打印设备端printf或dump的数据，打印完成后会清除数据
/// \note   目前仅支持昆仑2设备
/// \param[in]  stream      打印数据的kernel所属的stream
DLL_PUBLIC int xpu_kernel_debug(
#if defined(__cplusplus)
        XPUStream stream = NULL
#else
        XPUStream stream
#endif
        );

/// \brief  清除设备端printf或dump的数据
/// \note   目前仅支持昆仑2设备
DLL_PUBLIC int xpu_kernel_debug_reset();

#if defined(__cplusplus)
} // extern "C"
#endif /* __cplusplus */
