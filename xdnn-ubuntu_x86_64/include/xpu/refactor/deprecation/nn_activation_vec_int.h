#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_ACTIVATION_VEC_INT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_ACTIVATION_VEC_INT_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {
template<typename T> static inline int glu(Context* ctx, const T* x, T* y, const std::vector<int>& xshape, int64_t axis,
        const float* max_x = nullptr, float* max_y = nullptr) {
    return glu(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), axis, max_x, max_y);
}
template<typename T> static inline int glu_grad(Context* ctx, const T* x, const T* dy, T* dx,
        const std::vector<int>& xshape, int64_t axis) {
    return glu_grad(ctx, x, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename T> static inline int log_softmax(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, int64_t axis) {
    return log_softmax(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename T> static inline int log_softmax_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::vector<int>& xshape, int64_t axis) {
    return log_softmax_grad(ctx, y, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename T, typename TS> static inline int prelu(Context* ctx, const T* x, const TS* slope, T* y,
        const std::vector<int>& xshape, const std::vector<int>& slope_shape,
        const float* max_x = nullptr, float* max_y = nullptr) {
    return prelu(ctx, x, slope, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(slope_shape.begin(), slope_shape.end()), max_x, max_y);
}
template<typename T, typename TS> static inline int prelu_grad(Context* ctx, const T* x, const T* y, const TS* slope,
        const T* dy, T* dx, TS* dslope, const std::vector<int>& xshape, int64_t mode = 0) {
    return prelu_grad(ctx, x, y, slope, dy, dx, dslope, std::vector<int64_t>(xshape.begin(), xshape.end()), mode);
}
template<typename T> static inline int softmax(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, int64_t axis) {
    return softmax(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename T> static inline int softmax_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::vector<int>& xshape, int64_t axis) {
    return softmax_grad(ctx, y, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename T> static inline int softmin(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, int64_t axis) {
    return softmin(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename T> static inline int softmin_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::vector<int>& xshape, int64_t axis) {
    return softmin_grad(ctx, y, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}

template<typename T> static inline int glu(Context* ctx, const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        int64_t axis, const float* max_x = nullptr, float* max_y = nullptr) {
    return glu(ctx, x, y, std::vector<int64_t>(xshape), axis, max_x, max_y);
}
template<typename T> static inline int glu_grad(Context* ctx, const T* x, const T* dy, T* dx,
        const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return glu_grad(ctx, x, dy, dx, std::vector<int64_t>(xshape), axis);
}
template<typename T> static inline int log_softmax(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return log_softmax(ctx, x, y, std::vector<int64_t>(xshape), axis);
}
template<typename T> static inline int log_softmax_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return log_softmax_grad(ctx, y, dy, dx, std::vector<int64_t>(xshape), axis);
}
template<typename T, typename TS> static inline int prelu(Context* ctx, const T* x, const TS* slope, T* y,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& slope_shape,
        const float* max_x = nullptr, float* max_y = nullptr) {
    return prelu(ctx, x, slope, y, std::vector<int64_t>(xshape), std::vector<int64_t>(slope_shape), max_x, max_y);
}
template<typename T, typename TS> static inline int prelu_grad(Context* ctx, const T* x, const T* y, const TS* slope,
        const T* dy, T* dx, TS* dslope, const std::initializer_list<int64_t>& xshape, int64_t mode = 0) {
    return prelu_grad(ctx, x, y, slope, dy, dx, dslope, std::vector<int64_t>(xshape), mode);
}
template<typename T> static inline int softmax(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return softmax(ctx, x, y, std::vector<int64_t>(xshape), axis);
}
template<typename T> static inline int softmax_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return softmax_grad(ctx, y, dy, dx, std::vector<int64_t>(xshape), axis);
}
template<typename T> static inline int softmin(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return softmin(ctx, x, y, std::vector<int64_t>(xshape), axis);
}
template<typename T> static inline int softmin_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return softmin_grad(ctx, y, dy, dx, std::vector<int64_t>(xshape), axis);
}
}
}
}
#endif
