#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_POOL_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_POOL_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {

template<typename T> DLL_EXPORT int adaptive_avg_pool1d(Context* ctx, const T* x, T* y,
        int64_t n, int64_t c, int64_t xw, int64_t yw,
        bool is_ncw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T> DLL_EXPORT int adaptive_avg_pool2d(Context* ctx, const T* x, T* y,
        int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw, bool is_nchw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T> DLL_EXPORT int adaptive_avg_pool2d_grad(Context* ctx, const T* dy, T* dx,
        int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw, bool is_nchw);
template<typename T> DLL_EXPORT int adaptive_avg_pool3d(Context* ctx, const T* x, T* y,
        int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, int64_t yd, int64_t yh, int64_t yw, bool is_ncdhw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T> DLL_EXPORT int adaptive_avg_pool3d_grad(Context* ctx, const T* dy, T* dx,
        int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, int64_t yd, int64_t yh, int64_t yw, bool is_ncdhw);
template<typename T, typename TID = int> DLL_EXPORT int adaptive_max_pool1d(Context* ctx, const T* x, T* y, TID* indices,
        int64_t n, int64_t c, int64_t xw, int64_t yw, bool is_ncw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T, typename TID = int> DLL_EXPORT int adaptive_max_pool2d(Context* ctx, const T* x, T* y, TID* indices,
        int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw, bool is_nchw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T, typename TID = int> DLL_EXPORT int adaptive_max_pool2d_grad(Context* ctx, const T* x, const T* y,
        const TID* indices, const T* dy, T* dx, int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw, bool is_nchw);
template<typename T, typename TID = int> DLL_EXPORT int adaptive_max_pool3d(Context* ctx, const T* x, T* y, TID* indice,
        int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, int64_t yd, int64_t yh, int64_t yw, bool is_ncdhw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T, typename TID = int> DLL_EXPORT int adaptive_max_pool3d_grad(Context* ctx, const T* x, const T* y,
        const TID* indices, const T* dy, T* dx, int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, int64_t yd,
        int64_t yh, int64_t yw, bool is_ncdhw);
template<typename T> DLL_EXPORT int avg_pool1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t w,
        int64_t ksize, int64_t stride, const std::vector<int64_t>& pad, bool count_include_pad, bool is_ncw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T> DLL_EXPORT int avg_pool2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        bool count_include_pad, bool is_nchw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T> DLL_EXPORT int avg_pool2d_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx,
        int64_t n, int64_t c, int64_t h, int64_t w, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, bool count_include_pad, bool is_nchw);
template<typename T> DLL_EXPORT int avg_pool3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        bool count_include_pad, bool is_ncdhw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T> DLL_EXPORT int avg_pool3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xd, int64_t xh,
        int64_t xw, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, bool count_include_pad, bool is_ncdhw);
template<typename T, typename TID = int> DLL_EXPORT int max_pool1d(Context* ctx, const T* x, T* y, TID* indices, int64_t n,
        int64_t c, int64_t w, int64_t ksize, int64_t stride, const std::vector<int64_t>& pad, bool is_ncw,
        const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T, typename TID = int> DLL_EXPORT int max_pool2d(Context* ctx, const T* x, T* y, TID* indices, int64_t n, int64_t c,
        int64_t h, int64_t w, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        bool is_nchw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr, bool xpu1_pad = true, int64_t ld_out_c = -1);
template<typename T, typename TID = int> DLL_EXPORT int max_pool2d_grad(Context* ctx, const T* x, const T* y, const TID* indices,
        const T* dy, T* dx, int64_t n, int64_t c, int64_t h, int64_t w, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, bool is_nchw);
template<typename T, typename TID = int> DLL_EXPORT int max_pool3d(Context* ctx, const T* x, T* y, TID* indices, int64_t n, int64_t c, int64_t d,
        int64_t h, int64_t w, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        bool is_ncdhw, const float* x_maxptr = nullptr, float* y_maxptr = nullptr);
template<typename T, typename TID = int> DLL_EXPORT int max_pool3d_grad(Context* ctx, const T* x, const T* y, const TID* indices,
        const T* dy, T* dx, int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::vector<int64_t>& ksize,
        const std::vector<int64_t>& stride, const std::vector<int64_t>& pad, bool is_ncdhw);
template<typename T, typename TID = int> DLL_EXPORT int max_unpool1d(Context* ctx, const T* x, const TID* index, T* y,
        int64_t n, int64_t c, int64_t xw, int64_t ksize, int64_t stride, const std::vector<int64_t>& pad, bool is_ncw);
template<typename T, typename TID = int> DLL_EXPORT int max_unpool2d(Context* ctx, const T* x, const TID* index, T* y,
        int64_t n, int64_t c, int64_t xh, int64_t xw, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, bool is_nchw, const int64_t yh = 0, const int64_t yw = 0);
template<typename T, typename TID = int> DLL_EXPORT int max_unpool2d_grad(Context* ctx, const T* dy, const TID* index, T* dx,
        int64_t n, int64_t c, int64_t xh, int64_t xw, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, bool is_nchw);
template<typename T, typename TID = int> DLL_EXPORT int max_unpool3d(Context* ctx, const T* x, const TID* index, T* y,
        int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, bool is_ncdhw, const int64_t yh = 0, const int64_t yw = 0);
template<typename T, typename TID = int> DLL_EXPORT int max_unpool3d_grad(Context* ctx, const T* dy, const TID* index, T* dx, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, bool is_ncdhw);
template<typename TID> DLL_EXPORT int max_pool2d_idx_trans(Context* ctx, const TID* idx_src, TID* idx_dst, int64_t n, int64_t c,
        int64_t h, int64_t w, const std::vector<int64_t>& _ksize, const std::vector<int64_t>& _stride,
        const std::vector<int64_t>& _pad, bool is_nchw, bool loc2glo);
}
}
}
#endif

