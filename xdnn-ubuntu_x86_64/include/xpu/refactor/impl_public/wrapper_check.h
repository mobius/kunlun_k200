#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_WRAPPER_CHECK_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_WRAPPER_CHECK_H
#include "xpu/xdnn.h"
#include "xpu/refactor/impl_public/xdnn_debug.h"
#include "xpu/refactor/impl_public/xdnn_time.h"
#include "xpu/refactor/impl_public/xdnn_pointer.h"
#include "xpu/refactor/impl_public/xdnn_to_str.h"
#include "xpu/refactor/impl_public/memory_region.h"
#include "xpu/refactor/impl_public/function_info.h"
#include "xpu/refactor/impl_public/profiling.h"
#include "xpu/refactor/impl_public/json.h"
#include <climits>
#include <vector>
#include <stdio.h>

#ifndef LIB_NAME
#define LIB_NAME "XDNN"
#endif

namespace baidu {
namespace xpu {
namespace api {

class XpuPerfTimer {
public:
    std::string s;
    Device dev;
    void Start() {
        _start = std::chrono::steady_clock::now();
        xpu_last_kernel_exec_time(&profile_start);
    }
    void Stop() {
        _end = std::chrono::steady_clock::now();
        xpu_last_kernel_exec_time(&profile_end);
    }
    float duration_ns() {
        if (profile_end > profile_start) {
            return (profile_end - profile_start) / 1.f;
        } else {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(_end - _start).count();
        }
        return (profile_end - profile_start) / 1.f;
    }
    float dma_gbps(int64_t bytes) {
        return bytes / (1.024 * 1.024 * 1.024) / duration_ns();
    }
    float mac_tops(int64_t mac_op_cnt) {
        return mac_op_cnt / (duration_ns() / 1e9) / 1e12;
    }
    float ew_op_per_cycle(int64_t ew_op_cnt) {
        if (dev.type() == api::kXPU2) {
            return ew_op_cnt / (duration_ns() * 1.3);
        }
        if (dev.type() == api::kXPU1) {
            return ew_op_cnt / (duration_ns() * 1.0);
        }
        return 0.0;
    }
    XpuPerfTimer(Device dev) : dev(dev) {}
private:
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> _start;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> _end;
    uint64_t profile_start;
    uint64_t profile_end;
};
DLL_EXPORT std::string space_prefix(Context* ctx);

template <typename T> inline std::string __wrapper_tostring(T v) {
    return std::to_string(v);
}
template <typename T> inline std::string __wrapper_tostring(T* v) {
    return std::string("ptr(") + std::to_string(reinterpret_cast<unsigned long long>(v)) + ")";
}
template <> inline std::string __wrapper_tostring<std::nullptr_t>(std::nullptr_t v) {
    return std::string("nullptr");
}

template <typename T> static bool __wrapper_is_input(const T* ptr) {
    return true;
}
template <typename T> static bool __wrapper_is_input(T* ptr) {
    return false;
}
static void __wrapper_set_lenptr_when_not_null(std::nullptr_t, int64_t v) {}
static void __wrapper_set_lenptr_when_not_null(int* lenptr, int64_t v) {
    *lenptr = static_cast<int>(v);
}
static void __wrapper_set_lenptr_when_not_null(int64_t* lenptr, int64_t v) {
    *lenptr = v;
}
static inline bool check_int_overflow(Context* ctx, std::nullptr_t) {
    return ctx->dev().type() == api::kXPU1 || ctx->dev().type() == api::kXPU2;
}

static inline bool check_int_overflow(Context* ctx, int64_t* lenptr) {
    return ctx->dev().type() == api::kXPU1 || ctx->dev().type() == api::kXPU2;
}

static inline bool check_int_overflow(Context* ctx, int* lenptr) {
    return true;
}

static int64_t shape_to_len(const std::vector<int64_t>& shape, bool check_int_overflow = true) {
    int64_t final_len = 1;
    if (shape.size() == 0) {
        return -1;
    }
    for (auto dim : shape) {
        if (dim <= 0) {
            return -1;
        }
        final_len = final_len * dim;
        if (check_int_overflow && final_len > 0x7FFFFFFF) { // int32-overflow
            return -1;
        }
    }
    return final_len;
}

static int64_t shape_to_len_int64(const std::vector<int64_t>& shape, bool check_int_overflow = true) {
    int64_t final_len = 1;
    if (shape.size() == 0) {
        return -1;
    }
    for (auto dim : shape) {
        if (dim <= 0) {
            return -1;
        }
        final_len = final_len * dim;
        if (check_int_overflow && final_len > 0xFFFFFFFF) { // int64-overflow
            return -1;
        }
    }
    return final_len;
}

// TODO: need to remove int type shape when int64_t is ready for all ops
static int64_t shape_to_len(const std::initializer_list<int64_t>& shape, bool check_int_overflow = true) {
    return shape_to_len(std::vector<int64_t>(shape), check_int_overflow);
}

static int64_t shape_to_len_int64(const std::initializer_list<int64_t>& shape, bool check_int_overflow = true) {
    return shape_to_len_int64(std::vector<int64_t>(shape), check_int_overflow);
}

// TODO: need to remove int type shape when int64_t is ready for all ops
static int64_t shape_to_len(const std::vector<int>& shape, bool check_int_overflow = true) {
    int64_t final_len = 1;
    if (shape.size() == 0) {
        return -1;
    }
    for (auto dim : shape) {
        if (dim <= 0) {
            return -1;
        }
        final_len = final_len * dim;
        if (check_int_overflow && final_len > 0x7FFFFFFF) { // int32-overflow
            return -1;
        }
    }
    return final_len;
}

static int64_t shape_to_len_int64(const std::vector<int>& shape, bool check_int_overflow = true) {
    int64_t final_len = 1;
    if (shape.size() == 0) {
        return -1;
    }
    for (auto dim : shape) {
        if (dim <= 0) {
            return -1;
        }
        final_len = final_len * dim;
        if (check_int_overflow && final_len > 0xFFFFFFFF) { // int32-overflow
            return -1;
        }
    }
    return final_len;
}

DLL_EXPORT void _wrapper_check_dump_plan(Context* ctx, const std::vector<std::string>& plan_tiles);

class DLL_EXPORT ContextStackGuard {
private:
    Context* _ctx;
    KernelType _kernel_type;
public:
    int64_t total_bytes;
    profiling_data data;
    int current_debug_level;
    function_info f;                        // trace
    std::vector<memory_region> mr_vec;      // checksum & npy
    XpuPerfTimer* _timer_ptr;               // profile
    XpuPerfTimer* _timer_cpu_ptr;           // cpu profile
    std::string _lib_name; // lib_name for dump log
    ContextStackGuard(const ContextStackGuard&) = delete;
    ContextStackGuard& operator=(const ContextStackGuard&) = delete;
    ContextStackGuard(Context* ctx);
    ContextStackGuard(Context* ctx, std::string lib_name);
    void profile(const std::string& name);
    void trace();
    void json_trace();
    void set_kernel_type(KernelType type);
    KernelType kernel_type();
    ~ContextStackGuard();
    void clear_statistics();
    template<typename F, typename... Args>
    void calculate_profiling_data(const F& calc_func, Args... args) {
        data = calc_func(args...);
        total_bytes = data.total_bytes();
    }
};

int DLL_EXPORT _get_ctx_opcount(Context* ctx);
int DLL_EXPORT _get_stack_level(Context* ctx);
void DLL_EXPORT _log_compare_fail(Context* ctx, const char* file, int line,
        const std::string& a, const std::string& op, const std::string& b);
void DLL_EXPORT _log_unimplement(Context* ctx, const char* file, int line);
void DLL_EXPORT _log_invalid_shape(Context* ctx, const char* file, int line, const std::vector<int64_t>& shape);
// TODO: need to remove initializer_list type shape when int64_t is ready for all ops
void DLL_EXPORT _log_invalid_shape(Context* ctx, const char* file, int line, const std::initializer_list<int64_t>& shape);
// TODO: need to remove int type shape when int64_t is ready for all ops
void DLL_EXPORT _log_invalid_shape(Context* ctx, const char* file, int line, const std::vector<int>& shape);
void DLL_EXPORT _log_invalid_memory(Context* ctx, memory_region& mr);
void DLL_EXPORT _log_memory(ContextStackGuard& guard, Context* ctx, memory_region& mr);
// just for temporary,while runtime fix the bug ,use xpu_strerror
const char* DLL_EXPORT xre_strerror(int _errno);
void DLL_EXPORT _log_kernel_fail(Context* ctx, const char* file, int line, int ret);

#define WRAPPER_CHECK_CTX(ctx)                                                                                         \
    if (ctx == nullptr) {                                                                                              \
        return baidu::xpu::api::INVALID_PARAM;                                                                         \
    }                                                                                                                  \
    baidu::xpu::api::ContextStackGuard _internal_stack_guard(ctx, LIB_NAME);

#define WRAPPER_WARN_DEPRECATED(ctx, op_name) \
    ctx->impl->logger() << "[XDNN_WARNING] use deprecated api: " << op_name << std::endl;

// level0:check, return when fail
// level>=1:check, return and print when fail
#define WRAPPER_CHECK_SHAPE(ctx, len_ptr, ...) {        \
    do {                                                \
        int64_t __internal_len = shape_to_len(__VA_ARGS__, check_int_overflow(ctx, len_ptr)); \
        __wrapper_set_lenptr_when_not_null(len_ptr, __internal_len);\
        if (__internal_len <= 0) {                      \
            _log_invalid_shape(ctx, __FILE__, __LINE__, __VA_ARGS__);\
            return baidu::xpu::api::INVALID_PARAM;      \
        }                                               \
    } while(0);                                         \
}

// level0:check, return when fail
// level>=1:check, return and print when fail
#define WRAPPER_CHECK_SHAPE_INT64(ctx, len_ptr, ...) {        \
    do {                                                \
        int64_t __internal_len = shape_to_len_int64(__VA_ARGS__, check_int_overflow(ctx, len_ptr)); \
        __wrapper_set_lenptr_when_not_null(len_ptr, __internal_len);\
        if (__internal_len <= 0) {                      \
            _log_invalid_shape(ctx, __FILE__, __LINE__, __VA_ARGS__);\
            return baidu::xpu::api::INVALID_PARAM;      \
        }                                               \
    } while(0);                                         \
}

// 1. each sequence > 0
// 2. each lod element <= int32.max
// 3. totalsize < int32.max
template <typename TID>
static void __wrapper_check_lod(VectorParam<TID> lod, int dim, int64_t* max_seq_ptr, int64_t* total_len_ptr) {
    int batch = lod.len - 1;
    bool lod_is_valid = (batch > 0) && (lod.cpu != nullptr) && (lod.cpu[0] == 0);
    int seqlen_max = 0;
    if (lod_is_valid) {
        for (int i = 0; i < batch; i++) {
            int seqlen = lod.cpu[i + 1] - lod.cpu[i];
            seqlen_max = std::max<int>(seqlen_max, seqlen);
            lod_is_valid = lod_is_valid && (lod.cpu[i + 1] <= INT_MAX) && (seqlen > 0);
        }
    }
    if (!lod_is_valid) {
        *total_len_ptr = -1;
        *max_seq_ptr = -1;
    } else {
        *max_seq_ptr = seqlen_max;
        *total_len_ptr = shape_to_len({(int)lod.cpu[batch], dim});
    }
}

// 1. each sequence > 0
// 2. each lod element <= int64.max
// 3. totalsize < int64.max
template <> void __wrapper_check_lod(VectorParam<int64_t> lod, int dim, int64_t* max_seq_ptr, int64_t* total_len_ptr) {
    int batch = lod.len - 1;
    bool lod_is_valid = (batch > 0) && (lod.cpu != nullptr) && (lod.cpu[0] == 0);
    int64_t seqlen_max = 0;
    if (lod_is_valid) {
        for (int i = 0; i < batch; i++) {
            int64_t seqlen = lod.cpu[i + 1] - lod.cpu[i];
            seqlen_max = std::max<int64_t>(seqlen_max, seqlen);
            lod_is_valid = lod_is_valid && (seqlen > 0);
        }
    }
    if (!lod_is_valid) {
        *total_len_ptr = -1;
        *max_seq_ptr = -1;
    } else {
        *max_seq_ptr = seqlen_max;
        *total_len_ptr = shape_to_len({lod.cpu[batch], dim}, false);
    }
}

#define WRAPPER_CHECK_LOD_SHAPE(ctx, total_len_ptr, max_seq_ptr, VP, dim) {   \
    do {                                                \
        int64_t __internal_total_len;\
        int64_t __internal_max_seq;\
        __wrapper_check_lod(VP, dim, &__internal_max_seq, &__internal_total_len);\
        __wrapper_set_lenptr_when_not_null(total_len_ptr, __internal_total_len);\
        __wrapper_set_lenptr_when_not_null(max_seq_ptr, __internal_max_seq);    \
        if (__internal_total_len <= 0) {                                        \
            _log_invalid_shape(ctx, __FILE__, __LINE__, {-1, dim});             \
            return baidu::xpu::api::INVALID_PARAM;                              \
        }                                                                       \
    } while(0);                                                                 \
}
// level0:check, return when fail
// level1:check, return and print when fail
// level2:check, return and print when fail, checksum when success

