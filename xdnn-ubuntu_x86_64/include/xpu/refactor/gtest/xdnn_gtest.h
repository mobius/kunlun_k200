#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_H

#include <map>
#include <cmath>
#include <time.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "xpu/refactor/core/quant.h"
#include "xpu/refactor/gtest/tensor.h"
#include "xpu/refactor/gtest/quant_tensor.h"
#include "xpu/refactor/impl_public/xdnn_time.h"
#include "xpu/refactor/impl_public/npy_dump.h"
#include "xpu/refactor/impl_public/function_info.h"
#include "xpu/refactor/impl_public/xdnn_to_str.h"
#include "xpu/refactor/gtest/gtest_util.h"
#include "xpu/refactor/api_header/api_test_case_loader.h"

namespace xdnn = baidu::xpu::api;
namespace api = baidu::xpu::api;
static const std::string GM = "GM";
static const std::string L3 = "L3";
static const std::string NIL = "NULL";

DECLARE_int32(device_id);
DECLARE_int32(cpu_runloop);
DECLARE_int32(xpu_runloop);
DECLARE_int32(early_stop);
DECLARE_int32(ncluster);
DECLARE_int32(nsdnn);
DECLARE_int32(random_duration);
DECLARE_bool(print_perf);

int xdnn_gtest_main();

static int gtest_early_stop_time = 0;
static int gtest_run_time = 0;

int get_gtest_early_stop_time();
int get_gtest_cpu_runloop();
int get_gtest_xpu_runloop();
int get_gtest_ncluster_num();
int get_gtest_nsdnn_num();
int get_gtest_unittest_phase();
int get_gtest_run_time();
int get_gtest_random_duration();
bool get_gtest_print_perf();

inline void save_or_load_input_tensor(api::DeviceType dev, api::Tensor* cpu_tensor_ptr,
        api::Tensor* xpu_tensor_ptr, int unittest_phase = 0, bool need_move = true) {
    if (unittest_phase == 1) {
        ASSERT_EQ(0, cpu_tensor_ptr->save_to_npy(api::kINPUT));
    }
    if (unittest_phase == 2) {
        ASSERT_EQ(0, cpu_tensor_ptr->load_from_npy(api::kINPUT));
    }
    if (unittest_phase != 1 && unittest_phase != 3) {
        if (need_move) { // the first time call for reused tensor; we needs to alloc gm space
            (*xpu_tensor_ptr) = cpu_tensor_ptr->to(dev);
        } else if (unittest_phase == 2) {
            // the second time call for the situation that makes output tensor -> innout tensor
            // don't need to alloc gm space but need to cpy cpu input data to gm space
            xpu_tensor_ptr->do_memcpy(*cpu_tensor_ptr);
        }
    }
}

inline void save_or_load_input_tensor(api::DeviceType dev, api::QuantTensor* cpu_tensor_ptr,
        api::QuantTensor* xpu_tensor_ptr, int unittest_phase = 0) {
    if (unittest_phase == 1) {
        ASSERT_EQ(0, cpu_tensor_ptr->save_to_npy(api::kINPUT));
    }
    if (unittest_phase == 2) {
        ASSERT_EQ(0, cpu_tensor_ptr->load_from_npy(api::kINPUT));
    }
    if (unittest_phase != 1 && unittest_phase != 3) {
        (*xpu_tensor_ptr) = cpu_tensor_ptr->to(dev);
    }
}

inline void save_or_load_output_tensor(api::DeviceType dev, api::Tensor* tensor1_ptr,
        api::Tensor* tensor2_ptr, int unittest_phase = 0) {
    if (unittest_phase == 1) {
        ASSERT_EQ(0, tensor1_ptr->save_to_npy(api::kOUTPUT));
    }
    if (unittest_phase == 2) {
        ASSERT_EQ(0, tensor2_ptr->save_to_npy(api::kOUTPUT));
    }
    if (unittest_phase == 3) {
        ASSERT_EQ(0, tensor1_ptr->load_from_npy(api::kOUTPUT));
        ASSERT_EQ(0, tensor2_ptr->load_from_npy(dev, api::kOUTPUT));
    }
}

inline void save_or_load_output_tensor(api::DeviceType dev, api::QuantTensor* tensor1_ptr,
        api::QuantTensor* tensor2_ptr, int unittest_phase = 0) {
    if (unittest_phase == 1) {
        ASSERT_EQ(0, tensor1_ptr->save_to_npy(api::kOUTPUT));
    }
    if (unittest_phase == 2) {
        ASSERT_EQ(0, tensor2_ptr->save_to_npy(api::kOUTPUT));
    }
    if (unittest_phase == 3) {
        ASSERT_EQ(0, tensor1_ptr->load_from_npy(api::kOUTPUT));
        ASSERT_EQ(0, tensor2_ptr->load_from_npy(dev, api::kOUTPUT));
    }
}

template <typename TX, typename TB>
inline TX roundup_to_val(TX x, TB val) {
    if (val <= static_cast<TB>(1)) {
        return x;
    }
    return (x + static_cast<TX>(val) - 1) / static_cast<TX>(val) * static_cast<TX>(val);
}

#define GTEST_INIT_CTX(ctx, l3size)                                                     \
    init_seed(__FUNCTION__);                                                            \
    int64_t tensor_l3_size = 0;                                                         \
    int64_t reserved_l3_size = l3size;                                                  \
    int inplace_ret = 0;                                                                \
    std::vector<std::tuple<api::Tensor*, char**, bool>> l3_tensor_pending_list;         \
    void* __internal_l3ptr = nullptr;                                                   \
    char* __tensor_l3ptr = nullptr;                                                     \
    gtest_run_time = get_gtest_run_time();                                              \
    gtest_early_stop_time = get_gtest_early_stop_time();                                \
    int gtest_unittest_phase = get_gtest_unittest_phase();                              \
    if (gtest_unittest_phase >= 1 && gtest_unittest_phase <= 3) {                       \
        std::cout << "unittest_phase" << gtest_unittest_phase << " start!"<< std::endl; \
    }                                                                                   \
    int _gtest_ncluster_num = get_gtest_ncluster_num();                                 \
    int _gtest_nsdnn_num = get_gtest_nsdnn_num();                                       \
    api::function_info func_info;                                                       \
    std::string test_case_prefix;                                                       \
    if (gtest_early_stop_time != 0 && gtest_run_time > gtest_early_stop_time) {         \
        return;                                                                         \
    }                                                                                   \
    if (_gtest_ncluster_num > 0) {                                                      \
        if (_gtest_ncluster_num <= 4 && (ctx)->dev().type() == api::kXPU1) {            \
            (ctx)->set_ncluster(_gtest_ncluster_num);                                   \
        } else if (_gtest_ncluster_num <= 8 && (ctx)->dev().type() == api::kXPU2) {     \
            (ctx)->set_ncluster(_gtest_ncluster_num);                                   \
        } else if (_gtest_ncluster_num <= 12 && (ctx)->dev().type() == api::kXPU3) {    \
            (ctx)->set_ncluster(_gtest_ncluster_num);                                   \
        } else {                                                                        \
            std::cout << "The number of cluster is wrong!" << std::endl;                \
            return;                                                                     \
        }                                                                               \
    }                                                                                   \
    if (_gtest_nsdnn_num > 0) {                                                         \
        if (_gtest_nsdnn_num <= 4 && (ctx)->dev().type() == api::kXPU1) {               \
            (ctx)->set_nsdnn(_gtest_nsdnn_num);                                         \
        } else if (_gtest_nsdnn_num <= 6 && (ctx)->dev().type() == api::kXPU2) {        \
            (ctx)->set_nsdnn(_gtest_nsdnn_num);                                         \
        } else if (_gtest_nsdnn_num <= 12 && (ctx)->dev().type() == api::kXPU3) {       \
            (ctx)->set_nsdnn(_gtest_nsdnn_num);                                         \
        } else {                                                                        \
            std::cout << "The number of sdnn is wrong!" << std::endl;                   \
            return;                                                                     \
        }                                                                               \
    }

