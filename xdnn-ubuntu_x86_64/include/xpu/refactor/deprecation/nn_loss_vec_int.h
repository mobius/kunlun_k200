#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_LOSS_VEC_INT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_LOSS_VEC_INT_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {
template<typename T> static inline int sigmoid_cross_entropy_with_logits(Context* ctx,
        const T* x, const T* label, T* y, int64_t m, int64_t n, std::nullptr_t hit, int64_t ignore_index = -100, const T* pos_weight = nullptr) {
    return sigmoid_cross_entropy_with_logits<T, int>(ctx, x, label, y, m, n, hit, ignore_index, pos_weight);
}
template<typename T> static inline int sigmoid_cross_entropy_with_logits_grad(Context* ctx,
        const T* x, const T* label, const T* dy, T* dx, int64_t m, int64_t n, std::nullptr_t hit,
        int64_t ignore_index = -100, const T* pos_weight = nullptr) {
    return sigmoid_cross_entropy_with_logits_grad<T, int>(ctx, x, label, dy, dx, m, n, hit, ignore_index, pos_weight);
}
template<typename T> static inline int nll_loss(Context* ctx, const T* x, T* y, T* total_weight,
        const std::vector<int>& x_shape, const int32_t* target, const float* weight = NULL,
        int64_t reduction = 1, int64_t ignore_index = -100) {
    return nll_loss(ctx, x, y, total_weight, std::vector<int64_t>(x_shape.begin(), x_shape.end()),
            target, weight, reduction, ignore_index);
}
template<typename T> static inline int nll_loss_grad(Context* ctx, const float* dy,  T* dx,
        const std::vector<int>& shape, const int32_t* target, const float* weight = NULL,
        int64_t reduction = 1, int64_t ignore_index = -100, const T* total_weight = NULL) {
    return nll_loss_grad(ctx, dy, dx, std::vector<int64_t>(shape.begin(), shape.end()), target,
            weight, reduction, ignore_index, total_weight);
}
template<typename T> static inline int nll_loss(Context* ctx, const T* x, T* y, T* total_weight,
        const std::initializer_list<int64_t>& x_shape, const int32_t* target, const float* weight = NULL,
        int64_t reduction = 1, int64_t ignore_index = -100) {
    return nll_loss(ctx, x, y, total_weight, std::vector<int64_t>(x_shape), target, weight,
            reduction, ignore_index);
}
template<typename T> static inline int nll_loss_grad(Context* ctx, const float* dy,  T* dx,
        const std::initializer_list<int64_t>& shape, const int32_t* target, const float* weight = NULL,
        int64_t reduction = 1, int64_t ignore_index = -100, const T* total_weight = NULL) {
    return nll_loss_grad(ctx, dy, dx, std::vector<int64_t>(shape), target, weight, reduction, ignore_index,
            total_weight);
}

}
}
}
#endif
