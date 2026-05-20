#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include "xpu/refactor/deprecation/deprecated.h"
#include "xpu/refactor/nn_activation.h"
#include "xpu/refactor/deprecation/nn_activation_vec_int.h"
#include "xpu/refactor/nn_pool.h"
#include "xpu/refactor/deprecation/nn_pool_vec_int.h"
#include "xpu/refactor/nn_loss.h"
#include "xpu/refactor/deprecation/nn_loss_vec_int.h"
#include "xpu/refactor/nn_pad.h"
#include "xpu/refactor/deprecation/nn_pad_vec_int.h"
#include "xpu/refactor/nn_box.h"

namespace baidu {
namespace xpu {
namespace api {
// Conv
template<typename T> DLL_EXPORT int im2col1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
        int64_t ksize, int64_t stride, const std::vector<int64_t>& pad, int64_t dilation, bool is_ncw);
template<typename T> DLL_EXPORT int im2col(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, const std::vector<int64_t>& ksize,
        const std::vector<int64_t>& stride, const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation,
        bool is_nchw);
template<typename T> DLL_EXPORT int im2col3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int64_t>& _ksize, const std::vector<int64_t>& _stride,
        const std::vector<int64_t>& _pad, const std::vector<int64_t>& _dilation, bool is_ncdhw);
template<typename T> DLL_EXPORT int im2im(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, bool is_nchw);
template<typename T> DLL_EXPORT int col2im1d(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xw,
        int64_t ksize, int64_t stride, const std::vector<int64_t>& pad, int64_t dilation, bool is_ncw);
template<typename T> DLL_EXPORT int col2im(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation, bool is_nchw);
template<typename T> DLL_EXPORT int col2im_v2(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xh, int64_t xw,
        const std::vector<int64_t>& _ksize, const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad,
        const std::vector<int64_t>& _dilation, bool is_nchw);
template<typename T> DLL_EXPORT int col2im3d(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation, bool is_ndhwc);
template<typename T> DLL_EXPORT int deformable_im2col(Context* ctx, const T* x, const float* offset, const float* mask,
        T* y, int64_t n, int64_t c, int64_t xh, int64_t xw, const std::vector<int64_t>& ksize,
        const std::vector<int64_t>& stride, const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation,
        const int64_t deformable_group, bool is_nchw);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv2d(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f, const std::vector<int64_t>& ksize,
        const std::vector<int64_t>& stride, const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation, int64_t group,
        const float* x_maxptr, const float* weight_maxptr, float* y_maxptr, bool is_nchw);
template<typename TX, typename TW, typename TID = int> DLL_EXPORT int
search_varconv(Context* ctx, int64_t batch, int64_t c, int64_t f, int64_t winh, int64_t winw, int64_t strideh, int64_t stridew,
        const TX* input_im, const TW* weight, const VectorParam<TID>& offset_x, const VectorParam<TID>& offset_y,
        float* output_im, float max_weight, const Activation_t act, bool is_nchw = true);
template<typename TID = int> DLL_EXPORT int batched_search_varconv(Context* ctx, int64_t batch, int64_t c, int64_t f,
        int64_t winh, int64_t winw, int64_t strideh, int64_t stridew,
        const float* input_im, const float* weight, const VectorParam<TID>& offset_x, const VectorParam<TID>& offset_y,
        float* output_im, float max_weight, const Activation_t act, bool is_nchw = true);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv1d_grad(Context* ctx,
        const TX* x, const TW* weight, const TY* dy, TX* dx, TW* dweight, int64_t n, int64_t c, int64_t xw,
        int64_t f, int64_t ksize_w, int64_t stride_w, const std::vector<int64_t>& _pad, int64_t dilation_w,
        int64_t group, const float* x_maxptr, const float* w_maxptr, const float* dy_maxptr, float* dx_maxptr,
        float* dw_maxptr, bool is_nchw);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv2d_grad(Context* ctx,
        const TX* x, const TW* weight, const TY* dy, TX* dx, TW* dweight, int64_t n, int64_t c, int64_t h, int64_t w,
        int64_t f, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const float* x_maxptr, const float* w_maxptr,
        const float* dy_maxptr, float* dx_maxptr, float* dw_maxptr, bool is_nchw);
template<typename TY, typename TW, typename TX, typename TGEMM> DLL_EXPORT int conv2d_transpose(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t yh, int64_t yw, int64_t xc,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const float* y_maxptr,
        const float* weight_maxptr, float* x_maxptr, bool is_nchw);
template<typename TY, typename TW, typename TX, typename TGEMM> DLL_EXPORT int conv2d_transpose_v2(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t xh, int64_t xw, int64_t xc,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const float* y_maxptr,
        const float* weight_maxptr, float* x_maxptr, bool is_nchw);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv3d_grad(Context* ctx,
        const TX* x, const TW* weight, const TY* dy, TX* dx, TW* dweight, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w,
        int64_t f, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const float* x_maxptr, const float* w_maxptr,
        const float* dy_maxptr, float* dx_maxptr, float* dw_maxptr, bool is_ncdhw);
template<typename TY, typename TW, typename TX, typename TGEMM> DLL_EXPORT int conv3d_transpose(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t yd, int64_t yh, int64_t yw, int64_t xc,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const float* y_maxptr,
        const float* weight_maxptr, float* x_maxptr, bool is_ndhwc);
template<typename TY, typename TW, typename TX, typename TGEMM> DLL_EXPORT int conv3d_transpose_grad(
        Context* ctx, const TY* y, const TW* weight, const TX* dx, TY* dy, TW* dweight,
        int64_t n, int64_t yc, int64_t yd, int64_t yh, int64_t yw, int64_t xc,
        const std::vector<int64_t>& _ksize, const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad,
        const std::vector<int64_t>& _dilation, int64_t group, const float* y_maxptr, const float* w_maxptr,
        const float* dx_maxptr, float* dy_maxptr, float* dw_maxptr, bool is_ncdhw);
template<typename TY, typename TW, typename TX, typename TGEMM> DLL_EXPORT int conv2d_transpose_grad(
        Context* ctx, const TY* y, const TW* weight, const TX* dx, TY* dy, TW* dweight,
        int64_t n, int64_t yc, int64_t yh, int64_t yw, int64_t xc, int64_t xh, int64_t xw,
        const std::vector<int64_t>& _ksize, const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad,
        const std::vector<int64_t>& _dilation, int64_t group, const float* y_maxptr, const float* w_maxptr,
        const float* dx_maxptr, float* dy_maxptr, float* dw_maxptr, bool is_nchw);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv3d(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w, int64_t f,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const float* x_maxptr, const float* weight_maxptr,
        float* y_maxptr, bool is_ncdhw);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int deformable_conv(
        Context* ctx, const TX* x, const TW* weight, const float* offset, const float* mask,
        TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, int64_t deformable_group,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr, bool is_nchw);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int deformable_conv_grad(
        Context* ctx, const TX* x, const TW* weight, const float* offset, const float* mask,
        const TY* dy, TX* dx, TW* dw, float* doffset, float* dmask, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, int64_t deformable_group,
        const float* x_maxptr, const float* w_maxptr, float* dy_maxptr, float* dx_maxptr, float* dw_maxptr, bool is_nchw);

// Norm
template<typename T> DLL_EXPORT int data_norm(Context *ctx, const T *x, const T *batch_size,
                       const T *batch_sum, const T* batch_square_sum, const T *scale_w,
                       const T *bias, T *y, T* mean, T* scale, int64_t N, int64_t C);
template<typename T> DLL_EXPORT int data_norm_grad(Context *ctx, const T* dy, const T* scale, T* x, T* means,
                 T* batch_size, T* batch_square_sum, T* batch_sum, T* dx,
                 T squared_sum_epsilon, int64_t N, int64_t C);
template<typename T> DLL_EXPORT int kernel_update_param(Context *ctx, const T* d_batch_size, const T* d_batch_sum,
                  const T* d_batch_square_sum, T* batch_size, T* batch_sum, T* batch_square_sum, const T decay_rate, int64_t c);
template<typename T> DLL_EXPORT int batch_norm(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        float eps, float momentum, const float* scale, const float* bias,
        float* batch_mean, float* batch_inv_std, float* global_mean, float* global_var, bool is_nchw, bool unbiased_global_var = false);
template<typename T> DLL_EXPORT int batch_norm_grad(Context* ctx, const T* x, const T* dy, T* dx,
        int64_t n, int64_t c, int64_t h, int64_t w, const float* scale, const float* batch_mean,
        const float* batch_inv_std, float* dscale, float* dbias, bool is_nchw, const float* global_mean = nullptr,
        const float* global_var = nullptr, const float epsilon = 1e-5f);
template<typename T> DLL_EXPORT int batch_norm_grad_grad(Context* ctx, const T* x, const float* scale, const T* dy, const T* ddx, const float* ddscale, const float* ddbias,
        const float* global_mean, const float* global_var, const float* batch_mean, const float* batch_inv_std,
        T* dx, float* dscale, T* ddy, int64_t n, int64_t c, int64_t h, int64_t w, bool use_global_stats, bool is_nchw = false, float epsilon = 1e-5);
template<typename T> DLL_EXPORT int batch_norm_infer(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h,
        int64_t w, float eps, const float* scale, const float* bias, const float* batch_mean,
        const float* batch_var, bool is_nchw);
template<typename T, typename TW = float> DLL_EXPORT int layer_norm(Context* ctx, const T* x, T* y, int64_t m, int64_t n, float eps,
        const TW* scale, const TW* bias, float* mean, float* var, bool is_rstd = false);
template<typename T, typename TW = float> DLL_EXPORT int layer_norm_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t m, int64_t n,
        float eps, const TW* scale, const float* mean, const float* var, TW* dscale, TW* dbias, bool is_rstd = false);
