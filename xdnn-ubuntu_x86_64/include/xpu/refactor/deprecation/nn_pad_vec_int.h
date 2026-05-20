#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_PAD_VEC_INT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_PAD_VEC_INT_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {

template<typename T> static inline int reflection_pad1d(Context* ctx, const T* x, T* y, int64_t n,
        int64_t c, int64_t xw, const std::vector<int>& pads, bool is_ncw) {
    return reflection_pad1d(ctx, x, y, n, c, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_ncw);
}
template<typename T> static inline int replication_pad1d(Context* ctx, const T* x, T* y, int64_t n,
        int64_t c, int64_t xw, const std::vector<int>& pads, bool is_ncw) {
    return replication_pad1d(ctx, x, y, n, c, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_ncw);
}
template<typename T> static inline int constant_pad1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
        const std::vector<int>& pads, T pad_value, bool is_ncw) {
    return constant_pad1d(ctx, x, y, n, c, xw, std::vector<int64_t>(pads.begin(), pads.end()), pad_value, is_ncw);
}
template<typename T> static inline int reflection_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::vector<int>& pads, bool is_nchw) {
    return reflection_pad2d(ctx, x, y, n, c, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_nchw);
}
template<typename T> static inline int replication_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::vector<int>& pads, bool is_nchw) {
    return replication_pad2d(ctx, x, y, n, c, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_nchw);
}
template<typename T> static inline int constant_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::vector<int>& pads, T pad_value, bool is_nchw) {
    return constant_pad2d(ctx, x, y, n, c, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()), pad_value, is_nchw);
}
template<typename T> static inline int reflection_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& pads, bool is_ncdhw) {
    return reflection_pad3d(ctx, x, y, n, c, xd, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_ncdhw);
}
template<typename T> static inline int replication_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& pads, bool is_ncdhw) {
    return replication_pad3d(ctx, x, y, n, c, xd, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_ncdhw);
}
template<typename T> static inline int constant_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int>& pads, T pad_value, bool is_ncdhw) {
    return constant_pad3d(ctx, x, y, n, c, xd, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()),
            pad_value, is_ncdhw);
}
template<typename T> static inline int reflection_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xw, const std::vector<int>& pads, bool is_ncw) {
    return reflection_pad1d_grad(ctx, dy, dx, n, c, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_ncw);
}
template<typename T> static inline int replication_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xw, const std::vector<int>& pads, bool is_ncw) {
    return replication_pad1d_grad(ctx, dy, dx, n, c, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_ncw);
}
template<typename T> static inline int constant_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xw, const std::vector<int>& pads, T pad_value, bool is_ncw) {
    return constant_pad1d_grad(ctx, dy, dx, n, c, xw, std::vector<int64_t>(pads.begin(), pads.end()),
            pad_value, is_ncw);
}
template<typename T> static inline int reflection_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::vector<int>& pads, bool is_nchw) {
    return reflection_pad2d_grad(ctx, dy, dx, n, c, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_nchw);
}
template<typename T> static inline int replication_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::vector<int>& pads, bool is_nchw) {
    return replication_pad2d_grad(ctx, dy, dx, n, c, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()), is_nchw);
}
template<typename T> static inline int constant_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::vector<int>& pads, T pad_value, bool is_nchw) {
    return constant_pad2d_grad(ctx, dy, dx, n, c, xh, xw, std::vector<int64_t>(pads.begin(), pads.end()),
            pad_value, is_nchw);
}
template<typename T> static inline int reflection_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& pads, bool is_ncdhw) {
    return reflection_pad3d_grad(ctx, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(pads.begin(),
            pads.end()), is_ncdhw);
}
template<typename T> static inline int replication_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& pads, bool is_ncdhw) {
    return replication_pad3d_grad(ctx, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(pads.begin(),
            pads.end()), is_ncdhw);
}
template<typename T> static inline int constant_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::vector<int>& pads, T pad_value, bool is_ncdhw) {
    return constant_pad3d_grad(ctx, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(pads.begin(),
            pads.end()), pad_value, is_ncdhw);
}
template<typename T> static inline int reflection_pad1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xw, const std::initializer_list<int64_t>& pads, bool is_ncw) {
    return reflection_pad1d(ctx, x, y, n, c, xw, std::vector<int64_t>(pads), is_ncw);
}
template<typename T> static inline int replication_pad1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xw, const std::initializer_list<int64_t>& pads, bool is_ncw) {
    return replication_pad1d(ctx, x, y, n, c, xw, std::vector<int64_t>(pads), is_ncw);
}
template<typename T> static inline int constant_pad1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
        const std::initializer_list<int64_t>& pads, T pad_value, bool is_ncw) {
    return constant_pad1d(ctx, x, y, n, c, xw, std::vector<int64_t>(pads), pad_value, is_ncw);
}
template<typename T> static inline int reflection_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_nchw) {
    return reflection_pad2d(ctx, x, y, n, c, xh, xw, std::vector<int64_t>(pads), is_nchw);
}
template<typename T> static inline int replication_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_nchw) {
    return replication_pad2d(ctx, x, y, n, c, xh, xw, std::vector<int64_t>(pads), is_nchw);
}
template<typename T> static inline int constant_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, T pad_value, bool is_nchw) {
    return constant_pad2d(ctx, x, y, n, c, xh, xw, std::vector<int64_t>(pads), pad_value, is_nchw);
}
template<typename T> static inline int reflection_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_ncdhw) {
    return reflection_pad3d(ctx, x, y, n, c, xd, xh, xw, std::vector<int64_t>(pads), is_ncdhw);
}
template<typename T> static inline int replication_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_ncdhw) {
    return replication_pad3d(ctx, x, y, n, c, xd, xh, xw, std::vector<int64_t>(pads), is_ncdhw);
}
template<typename T> static inline int constant_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, T pad_value, bool is_ncdhw) {
    return constant_pad3d(ctx, x, y, n, c, xd, xh, xw, std::vector<int64_t>(pads), pad_value, is_ncdhw);
}
template<typename T> static inline int reflection_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_ncw) {
    return reflection_pad1d_grad(ctx, dy, dx, n, c, xw, std::vector<int64_t>(pads), is_ncw);
}
template<typename T> static inline int replication_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_ncw) {
    return replication_pad1d_grad(ctx, dy, dx, n, c, xw, std::vector<int64_t>(pads), is_ncw);
}
template<typename T> static inline int constant_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xw, const std::initializer_list<int64_t>& pads, T pad_value, bool is_ncw) {
    return constant_pad1d_grad(ctx, dy, dx, n, c, xw, std::vector<int64_t>(pads), pad_value, is_ncw);
}
template<typename T> static inline int reflection_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_nchw) {
    return reflection_pad2d_grad(ctx, dy, dx, n, c, xh, xw, std::vector<int64_t>(pads), is_nchw);
}
template<typename T> static inline int replication_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_nchw) {
    return replication_pad2d_grad(ctx, dy, dx, n, c, xh, xw, std::vector<int64_t>(pads), is_nchw);
}
template<typename T> static inline int constant_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, T pad_value, bool is_nchw) {
    return constant_pad2d_grad(ctx, dy, dx, n, c, xh, xw, std::vector<int64_t>(pads), pad_value, is_nchw);
}
template<typename T> static inline int reflection_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_ncdhw) {
    return reflection_pad3d_grad(ctx, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(pads), is_ncdhw);
}
template<typename T> static inline int replication_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads, bool is_ncdhw) {
    return replication_pad3d_grad(ctx, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(pads), is_ncdhw);
}
template<typename T> static inline int constant_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::initializer_list<int64_t>& pads,
        T pad_value, bool is_ncdhw) {
    return constant_pad3d_grad(ctx, dy, dx, n, c, xd, xh, xw, std::vector<int64_t>(pads), pad_value, is_ncdhw);
}
}
}
}
#endif