#define GTEST_INIT(ctx)                                                                 \
    GTEST_INIT_CTX(ctx, 0);

#define GTEST_UNITTEST_PHASE_UNIMPLEMENTED(ctx, op_name)                                                       \
    if (get_gtest_unittest_phase() >= 1 && get_gtest_unittest_phase() <= 3) {                                  \
        std::cout << "[XDNN_GTEST] " << op_name << " has not supported unittest phase function!" << std::endl; \
        return;                                                                                                \
    }

#define GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, is_l3, tensor, ptr, is_inplace)                       \
    if (is_l3) {                                                                                     \
        if (!is_inplace) {                                                                           \
            tensor_l3_size += roundup_to_val(tensor.numel() * Dtype_size(tensor.dtype()), 8);        \
        }                                                                                            \
        l3_tensor_pending_list.push_back(std::make_tuple<api::Tensor*, char**, bool>(&tensor,        \
                reinterpret_cast<char**>(reinterpret_cast<long>(&ptr)), is_inplace));                \
    }

#define GTEST_ALLOC_L3_TENSOR(ctx)                                                                          \
    auto __temp_l3_size = 0;                                                                                \
    for (int64_t i = 0; i < l3_tensor_pending_list.size(); ++i) {                                           \
        auto tensor_ptr = std::get<0>(l3_tensor_pending_list[i]);                                           \
        auto ptr_ptr = std::get<1>(l3_tensor_pending_list[i]);                                              \
        bool is_inplace = std::get<2>(l3_tensor_pending_list[i]);                                           \
        if (is_inplace) {                                                                                   \
            *ptr_ptr = tensor_ptr->data<char>();                                                            \
        } else {                                                                                            \
            *ptr_ptr = __tensor_l3ptr + __temp_l3_size;                                                     \
            __temp_l3_size += roundup_to_val(tensor_ptr->numel() * Dtype_size((tensor_ptr->dtype())), 8);   \
            ASSERT_LE(__temp_l3_size, tensor_l3_size);                                                      \
            tensor_ptr->to_l3((ctx)->dev(), *ptr_ptr);                                                      \
        }                                                                                                   \
    }

#define GTEST_XPU_START(ctx)                                                                    \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 3) {                   \
        int64_t total_l3_size = tensor_l3_size + reserved_l3_size;                              \
        if (total_l3_size > 0) {                                                                \
            xpu_malloc((void**)&__internal_l3ptr, total_l3_size, XPU_MEM_L3);                   \
            if (__internal_l3ptr == nullptr) {                                                  \
                std::cout << "fail to alloc L3, total size=" << total_l3_size << std::endl;     \
            } else {                                                                            \
                (ctx)->_l3_mgr.set(__internal_l3ptr, reserved_l3_size, true);                   \
                __tensor_l3ptr = (char*)__internal_l3ptr + reserved_l3_size;                    \
                std::cout << "alloc L3, total size=" << total_l3_size << std::endl;             \
                GTEST_ALLOC_L3_TENSOR(ctx);                                                     \
            }                                                                                   \
        }                                                                                       \
        int _gtest_internal_runloop = get_gtest_xpu_runloop();                                  \
        timeval _gtest_internal_time_start;                                                     \
        gettimeofday(&_gtest_internal_time_start, NULL);                                        \
        for (int _gtest_internal_iter = 0; _gtest_internal_iter < _gtest_internal_runloop;      \
             _gtest_internal_iter++) {


#define GTEST_XPU_END(ctx)                                                                   \
    }                                                                                        \
    xpu_wait((ctx)->xpu_stream);                                                             \
    timeval _gtest_internal_time_end;                                                        \
    gettimeofday(&_gtest_internal_time_end, NULL);                                           \
    uint32_t _gtest_internal_time_diff =                                                     \
            1000000 * (_gtest_internal_time_end.tv_sec - _gtest_internal_time_start.tv_sec); \
    _gtest_internal_time_diff +=                                                             \
            _gtest_internal_time_end.tv_usec - _gtest_internal_time_start.tv_usec;           \
    if (_gtest_internal_runloop == 0) {                                                      \
        _gtest_internal_time_diff = 0;                                                       \
    } else {                                                                                 \
        _gtest_internal_time_diff = _gtest_internal_time_diff / _gtest_internal_runloop;     \
    }                                                                                        \
    if (get_gtest_print_perf()) {                                                            \
        std::cout << "avg-time: " << _gtest_internal_time_diff << " us" << std::endl;        \
    }                                                                                        \
    }

#define GTEST_XPU_RUN_ONE_TIME_START(ctx) \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 3) {
#define GTEST_XPU_RUN_ONE_TIME_END(ctx) }

#define GTEST_XPU_END_DMA_FMT(ctx, total_rw, fmt, arg...)                                      \
    }                                                                                          \
    xpu_wait((ctx)->xpu_stream);                                                               \
    timeval _gtest_internal_time_end;                                                          \
    gettimeofday(&_gtest_internal_time_end, NULL);                                             \
    uint32_t _gtest_internal_time_diff =                                                       \
            1000000 * (_gtest_internal_time_end.tv_sec - _gtest_internal_time_start.tv_sec);   \
    _gtest_internal_time_diff +=                                                               \
            _gtest_internal_time_end.tv_usec - _gtest_internal_time_start.tv_usec;             \
    float _gtest_internal_us = _gtest_internal_time_diff / (float)_gtest_internal_runloop;     \
    float _gtest_internal_gbps = (total_rw) / 1.024 / 1.024 / 1024 * _gtest_internal_runloop / \
            _gtest_internal_time_diff;                                                         \
    if (get_gtest_print_perf()) {                                                              \
        printf(fmt, ##arg);                                                                    \
        printf(": avg-time = %u us; ", _gtest_internal_time_diff);                             \
        printf(": avg-dma = %f GB/s\n", _gtest_internal_gbps);                                 \
    }                                                                                          \
    }

#define GTEST_XPU_END_PROF(ctx, dmarw, macops, ewops)                                        \
    }                                                                                        \
    xpu_wait((ctx)->xpu_stream);                                                             \
    timeval _gtest_internal_time_end;                                                        \
    gettimeofday(&_gtest_internal_time_end, NULL);                                           \
    uint32_t _gtest_internal_time_diff =                                                     \
            1000000 * (_gtest_internal_time_end.tv_sec - _gtest_internal_time_start.tv_sec); \
    _gtest_internal_time_diff +=                                                             \
            _gtest_internal_time_end.tv_usec - _gtest_internal_time_start.tv_usec;           \
    _gtest_internal_time_diff = _gtest_internal_time_diff / _gtest_internal_runloop;         \
    float _gtest_internal_gbps = (dmarw) / 1.024 / 1.024 / 1024 / _gtest_internal_time_diff; \
    float _gtest_internal_mac_tops = (macops) / 1024.f / 1024.f / _gtest_internal_time_diff; \
    float _gtest_internal_ew_tops = (ewops) / 1300.0 / _gtest_internal_time_diff;            \
    if (get_gtest_print_perf()) {                                                            \
        std::cout << "avg-time: " << _gtest_internal_time_diff << " us; ";                   \
        std::cout << "avg-dma: " << _gtest_internal_gbps << " GB/s; ";                       \
        std::cout << "avg-mac: " << _gtest_internal_mac_tops << " TOPS; ";                   \
        std::cout << "avg-ew: " << _gtest_internal_ew_tops << " /cycle " << std::endl;       \
    }                                                                                        \
    }