template<typename T, typename TW=float> DLL_EXPORT int rms_layer_norm(Context* ctx, const T* x, T* y, int64_t m, int64_t n, float eps,
        const TW* scale, const TW* bias, float* var, bool is_rstd = false);
template<typename T, typename TW=float> DLL_EXPORT int rms_layer_norm_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t m, int64_t n,
        float eps, const TW* scale, const float* var, TW* dscale, TW* dbias, bool is_rstd = false);
template<typename T> DLL_EXPORT int l2_norm(Context* ctx,
        const T* x, T* y, T* norm, const std::vector<int64_t>& xshape, int64_t axis, float eps);
template<typename T> DLL_EXPORT int lrn(Context* ctx, const T* x, T* y,
        int64_t n, int64_t c, int64_t h, int64_t w, int64_t m, float k, float alpha, float beta);
template<typename T> DLL_EXPORT int instance_norm(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        float eps, const float* scale, const float* bias, float* saved_mean, float* saved_var, bool is_nchw);
template<typename T> DLL_EXPORT int instance_norm_grad(Context* ctx, const T* x, const T* dy,  T* dx,
        const float* scale, const float* mean, const float* var, float* dscale, float* dbias,
        int64_t n, int64_t c, int64_t h, int64_t w, float eps, bool is_nchw);
template<typename T> DLL_EXPORT int clip_by_norm(Context* ctx, const T* x, T* y, float max_norm,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int group_norm(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        int64_t groups, float eps, const float* scale, const float* bias, float* mean, float* var, bool is_nchw);
template<typename T> DLL_EXPORT int group_norm_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t n, int64_t c, int64_t h,
        int64_t w, int64_t groups, float eps, const float* scale, const float* bias, const float* mean,
        const float* var, float* dscale, float* dbias, bool is_nchw, bool is_rstd = false);
template<typename T> DLL_EXPORT int label_smooth(Context* ctx, const T* x, T* y, int64_t n, float epsilon, int64_t label_dim);
template <typename T> DLL_EXPORT int norm(Context* ctx, const T* x, T* y, const std::vector<int64_t>& x_shape,
        const std::vector<int64_t>& dim, float p);
template<typename T> DLL_EXPORT int per_row_norm(Context* ctx, const T* x, T* y, T* scales, int64_t row, int64_t col,
        T norm_max);
