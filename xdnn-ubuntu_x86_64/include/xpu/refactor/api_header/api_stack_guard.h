#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_API_STACK_GUARD_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_API_STACK_GUARD_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/refactor/impl_public/function_info.h"
#include "xpu/refactor/impl_public/xdnn_debug.h"
#include "xpu/refactor/api_header/api_env_lt.h"
namespace baidu {
namespace xpu {
namespace api {
class DLL_EXPORT ApiStackGuard {
private:
    Context* _ctx = nullptr;
public:
    int current_debug_level;
    function_info f;       // trace
    std::string _lib_name; // lib_name for dump log
    ApiStackGuard(const ApiStackGuard&) = delete;
    ApiStackGuard& operator=(const ApiStackGuard&) = delete;
    ApiStackGuard(std::string lib_name);
    ApiStackGuard(Context* ctx, std::string lib_name);
    void json_trace();
    ~ApiStackGuard();
};
} // namespace api
} // namespace xpu
} // namespace baidu
#endif //BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_API_STACK_GUARD_H