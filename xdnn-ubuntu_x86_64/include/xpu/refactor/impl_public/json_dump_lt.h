#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NEW_WRAPPER_DUMP_INFO_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NEW_WRAPPER_DUMP_INFO_H
#include "xpu/refactor/impl_public/json.h"
#include "xpu/refactor/api_header/api_env_lt.h"
#include "xpu/refactor/api_header/api_stack_guard.h"
#include "xpu/refactor/api_header/api_wrapper_dump_util.h"
////// This file provides a lightweight implementation of the JSON dump API.
namespace baidu {
namespace xpu {
namespace api {

#ifndef LIB_NAME
#define LIB_NAME "XDNN"
#endif

#define _DO_WRAPPER_DUMP_TYPE_JSON_A(T0)                                                                               \
    _internal_stack_guard_lt.f.type_raw_vec.push_back(baidu::xpu::api::basic_type_to_str<T0>());
#define REFACTOR_WRAPPER_DUMP_FUNCTION_JSON_LT(func_name, ...)                                                         \
    baidu::xpu::api::ApiStackGuard _internal_stack_guard_lt(LIB_NAME);                                                 \
    _internal_stack_guard_lt.f.fname = func_name;                                                                      \
    WRAPPER_NESTED_DEFINE(_DO_WRAPPER_DUMP_TYPE_JSON_A, __VA_ARGS__); /* dump types */

#define _DO_WRAPPER_DUMP_PARAM_LT(x) _internal_stack_guard_lt.f.add_param(#x, x);
#define REFACTOR_WRAPPER_DUMP_PARAMS_JSON_LT(...)                                                                      \
    if (baidu::xpu::api::EnvLT::get_env("XPUAPI_DEBUG") != nullptr) {                                                                   \
        WRAPPER_NESTED_DEFINE(_DO_WRAPPER_DUMP_PARAM_LT, __VA_ARGS__)                                                  \
        _internal_stack_guard_lt.json_trace();                                                                         \
    }

} // namespace api
} // namespace xpu
} // namespace baidu
#endif /* BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NEW_WRAPPER_DUMP_INFO_H */