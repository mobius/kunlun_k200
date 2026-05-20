#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_PAD_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_PAD_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {

template<typename T> DLL_EXPORT int reflection_pad1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
        const std::vector<int64_t>& pads, bool is_ncw);
template<typename T> DLL_EXPORT int reflection_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::vector<int64_t>& pads, bool is_nchw);
template<typename T> DLL_EXPORT int reflection_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int64_t>& pads, bool is_ncdhw);

template<typename T> DLL_EXPORT int replication_pad1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
        const std::vector<int64_t>& pads, bool is_ncw);
template<typename T> DLL_EXPORT int replication_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::vector<int64_t>& pads, bool is_nchw);
template<typename T> DLL_EXPORT int replication_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int64_t>& pads, bool is_ncdhw);

template<typename T> DLL_EXPORT int constant_pad1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
        const std::vector<int64_t>& pads, T pad_value, bool is_ncw);
template<typename T> DLL_EXPORT int constant_pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::vector<int64_t>& pads, T pad_value, bool is_nchw);
template<typename T> DLL_EXPORT int constant_pad3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int64_t>& pads, T pad_value, bool is_ncdhw);

template<typename T> DLL_EXPORT int reflection_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xw,
        const std::vector<int64_t>& pads, bool is_ncw);
template<typename T> DLL_EXPORT int reflection_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::vector<int64_t>& pads, bool is_nchw);
template<typename T> DLL_EXPORT int reflection_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int64_t>& pads, bool is_ncdhw);

template<typename T> DLL_EXPORT int replication_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xw,
        const std::vector<int64_t>& pads, bool is_ncw);
template<typename T> DLL_EXPORT int replication_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::vector<int64_t>& pads, bool is_nchw);
template<typename T> DLL_EXPORT int replication_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int64_t>& pads, bool is_ncdhw);

template<typename T> DLL_EXPORT int constant_pad1d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xw,
        const std::vector<int64_t>& pads, T pad_value, bool is_ncw);
template<typename T> DLL_EXPORT int constant_pad2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::vector<int64_t>& pads, T pad_value, bool is_nchw);
template<typename T> DLL_EXPORT int constant_pad3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int64_t>& pads, T pad_value, bool is_ncdhw);
}
}
}
#endif