template <typename T> DLL_EXPORT int weight_norm(Context* ctx, const T* v, const T* g, T* w, T* norm,
        const std::vector<int64_t>& vshape, int64_t axis);
template <typename T> DLL_EXPORT int weight_norm_grad(Context* ctx, const T* grad_w, const T* saved_v, const T* saved_g, const T* saved_norm,
        T* grad_v, T* grad_g, const std::vector<int64_t>& vshape, int64_t axis);
// FC
template<typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int fc(Context* ctx, const TX* x, const TW* w, TY* y, int64_t m,
        int64_t n, int64_t k, bool x_trans, bool w_trans,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr);
template<typename TX, typename TW, typename TY, typename TGEMM, int MAX_PTR_TYPE = 0>
DLL_EXPORT int fc_batched(Context* ctx, int64_t batch_size, bool x_trans, bool w_trans,
        int64_t m, int64_t n, int64_t k, float alpha, const TX* x, int64_t stride_a,
        const TW* w, int64_t stride_b, float beta, TY* y, int64_t stride_c,
        const float* x_maxptr, const float* w_maxptr);
template<typename TX, typename TW, typename TY, typename TID, typename TGEMM>
DLL_EXPORT int fc_batched_vsl(Context* ctx, const TX* x, const TW* w, TY* y, const VectorParam<TID>& m_list,
        const VectorParam<TID>& n_list, const VectorParam<TID>& k_list, bool x_trans, bool w_trans, float alpha,
        bool need_softmax);

// Optimizer
template<typename T> DLL_EXPORT int adam(Context* ctx,
        const T* g, const float* mom1, const float* mom2, const T* param,
        const float* beta1_pow, const float* beta2_pow, const float* lr,
        float* moment1_out, float* moment2_out, T* param_out,
        float beta1, float beta2, float epsilon, int64_t n);
template<typename TX, typename TY> DLL_EXPORT int multi_tensor_adam(Context* ctx,
        std::vector<TX*> g_tensor_list, std::vector<TX*> param_tensor_list,
        std::vector<TY*> mom1_tensor_list, std::vector<TY*> mom2_tensor_list,
        std::vector<int64_t> shape_list, float lr, float beta1, float beta2, float epsilon,
        int64_t step, int64_t mode, int64_t bias_correction, float weight_decay, float beta1_pow = 0.9, float beta2_pow = 0.99);
template<typename T, typename TID = int> DLL_EXPORT int sparse_adam(Context* ctx,
        const T* g, const float* mom1, const float* mom2, const T* param,
        const float* beta1_pow, const float* beta2_pow, const float* lr,
        float* moment1_out, float* moment2_out, T* param_out,
        float beta1, float beta2, float epsilon, int64_t ori_rows,
        const TID* rows, int64_t row_numel, int64_t row_count, int64_t lazy_mode);
template<typename T> DLL_EXPORT int adamw(Context* ctx,
        const T* g, const float* mom1, const float* mom2, const T* param,
        const float* beta1_pow, const float* beta2_pow, const float* lr,
        float* moment1_out, float* moment2_out, T* param_out,
        float beta1, float beta2, float epsilon, float coeff, int64_t n, float lr_ratio = 1.0f, float* beta1_pow_out = nullptr, float* beta2_pow_out = nullptr);
template<typename T> DLL_EXPORT int lamb(Context* ctx, const T* g, const float* mom1,
        const float* mom2, const T* param, const float* beta1_pow, const float* beta2_pow,
        float* mom1_out, float* mom2_out, T* param_out, float* beta1_pow_out, float* beta2_pow_out,
        float beta1, float beta2, float epsilon, float weight_decay, const float* lr, int64_t n, float param_norm_clamp_value = 0);

template <typename T, typename TG, typename MT> DLL_EXPORT int adamw_v2(Context* ctx,
        MT beta1, MT beta2, MT epsilon, MT coeff, MT lr_ratio,
        const MT* beta1_pow, MT* beta1_pow_out, const MT* beta2_pow, MT* beta2_pow_out,
        const MT* moment1, MT* moment1_out, const MT* moment2, MT* moment2_out,
        const MT* lr, const TG* grad, const T* param, T* param_out, const MT* master_param, MT* master_param_out, int64_t n, bool round_bf16_output = false);

template <typename T, typename TG, typename MT> DLL_EXPORT int adamw_v2(Context* ctx,
        MT beta1, MT beta2, MT epsilon, MT coeff, MT lr_ratio,
        const MT* beta1_pow, MT beta1_pow_scalar, const MT* beta2_pow, MT beta2_pow_scalar,
        const MT* moment1, MT* moment1_out, const MT* moment2, MT* moment2_out,
        const MT* lr, const TG* grad, const T* param, T* param_out, const MT* master_param, MT* master_param_out, int64_t n, bool round_bf16_output);

template<typename T> DLL_EXPORT int update_lamb_param_and_beta_pows(Context* ctx, const T* params,
        const float* trust_ratio_div, const float* param_square_norm,
        const float* trust_ratio_div_square_norm, T* params_out, float lr, int64_t n);

template<typename T> DLL_EXPORT int update_lamb_mom_and_trust_ratio_div(Context* ctx, const T* param, const T* grad, float* mom1,
        float* mom2,  float* trust_ratio_div,
        float neg_beta1, float neg_beta2, float weight_decay, int weight_decay_end_numel,
        float beta1, float beta2, float epsilon, float scale, int64_t n);


template<typename T> DLL_EXPORT int momentum(Context* ctx,
        const T* param, const T* velocity, const T* grad, T* param_out, T* velocity_out,
        int64_t len, const float* lr, int use_nesterov, float mu, float l2_weight_decay = 0.0f);
template<typename T> DLL_EXPORT int rmsprop(Context* ctx, const T* g, const T* p, const float* ms, const float* mom,
        T* p_out, float* ms_out, float* mom_out, float epsilon, float rho, float momentum, float lr, int64_t n, bool centered = false,
        const float* mg = nullptr, float* mg_out = nullptr);
template<typename T> DLL_EXPORT int sgd(Context* ctx, const T* grad, const T* param,
        const float* lr, T* param_out, int64_t n);