#define WRAPPER_CHECK_PTR(ctx, T, len, ptr)                                                        \
    {                                                                                              \
        do {                                                                                       \
            baidu::xpu::api::memory_region mr(baidu::xpu::api::__wrapper_is_input(ptr), ptr, len,  \
                                              debug_checksum_level(ctx));                          \
            mr.init_detail<T>((debug_any_enable(ctx)), baidu::xpu::api::_get_ctx_opcount(ctx),     \
                              __FILE__, __LINE__, #ptr);                                           \
            bool __internal_is_valid = mr.is_valid<T>(ctx);                                        \
            if (!__internal_is_valid) {                                                            \
                _log_invalid_memory(ctx, mr);                                                      \
                return baidu::xpu::api::INVALID_PARAM;                                             \
            } else {                                                                               \
                _log_memory(_internal_stack_guard, ctx, mr);                                       \
            }                                                                                      \
        } while (0);                                                                               \
    }
#define WRAPPER_CHECK_PTR_WITH_LD(ctx, T, m, n, ld, ptr)                                           \
    {                                                                                              \
        do {                                                                                       \
            baidu::xpu::api::memory_region mr(baidu::xpu::api::__wrapper_is_input(ptr), ptr, m, n, \
                                              ld, debug_checksum_level(ctx));                      \
            mr.init_detail<T>((debug_any_enable(ctx)), baidu::xpu::api::_get_ctx_opcount(ctx),     \
                              __FILE__, __LINE__, #ptr);                                           \
            bool __internal_is_valid = mr.is_valid<T>(ctx);                                        \
            if (!__internal_is_valid) {                                                            \
                _log_invalid_memory(ctx, mr);                                                      \
                return baidu::xpu::api::INVALID_PARAM;                                             \
            } else {                                                                               \
                _log_memory(_internal_stack_guard, ctx, mr);                                       \
            }                                                                                      \
        } while (0);                                                                               \
    }

#define WRAPPER_UPDATE_IS_SDNN_KERNEL(ctx, is_sdnn_kernel)                                         \
    {                                                                                              \
        do {                                                                                       \
            if (debug_any_enable(ctx)) {                                                           \
                if (is_sdnn_kernel) {                                                              \
                    _internal_stack_guard.set_kernel_type(baidu::xpu::api::KernelType::SDNN);      \
                } else {                                                                           \
                    _internal_stack_guard.set_kernel_type(baidu::xpu::api::KernelType::CLUSTER);   \
                }                                                                                  \
            }                                                                                      \
        } while (0);                                                                               \
    }

#define WRAPPER_UPDATE_MAC_STAT(ctx, mac_cnt, mac_data_type)                                       \
    {                                                                                              \
        do {                                                                                       \
            if (debug_any_enable(ctx)) {                                                           \
                if (_internal_stack_guard.kernel_type() == baidu::xpu::api::KernelType::SDNN) {    \
                    _internal_stack_guard.data.set_mac_stat(mac_cnt, mac_data_type);               \
                }                                                                                  \
            }                                                                                      \
        } while (0);                                                                               \
    }

#define WRAPPER_CALCULATE_PROFILING_DATA(ctx, func, ...)                                           \
    {                                                                                              \
        do {                                                                                       \
            if (debug_any_enable(ctx)) {                                                           \
                _internal_stack_guard.calculate_profiling_data(func, __VA_ARGS__);                 \
                _internal_stack_guard.total_bytes = _internal_stack_guard.data.total_bytes();      \
            }                                                                                      \
        } while (0);                                                                               \
    }

#define WRAPPER_CHECK_PTR_OR_NULL(ctx, T, len, ptr)                                         \
    if (ptr != nullptr) {                                                                   \
        WRAPPER_CHECK_PTR(ctx, T, len, ptr);                                                \
    }

#define WRAPPER_CHECK_2PTRS(ctx, T, len, ptr0, ptr1)                                                                   \
    WRAPPER_CHECK_PTR(ctx, T, len, ptr0);                                                                              \
    WRAPPER_CHECK_PTR(ctx, T, len, ptr1);
#define WRAPPER_CHECK_3PTRS(ctx, T, len, ptr0, ptr1, ptr2)                                                             \
    WRAPPER_CHECK_2PTRS(ctx, T, len, ptr0, ptr1);                                                                      \
    WRAPPER_CHECK_PTR(ctx, T, len, ptr2);
#define WRAPPER_CHECK_4PTRS(ctx, T, len, ptr0, ptr1, ptr2, ptr3)                                                       \
    WRAPPER_CHECK_3PTRS(ctx, T, len, ptr0, ptr1, ptr2);                                                                \
    WRAPPER_CHECK_PTR(ctx, T, len, ptr3);
#define WRAPPER_CHECK_5PTRS(ctx, T, len, ptr0, ptr1, ptr2, ptr3, ptr4)                                                 \
    WRAPPER_CHECK_4PTRS(ctx, T, len, ptr0, ptr1, ptr2, ptr3);                                                          \
    WRAPPER_CHECK_PTR(ctx, T, len, ptr4);
#define WRAPPER_CHECK_6PTRS(ctx, T, len, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5)                                           \
    WRAPPER_CHECK_5PTRS(ctx, T, len, ptr0, ptr1, ptr2, ptr3, ptr4);                                                    \
    WRAPPER_CHECK_PTR(ctx, T, len, ptr5);

#define WRAPPER_CHECK_VP(ctx, T, VP)                \
    WRAPPER_ASSERT_NE(ctx, VP.cpu, nullptr);        \
    if (ctx->dev().type() != api::kCPU) {           \
        WRAPPER_CHECK_PTR(ctx, T, VP.len, VP.xpu);  \
    }

#define WRAPPER_PRINT_XPU_WAIT(ctx) ;
// #define WRAPPER_PRINT_INFO(ctx, str, var)                       \
//     if (ctx->dev().type() != api::kCPU) {                       \
//         if (debug_any_enable(ctx)){                             \
//             ctx->impl->logger()                                 \
//                 << std::string(4 * ctx->impl->_stack_level, ' ')\
//                 << str << var << std::endl;                     \
//         }                                                       \
//     }
//
// #define WRAPPER_PRINT_XPU_WAIT(ctx){                            \
//     do {                                                        \
//         WRAPPER_PRINT_INFO(ctx,                                 \
//             "[XDNN_XPU_WAIT_INFO]: Doing xpu_wait, XPUStream ", \
//             ctx->xpu_stream);                                   \
//     }while(0);                                                  \
// }

#define WRAPPER_DUMP_FUNCTION_T1(ctx, name, TA)                                 \
    if (debug_any_enable(ctx)) {                                                \
        _internal_stack_guard.profile(name);                                    \
        _internal_stack_guard.f.fname = name;                                   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
    }
#define WRAPPER_DUMP_FUNCTION_T2(ctx, name, TA, TB)                             \
    if (debug_any_enable(ctx)) {                                                \
        _internal_stack_guard.profile(name);                                    \
        _internal_stack_guard.f.fname = name;                                   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TB>());   \
    }
#define WRAPPER_DUMP_FUNCTION_T3(ctx, name, TA, TB, TC)                         \
    if (debug_any_enable(ctx)) {                                                \
        _internal_stack_guard.profile(name);                                    \
        _internal_stack_guard.f.fname = name;                                   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TB>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TC>());   \
    }