#define GTEST_XPU_END_PROF_FMT(ctx, dmarw, macops, ewops, fmt, arg...)                       \
    }                                                                                        \
    xpu_wait((ctx)->xpu_stream);                                                             \
    timeval _gtest_internal_time_end;                                                        \
    gettimeofday(&_gtest_internal_time_end, NULL);                                           \
    uint32_t _gtest_internal_time_diff =                                                     \
            1000000 * (_gtest_internal_time_end.tv_sec - _gtest_internal_time_start.tv_sec); \
    _gtest_internal_time_diff +=                                                             \
            _gtest_internal_time_end.tv_usec - _gtest_internal_time_start.tv_usec;           \
    _gtest_internal_time_diff = _gtest_internal_time_diff / _gtest_internal_runloop;         \
    float _gtest_internal_gbps = (dmarw) / 1.024 / 1.024 / 1024 / _gtest_internal_time_diff; \
    float _gtest_internal_mac_tops = (macops) / 1024.f / 1024.f / _gtest_internal_time_diff; \
    float _gtest_internal_ew_tops = (ewops) / 1300.0 / _gtest_internal_time_diff;            \
    if (get_gtest_print_perf()) {                                                            \
        printf(fmt, ##arg);                                                                  \
        printf(": avg-time = %u us; ", _gtest_internal_time_diff);                           \
        printf(": avg-dma = %f GB/s; ", _gtest_internal_gbps);                               \
        printf(": avg-mac = %f TOPS; ", _gtest_internal_mac_tops);                           \
        printf(": avg-ew = %f /cycle\n", _gtest_internal_ew_tops);                           \
    }                                                                                        \
    }

#define GTEST_CPU_START(ctx)                                           \
    int _gtest_internal_cpu = get_gtest_cpu_runloop();                 \
    if (_gtest_internal_cpu == 1 && get_gtest_unittest_phase() != 2 && \
        get_gtest_unittest_phase() != 3) {
#define GTEST_CPU_END(ctx) }

#define GTEST_CPU_RUN_ONE_TIME_START(ctx) \
    if (get_gtest_unittest_phase() != 2 && get_gtest_unittest_phase() != 3) {
#define GTEST_CPU_RUN_ONE_TIME_END(ctx) }

#define APPLY_NPY(name0ptr, name1ptr, cpp_type, file)            \
    {                                                            \
        api::Dtype dt = api::CPPTypeToDtype<cpp_type>();         \
        auto t_npy = api::tensor(file, dt);                      \
        int64_t mem_size = t_npy.numel() * Dtype_size(dt);       \
        std::memcpy(name0ptr, t_npy.data<cpp_type>(), mem_size); \
        xpu_memcpy(                                              \
                (void*)name1ptr,                                 \
                (const void*)(t_npy.data<cpp_type>()),           \
                mem_size,                                        \
                XPUMemcpyKind::XPU_HOST_TO_DEVICE);              \
    }

#define APPLY_NPY_CPU(name0ptr, cpp_type, file)                  \
    {                                                            \
        api::Dtype dt = api::CPPTypeToDtype<cpp_type>();         \
        auto t_npy = api::tensor(file, dt);                      \
        int64_t mem_size = t_npy.numel() * Dtype_size(dt);       \
        std::memcpy(name0ptr, t_npy.data<cpp_type>(), mem_size); \
    }

#define APPLY_NPY_XPU(name1ptr, cpp_type, file)            \
    {                                                      \
        api::Dtype dt = api::CPPTypeToDtype<cpp_type>();   \
        auto t_npy = api::tensor(file, dt);                \
        int64_t mem_size = t_npy.numel() * Dtype_size(dt); \
        xpu_memcpy(                                        \
                (void*)name1ptr,                           \
                (const void*)(t_npy.data<cpp_type>()),     \
                mem_size,                                  \
                XPUMemcpyKind::XPU_HOST_TO_DEVICE);        \
    }

#define HEAD_MAX(ptr, cpp_type, len)    \
    {                                   \
        float max = -0x1.FFFFFEp127f;   \
        for (int i = 0; i < len; ++i) { \
            if (ptr[i] > max) {         \
                max = ptr[i];           \
            }                           \
            ptr[0] = max;               \
        }                               \
    }

#define GTEST_DEFINE_PTR(TYPE, str, cpu_tensor, xpu_tensor, cpu_ptr_name, xpu_ptr_name)                        \
    api::Tensor xpu_tensor(                                                                                    \
            1, cpu_tensor.dtype(), cpu_tensor.in_or_out_kind(), cpu_tensor.tensor_name());                     \
    save_or_load_input_tensor(dev, &cpu_tensor, &xpu_tensor, get_gtest_unittest_phase());                      \
    TYPE* cpu_ptr_name = cpu_tensor.template data<TYPE>();                                                     \
    TYPE* xpu_ptr_name = xpu_tensor.template data<TYPE>();                                                     \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor, xpu_ptr_name, false); \
    if (std::string(str) == "NULL") {                                                                          \
        cpu_ptr_name = nullptr;                                                                                \
        xpu_ptr_name = nullptr;                                                                                \
    }

#define GTEST_DEFINE_QUANT_PTR(                                                                                      \
        TYPE,                                                                                                        \
        str,                                                                                                         \
        max_str,                                                                                                     \
        cpu_tensor,                                                                                                  \
        xpu_tensor,                                                                                                  \
        cpu_ptr_name,                                                                                                \
        xpu_ptr_name,                                                                                                \
        cpu_maxptr_name,                                                                                             \
        xpu_maxptr_name)                                                                                             \
    api::QuantTensor xpu_tensor = cpu_tensor;                                                                        \
    save_or_load_input_tensor(dev, &cpu_tensor, &xpu_tensor, get_gtest_unittest_phase());                            \
    TYPE* cpu_ptr_name = cpu_tensor.val().data<TYPE>();                                                              \
    TYPE* xpu_ptr_name = xpu_tensor.val().data<TYPE>();                                                              \
    float* cpu_maxptr_name = cpu_tensor.max().data<float>();                                                         \
    float* xpu_maxptr_name = xpu_tensor.max().data<float>();                                                         \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor.val(),                       \
            xpu_ptr_name, false);                                                                                    \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(float, (max_str[0] == 'L' && max_str[1] == '3'),                                \
            xpu_tensor.max(), xpu_maxptr_name, false);                                                               \
    if (std::string(str) == "NULL") {                                                                                \
        cpu_ptr_name = nullptr;                                                                                      \
        xpu_ptr_name = nullptr;                                                                                      \
    }                                                                                                                \
    if (std::string(max_str) == "NULL") {                                                                            \
        cpu_maxptr_name = nullptr;                                                                                   \
        xpu_maxptr_name = nullptr;                                                                                   \
    }

#define GTEST_DEFINE_PTR_IN_LIST(                                                                             \
        TYPE, str, cpu_tensor, xpu_tensor, cpu_ptr, xpu_ptr, start, end, len)                                 \
    if (std::string(str) == "NULL") {                                                                         \
        cpu_ptr = nullptr;                                                                                    \
        xpu_ptr = nullptr;                                                                                    \
    } else {                                                                                                  \
        cpu_tensor = api::randfloat(start, end, len).astype(api::CPPTypeToDtype<TYPE>());                     \
        xpu_tensor = cpu_tensor.to(dev);                                                                      \
        cpu_ptr = cpu_tensor.template data<TYPE>();                                                           \
        xpu_ptr = xpu_tensor.template data<TYPE>();                                                           \
        GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor, xpu_ptr, false); \
    }