template<typename T, typename TID> DLL_EXPORT int sparse_sgd(Context* ctx, const T* sparse_grad, const T* param,
        const float* lr, const VectorParam<TID>& rows, T* param_out, TID row_numel, TID row_count);
template <typename T> DLL_EXPORT int merged_momentum(Context* ctx, const std::vector<T*>& param_list,
        const std::vector<T*>& velocity_list,
        const std::vector<T*>& grad_list,
        std::vector<T*>& param_out_list,
        std::vector<T*>& velocity_out_list,
        const std::vector<float>& l2_weight_decay,
        const std::vector<int64_t>& sizes, const float* lr,
        float mu, int64_t use_nesterov);
template <typename T> DLL_EXPORT int lars_momentum(Context* ctx, const std::vector<T*>& param_list,
        const std::vector<T*>& grad_list,
        const std::vector<float*>& velocity_list, const std::vector<float*>& lrs,
        const std::vector<float*>& master_param_list,
        const std::vector<T*>& param_out_list, const std::vector<float*>& velocity_out_list,
        const std::vector<float*>& master_param_out_list, const std::vector<float>& lars_weight_decay,
        const std::vector<int64_t>& param_sizes, float mu, float lars_coeff, float epsilon,
        float rescale_grad);
template<typename T, typename TM> DLL_EXPORT int adadelta(Context* ctx,
        const T* param, const T* g, const TM* eg, const TM* edx,
        T* param_out, TM* eg_out, TM* edx_out, int64_t len, float rho = 0.95, float epsilon = 1e-6);
template<typename T> DLL_EXPORT
int adamax(Context* ctx, const T* param, const T* g, const float* lr, const float* moment,
        const float* inf_norm, const float* beta1_pow, T* param_out, float* moment_out, float* inf_norm_out,
        int64_t len, float beta1 = 0.9, float beta2 = 0.999, float epsilon = 1e-8);
template<typename T> DLL_EXPORT int adagrad(Context* ctx, const T* param, const T* grad, const float* moment,
        const float* lr, T* param_out, float* moment_out, int64_t len, float epsilon = 1e-6);
// Vision
template<typename T> DLL_EXPORT int nearest_resize1d(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t xw, int64_t yw,
        int64_t coordinate_transformation_mode, int64_t nearest_mode, bool is_ncw, float scale_w = -1.0f,
        int64_t n_stride = -1, int64_t n_outld_stride = -1);
template<typename T> DLL_EXPORT int  nearest_resize2d(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw,
        int64_t coordinate_transformation_mode, int64_t nearest_mode, bool is_nchw, float scale_h = -1.0f,
        float scale_w = -1.0f, int64_t ld_out_c = -1);
template<typename T> DLL_EXPORT int nearest_resize3d(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, int64_t yd, int64_t yh, int64_t yw,
        int64_t coordinate_transformation_mode, int64_t nearest_mode, bool is_ncdhw,
        float scale_d = -1.0f, float scale_h = -1.0f, float scale_w = -1.0f);
template<typename T> DLL_EXPORT int linear_resize1d(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t xw, int64_t yw,
        int64_t coordinate_transformation_mode, bool is_ncw, const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int linear_resize2d(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw,
        int64_t coordinate_transformation_mode, bool is_nchw, const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int linear_resize3d(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw, int64_t yd, int64_t yh, int64_t yw,
        int64_t coordinate_transformation_mode, bool is_ncdhw, const float* max_x = nullptr, float* max_y = nullptr);
template<typename T> DLL_EXPORT int bicubic_resize1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
    int64_t yw, int64_t coordinate_transformation_mode, bool is_ncw, float scale = -1.0f);
template<typename T> DLL_EXPORT int bicubic_resize2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xh, int64_t xw,
        int64_t yh, int64_t yw, int64_t coordinate_transformation_mode, bool is_nchw, float scale_h = -1.0f, float scale_w = -1.0f);
template<typename T> DLL_EXPORT int nearest_resize1d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xw,
        int64_t yw, int64_t coordinate_transformation_mode, int64_t nearest_mode, bool is_ncw, float scale_w = -1.0f);
template<typename T> DLL_EXPORT int nearest_resize2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xh, int64_t xw, int64_t yh, int64_t yw, int64_t coordinate_transformation_mode, int64_t nearest_mode,
        bool is_nchw, float scale_h = -1.0f, float scale_w = -1.0f);
template<typename T> DLL_EXPORT int nearest_resize3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, int64_t yd, int64_t yh, int64_t yw, int64_t coordinate_transformation_mode,
        int64_t nearest_mode, bool is_ncdhw, float scale_d = -1.0f, float scale_h = -1.0f, float scale_w = -1.0f);
template<typename T> DLL_EXPORT int linear_resize1d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xw, int64_t yw,
        int64_t coordinate_transformation_mode, bool is_ncw);
template<typename T> DLL_EXPORT int linear_resize2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c,
        int64_t xh, int64_t xw, int64_t yh, int64_t yw, int64_t coordinate_transformation_mode, bool is_nchw);
template<typename T> DLL_EXPORT int linear_resize3d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xd, int64_t xh, int64_t xw,
        int64_t yd, int64_t yh, int64_t yw, int64_t coordinate_transformation_mode, bool is_ncdhw);

// shuffle
template<typename T> DLL_EXPORT int space_to_depth(Context* ctx, const T* x, T* y, int64_t n, int64_t xc, int64_t xh, int64_t xw,
        int64_t block_size, bool is_nchw);
template<typename T> DLL_EXPORT int depth_to_space(Context* ctx, const T* x, T* y, int64_t n, int64_t xc, int64_t xh, int64_t xw,
        int64_t block_size, bool is_nchw);
template<typename T> DLL_EXPORT int pixel_shuffle(Context* ctx, const T* x, T* y, int64_t n, int64_t xc, int64_t xh, int64_t xw,
        int64_t r, bool is_nchw);
template<typename T> DLL_EXPORT int pixel_unshuffle(Context* ctx, const T* x, T* y, int64_t n, int64_t xc, int64_t xh, int64_t xw,
        int64_t r, bool is_nchw);