#define WRAPPER_DUMP_FUNCTION_T4(ctx, name, TA, TB, TC, TD)                     \
    if (debug_any_enable(ctx)) {                                                \
        _internal_stack_guard.profile(name);                                    \
        _internal_stack_guard.f.fname = name;                                   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TB>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TC>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TD>());   \
    }

#define WRAPPER_DUMP_FUNCTION_T5(ctx, name, TA, TB, TC, TD, TE)                 \
    if (debug_any_enable(ctx)) {                                 \
        _internal_stack_guard.profile(name);                                        \
        _internal_stack_guard.f.fname = name;                                       \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TB>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TC>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TD>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TE>());   \
    }

#define WRAPPER_DUMP_FUNCTION_T6(ctx, name, TA, TB, TC, TD, TE, TF)                 \
    if (debug_any_enable(ctx)) {                                 \
        _internal_stack_guard.profile(name);                                        \
        _internal_stack_guard.f.fname = name;                                       \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TB>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TC>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TD>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TE>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TF>());   \
    }

#define WRAPPER_DUMP_FUNCTION_T7(ctx, name, TA, TB, TC, TD, TE, TF, TG)                             \
    if (debug_any_enable(ctx)) {                                                                    \
        _internal_stack_guard.profile(name);                                                        \
        _internal_stack_guard.f.fname = name;                                                       \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TB>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TC>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TD>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TE>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TF>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TG>());   \
    }
