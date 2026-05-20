#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_INTERNAL_VEC_INT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_INTERNAL_VEC_INT_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include "xpu/refactor/deprecation/deprecated.h"

namespace baidu {
namespace xpu {
namespace api {

template<typename TW> int conv2d_filter_change_group(Context* ctx,
        const TW* old_filter, TW* new_filter, int64_t f, int64_t c,
        const std::vector<int>& ksize, int64_t oldg, int64_t newg, bool is_nchw);
template<typename TW> TW* conv2d_filter_change_group(api::ctx_guard& RAII_GUARD, Context* ctx,
        const TW* old_filter, int64_t f, int64_t c, const std::vector<int>& ksize,
        int64_t oldg, int64_t newg, bool is_nchw);

int im2col_param_check(Context* ctx, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::vector<int>& ksize_extend, const std::vector<int>& stride_extend,
        const std::vector<int>& pad_extend, const std::vector<int>& dilation_extend,
        bool allow_pad_ksize_equal = false);
int im2col_param_check(Context* ctx, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::initializer_list<int64_t>& ksize_extend, const std::initializer_list<int64_t>& stride_extend,
        const std::initializer_list<int64_t>& pad_extend, const std::initializer_list<int64_t>& dilation_extend,
        bool allow_pad_ksize_equal = false);
int im2col3d_param_check(Context* ctx, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w,
        const std::vector<int>& ksize_extend, const std::vector<int>& stride_extend,
        const std::vector<int>& pad_extend, const std::vector<int>& dilation_extend);
int im2col3d_param_check(Context* ctx, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w,
        const std::initializer_list<int64_t>& ksize_extend, const std::initializer_list<int64_t>& stride_extend,
        const std::initializer_list<int64_t>& pad_extend, const std::initializer_list<int64_t>& dilation_extend);

}
}
}
#endif