template<typename T> DLL_EXPORT int channel_shuffle(Context* ctx, const T* x, T* y, int64_t n, int64_t xc, int64_t xh, int64_t xw,
        int64_t group, bool is_nchw);
template<typename T> DLL_EXPORT int interpolate2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xh, int64_t xw,
        int64_t yh, int64_t yw, bool is_nearest, int64_t trans_mode, bool is_nchw,
        const float* max_x = nullptr, float* max_y = nullptr, int64_t ld_out_c = -1);
template<typename T> DLL_EXPORT int interpolate2d_grad(Context* ctx, const T* dy, T* dx, int64_t n, int64_t c, int64_t xh, int64_t xw,
        int64_t yh, int64_t yw, bool is_nearest, int64_t trans_mode, bool is_nchw);
template<typename T> DLL_EXPORT int interpolate1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xlen,
        int64_t ylen, bool is_nearest, int64_t trans_mode, bool is_ncw);
template<typename T, typename TID = int> DLL_EXPORT int roi_align(Context* ctx, const T* x, T* y,
        const T* rois, const TID* lod, int64_t xn, int64_t c, int64_t xh, int64_t xw, int64_t yn,
        int64_t yh, int64_t yw, float spatial_scale, int64_t sampling_ratio, bool is_nchw,
        int64_t mode = 0, bool has_theta = false);
template<typename T, typename TID = int> DLL_EXPORT int roi_align_grad(Context* ctx, const T* dy, T* dx, const T* rois,
        const TID* lod, int64_t xn, int64_t c, int64_t xh, int64_t xw, int64_t yn, int64_t yh, int64_t yw,
        float spatial_scale, int64_t sampling_ratio, bool is_nchw, bool continuous_coordinate = false);
template<typename T, typename TID = int> DLL_EXPORT int ps_roi_align(Context* ctx,
        const T* x, T* y, const T* rois, TID* channel_mapping, int64_t xn, int64_t xc, int64_t xh, int64_t xw,
        int64_t yn, int64_t yh, int64_t yw, float spatial_scale, int64_t sampling_ratio, bool is_nchw);
template<typename T, typename TID = int> DLL_EXPORT int roi_pool(Context* ctx,
        const T* x, T* y, const T* rois, TID* indices, int64_t xn, int64_t c, int64_t xh, int64_t xw,
        int64_t yn, int64_t yh, int64_t yw, float spatial_scale, bool is_nchw);
template<typename T, typename TID = int> DLL_EXPORT int ps_roi_pool(Context* ctx,
        const T* x, T* y, const T* rois, TID* channel_mapping, int64_t xn, int64_t xc, int64_t xh, int64_t xw,
        int64_t yn, int64_t yh, int64_t yw, float spatial_scale, bool is_nchw);
template<typename T> DLL_EXPORT int density_prior_box(Context* ctx, T* boxes,
        int64_t img_h, int64_t img_w, int64_t feature_h, int64_t feature_w,
        const std::vector<float>& fixed_sizes, const std::vector<float>& fixed_ratios,
        const std::vector<int64_t>& densities, float step_w, float step_h, float offset, bool is_clip);
template<typename T> DLL_EXPORT int pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::vector<int64_t>& pad, const char* mode, T value = 0, bool is_nchw = true);
template<typename T> DLL_EXPORT int anchor_generator(Context* ctx, T* anchors, int64_t h, int64_t w,
        const std::vector<float>& aspect_ratios, const std::vector<float>& anchor_sizes,
        const std::vector<float>& strides, float offset);
template<typename T> DLL_EXPORT int box_decoder(Context* ctx, const T* prior_box, const T* prior_box_var,
        const T* target_box,
        T* proposals, int64_t n_boxes, bool normalized, bool clip = false, const T* im_info = nullptr);
template<typename T> DLL_EXPORT int box_coder_encoder(Context* ctx, const T* prior_box, const T* target_box,
        const T* prior_box_var, const T* variance, T* output, int64_t row, int64_t col, bool normalized = true);
template<typename T> DLL_EXPORT int box_coder_decoder(Context* ctx, const T* prior_box, const T* target_box,
        const T* prior_box_var, const T* variance, T* output, int64_t row, int64_t col, int64_t axis = 0, bool normalized = true);
template<typename T, typename TID = int> DLL_EXPORT int yolo_box(Context* ctx, const T* input, const TID* img_size, T* boxes_data,
        T* scores_data,
        int64_t n, int64_t h, int64_t w, const std::vector<int64_t>& anchors, int64_t anchor_num, int64_t class_num,
        float conf_thresh, int64_t downsample_ratio, float scale = 1.0f, float bias = 0.0f, bool score_transpose = false,
        bool iou_aware = false, const float iou_aware_factor = 0.5f);
template<typename T, typename TID = int> DLL_EXPORT int sorted_nms(Context* ctx, const T* boxes, TID* index, int64_t& n_keep,
        int64_t n_boxes, float iou_thres, bool pixel_offset = true);
template<typename T, typename TID = int> DLL_EXPORT int accuracy(Context* ctx, const T* x, const T* y, int64_t m, int64_t n,
        TID* correct, TID* total, float* accuracy);
template<typename T> DLL_EXPORT int correlation(Context* ctx, const T* x, const T* y, T* z, int64_t n, int64_t xc,
        int64_t xh, int64_t xw, int64_t pad_size, int64_t kernel_size, int64_t stride1, int64_t stride2,
        int64_t max_displacement, int64_t corr_type_multiply);
template<typename T> DLL_EXPORT int grid_sample(Context* ctx, const T* x, const T* grid, T* y, int64_t n, int64_t c,
        int64_t xh, int64_t xw, int64_t yh, int64_t yw, bool is_nearest, bool align_corners, int64_t padding_mode,
        bool is_nchw);
template<typename T> DLL_EXPORT int grid_sample_grad(Context* ctx, const T* x, const T* grid, const T* dy, T* dx,
        T* d_grid, int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t yh, int64_t yw, bool is_nearest, bool align_corners,
        int64_t padding_mode, bool is_nchw);