#define WRAPPER_DUMP_FUNCTION_T8(ctx, name, TA, TB, TC, TD, TE, TF, TG, TH)                         \
    if (debug_any_enable(ctx)) {                                                                    \
        _internal_stack_guard.profile(name);                                                        \
        _internal_stack_guard.f.fname = name;                                                       \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TB>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TC>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TD>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TE>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TF>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TG>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TH>());   \
    }
#define WRAPPER_DUMP_FUNCTION_T9(ctx, name, TA, TB, TC, TD, TE, TF, TG, TH, TI)                         \
    if (debug_any_enable(ctx)) {                                                                    \
        _internal_stack_guard.profile(name);                                                        \
        _internal_stack_guard.f.fname = name;                                                       \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TA>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TB>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TC>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TD>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TE>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TF>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TG>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TH>());   \
        _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<TI>());   \
    }

#define WRAPPER_DUMP_PARAM1(ctx, p0)                                        \
    if (debug_any_enable(ctx)) {                                 \
        _internal_stack_guard.f.add_param(ctx,p0);                               \
    }
#define WRAPPER_DUMP_PARAM2(ctx, p0, p1)                                          \
    if (debug_any_enable(ctx)) {                                                  \
        _internal_stack_guard.f.add_param(ctx,p0);                               \
        _internal_stack_guard.f.add_param(ctx,p1);                               \
    }