#define GTEST_DEFINE_PTR_LIST_1(                                                         \
        TYPE, str_list, cpu_tensor_list, xpu_tensor_list, cpu_ptr_list, xpu_ptr_list)    \
    ASSERT_EQ(str_list.size(), cpu_tensor_list.size());                                  \
    std::vector<api::Tensor> xpu_tensor_list(str_list.size());                           \
    std::vector<TYPE*> cpu_ptr_list(str_list.size(), nullptr);                           \
    std::vector<TYPE*> xpu_ptr_list(str_list.size(), nullptr);                           \
    for (size_t i = 0; i < str_list.size(); ++i) {                                       \
        GTEST_DEFINE_PTR(                                                                \
                TYPE,                                                                    \
                str_list[i],                                                             \
                cpu_tensor_list[i],                                                      \
                tmp_xpu_tensor,                                                          \
                temp_cpu_ptr_name,                                                       \
                temp_xpu_ptr_name);                                                      \
        xpu_tensor_list[i] = std::move(tmp_xpu_tensor);                                  \
        cpu_ptr_list[i] = temp_cpu_ptr_name;                                             \
        xpu_ptr_list[i] = temp_xpu_ptr_name;                                             \
        if (str_list[i][0] == 'L' && str_list[i][1] == '3') {                            \
            std::get<0>(l3_tensor_pending_list.back()) = &xpu_tensor_list[i];            \
            std::get<1>(l3_tensor_pending_list.back())                                   \
                = reinterpret_cast<char**>(reinterpret_cast<long>(&xpu_ptr_list[i]));    \
        }                                                                                \
    }

#define GTEST_DEFINE_PTR_LIST(                                    \
        TYPE,                                                     \
        PTR_TYPE,                                                 \
        str_list,                                                 \
        cpu_tensor_list,                                          \
        xpu_tensor_list,                                          \
        cpu_ptr_list,                                             \
        xpu_ptr_list,                                             \
        start,                                                    \
        end,                                                      \
        len_list)                                                 \
    ASSERT_EQ(str_list.size(), len_list.size());                  \
    std::vector<api::Tensor> cpu_tensor_list(str_list.size());    \
    std::vector<api::Tensor> xpu_tensor_list(str_list.size());    \
    std::vector<PTR_TYPE> cpu_ptr_list(str_list.size(), nullptr); \
    std::vector<PTR_TYPE> xpu_ptr_list(str_list.size(), nullptr); \
    for (int i = 0; i < str_list.size(); i++) {                   \
        GTEST_DEFINE_PTR_IN_LIST(                                 \
                TYPE,                                             \
                str_list[i],                                      \
                cpu_tensor_list[i],                               \
                xpu_tensor_list[i],                               \
                cpu_ptr_list[i],                                  \
                xpu_ptr_list[i],                                  \
                start,                                            \
                end,                                              \
                len_list[i]);                                     \
    }

#define GTEST_DEFINE_QUANT_PTR_IN_LIST(                                                   \
        TYPE,                                                                             \
        str,                                                                              \
        max_str,                                                                          \
        cpu_tensor,                                                                       \
        xpu_tensor,                                                                       \
        cpu_ptr,                                                                          \
        xpu_ptr,                                                                          \
        cpu_maxptr,                                                                       \
        xpu_maxptr,                                                                       \
        start,                                                                            \
        end,                                                                              \
        len)                                                                              \
    if (str == "NULL") {                                                                  \
        cpu_ptr = nullptr;                                                                \
        xpu_ptr = nullptr;                                                                \
        cpu_maxptr = nullptr;                                                             \
        xpu_maxptr = nullptr;                                                             \
    } else {                                                                              \
        cpu_tensor = Quant(api::randfloat(start, end, len), api::CPPTypeToDtype<TYPE>()); \
        xpu_tensor = cpu_tensor.to(dev);                                                  \
        cpu_ptr = cpu_tensor.val().data<TYPE>();                                          \
        xpu_ptr = xpu_tensor.val().data<TYPE>();                                          \
        cpu_maxptr = cpu_tensor.max().data<float>();                                      \
        xpu_maxptr = xpu_tensor.max().data<float>();                                      \
        GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'),          \
                xpu_tensor.val(), xpu_ptr, false);                                        \
        if (max_str == "NULL") {                                                          \
            cpu_maxptr = nullptr;                                                         \
            xpu_maxptr = nullptr;                                                         \
        }                                                                                 \
        GTEST_ADD_L3_TENSOR_PENDING_LIST(float, (max_str[0] == 'L' && max_str[1] == '3'), \
                xpu_tensor.max(), xpu_maxptr, false);                                     \
    }

#define GTEST_DEFINE_QUANT_PTR_LIST(                                     \
        TYPE,                                                            \
        PTR_TYPE,                                                        \
        MAX_PTR_TYPE,                                                    \
        str_list,                                                        \
        max_str_list,                                                    \
        cpu_tensor_list,                                                 \
        xpu_tensor_list,                                                 \
        cpu_ptr_list,                                                    \
        xpu_ptr_list,                                                    \
        cpu_maxptr_list,                                                 \
        xpu_maxptr_list,                                                 \
        start,                                                           \
        end,                                                             \
        len_list)                                                        \
    ASSERT_EQ(str_list.size(), len_list.size());                         \
    ASSERT_EQ(max_str_list.size(), len_list.size());                     \
    std::vector<api::QuantTensor> cpu_tensor_list(                       \
            str_list.size(), Quant(api::Tensor(), api::kINT16));         \
    std::vector<api::QuantTensor> xpu_tensor_list(                       \
            str_list.size(), Quant(api::Tensor(), api::kINT16));         \
    std::vector<PTR_TYPE> cpu_ptr_list(str_list.size(), nullptr);        \
    std::vector<PTR_TYPE> xpu_ptr_list(str_list.size(), nullptr);        \
    std::vector<MAX_PTR_TYPE> cpu_maxptr_list(str_list.size(), nullptr); \
    std::vector<MAX_PTR_TYPE> xpu_maxptr_list(str_list.size(), nullptr); \
    for (int i = 0; i < str_list.size(); i++) {                          \
        GTEST_DEFINE_QUANT_PTR_IN_LIST(                                  \
                TYPE,                                                    \
                str_list[i],                                             \
                max_str_list[i],                                         \
                cpu_tensor_list[i],                                      \
                xpu_tensor_list[i],                                      \
                cpu_ptr_list[i],                                         \
                xpu_ptr_list[i],                                         \
                cpu_maxptr_list[i],                                      \
                xpu_maxptr_list[i],                                      \
                start,                                                   \
                end,                                                     \
                len_list[i]);                                            \
    }

#define GTEST_DEFINE_MAXPTR_IN_LIST(                                                             \
        str, cpu_tensor, xpu_tensor, cpu_ptr, xpu_ptr, start, end, len)                          \
    if (str == "NULL") {                                                                         \
        cpu_ptr = nullptr;                                                                       \
        xpu_ptr = nullptr;                                                                       \
    } else {                                                                                     \
        cpu_tensor = api::randfloat(start, end, len);                                            \
        cpu_ptr = cpu_tensor.template data<float>();                                             \
        cpu_tensor.template data<float>()[0] = api::quant_findmax<float>(cpu_ptr, len, nullptr); \
        xpu_tensor = cpu_tensor.to(dev);                                                         \
        xpu_ptr = xpu_tensor.template data<float>();                                             \
        if (str[0] == 'L' && str[1] == '3') {                                                    \
            xpu_tensor.to_l3(dev);                                                               \
            xpu_ptr = xpu_tensor.template data<float>();                                         \
        }                                                                                        \
    }

#define GTEST_DEFINE_MAXPTR_LIST(                                       \
        PTR_TYPE,                                                       \
        str_list,                                                       \
        cpu_tensor_list,                                                \
        xpu_tensor_list,                                                \
        cpu_ptr_list,                                                   \
        xpu_ptr_list,                                                   \
        start,                                                          \
        end,                                                            \
        ctx_xpu,                                                        \
        len_list)                                                       \
    std::vector<int> len_list(str_list.size(), ctx_xpu.max_ptr_size()); \
    std::vector<api::Tensor> cpu_tensor_list(str_list.size());          \
    std::vector<api::Tensor> xpu_tensor_list(str_list.size());          \
    std::vector<PTR_TYPE> cpu_ptr_list(str_list.size(), nullptr);       \
    std::vector<PTR_TYPE> xpu_ptr_list(str_list.size(), nullptr);       \
    for (int i = 0; i < str_list.size(); i++) {                         \
        GTEST_DEFINE_MAXPTR_IN_LIST(                                    \
                str_list[i],                                            \
                cpu_tensor_list[i],                                     \
                xpu_tensor_list[i],                                     \
                cpu_ptr_list[i],                                        \
                xpu_ptr_list[i],                                        \
                start,                                                  \
                end,                                                    \
                len_list[i]);                                           \
    }