template<typename T> DLL_EXPORT int grid_sample3d(Context* ctx, const T* x, const T* grid, T* y, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, int64_t yd, int64_t yh, int64_t yw, bool is_nearest, bool align_corners,
        int64_t padding_mode, bool is_ncdhw);
template<typename T> DLL_EXPORT int iou_similarity(Context* ctx, const T* x, const T* y, T* z, int64_t m, int64_t n,
        float eps, bool normalized);
template<typename T> DLL_EXPORT int gen_prior_box(Context* ctx, T* boxes,
        const VectorParam<float>& aspect_ratios, const VectorParam<float>& min_sizes,
        const VectorParam<float>& max_sizes, int64_t height, int64_t width, int64_t im_height,
        int64_t im_width, float offset, float step_height, float step_width, bool is_clip,
        bool min_max_aspect_ratios_order);
template<typename T> DLL_EXPORT int region(Context* ctx, const T* x, T* y, int64_t n, int64_t w, int64_t h,
        int64_t num_box, int64_t classes, int64_t coords);
template<typename T> DLL_EXPORT int temporal_shift(Context* ctx, const T* x, T* y, int64_t n,
        int64_t c, int64_t h, int64_t w, int64_t t, float shift_ratio, bool is_nhwc);
template<typename T> DLL_EXPORT int temporal_shift_grad(Context* ctx, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t h, int64_t w, int64_t t, float shift_ratio, bool is_nhwc);

//REC
template<typename T, typename TID> DLL_EXPORT int merge_dup_rows(Context* ctx, const T* x_data, T* y_data,
        const TID* x_rows, const TID* y_rows, int64_t xm, int64_t n, int64_t ym);
// NLP
template<typename T, typename TID> DLL_EXPORT int embedding(Context* ctx, const T* x, const TID* indices, T* y,
        int64_t xm, int64_t n, int64_t ym, int64_t padding_idx, TID start_index = 0);
template<typename T, typename TID> DLL_EXPORT int embedding_grad(Context* ctx, const T* dy, const TID* indices, T* dx,
        int64_t xm, int64_t n, int64_t ym, int64_t padding_idx, TID start_index = 0);
template<typename T, typename TID> DLL_EXPORT int sequence_max_pool(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lod,
        int64_t batch, int64_t dim, float pad_value, TID* max_index);
template<typename T, typename TID> DLL_EXPORT int sequence_first_pool(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lod,
        int64_t batch, int64_t dim, float pad_value);
template<typename T, typename TID> DLL_EXPORT int sequence_last_pool(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lod,
        int64_t batch, int64_t dim, float pad_value);
template<typename T, typename TID> DLL_EXPORT int sequence_sum_pool(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lod,
        int64_t batch, int64_t dim, float pad_value);
template<typename T, typename TID> DLL_EXPORT int sequence_mean_pool(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lod,
        int64_t batch, int64_t dim, float pad_value);
template<typename T, typename TID> DLL_EXPORT int sequence_max_pool_grad(Context* ctx, const T* dy, const TID* lod,
        T* dx, int64_t batch, int64_t dim, const TID* max_index);
template<typename T, typename TID> DLL_EXPORT int sequence_first_pool_grad(Context* ctx, const T* dy, const TID* lod,
        T* dx, int64_t batch, int64_t dim);
template<typename T, typename TID> DLL_EXPORT int sequence_last_pool_grad(Context* ctx, const T* dy, const TID* lod,
        T* dx, int64_t batch, int64_t dim);
template<typename T, typename TID> DLL_EXPORT int sequence_sum_pool_grad(Context* ctx, const T* dy, const TID* lod,
        T* dx, int64_t batch, int64_t dim);
template <typename T, typename TID> DLL_EXPORT int sequence_pad(Context* ctx, const T* x, const TID* lod, T* y,
        int64_t batch, int64_t seqlen, int64_t dim, float pad_value);
template <typename T, typename TID> DLL_EXPORT int sequence_pad(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lod,
        int64_t batch, int64_t seqlen, int64_t dim, float pad_value, bool seq_allow_zero = false);
template <typename T, typename TID> DLL_EXPORT int sequence_pad_non_uniform(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lod,
        int64_t batch, const VectorParam<TID>& max_seqlen_lod, int64_t dim, float pad_value);
template <typename T, typename TID> DLL_EXPORT int sequence_pad(Context* ctx, const T* x, const TID* lod,
        T* y, int64_t batch, int64_t max_seq_len, int64_t dim, T* pad_value_ptr, int64_t pad_value_size, TID* length_ptr);
template <typename T, typename TID> DLL_EXPORT int sequence_slice(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lodx,
        const VectorParam<TID>& offset, const VectorParam<TID>& lody, int64_t dims);
template <typename T, typename TID> DLL_EXPORT int sequence_insert(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lodx, const VectorParam<TID>& lody, int64_t dims);
template <typename T, typename TID> DLL_EXPORT int sequence_unpad(Context* ctx,
        const T* x, T* y, const VectorParam<TID>& lody, int64_t max_seqlen, int64_t dim, bool seq_allow_zero = false);
template<typename T, typename TID> DLL_EXPORT int sequence_topk_avg_pooling(Context* ctx, const T* x, T* y, TID* y_pos,
        int64_t channel_num,
        const VectorParam<TID>& row_lod,
        const VectorParam<TID>& col_lod,
        const VectorParam<TID>& topks);
template<typename T, typename TID> DLL_EXPORT int sequence_expand(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& lodx,
        const VectorParam<TID>& lody,
        const VectorParam<TID>& lod_ref,
        int64_t dims);
template <typename TX, typename TY> DLL_EXPORT int sequence_mask(Context* ctx, const TX* x, TY* y,
        int64_t batch, int64_t max_seq_len);
template <typename T, typename TID> DLL_EXPORT int sequence_reverse(Context* ctx, const T* x, T* y,
        const VectorParam<TID>& x_lod, int64_t dim);
template <typename T> DLL_EXPORT int sequence_concat(Context* ctx, const std::vector<const T*>& x_list,
        const std::vector<std::vector<int64_t>>& seqlens_list, T* y, int64_t dim);
