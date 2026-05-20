/// \file   types.h
/// \brief  XPU API types
/// \author shijiaxin01@baidu.com
/// \copyright (C) 2019 Baidu, Inc
#ifndef BAIDU_XPU_API_INCLUDE_XPU_TYPES_H
#define BAIDU_XPU_API_INCLUDE_XPU_TYPES_H

#include <cstdint>
#include <stdlib.h>
#include "xpu/refactor/util/float16.h"
#include "xpu/refactor/util/int_with_ll_t.h"
#include "xpu/refactor/util/int8_wo_t.h"
#include "xpu/refactor/util/int15_wo_t.h"
#include "xpu/refactor/context/newcontext.h"
#include "xpu/refactor/api_header/api_wrapper_dump_util.h"
#include "xpu/xdnn_error.h"
#include "xpu/dll_export.h"
#include <vector>

namespace baidu {
namespace xpu {
namespace api {

extern int do_host2device(Context* ctx, const void* src, void* dst, int64_t bytes);
template <typename T>
class DLL_EXPORT VectorParam {
public:
    const T* cpu;
    int64_t len;
    T* xpu;
    VectorParam to_xpu(ctx_guard& RAII) const {
        VectorParam ret{cpu, len, xpu};
        if (ret.xpu == nullptr && RAII._ctx->dev().type() != api::kCPU) {
            ret.xpu = RAII.alloc<T>(len);
            if (ret.xpu != nullptr) { // alloc success
                int copy_success = do_host2device(RAII._ctx, cpu, ret.xpu, len * sizeof(T));
                if (copy_success != 0) {
                    ret.xpu = nullptr;
                }
            }
        }
        return ret;
    }
};

static inline std::vector<std::vector<int64_t>> vvi32_to_vvi64(const std::vector<std::vector<int>>& vvi32) {
    std::vector<std::vector<int64_t>> vvi64(vvi32.size());
    for (size_t i = 0; i < vvi32.size(); ++i) {
        vvi64[i] = std::vector<int64_t>(vvi32[i].begin(), vvi32[i].end());
    }
    return vvi64;
}
static inline VectorParam<int64_t> vpi32_to_vpi64(const VectorParam<int>& vpi32, int64_t* vpi64_cpu_ptr) {
    for (int64_t i = 0; i < vpi32.len; ++i) {
        vpi64_cpu_ptr[i] = static_cast<int64_t>(vpi32.cpu[i]);
    }
    return VectorParam<int64_t>{vpi64_cpu_ptr, vpi32.len, nullptr};
}
static inline VectorParam<int> vpi64_to_vpi32(const VectorParam<int64_t>& vpi64, int* vpi32_cpu_ptr) {
    for (int64_t i = 0; i < vpi64.len; ++i) {
        vpi32_cpu_ptr[i] = static_cast<int>(vpi64.cpu[i]);
    }
    return VectorParam<int>{vpi32_cpu_ptr, vpi64.len, nullptr};
}

template<typename T>
struct type_to_string;

#define DECLARE_API_TYPE_TO_STRING_NEW(T) [&]() -> const char* { return #T; }()

}  // namespace api
}  // namespace xpu
}  // namespace baidu

#endif //BAIDU_XPU_API_INCLUDE_XPU_TYPES_H