namespace baidu {
namespace xpu {
namespace api {
class TensorPosMapping {
public:
    TensorPosMapping(){};
    std::map<std::string, Tensor*> map_cpu;
    std::map<std::string, Tensor*> map_xpu;
    template <typename TYPE>
    int add(bool is_write,
            api::DeviceType dev,
            std::string str,
            api::Tensor* cpu_tensor_ptr,
            api::Tensor* xpu_tensor_ptr,
            int unittest_phase = 0) {
        bool is_gm = (str[0] == 'G' && str[1] == 'M');
        bool is_l3 = (str[0] == 'L' && str[1] == '3');
        if (map_cpu.find(str) != map_cpu.end()) {  // already_exist
            if (is_write) {                        // write can only be first tensor
                return 3;
            }
            bool is_write_before_share = (map_cpu[str]->in_or_out_kind() == api::kOUTPUT);
            if (is_write_before_share) { // then the tensor shoule be INNOUT
                map_cpu[str]->set_in_or_out_kind(api::kINNOUT);
                map_xpu[str]->set_in_or_out_kind(api::kINNOUT);
                save_or_load_input_tensor(dev, map_cpu[str], map_xpu[str], unittest_phase, false);
            }
            cpu_tensor_ptr->share(map_cpu[str]);  // share data, change in_or_out_kind / tensor_name
            xpu_tensor_ptr->share(map_xpu[str]);  // share data, change in_or_out_kind / tensor_name
            return 1;
        }
        save_or_load_input_tensor(dev, cpu_tensor_ptr, xpu_tensor_ptr, unittest_phase);
        // NULL-cases
        if (str == "NULL") {
            return 0;
        }
        if (str.size() < 2) {
            return 2;  // wrong-name
        }
        if (is_gm || is_l3) {
            if (str.size() > 2) {  // set map
                map_cpu[str] = cpu_tensor_ptr;
                map_xpu[str] = xpu_tensor_ptr;
            }
            return 0;
        } else {
            return 2;  // wrong-name
        }
    }
};
}  // namespace api
}  // namespace xpu
}  // namespace baidu

#define GTEST_REUSE_PTR_DEFINE() api::TensorPosMapping gtest_tensor_pos_mapping;

#define GTEST_DEFINE_PTR_RO(TYPE, str, cpu_tensor, xpu_tensor, cpu_ptr_name, xpu_ptr_name)                          \
    api::Tensor xpu_tensor(                                                                                         \
            1, cpu_tensor.dtype(), cpu_tensor.in_or_out_kind(), cpu_tensor.tensor_name());                          \
    inplace_ret = gtest_tensor_pos_mapping.add<TYPE>(                                                               \
                    false, dev, str, &cpu_tensor, &xpu_tensor, get_gtest_unittest_phase());                         \
    ASSERT_EQ(true, (inplace_ret == 0 || inplace_ret == 1));                                                        \
    TYPE* cpu_ptr_name = cpu_tensor.data<TYPE>();                                                                   \
    TYPE* xpu_ptr_name = xpu_tensor.data<TYPE>();                                                                   \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor, xpu_ptr_name,              \
            (inplace_ret == 1));                                             \
    if (str == "NULL") {                                                                                            \
        cpu_ptr_name = nullptr;                                                                                     \
        xpu_ptr_name = nullptr;                                                                                     \
    }

#define GTEST_DEFINE_PTR_WO(TYPE, str, cpu_tensor, xpu_tensor, cpu_ptr_name, xpu_ptr_name)                          \
    api::Tensor xpu_tensor(                                                                                         \
            1, cpu_tensor.dtype(), cpu_tensor.in_or_out_kind(), cpu_tensor.tensor_name());                          \
    inplace_ret = gtest_tensor_pos_mapping.add<TYPE>(                                                               \
                    true, dev, str, &cpu_tensor, &xpu_tensor, get_gtest_unittest_phase());                          \
    ASSERT_EQ(true, (inplace_ret == 0 || inplace_ret == 1));                                                        \
    TYPE* cpu_ptr_name = cpu_tensor.data<TYPE>();                                                                   \
    TYPE* xpu_ptr_name = xpu_tensor.data<TYPE>();                                                                   \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor, xpu_ptr_name,              \
            (inplace_ret == 1));                                                                                    \
    if (str == "NULL") {                                                                                            \
        cpu_ptr_name = nullptr;                                                                                     \
        xpu_ptr_name = nullptr;                                                                                     \
    }

#define GTEST_DEFINE_QUANT_PTR_WO(                                                                           \
        TYPE,                                                                                                \
        str,                                                                                                 \
        max_str,                                                                                             \
        cpu_tensor,                                                                                          \
        xpu_tensor,                                                                                          \
        cpu_ptr_name,                                                                                        \
        xpu_ptr_name,                                                                                        \
        cpu_maxptr_name,                                                                                     \
        xpu_maxptr_name)                                                                                     \
    api::QuantTensor xpu_tensor = cpu_tensor;                                                                \
    inplace_ret = gtest_tensor_pos_mapping.add<TYPE>(true, dev, str,                                         \
            &(cpu_tensor.val()), &(xpu_tensor.val()), get_gtest_unittest_phase());                           \
    ASSERT_EQ(true, (inplace_ret == 0 || inplace_ret == 1));                                                 \
    TYPE* cpu_ptr_name = cpu_tensor.val().data<TYPE>();                                                      \
    TYPE* xpu_ptr_name = xpu_tensor.val().data<TYPE>();                                                      \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor.val(), xpu_ptr_name, \
            (inplace_ret == 1));                                                                             \
    if (str == "NULL") {                                                                                     \
        cpu_ptr_name = nullptr;                                                                              \
        xpu_ptr_name = nullptr;                                                                              \
    }                                                                                                        \
    inplace_ret = gtest_tensor_pos_mapping.add<float>(true, dev, max_str,                                    \
            &(cpu_tensor.max()), &(xpu_tensor.max()), get_gtest_unittest_phase());                           \
    ASSERT_EQ(true, (inplace_ret == 0 || inplace_ret == 1));                                                 \
    float* cpu_maxptr_name = cpu_tensor.max().data<float>();                                                 \
    float* xpu_maxptr_name = xpu_tensor.max().data<float>();                                                 \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(float, (max_str[0] == 'L' && max_str[1] == '3'), xpu_tensor.max(),      \
            xpu_maxptr_name, (inplace_ret == 1));                                                            \
    if (max_str == "NULL") {                                                                                 \
        cpu_maxptr_name = nullptr;                                                                           \
        xpu_maxptr_name = nullptr;                                                                           \
    }