template <typename T, typename TID> DLL_EXPORT int sequence_context_projection(Context* ctx,
        const T* x, T* y, const T* padding_data, const VectorParam<TID>& lodx, int64_t dim,
        int64_t context_start, int64_t context_len, int64_t context_stride, const std::vector<int64_t>& pad);
template <typename T, typename TID> DLL_EXPORT int sequence_context_projection_grad(Context* ctx,
        T* x, const T* y, const T* padding_data, const VectorParam<TID>& lodx, int64_t dim,
        int64_t context_start, int64_t context_len, int64_t context_stride, const std::vector<int64_t>& pad);
template <typename T, typename TID> DLL_EXPORT int sequence_to_batch(Context* ctx, const T* x, T* y, int64_t dim,
        const VectorParam<TID>& idx_sorted, const VectorParam<TID>& x_lod, const VectorParam<TID>& y_lod,
        bool is_reverse = false);
template <typename T, typename TID> DLL_EXPORT int batch_to_sequence(Context* ctx, const T* y, T* x, int64_t dim,
        const VectorParam<TID>& idx_sorted, const VectorParam<TID>& y_lod, const VectorParam<TID>& x_lod,
        bool is_reverse = false);
template <typename T, typename TID> DLL_EXPORT int attention_padding_mask(Context* ctx, const T* x,
        const TID* pad_begin, T* y,
        int64_t att_batch, int64_t att_len, int64_t src_len, int64_t src_batch, float mask);

template<typename T> DLL_EXPORT int one_hot(Context* ctx, const T* x, float* y, int64_t len, int depth,
        float on_value = 1.0f, float off_value = 0.0f);
template<typename T, typename TID = int> DLL_EXPORT int nms(Context* ctx, const T* boxes, const T* scores, TID* keep,
        int64_t n_boxes, int64_t nms_topk, float iou_thres, float score_thres, int64_t& keep_num, bool pixel_offset = true);
template<typename T, typename TID = int> DLL_EXPORT int nms_rotated(Context* ctx, const T* boxes, const T* scores, TID* keep,
        int64_t n_boxes, int64_t nms_topk, float iou_thres, float score_thres, int64_t& keep_num, bool clockwise = true);
template<typename T, typename TID = int>
DLL_EXPORT int multiclass_nms(Context* ctx, const T* bboxes, const T* scores,
        const std::vector<int64_t>& rois_num, std::vector<T>& out, std::vector<TID>& out_index,
        std::vector<size_t>& accumlated_det_num, int64_t n, int64_t b, int64_t class_num, int64_t out_dim,
        int64_t nms_topk, float score_threshold, int64_t keep_top_k, float nms_threshold,
        int64_t background_label, bool normalized, float nms_eta, bool return_index, bool is_lod);
template<typename T, typename TID = int>
DLL_EXPORT int multiclass_nms2(Context* ctx, const T* bboxes, const T* scores,
        std::vector<T>& out, std::vector<TID>& out_index, std::vector<size_t>& accumlated_det_num,
        int64_t n, int64_t b, int64_t class_num, int64_t out_dim, int64_t nms_topk,
        float score_threshold, int64_t keep_top_k, float nms_threshold,
        int64_t background_label, bool normalized, float nms_eta, bool return_index);
template<typename T, typename TID = int>
DLL_EXPORT int matrix_nms(Context* ctx, const T* bboxes, const T* scores,
        std::vector<T>& out, std::vector<TID>& out_index, std::vector<size_t>& accumlated_det_num,
        int64_t n, int64_t b, int64_t class_num, int64_t out_dim, int64_t nms_top_k,
        float score_threshold, int64_t keep_top_k, float post_threshold, bool use_gaussian, float gaussian_sigma,
        int64_t background_label, bool normalized, bool return_index);
template<typename T> DLL_EXPORT int polygon_box_transform(Context* ctx, const T* input, T* output,
        int64_t batch, int64_t geo_channel, int64_t height, int64_t width);
template<typename T> DLL_EXPORT int dropout(Context* ctx, const T* input, T* res, T* mask,
        unsigned int seed, int64_t n, bool is_upscale, float dropout_prob);
template<typename T> DLL_EXPORT int dropout_v2(Context* ctx, const T* input, T* res, uint8_t* mask,
        unsigned int seed, int64_t n, bool is_upscale, float dropout_prob);
template<typename T> DLL_EXPORT int dropout_v3(Context* ctx, const T* input, T* res, uint8_t* mask,
        unsigned int seed, int64_t n, bool is_upscale, float dropout_prob);
template<typename T> DLL_EXPORT int dropout_grad(Context* ctx, const T* mask, const T* dy, T* dx, float dropout_prob,
        int64_t n);
template<typename T> DLL_EXPORT int dropout_grad_v2(Context* ctx, const uint8_t* mask, const T* dy, T* dx, float dropout_prob,
        int64_t n);

template<typename T = float>
DLL_EXPORT int pow2_decay_with_linear_warmup(Context* ctx, T* lr, int64_t* step,
        int64_t warmup_steps, int64_t total_steps, float base_lr, float end_lr);

template<typename T> DLL_EXPORT int cvm(Context* ctx, const T* x, T* y, int64_t batch_size, int64_t len, bool use_cvm);
template<typename T, typename TID = int> DLL_EXPORT int cvm_grad(Context *ctx, const T* cvm, const T* dy, T* dx,
        const TID* lod, int64_t lod_size, int64_t batch_size, int64_t item_width, bool use_cvm);
