#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_ACTIVATION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_ACTIVATION_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {

template<typename T> DLL_EXPORT int approximate_gelu(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int celu(Context* ctx, const T* x, T* y, int64_t len,
        float alpha = 1.0f, const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int celu_grad(Context* ctx, const T* x, const T* dy, T* dx,
        int64_t len, float alpha);
template<typename T> DLL_EXPORT int elu(Context* ctx, const T* x, T* y, int64_t len,
        float alpha = 1.0f, const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int elu_grad(Context* ctx, const T* x, const T* dy, T* dx,
        int64_t len, float alpha);
template<typename T> DLL_EXPORT int fast_mish(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int fast_sigmoid(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int fast_tanh(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int gelu(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int gelu_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int approximate_gelu_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int glu(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int glu_grad(Context* ctx, const T* x, const T* dy, T* dx,
        const std::vector<int64_t>& xshape, int64_t axis);
template<typename T> DLL_EXPORT int hard_shrink(Context* ctx, const T* x, T* y, int64_t len, float threshold,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int hard_shrink_grad(Context* ctx, const T* x, const T* dy, T* dx,
        int64_t len, float threshold);
template<typename T> DLL_EXPORT int hard_sigmoid(Context* ctx, const T* x, T* y, int64_t len, float slope,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int hard_sigmoid_grad(Context* ctx, const T* x, const T* y,const T* dy, T* dx,
        int64_t len, float slope);
template<typename T> DLL_EXPORT int hard_swish(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int hard_swish_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int hard_tanh(Context* ctx, const T* x, T* y, int64_t len, float minval, float maxval,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int hard_tanh_grad(Context* ctx, const T* x, const T* dy, T* dx,
        int64_t len, float minval, float maxval);
template<typename T> DLL_EXPORT int leaky_relu(Context* ctx, const T* x, T* y, int64_t len, float alpha,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int leaky_relu_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx,
        int64_t len, float alpha);
template<typename T> DLL_EXPORT int logit(Context* ctx, const T* x, T* y, int64_t len, float eps,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int logit_grad(Context* ctx, const T* x, const T* dy, T* dx,
        int64_t len, float eps);
template<typename T> DLL_EXPORT int log_sigmoid(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int log_sigmoid_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int log_softmax(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis);
template<typename T> DLL_EXPORT int log_softmax_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::vector<int64_t>& xshape, int64_t axis);
template<typename T> DLL_EXPORT int mish(Context* ctx, const T* x, T* y, int64_t len, float threshold = 20.0f,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int mish_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx,
        int64_t len, float threshold = 20.0f);
template<typename T, typename TS> DLL_EXPORT int prelu(Context* ctx, const T* x, const TS* slope, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& slope_shape,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T, typename TS> DLL_EXPORT int prelu_grad(Context* ctx, const T* x, const T* y, const TS* slope,
        const T* dy, T* dx, TS* dslope, const std::vector<int64_t>& xshape, int64_t mode = 0);
template<typename T> DLL_EXPORT int quick_gelu(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int relu(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int relu6(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int relu6_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int relu_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int selu(Context* ctx, const T* x, T* y, int64_t len,
        float scale = 1.050701, float alpha = 1.6732632, const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int selu_grad(Context* ctx, const T* x, const T* dy, T* dx,
        int64_t len, float scale, float alpha);
template<typename T> DLL_EXPORT int sigmoid(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int sigmoid_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int softmax(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis);
template<typename T> DLL_EXPORT int softmax_1(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis);
template<typename T> DLL_EXPORT int softmax_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::vector<int64_t>& xshape, int64_t axis, float input_scale=1);
template<typename T> DLL_EXPORT int softmin(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis);
template<typename T> DLL_EXPORT int softmin_grad(Context* ctx, const T* y, const T* dy, T* dx,
        const std::vector<int64_t>& xshape, int64_t axis);
template<typename T> DLL_EXPORT int softplus(Context* ctx, const T* x, T* y, int64_t len, float beta = 1.0f,
        float threshold = 20.0f, const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int softplus_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx,
        int64_t len, float beta = 1.0f, float threshold = 20.0f);
template<typename T> DLL_EXPORT int softsign(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int softsign_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int soft_shrink(Context* ctx, const T* x, T* y, int64_t len, float threshold,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int soft_shrink_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len,
        float threshold);
template<typename T> DLL_EXPORT int swish(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int swish_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int fast_swish(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int fast_swish_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int tanh(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int tanh_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int tanh_shrink(Context* ctx, const T* x, T* y, int64_t len,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int tanh_shrink_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int threshold(Context* ctx, const T* x, T* y, int64_t len, float thre, float value,
        const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int threshold_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len, float thre);
template<typename T> DLL_EXPORT int swiglu(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis,
        bool turn = false, const float* max_x = nullptr, float* max_y = nullptr, const T* x2 = nullptr);
template<typename T> DLL_EXPORT int swiglu_grad(Context* ctx, const T* x, const T* dy, T* dx,
        const std::vector<int64_t>& xshape, int64_t axis, bool turn = false, const T* x2 = nullptr, T* dx2 = nullptr);
template<typename T> DLL_EXPORT int fast_swiglu(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis,
        bool turn = false, const float* max_x = nullptr, float* max_y = nullptr);
}
}
}
#endif