#define WRAPPER_DUMP_PARAM3(ctx, p0, p1, p2)                                      \
    if (debug_any_enable(ctx)) {                                                  \
        _internal_stack_guard.f.add_param(ctx,p0);                               \
        _internal_stack_guard.f.add_param(ctx,p1);                               \
        _internal_stack_guard.f.add_param(ctx,p2);                               \
    }
#define WRAPPER_DUMP_PARAM4(ctx, p0, p1, p2, p3)                                  \
    if (debug_any_enable(ctx)) {                                                  \
        _internal_stack_guard.f.add_param(ctx,p0);                               \
        _internal_stack_guard.f.add_param(ctx,p1);                               \
        _internal_stack_guard.f.add_param(ctx,p2);                               \
        _internal_stack_guard.f.add_param(ctx,p3);                               \
    }
#define WRAPPER_DUMP_PARAM5(ctx, p0, p1, p2, p3, p4)                              \
    if (debug_any_enable(ctx)) {                                                  \
        _internal_stack_guard.f.add_param(ctx,p0);                               \
        _internal_stack_guard.f.add_param(ctx,p1);                               \
        _internal_stack_guard.f.add_param(ctx,p2);                               \
        _internal_stack_guard.f.add_param(ctx,p3);                               \
        _internal_stack_guard.f.add_param(ctx,p4);                               \
    }