// GRU
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT
int gru_unit(Context* ctx,
        const TX* x/*input*/, const TX* hidden_prev/*can be nullptr*/, const TW* weight, TY* y/*hidden*/,
        int64_t batch, int64_t hdim,
        const float* x_maxptr/*can be nullptr*/, const float* hidden_prev_maxptr/*can be nullptr*/,
        const float* w_maxptr/*float[8]*/, float* y_maxptr/*can be nullptr*/,
        const float* bias/*can be nullptr*/,
        const Activation_t& act = Activation_t::TANH,
        const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT
int gru_core(Context* ctx,
        const TX* x/*input*/, const TX* hidden_prev/*can be nullptr*/, const TW* weight, TY* y/*hidden*/,
        int64_t batch, int64_t seq_len, int64_t hdim,
        const float* x_maxptr/*can be nullptr*/, const float* hidden_prev_maxptr/*can be nullptr*/,
        const float* w_maxptr/*float[8]*/, float* y_maxptr/*can be nullptr*/,
        const float* bias/*can be nullptr*/,
        const Activation_t& act = Activation_t::TANH,
        const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false,
        bool is_reverse = false,
        bool reset_after = false);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT
int gru_core(Context* ctx,
        const TX* x/*input*/, const TX* hidden_prev/*can be nullptr*/, const TW* weight, TY* y/*hidden*/,
        const std::vector<int64_t>& seq_len_lod, int64_t hdim,
        const float* x_maxptr/*can be nullptr*/, const float* hidden_prev_maxptr/*can be nullptr*/,
        const float* w_maxptr/*float[8]*/, float* y_maxptr/*can be nullptr*/,
        const float* bias/*can be nullptr*/,
        const Activation_t& act = Activation_t::TANH,
        const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false,
        bool is_reverse = false);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT
int bigru_core(Context* ctx,
        const TX* fw_x/*fw_input*/, const TX* bw_x/*bw_input*/,
        const TX* fw_hidden_prev, const TX* bw_hidden_prev, // can be nullptr
        const TW* fw_weight, const TW* bw_weight,
        TY* fw_y/*fw_hidden*/, TY* bw_y/*bw_hidden*/,
        int64_t batch, int64_t seq_len, int64_t hdim,
        const float* fw_x_maxptr, const float* bw_x_maxptr, // can be nullptr
        const float* fw_hidden_prev_maxptr, const float* bw_hidden_prev_maxptr, // can be nullptr
        const float* fw_w_maxptr, const float* bw_w_maxptr, // float[8]
        float* fw_y_maxptr, float* bw_y_maxptr, // can be nullptr
        const float* fw_bias, const float* bw_bias, // can be nullptr
        const Activation_t& act = Activation_t::TANH,
        const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false);
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT
int bigru_core(Context* ctx,
        const TX* fw_x/*fw_input*/, const TX* bw_x/*bw_input*/,
        const TX* fw_hidden_prev, const TX* bw_hidden_prev, // can be nullptr
        const TW* fw_weight, const TW* bw_weight,
        TY* fw_y/*fw_hidden*/, TY* bw_y/*bw_hidden*/,
        const std::vector<int64_t>& seq_len_lod, int64_t hdim,
        const float* fw_x_maxptr, const float* bw_x_maxptr, // can be nullptr
        const float* fw_hidden_prev_maxptr, const float* bw_hidden_prev_maxptr, // can be nullptr
        const float* fw_w_maxptr, const float* bw_w_maxptr, // float[8]
        float* fw_y_maxptr, float* bw_y_maxptr, // can be nullptr
        const float* fw_bias, const float* bw_bias, // can be nullptr
        const Activation_t& act = Activation_t::TANH,
        const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false);
template<typename T, typename TID = int> DLL_EXPORT
int sequence_softmax(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis,
        const VectorParam<TID>& lod);

template <typename T, typename T_IDX>
DLL_EXPORT int bidirection_embedding_add(Context* ctx, const T* x, T* y0, T* y1, const VectorParam<T_IDX>& lodVP,
        const VectorParam<T_IDX>& idx0VP, const VectorParam<T_IDX>& idx1VP,
        int64_t table_len, int64_t dim, T_IDX padding_idx);

template<typename T> DLL_EXPORT int leaky_relu_grad_grad(Context* ctx, const T* x, const T* ddx, T* ddout,
        float alpha, int64_t len);
template<typename T> DLL_EXPORT int log_grad_grad(Context* ctx, const T* x, const T* dout, const T* ddx,
        T* dx, T* ddout, int64_t len);

template<typename T, typename TID, typename TLV = int> DLL_EXPORT int distribute_fpn_proposals_helper(Context* ctx,
        const T* fpn_rois, const api::VectorParam<TID>& rois_lod, TID* sub_lod_list, TLV* target_levels,
        int64_t min_level, int64_t max_level, int64_t refer_level, int64_t refer_scale, bool pixel_offset);
template<typename T1, typename T2, bool NeedAccumulate>
DLL_EXPORT int acc_merge_cpu(Context* ctx, const T1 *acc_ptr, int64_t total_val, T2 *out_ptr);
template<typename T1, typename T2, bool NeedAccumulate>
DLL_EXPORT int acc_merge_xpu(Context* ctx, const T1 *acc_ptr, const T1 *total_ptr, T2 *out_ptr);

// pytorch/paddle user defined op
template<typename T> DLL_EXPORT int boxes_iou_bev(Context* ctx, const T* boxes_a, const T* boxes_b,
        T* ans_iou, int64_t boxdim, int64_t n, int64_t m, int64_t criterion = -1);
template<typename T, typename TID = int> DLL_EXPORT int nms3d(Context* ctx, const T* boxes, TID* keep,
        int64_t n_boxes, float iou_thres, int64_t &keep_num, bool is_normal = false);
// hard_voxelize has only cpu impl
template<typename T, typename TID = int> DLL_EXPORT int hard_voxelize(Context* ctx, const T* points, T* voxels,
        TID* coords, TID* num_points_per_voxel, TID* num_voxels, const int64_t num_points, const int64_t num_point_dim,
        const std::vector<float>& voxel_size, const std::vector<float>& point_cloud_range,
        const int64_t max_num_points_in_voxel, const int64_t max_voxels);

template<typename T, typename TID>
DLL_EXPORT int set_mask_value(Context* ctx, T* x, const VectorParam<TID>& seqs,
        const bool* stop_flags, TID* output, const std::vector<int64_t>& xshape);

template <typename T> DLL_EXPORT int multi_tensor_scale(Context* ctx,
        std::vector<const T*> param_list, std::vector<int64_t> sizes, float scale, std::vector<T*> param_list_out, bool* noop_flag);
}

}
}
#endif