#define GTEST_DEFINE_QUANT_PTR_RO(                                                                      \
        TYPE,                                                                                           \
        str,                                                                                            \
        max_str,                                                                                        \
        cpu_tensor,                                                                                     \
        xpu_tensor,                                                                                     \
        cpu_ptr_name,                                                                                   \
        xpu_ptr_name,                                                                                   \
        cpu_maxptr_name,                                                                                \
        xpu_maxptr_name)                                                                                \
    api::QuantTensor xpu_tensor = cpu_tensor;                                                           \
    inplace_ret = gtest_tensor_pos_mapping.add<TYPE>(false, dev, str,                                   \
            &(cpu_tensor.val()), &(xpu_tensor.val()), get_gtest_unittest_phase());                      \
    ASSERT_EQ(true, (inplace_ret == 0 || inplace_ret == 1));                                            \
    TYPE* cpu_ptr_name = cpu_tensor.val().data<TYPE>();                                                 \
    TYPE* xpu_ptr_name = xpu_tensor.val().data<TYPE>();                                                 \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor.val(),          \
            xpu_ptr_name, (inplace_ret == 1));                                                          \
    if (str == "NULL") {                                                                                \
        cpu_ptr_name = nullptr;                                                                         \
        xpu_ptr_name = nullptr;                                                                         \
    }                                                                                                   \
    inplace_ret = gtest_tensor_pos_mapping.add<float>(false, dev, max_str,                              \
            &(cpu_tensor.max()), &(xpu_tensor.max()), get_gtest_unittest_phase());                      \
    ASSERT_EQ(true, (inplace_ret == 0 || inplace_ret == 1));                                            \
    float* cpu_maxptr_name = cpu_tensor.max().data<float>();                                            \
    float* xpu_maxptr_name = xpu_tensor.max().data<float>();                                            \
    GTEST_ADD_L3_TENSOR_PENDING_LIST(float, (max_str[0] == 'L' && max_str[1] == '3'), xpu_tensor.max(), \
            xpu_maxptr_name, (inplace_ret == 1));                                                       \
    if (max_str == "NULL") {                                                                            \
        cpu_maxptr_name = nullptr;                                                                      \
        xpu_maxptr_name = nullptr;                                                                      \
    }

#define GTEST_DEFINE_PTR_IN_LIST_RO(                                                             \
        TYPE, str, cpu_tensor, xpu_tensor, cpu_ptr, xpu_ptr, start, end, len)                    \
    if (str == "NULL") {                                                                         \
        cpu_ptr = nullptr;                                                                       \
        xpu_ptr = nullptr;                                                                       \
    } else {                                                                                     \
        cpu_tensor = api::randfloat(start, end, len).astype(api::CPPTypeToDtype<TYPE>());        \
        inplace_ret = gtest_tensor_pos_mapping.add<TYPE>(                                        \
                        false, dev, str, &cpu_tensor, &xpu_tensor, get_gtest_unittest_phase());  \
        ASSERT_EQ(true, (inplace_ret == 0 || inplace_ret == 1));                                 \
        cpu_ptr = cpu_tensor.template data<TYPE>();                                              \
        xpu_ptr = xpu_tensor.template data<TYPE>();                                              \
        GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor,     \
            xpu_ptr, (inplace_ret == 1));                                                        \
    }

#define GTEST_DEFINE_PTR_LIST_RO(                                 \
        TYPE,                                                     \
        PTR_TYPE,                                                 \
        str_list,                                                 \
        cpu_tensor_list,                                          \
        xpu_tensor_list,                                          \
        cpu_ptr_list,                                             \
        xpu_ptr_list,                                             \
        start,                                                    \
        end,                                                      \
        len_list)                                                 \
    ASSERT_EQ(str_list.size(), len_list.size());                  \
    std::vector<api::Tensor> cpu_tensor_list(str_list.size());    \
    std::vector<api::Tensor> xpu_tensor_list(str_list.size());    \
    std::vector<PTR_TYPE> cpu_ptr_list(str_list.size(), nullptr); \
    std::vector<PTR_TYPE> xpu_ptr_list(str_list.size(), nullptr); \
    for (int i = 0; i < str_list.size(); i++) {                   \
        GTEST_DEFINE_PTR_IN_LIST_RO(                              \
                TYPE,                                             \
                str_list[i],                                      \
                cpu_tensor_list[i],                               \
                xpu_tensor_list[i],                               \
                cpu_ptr_list[i],                                  \
                xpu_ptr_list[i],                                  \
                start,                                            \
                end,                                              \
                len_list[i]);                                     \
    }

#define GTEST_DEFINE_PTR_IN_LIST_WO(                                                            \
        TYPE, str, cpu_tensor, xpu_tensor, cpu_ptr, xpu_ptr, start, end, len)                   \
    if (str == "NULL") {                                                                        \
        cpu_ptr = nullptr;                                                                      \
        xpu_ptr = nullptr;                                                                      \
    } else {                                                                                    \
        cpu_tensor = api::randfloat(start, end, len).astype(api::CPPTypeToDtype<TYPE>());       \
        inplace_ret = gtest_tensor_pos_mapping.add<TYPE>(                                       \
                        true, dev, str, &cpu_tensor, &xpu_tensor, get_gtest_unittest_phase());  \
        ASSERT_EQ(true, (inplace_ret == 0 || inplace_ret == 1));                                \
        cpu_ptr = cpu_tensor.template data<TYPE>();                                             \
        xpu_ptr = xpu_tensor.template data<TYPE>();                                             \
        GTEST_ADD_L3_TENSOR_PENDING_LIST(TYPE, (str[0] == 'L' && str[1] == '3'), xpu_tensor,    \
            xpu_ptr, (inplace_ret == 1));                                                       \
    }

#define GTEST_DEFINE_PTR_LIST_WO(                                 \
        TYPE,                                                     \
        PTR_TYPE,                                                 \
        str_list,                                                 \
        cpu_tensor_list,                                          \
        xpu_tensor_list,                                          \
        cpu_ptr_list,                                             \
        xpu_ptr_list,                                             \
        start,                                                    \
        end,                                                      \
        len_list)                                                 \
    ASSERT_EQ(str_list.size(), len_list.size());                  \
    std::vector<api::Tensor> cpu_tensor_list(str_list.size());    \
    std::vector<api::Tensor> xpu_tensor_list(str_list.size());    \
    std::vector<PTR_TYPE> cpu_ptr_list(str_list.size(), nullptr); \
    std::vector<PTR_TYPE> xpu_ptr_list(str_list.size(), nullptr); \
    for (int i = 0; i < str_list.size(); i++) {                   \
        GTEST_DEFINE_PTR_IN_LIST_WO(                              \
                TYPE,                                             \
                str_list[i],                                      \
                cpu_tensor_list[i],                               \
                xpu_tensor_list[i],                               \
                cpu_ptr_list[i],                                  \
                xpu_ptr_list[i],                                  \
                start,                                            \
                end,                                              \
                len_list[i]);                                     \
    }

#define GTEST_GEN_HASH_FUNCTION_T1(name, TA)                                  \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        test_case_prefix = name;                                              \
        func_info.fname = name;                                               \
        func_info.template_vec.push_back(api::basic_type_to_str<TA>());       \
    }

#define GTEST_GEN_HASH_FUNCTION_T2(name, TA, TB)                              \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        test_case_prefix = name;                                              \
        func_info.fname = name;                                               \
        func_info.template_vec.push_back(api::basic_type_to_str<TA>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TB>());       \
    }

#define GTEST_GEN_HASH_FUNCTION_T3(name, TA, TB, TC)                          \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        test_case_prefix = name;                                              \
        func_info.fname = name;                                               \
        func_info.template_vec.push_back(api::basic_type_to_str<TA>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TB>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TC>());       \
    }

#define GTEST_GEN_HASH_FUNCTION_T4(name, TA, TB, TC, TD)                      \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        test_case_prefix = name;                                              \
        func_info.fname = name;                                               \
        func_info.template_vec.push_back(api::basic_type_to_str<TA>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TB>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TC>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TD>());       \
    }

#define GTEST_GEN_HASH_FUNCTION_T5(name, TA, TB, TC, TD, TE)                  \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        test_case_prefix = name;                                              \
        func_info.fname = name;                                               \
        func_info.template_vec.push_back(api::basic_type_to_str<TA>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TB>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TC>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TD>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TE>());       \
    }

#define GTEST_GEN_HASH_FUNCTION_T6(name, TA, TB, TC, TD, TE, TF)              \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        test_case_prefix = name;                                              \
        func_info.fname = name;                                               \
        func_info.template_vec.push_back(api::basic_type_to_str<TA>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TB>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TC>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TD>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TE>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TF>());       \
    }

#define GTEST_GEN_HASH_FUNCTION_T7(name, TA, TB, TC, TD, TE, TF, TG)          \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        test_case_prefix = name;                                              \
        func_info.fname = name;                                               \
        func_info.template_vec.push_back(api::basic_type_to_str<TA>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TB>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TC>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TD>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TE>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TF>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TG>());       \
    }

