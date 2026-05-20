#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_POOL_VEC_INT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_POOL_VEC_INT_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {
template<typename T> static inline int adaptive_max_pool1d(Context* ctx, const T* x, T* y, std::nullptr_t indices,
        int64_t n, int64_t c, int64_t xw, int64_t yw, bool is_ncw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return adaptive_max_pool1d<T, int>(ctx, x, y, indices, n, c, xw, yw, is_ncw, x_maxptr, y_maxptr);
}
template<typename T> static inline int adaptive_max_pool2d(Context* ctx, const T* x, T* y, std::nullptr_t indices,
        int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw, bool is_nchw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return adaptive_max_pool2d<T, int>(ctx, x, y, indices, n, c, xh, xw, yh, yw, is_nchw, x_maxptr, y_maxptr);
}
template<typename T> static inline int adaptive_max_pool2d_grad(Context* ctx, const T* x, const T* y,
        const std::nullptr_t indices, const T* dy, T* dx, int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw, bool is_nchw) {
    return adaptive_max_pool2d_grad<T, int>(ctx, x, y, indices, dy, dx, n, c, xh, xw, yh, yw, is_nchw);
}
template<typename T> static inline int adaptive_max_pool3d(Context* ctx, const T* x, T* y, std::nullptr_t indice,
        int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, int64_t yd, int64_t yh, int64_t yw, bool is_ncdhw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return adaptive_max_pool3d<T, int>(ctx, x, y, indice, n, c, xd, xh, xw, yd, yh, yw, is_ncdhw, x_maxptr, y_maxptr);
}
template<typename T> static inline int adaptive_max_pool3d_grad(Context* ctx, const T* x, const T* y,
        const std::nullptr_t indices, const T* dy, T* dx, int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, int64_t yd,
        int64_t yh, int64_t yw, bool is_ncdhw) {
    return adaptive_max_pool3d_grad<T, int>(ctx, x, y, indices, dy, dx, n, c, xd, xh, xw, yd, yh, yw, is_ncdhw);
}
template<typename T> static inline int avg_pool1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t w,
        int64_t ksize, int64_t stride, const std::vector<int>& pad, bool count_include_pad, bool is_ncw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return avg_pool1d(ctx, x, y, n, c, w, ksize, stride, std::vector<int64_t>(pad.begin(), pad.end()),
            count_include_pad, is_ncw, x_maxptr, y_maxptr);
}
template<typename T> static inline int max_pool1d(Context* ctx, const T* x, T* y, int* indices, int64_t n,
        int64_t c, int64_t w, int64_t ksize, int64_t stride, const std::vector<int>& pad, bool is_ncw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return max_pool1d(ctx, x, y, indices, n, c, w, ksize, stride, std::vector<int64_t>(pad.begin(), pad.end()),
            is_ncw, x_maxptr, y_maxptr);
}
template<typename T> static inline int avg_pool2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h,
        int64_t w, const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        bool count_include_pad, bool is_nchw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return avg_pool2d(ctx, x, y, n, c, h, w, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            count_include_pad, is_nchw, x_maxptr, y_maxptr);
}
template<typename T> static inline int max_pool2d(Context* ctx, const T* x, T* y, int* indices, int64_t n, int64_t c,
        int64_t h, int64_t w, const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        bool is_nchw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr, bool xpu1_pad = true) {
    return max_pool2d(ctx, x, y, indices, n, c, h, w, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            is_nchw, x_maxptr, y_maxptr, xpu1_pad);
}
template<typename T> static inline int avg_pool2d_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx,
        int64_t n, int64_t c, int64_t h, int64_t w, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, bool count_include_pad, bool is_nchw) {
    return avg_pool2d_grad(ctx, x, y, dy, dx, n, c, h, w, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), count_include_pad, is_nchw);
}
template<typename T> static inline int max_pool2d_grad(Context* ctx, const T* x, const T* y, const int* indices,
        const T* dy, T* dx, int64_t n, int64_t c, int64_t h, int64_t w, const std::vector<int>& ksize,
        const std::vector<int>& stride, const std::vector<int>& pad, bool is_nchw) {
    return max_pool2d_grad(ctx, x, y, indices, dy, dx, n, c, h, w, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()), is_nchw);
}
template<typename T> static inline int avg_pool3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t d,
        int64_t h, int64_t w, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, bool count_include_pad, bool is_ncdhw, const float* x_maxptr = nullptr,
        float* y_maxptr = nullptr) {
    return avg_pool3d(ctx, x, y, n, c, d, h, w, std::vector<int64_t>(ksize.begin(), ksize.end()),
             std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
             count_include_pad, is_ncdhw, x_maxptr, y_maxptr);
}
template<typename T> static inline int max_pool3d(Context* ctx, const T* x, T* y, int* indices, int64_t n, int64_t c,
        int64_t d, int64_t h, int64_t w, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, bool is_ncdhw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return max_pool3d(ctx, x, y, indices, n, c, d, h, w, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            is_ncdhw, x_maxptr, y_maxptr);
}
template<typename T> static inline int avg_pool3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, bool count_include_pad, bool is_ncdhw) {
    return avg_pool3d_grad(ctx, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            count_include_pad, is_ncdhw);
}
template<typename T> static inline int max_pool3d_grad(Context* ctx, const T* x, const T* y, const int* indices,
        const T* dy, T* dx, int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& ksize,
        const std::vector<int>& stride, const std::vector<int>& pad, bool is_ncdhw) {
    return max_pool3d_grad(ctx, x, y, indices, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(ksize.begin(),
            ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(),
            pad.end()), is_ncdhw);
}
template<typename T> static inline int max_unpool1d(Context* ctx, const T* x, const int* index, T* y, int64_t n,
        int64_t c, int64_t xw, int64_t ksize, int64_t stride, const std::vector<int>& pad, bool is_ncw) {
    return max_unpool1d(ctx, x, index, y, n, c, xw, ksize, stride, std::vector<int64_t>(pad.begin(), pad.end()),
            is_ncw);
}
template<typename T> static inline int max_unpool2d(Context* ctx, const T* x, const int* index, T* y, int64_t n,
        int64_t c, int64_t xh, int64_t xw, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, bool is_nchw, const int64_t yh = 0, const int64_t yw = 0) {
    return max_unpool2d(ctx, x, index, y, n, c, xh, xw, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            is_nchw, yh, yw);
}
template<typename T> static inline int max_unpool2d_grad(Context* ctx, const T* dy, const int* index, T* dx,
        int64_t n, int64_t c, int64_t xh, int64_t xw, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, bool is_nchw) {
    return max_unpool2d_grad(ctx, dy, index, dx, n, c, xh, xw, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()), is_nchw);
}
template<typename T> static inline int max_unpool3d(Context* ctx, const T* x, const int* index, T* y, int64_t n,
        int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, bool is_ncdhw, const int64_t yh = 0, const int64_t yw = 0) {
    return max_unpool3d(ctx, x, index, y, n, c, xd, xh, xw, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            is_ncdhw, yh, yw);
}
template<typename T> static inline int max_unpool3d_grad(Context* ctx, const T* dy, const int* index, T* dx, int64_t n,
        int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, bool is_ncdhw) {
    return max_unpool3d_grad(ctx, dy, index, dx, n, c, xd, xh, xw, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()), is_ncdhw);
}
template<typename T> static inline int avg_pool1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t w,
        int64_t ksize, int64_t stride, const std::initializer_list<int64_t>& pad, bool count_include_pad, bool is_ncw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return avg_pool1d(ctx, x, y, n, c, w, ksize, stride, std::vector<int64_t>(pad), count_include_pad, is_ncw,
            x_maxptr, y_maxptr);
}
template<typename T> static inline int max_pool1d(Context* ctx, const T* x, T* y, int* indices, int64_t n, int64_t c,
        int64_t w, int64_t ksize, int64_t stride, const std::initializer_list<int64_t>& pad, bool is_ncw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return max_pool1d(ctx, x, y, indices, n, c, w, ksize, stride, std::vector<int64_t>(pad),
            is_ncw, x_maxptr, y_maxptr);
}
template<typename T> static inline int avg_pool2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h,
        int64_t w, const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, bool count_include_pad, bool is_nchw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return avg_pool2d(ctx, x, y, n, c, h, w, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), count_include_pad, is_nchw, x_maxptr, y_maxptr);
}
template<typename T> static inline int max_pool2d(Context* ctx, const T* x, T* y, int* indices, int64_t n, int64_t c,
        int64_t h, int64_t w, const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, bool is_nchw, const float* x_maxptr = nullptr,
        float* y_maxptr = nullptr, bool xpu1_pad = true) {
    return max_pool2d(ctx, x, y, indices, n, c, h, w, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad),
            is_nchw, x_maxptr, y_maxptr, xpu1_pad);
}
template<typename T> static inline int avg_pool2d_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx,
        int64_t n, int64_t c, int64_t h, int64_t w, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad,
        bool count_include_pad, bool is_nchw) {
    return avg_pool2d_grad(ctx, x, y, dy, dx, n, c, h, w, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), count_include_pad, is_nchw);
}
template<typename T> static inline int max_pool2d_grad(Context* ctx, const T* x, const T* y, const int* indices,
        const T* dy, T* dx, int64_t n, int64_t c, int64_t h, int64_t w, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad, bool is_nchw) {
    return max_pool2d_grad(ctx, x, y, indices, dy, dx, n, c, h, w, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), is_nchw);
}
template<typename T> static inline int avg_pool3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t d,
        int64_t h, int64_t w, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad,
        bool count_include_pad, bool is_ncdhw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return avg_pool3d(ctx, x, y, n, c, d, h, w, std::vector<int64_t>(ksize),
             std::vector<int64_t>(stride), std::vector<int64_t>(pad),
             count_include_pad, is_ncdhw, x_maxptr, y_maxptr);
}
template<typename T> static inline int max_pool3d(Context* ctx, const T* x, T* y, int* indices, int64_t n, int64_t c,
        int64_t d, int64_t h, int64_t w, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad, bool is_ncdhw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr) {
    return max_pool3d(ctx, x, y, indices, n, c, d, h, w, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), is_ncdhw, x_maxptr, y_maxptr);
}
template<typename T> static inline int avg_pool3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad,
        bool count_include_pad, bool is_ncdhw) {
    return avg_pool3d_grad(ctx, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), count_include_pad, is_ncdhw);
}
template<typename T> static inline int max_pool3d_grad(Context* ctx, const T* x, const T* y, const int* indices,
        const T* dy, T* dx, int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, bool is_ncdhw) {
    return max_pool3d_grad(ctx, x, y, indices, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), is_ncdhw);
}
template<typename T> static inline int max_unpool1d(Context* ctx, const T* x, const int* index, T* y, int64_t n,
        int64_t c, int64_t xw, int64_t ksize, int64_t stride, const std::initializer_list<int64_t>& pad, bool is_ncw) {
    return max_unpool1d(ctx, x, index, y, n, c, xw, ksize, stride, std::vector<int64_t>(pad), is_ncw);
}
template<typename T> static inline int max_unpool2d(Context* ctx, const T* x, const int* index, T* y, int64_t n,
        int64_t c, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad, bool is_nchw,
        const int64_t yh = 0, const int64_t yw = 0) {
    return max_unpool2d(ctx, x, index, y, n, c, xh, xw, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), is_nchw, yh, yw);
}
template<typename T> static inline int max_unpool2d_grad(Context* ctx, const T* dy, const int* index, T* dx, int64_t n,
        int64_t c, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad, bool is_nchw) {
    return max_unpool2d_grad(ctx, dy, index, dx, n, c, xh, xw, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), is_nchw);
}
template<typename T> static inline int max_unpool3d(Context* ctx, const T* x, const int* index, T* y, int64_t n,
        int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad, bool is_ncdhw,
        const int64_t yh = 0, const int64_t yw = 0) {
    return max_unpool3d(ctx, x, index, y, n, c, xd, xh, xw, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), is_ncdhw, yh, yw);
}
template<typename T> static inline int max_unpool3d_grad(Context* ctx, const T* dy, const int* index, T* dx, int64_t n,
        int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad, bool is_ncdhw) {
    return max_unpool3d_grad(ctx, dy, index, dx, n, c, xd, xh, xw, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), is_ncdhw);
}

}
}
}
#endif