#define WRAPPER_DUMP_PARAM6(ctx, p0, p1, p2, p3, p4, p5)                          \
    if (debug_any_enable(ctx)) {                                                  \
        _internal_stack_guard.f.add_param(ctx,p0);                               \
        _internal_stack_guard.f.add_param(ctx,p1);                               \
        _internal_stack_guard.f.add_param(ctx,p2);                               \
        _internal_stack_guard.f.add_param(ctx,p3);                               \
        _internal_stack_guard.f.add_param(ctx,p4);                               \
        _internal_stack_guard.f.add_param(ctx,p5);                               \
    }
    

#define WRAPPER_DUMP(ctx)                               \
    if (debug_any_enable(ctx)) {                        \
        _internal_stack_guard.trace();                  \
    }

#define WRAPPER_PRINT_PLAN(ctx, plan)                                   \
    if (debug_plan_enable(ctx)) {                                       \
        _wrapper_check_dump_plan(ctx, plan.to_string_with_name());      \
    }

#define WRAPPER_ASSERT_EQ(ctx, expra, exprb)    \
if (!((expra) == (exprb))) {                    \
    _log_compare_fail(ctx, __FILE__, __LINE__, baidu::xpu::api::__wrapper_tostring(expra), \
            "==", baidu::xpu::api::__wrapper_tostring(exprb));\
    return baidu::xpu::api::INVALID_PARAM;      \
}
#define WRAPPER_ASSERT_NE(ctx, expra, exprb)    \
if (!((expra) != (exprb))) {                    \
    _log_compare_fail(ctx, __FILE__, __LINE__, baidu::xpu::api::__wrapper_tostring(expra), \
            "!=", baidu::xpu::api::__wrapper_tostring(exprb));\
    return baidu::xpu::api::INVALID_PARAM;      \
}
#define WRAPPER_ASSERT_GT(ctx, expra, exprb)    \
if (!((expra) > (exprb))) {                     \
    _log_compare_fail(ctx, __FILE__, __LINE__, baidu::xpu::api::__wrapper_tostring(expra), \
            ">", baidu::xpu::api::__wrapper_tostring(exprb));\
    return baidu::xpu::api::INVALID_PARAM;      \
}
#define WRAPPER_ASSERT_GE(ctx, expra, exprb)    \
if (!((expra) >= (exprb))) {                    \
    _log_compare_fail(ctx, __FILE__, __LINE__, baidu::xpu::api::__wrapper_tostring(expra), \
            ">=", baidu::xpu::api::__wrapper_tostring(exprb));\
    return baidu::xpu::api::INVALID_PARAM;      \
}
#define WRAPPER_ASSERT_LT(ctx, expra, exprb)    \
if (!((expra) < (exprb))) {                     \
    _log_compare_fail(ctx, __FILE__, __LINE__, baidu::xpu::api::__wrapper_tostring(expra), \
            "<", baidu::xpu::api::__wrapper_tostring(exprb));\
    return baidu::xpu::api::INVALID_PARAM;      \
}
#define WRAPPER_ASSERT_LE(ctx, expra, exprb)    \
if (!((expra) <= (exprb))) {                    \
    _log_compare_fail(ctx, __FILE__, __LINE__, baidu::xpu::api::__wrapper_tostring(expra), \
            "<=", baidu::xpu::api::__wrapper_tostring(exprb));\
    return baidu::xpu::api::INVALID_PARAM;      \
}

#define WRAPPER_ASSERT_SUCCESS(ctx, ret)        \
if (!((ret) == baidu::xpu::api::SUCCESS)) {     \
    _log_compare_fail(ctx, __FILE__, __LINE__, baidu::xpu::api::__wrapper_tostring(ret), "==", "SUCCESS");\
    return ret;                                 \
}
#define WRAPPER_ASSERT_WORKSPACE(ctx, pointer)          \
if (!((pointer) != nullptr)) {                               \
    _log_compare_fail(ctx, __FILE__, __LINE__, baidu::xpu::api::__wrapper_tostring(pointer), "!=", "nullptr");\
    return baidu::xpu::api::NO_ENOUGH_WORKSPACE;        \
}

#define WRAPPER_UNIMPLEMENTED(ctx)                              \
_log_unimplement(ctx, __FILE__, __LINE__);                      \
return baidu::xpu::api::NOT_IMPLEMENT;

#define KERNEL_ASSERT_SUCCESS(ctx, ret)        \
if (!((ret) == XPU_SUCCESS)) {     \
    _log_kernel_fail(ctx, __FILE__, __LINE__, ret);\
    return baidu::xpu::api::RUNTIME_ERROR;                                 \
}

}
}
}
#endif