#define GTEST_GEN_HASH_FUNCTION_T8(name, TA, TB, TC, TD, TE, TF, TG, TH)      \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        test_case_prefix = name;                                              \
        func_info.fname = name;                                               \
        func_info.template_vec.push_back(api::basic_type_to_str<TA>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TB>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TC>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TD>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TE>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TF>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TG>());       \
        func_info.template_vec.push_back(api::basic_type_to_str<TH>());       \
    }

#define GTEST_GEN_HASH_PARAM1(p0)                                             \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        func_info.add_param(&ctx_cpu, p0);                                    \
    }

#define GTEST_GEN_HASH_PARAM2(p0, p1)                                         \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        func_info.add_param(&ctx_cpu, p0);                                    \
        func_info.add_param(&ctx_cpu, p1);                                    \
    }

#define GTEST_GEN_HASH_PARAM3(p0, p1, p2)                                     \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        func_info.add_param(&ctx_cpu, p0);                                    \
        func_info.add_param(&ctx_cpu, p1);                                    \
        func_info.add_param(&ctx_cpu, p2);                                    \
    }

#define GTEST_GEN_HASH_PARAM4(p0, p1, p2, p3)                                 \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        func_info.add_param(&ctx_cpu, p0);                                    \
        func_info.add_param(&ctx_cpu, p1);                                    \
        func_info.add_param(&ctx_cpu, p2);                                    \
        func_info.add_param(&ctx_cpu, p3);                                    \
    }

#define GTEST_GEN_HASH_PARAM5(p0, p1, p2, p3, p4)                             \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        func_info.add_param(&ctx_cpu, p0);                                    \
        func_info.add_param(&ctx_cpu, p1);                                    \
        func_info.add_param(&ctx_cpu, p2);                                    \
        func_info.add_param(&ctx_cpu, p3);                                    \
        func_info.add_param(&ctx_cpu, p4);                                    \
    }

#define GTEST_GEN_HASH_PARAM6(p0, p1, p2, p3, p4, p5)                         \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 || \
        get_gtest_unittest_phase() == 3) {                                    \
        func_info.add_param(&ctx_cpu, p0);                                    \
        func_info.add_param(&ctx_cpu, p1);                                    \
        func_info.add_param(&ctx_cpu, p2);                                    \
        func_info.add_param(&ctx_cpu, p3);                                    \
        func_info.add_param(&ctx_cpu, p4);                                    \
        func_info.add_param(&ctx_cpu, p5);                                    \
    }

#define GTEST_GEN_HASH_END()                                                                   \
    if (get_gtest_unittest_phase() == 1 || get_gtest_unittest_phase() == 2 ||                  \
        get_gtest_unittest_phase() == 3) {                                                     \
        test_case_prefix +=                                                                    \
                "_" + std::to_string(std::hash<std::string>()(func_info.get_trace(&ctx_cpu))); \
    }

