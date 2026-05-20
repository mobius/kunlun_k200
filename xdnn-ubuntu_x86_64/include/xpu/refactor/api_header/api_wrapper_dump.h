#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NEW_WRAPPER_DUMP_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NEW_WRAPPER_DUMP_H
#include "xpu/refactor/impl_public/function_info.h"
#include "xpu/refactor/impl_public/json.h"
#include "xpu/refactor/impl_public/wrapper_check.h"
#include "xpu/xdnn.h"
#include "xpu/refactor/api_header/api_wrapper_dump_util.h"

namespace baidu {
namespace xpu {
namespace api {

#ifndef LIB_NAME
#define LIB_NAME "XDNN"
#endif

// TODO: (zhuanghaoqi) 未来从 wrapper_check.h 中逐渐搬运 wrapper dump 相关内容到该头文件中
// clang-format on

#define REFACTOR_WRAPPER_DUMP(ctx)                                                                                     \
    if (debug_any_enable(ctx)) {                                                                                       \
        if (enable_json_trace) {                                                                                       \
            _internal_stack_guard.json_trace();                                                                        \
        } else {                                                                                                       \
            _internal_stack_guard.trace();                                                                             \
        }                                                                                                              \
    }

////// origin dump path (gtest string) //////
// 应该可以跳过不用，直接使用 json 格式的新 dump 宏。 TODO: remove useless code in the future
#define _DO_WRAPPER_DUMP_TYPE_(T0)                                                                                     \
    _internal_stack_guard.f.template_vec.push_back(baidu::xpu::api::basic_type_to_str<T0>());

#define REFACTOR_WRAPPER_DUMP_FUNCTION(ctx, func_name, ...)                                                            \
    WRAPPER_CHECK_CTX(ctx) /* check ctx */                                                                             \
    bool enable_json_trace = false;                                                                                    \
    if (debug_any_enable(ctx)) {                                                                                       \
        _internal_stack_guard.f.fname = func_name;                                                                     \
        _internal_stack_guard.profile(func_name);                                                                      \
        WRAPPER_NESTED_DEFINE(_DO_WRAPPER_DUMP_TYPE_, __VA_ARGS__); /* dump types */                                   \
    }

#define _DO_WRAPPER_DUMP_PARAM_(ctx, p0) _internal_stack_guard.f.add_param(ctx, p0);
#define REFACTOR_WRAPPER_DUMP_PARAMS(ctx, ...)                                                                         \
    if (debug_any_enable(ctx)) {                                                                                       \
        WRAPPER_NESTED_DEFINE_WITH_CTX(_DO_WRAPPER_DUMP_PARAM_, ctx, __VA_ARGS__);                                     \
    }                                                                                                                  \
    REFACTOR_WRAPPER_DUMP(ctx) /* read dump invoke */

////// new dump path (json as string or save to file) //////
#define REFACTOR_WRAPPER_CHECK_CTX(ctx)                                                                                \
    if (ctx == nullptr) {                                                                                              \
        return baidu::xpu::api::INVALID_PARAM;                                                                         \
    }                                                                                                                  \
    baidu::xpu::api::ContextStackGuard _internal_stack_guard(ctx, LIB_NAME);

#define _DO_WRAPPER_DUMP_TYPE_JSON_(T0)                                                                                \
    _internal_stack_guard.f.type_raw_vec.push_back(baidu::xpu::api::basic_type_to_str<T0>());
#define REFACTOR_WRAPPER_DUMP_FUNCTION_JSON(ctx, func_name, ...)                                                       \
    REFACTOR_WRAPPER_CHECK_CTX(ctx) /* check ctx */                                                                    \
    bool enable_json_trace = true;                                                                                     \
    if (debug_any_enable(ctx)) {                                                                                       \
        _internal_stack_guard.f.fname = func_name;                                                                     \
        _internal_stack_guard.profile(func_name);                                                                      \
        WRAPPER_NESTED_DEFINE(_DO_WRAPPER_DUMP_TYPE_JSON_, __VA_ARGS__); /* dump types */                              \
    }

#define XDNN_EXTRA_DUMP_INFO() _internal_stack_guard.f.add_extra_info("xdnn_commit", std::string(__XDNN_COMMIT));

#define _DO_WRAPPER_DUMP_TYPE_JSON_CTX_(ctx, x) _internal_stack_guard.f.add_param(ctx, #x, x);
#define REFACTOR_WRAPPER_DUMP_PARAMS_JSON(ctx, ...)                                                                    \
    if (debug_any_enable(ctx)) {                                                                                       \
        WRAPPER_NESTED_DEFINE_WITH_CTX(_DO_WRAPPER_DUMP_TYPE_JSON_CTX_, ctx, __VA_ARGS__)                              \
    }                                                                                                                  \
    REFACTOR_WRAPPER_DUMP(ctx) /* read dump invoke */
// TODO: 验证可用，写文档
#define REFACTOR_WRAPPER_DUMP_PARAMS_JSON_EXTRA(EXTRA_FUNC, ctx, ...)                                                  \
    if (debug_any_enable(ctx)) {                                                                                       \
        WRAPPER_NESTED_DEFINE_WITH_CTX(_DO_WRAPPER_DUMP_TYPE_JSON_CTX_, ctx, __VA_ARGS__)                              \
    }                                                                                                                  \
    EXTRA_FUNC();                                                                                                      \
    REFACTOR_WRAPPER_DUMP(ctx) /* read dump invoke */

#define XDNN_WRAPPER_DUMP_PARAMS_JSON(ctx, ...)                                                                        \
    REFACTOR_WRAPPER_DUMP_PARAMS_JSON_EXTRA(XDNN_EXTRA_DUMP_INFO, ctx, __VA_ARGS__)

// clang-format on
} // namespace api
} // namespace xpu
} // namespace baidu

#endif