#define GTEST_INIT_TENSOR(IN_OR_OUT, TENSOR_NAME, DATA_TYPE, numel, random_func, ...)              \
    api::Tensor TENSOR_NAME##0(numel, api::CPPTypeToDtype<DATA_TYPE>());                           \
    if (get_gtest_unittest_phase() != 2 && get_gtest_unittest_phase() != 3) {                      \
        TENSOR_NAME##0 = random_func(__VA_ARGS__, numel).astype(api::CPPTypeToDtype<DATA_TYPE>()); \
    }                                                                                              \
    (TENSOR_NAME##0).set_in_or_out_kind(IN_OR_OUT);                                                \
    (TENSOR_NAME##0).set_tensor_name(test_case_prefix + "_" + #TENSOR_NAME);

#define GTEST_FORCE_INIT_TENSOR(IN_OR_OUT, TENSOR_NAME, DATA_TYPE, numel, random_func, ...)        \
    api::Tensor TENSOR_NAME##0(numel, api::CPPTypeToDtype<DATA_TYPE>());                           \
    if (get_gtest_unittest_phase() != 2 && get_gtest_unittest_phase() != 3) {                      \
        TENSOR_NAME##0 = random_func(__VA_ARGS__, numel, xpu_seed(), IN_OR_OUT, test_case_prefix + \
            "_" + #TENSOR_NAME, true).astype(api::CPPTypeToDtype<DATA_TYPE>());                    \
    }                                                                                              \
    (TENSOR_NAME##0).set_in_or_out_kind(IN_OR_OUT);                                                \
    (TENSOR_NAME##0).set_tensor_name(test_case_prefix + "_" + #TENSOR_NAME);

#define GTEST_COPY_INIT_TENSOR(IN_OR_OUT, TENSOR_NAME, COPY_TENSOR_NAME)                      \
    api::Tensor TENSOR_NAME##0((COPY_TENSOR_NAME##0).numel(), (COPY_TENSOR_NAME##0).dtype()); \
    if (get_gtest_unittest_phase() != 2 && get_gtest_unittest_phase() != 3) {                 \
        TENSOR_NAME##0 = COPY_TENSOR_NAME##0;                                                 \
    }                                                                                         \
    (TENSOR_NAME##0).set_in_or_out_kind(IN_OR_OUT);                                           \
    (TENSOR_NAME##0).set_tensor_name(test_case_prefix + "_" + #TENSOR_NAME);

#define GTEST_INIT_TENSOR_LIST(                                                              \
        IN_OR_OUT, TENSOR_LIST_NAME, DATA_TYPE, numel_list, random_func, ...)                \
    std::vector<api::Tensor>(TENSOR_LIST_NAME##0)(numel_list.size());                        \
    for (size_t i = 0; i < numel_list.size(); ++i) {                                         \
        if (get_gtest_unittest_phase() != 2 && get_gtest_unittest_phase() != 3) {            \
            (TENSOR_LIST_NAME##0)[i] = random_func(__VA_ARGS__, numel_list[i])               \
                                               .astype(api::CPPTypeToDtype<DATA_TYPE>());    \
        } else {                                                                             \
            (TENSOR_LIST_NAME##0)[i] =                                                       \
                    api::Tensor(numel_list[i], api::CPPTypeToDtype<DATA_TYPE>());            \
        }                                                                                    \
        (TENSOR_LIST_NAME##0)[i].set_in_or_out_kind(IN_OR_OUT);                              \
        (TENSOR_LIST_NAME##0)[i].set_tensor_name(                                            \
                test_case_prefix + "_" + #TENSOR_LIST_NAME + "[" + std::to_string(i) + "]"); \
    }

#define GTEST_INIT_QUANT_TENSOR(TENSOR_NAME, DATA_TYPE) \
    api::QuantTensor q##TENSOR_NAME##0 = Quant(TENSOR_NAME##0, api::CPPTypeToDtype<DATA_TYPE>());

#define GTEST_CHECK_START()                                          \
    {

#define GTEST_CHECK_END() }

#define GTEST_TENSOR_CLOSE_WITH_RETRY(tensor1, tensor2, rtol, atol, begin, end)                                        \
    save_or_load_output_tensor(dev, &tensor1, &tensor2, get_gtest_unittest_phase());                                   \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 && get_gtest_cpu_runloop() == 1) {          \
        ASSERT_EQ(tensor1.dtype(), tensor2.dtype());                                                                   \
        if (get_gtest_print_tensor_diff()) {                                                                           \
            ASSERT_EQ(0, baidu::xpu::api::print_tensor_diff(tensor1, tensor2, begin, end));                            \
        }                                                                                                              \
        if (get_gtest_print_tensor()) {                                                                                \
            print_tensor(tensor1);                                                                                     \
            print_tensor(tensor2);                                                                                     \
        }                                                                                                              \
        ASSERT_EQ_WITH_RETRY(end - begin, baidu::xpu::api::count_allclose(tensor1, tensor2, rtol, atol, begin, end));  \
    }

#define GTEST_TENSOR_CLOSE(tensor1, tensor2, rtol, atol, begin, end)                                                   \
    save_or_load_output_tensor(dev, &tensor1, &tensor2, get_gtest_unittest_phase());                                   \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 && get_gtest_cpu_runloop() == 1) {          \
        ASSERT_EQ(tensor1.dtype(), tensor2.dtype());                                                                   \
        if (get_gtest_print_tensor_diff()) {                                                                           \
            ASSERT_EQ(0, baidu::xpu::api::print_tensor_diff(tensor1, tensor2, begin, end));                            \
        }                                                                                                              \
        if (get_gtest_print_tensor()) {                                                                                \
            print_tensor(tensor1);                                                                                     \
            print_tensor(tensor2);                                                                                     \
        }                                                                                                              \
        ASSERT_EQ(tensor1.numel(), baidu::xpu::api::count_allclose(tensor1, tensor2, rtol, atol, begin, end));         \
    }

#define GTEST_TENSOR_ALLCLOSE(tensor1, tensor2, rtol, atol)                                        \
    save_or_load_output_tensor(dev, &tensor1, &tensor2, get_gtest_unittest_phase());               \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 &&                      \
        get_gtest_cpu_runloop() == 1) {                                                            \
        ASSERT_EQ(tensor1.dtype(), tensor2.dtype());                                               \
        ASSERT_EQ(tensor1.numel(), tensor2.numel());                                               \
        if (get_gtest_print_tensor_diff()) {                                                       \
            ASSERT_EQ(0, baidu::xpu::api::print_tensor_diff(tensor1, tensor2));                    \
        }                                                                                          \
        if (get_gtest_print_tensor()) {                                                            \
            print_tensor(tensor1);                                                                 \
            print_tensor(tensor2);                                                                 \
        }                                                                                          \
        ASSERT_EQ(tensor1.numel(), baidu::xpu::api::count_allclose(tensor1, tensor2, rtol, atol)); \
    }

#define GTEST_RETRY_SCOPE_BEGIN(retry_num, retry_threashold, enable)                                                   \
    int __retry_count__ = 0;                                                                                           \
    int __retry_num__ = retry_num;                                                                                     \
    int __retry_threashold__ = retry_threashold;                                                                       \
    int __failed_count__ = 0;                                                                                          \
    int __failed_this_time__ = 0;                                                                                      \
    if (!enable) {                                                                                                     \
        __retry_num__ = 1;                                                                                             \
        __retry_threashold__ = 0;                                                                                      \
    }                                                                                                                  \
    while (__retry_count__++ < __retry_num__) {

#define GTEST_INIT_CTX_WITH_RETRY(ctx, l3size, retry_num, retry_threashold)                                            \
    GTEST_RETRY_SCOPE_BEGIN(retry_num, retry_threashold, true)                                                         \
    GTEST_INIT_CTX(ctx, l3size)

// Perform retry only when the parameter condition is true.
#define GTEST_INIT_CTX_WITH_RETRY_IF(ctx, l3size, retry_num, retry_threashold, condition)                              \
    GTEST_RETRY_SCOPE_BEGIN(retry_num, retry_threashold, condition)                                                    \
    GTEST_INIT_CTX(ctx, l3size)

#define GTEST_RETRY_SCOPE_END()                                                                                        \
    __failed_count__ += __failed_this_time__;                                                                          \
    __failed_this_time__ = 0;                                                                                          \
    }                                                                                                                  \
    ASSERT_LE(__failed_count__, __retry_threashold__);

#define GTEST_CLOSE_CTX(ctx)                                                                                           \
    {}
#define GTEST_CLOSE_CTX_WITH_RETRY(ctx) GTEST_RETRY_SCOPE_END()

#define ASSERT_EQ_WITH_RETRY(a, b)                                                                                     \
    if (a != b) {                                                                                                      \
        __failed_this_time__ = 1;                                                                                      \
    }

#define GTEST_TENSOR_ALLCLOSE_WITH_RETRY(tensor1, tensor2, rtol, atol)                                                 \
    save_or_load_output_tensor(dev, &tensor1, &tensor2, get_gtest_unittest_phase());                                   \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 && get_gtest_cpu_runloop() == 1) {          \
        ASSERT_EQ_WITH_RETRY(tensor1.dtype(), tensor2.dtype());                                                        \
        ASSERT_EQ_WITH_RETRY(tensor1.numel(), tensor2.numel());                                                        \
        if (get_gtest_print_tensor_diff()) {                                                                           \
            ASSERT_EQ_WITH_RETRY(0, baidu::xpu::api::print_tensor_diff(tensor1, tensor2));                             \
        }                                                                                                              \
        if (get_gtest_print_tensor()) {                                                                                \
            print_tensor(tensor1);                                                                                     \
            print_tensor(tensor2);                                                                                     \
        }                                                                                                              \
        ASSERT_EQ_WITH_RETRY(tensor1.numel(), baidu::xpu::api::count_allclose(tensor1, tensor2, rtol, atol));          \
    }

#define GTEST_TENSOR_SNR_CHECK(TOUT, TGEMM, tensor1, tensor2, acc_dim, has_act, rtol, atol)                \
    save_or_load_output_tensor(dev, &tensor1, &tensor2, get_gtest_unittest_phase());                       \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 &&                              \
            get_gtest_cpu_runloop() == 1) {                                                                \
        ASSERT_EQ(tensor1.dtype(), tensor2.dtype());                                                       \
        ASSERT_EQ(tensor1.numel(), tensor2.numel());                                                       \
        if (get_gtest_print_tensor_diff()) {                                                               \
            ASSERT_EQ(0, baidu::xpu::api::print_tensor_diff(tensor1, tensor2));                            \
        }                                                                                                  \
        if (get_gtest_print_tensor()) {                                                                    \
            print_tensor(tensor1);                                                                         \
            print_tensor(tensor2);                                                                         \
        }                                                                                                  \
        if (std::is_same<TOUT, int8_t>::value || std::is_same<TOUT, int16_t>::value                        \
                || baidu::xpu::api::check_snr<TOUT, TGEMM>(tensor1, tensor2, acc_dim, has_act) == false) { \
            ASSERT_EQ(tensor1.numel(), baidu::xpu::api::count_allclose(tensor1, tensor2, rtol, atol));     \
        }                                                                                                  \
    }

#define GTEST_QUANT_TENSOR_MAXCLOSE(tensor1, tensor2, rtol, atol)                    \
    save_or_load_output_tensor(dev, &tensor1, &tensor2, get_gtest_unittest_phase()); \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 &&        \
        get_gtest_cpu_runloop() == 1) {                                              \
        ASSERT_EQ(1, api::max_allclose(tensor1, tensor2, rtol, atol));               \
    }

#define GTEST_QUANT_TENSOR_MAXCLOSE_WITH_RETRY(tensor1, tensor2, rtol, atol)                                           \
    save_or_load_output_tensor(dev, &tensor1, &tensor2, get_gtest_unittest_phase());                                   \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 && get_gtest_cpu_runloop() == 1) {          \
        ASSERT_EQ_WITH_RETRY(1, api::max_allclose(tensor1, tensor2, rtol, atol));                                      \
    }

#define GTEST_CUSTOMIZED_CHECK_TENSOR(tensor1, tensor2)                              \
    save_or_load_output_tensor(dev, &tensor1, &tensor2, get_gtest_unittest_phase()); \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 &&        \
        get_gtest_unittest_phase() != 3) {                                           \
        tensor2 = tensor2.to(api::kCPU);                                             \
    }

#define GTEST_CUSTOMIZED_CHECK_START()                                        \
    if (get_gtest_unittest_phase() != 1 && get_gtest_unittest_phase() != 2 && \
        get_gtest_cpu_runloop() == 1) {
#define GTEST_CUSTOMIZED_CHECK_END() }

#endif
