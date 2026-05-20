#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_VEC_INT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_VEC_INT_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include "xpu/refactor/deprecation/deprecated.h"
#include "xpu/refactor/util/vector_util.h"
#include <iostream>

namespace baidu {
namespace xpu {
namespace api {
template<typename T> static inline int cvm_grad(Context *ctx, const T* cvm, const T* dy, T* dx,
        const std::nullptr_t lod, int64_t lod_size, int64_t batch_size, int64_t item_width, bool use_cvm) {
    return cvm_grad<T, int>(ctx, cvm, dy, dx, lod, lod_size, batch_size, item_width, use_cvm);
}
template<typename T> static inline int im2col1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
        int64_t ksize, int64_t stride, const std::vector<int>& pad, int64_t dilation, bool is_ncw) {
    return im2col1d(ctx, x, y, n, c, xw, ksize, stride, std::vector<int64_t>(pad.begin(), pad.end()),
            dilation, is_ncw);
}
template<typename T> static inline int im2col(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, const std::vector<int>& ksize,
        const std::vector<int>& stride, const std::vector<int>& pad, const std::vector<int>& dilation,
        bool is_nchw) {
    return im2col(ctx, x, y, n, c, h, w, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            std::vector<int64_t>(dilation.begin(), dilation.end()), is_nchw);
}
template<typename T> static inline int im2col3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int>& _ksize, const std::vector<int>& _stride,
        const std::vector<int>& _pad, const std::vector<int>& _dilation, bool is_ncdhw) {
    return im2col3d(ctx, x, y, n, c, xd, xh, xw, std::vector<int64_t>(_ksize.begin(), _ksize.end()),
            std::vector<int64_t>(_stride.begin(), _stride.end()), std::vector<int64_t>(_pad.begin(), _pad.end()),
            std::vector<int64_t>(_dilation.begin(), _dilation.end()), is_ncdhw);
}
template<typename T> static inline int im2im(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, bool is_nchw) {
    return im2im(ctx, x, y, n, c, h, w, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            std::vector<int64_t>(dilation.begin(), dilation.end()), is_nchw);
}
template<typename T> static inline int col2im1d(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xw,
        int64_t ksize, int64_t stride, const std::vector<int>& pad, int64_t dilation, bool is_ncw) {
    return col2im1d(ctx, y, x, n, c, xw, ksize, stride, std::vector<int64_t>(pad.begin(), pad.end()), dilation,
            is_ncw);
}
template<typename T> static inline int col2im(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, const std::vector<int>& dilation, bool is_nchw) {
    return col2im(ctx, y, x, n, c, xh, xw, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            is_nchw);
}
template<typename T> static inline int col2im3d(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, const std::vector<int>& dilation, bool is_ndhwc) {
    return col2im3d(ctx, y, x, n, c, xd, xh, xw, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            is_ndhwc);
}
template<typename T> static inline int deformable_im2col(Context* ctx, const T* x, const float* offset,
        const float* mask, T* y, int64_t n, int64_t c, int64_t xh, int64_t xw, const std::vector<int>& ksize,
        const std::vector<int>& stride, const std::vector<int>& pad, const std::vector<int>& dilation,
        const int64_t deformable_group, bool is_nchw) {
    return deformable_im2col(ctx, x, offset, mask, y, n, c, xh, xw, std::vector<int64_t>(ksize.begin(), ksize.end()),
            std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            std::vector<int64_t>(dilation.begin(), dilation.end()), deformable_group, is_nchw);
}
template<typename T> static inline int pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::vector<int>& pad, const char* mode, T value = 0, bool is_nchw = true) {
    return pad2d(ctx, x, y, n, c, h, w, std::vector<int64_t>(pad.begin(), pad.end()), mode, value, is_nchw);
}
template<typename T> static inline int l2_norm(Context* ctx,
        const T* x, T* y, T* norm, const std::vector<int>& xshape, int64_t axis, float eps) {
    return l2_norm(ctx, x, y, norm, std::vector<int64_t>(xshape.begin(), xshape.end()), axis, eps);
}
template<typename T> static inline int clip_by_norm(Context* ctx, const T* x, T* y, float max_norm,
        const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return clip_by_norm(ctx, x, y, max_norm, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template <typename T> static inline int norm(Context* ctx, const T* x, T* y, const std::vector<int>& x_shape,
        const std::vector<int>& dim, float p) {
    return norm(ctx, x, y, std::vector<int64_t>(x_shape.begin(), x_shape.end()),
            std::vector<int64_t>(dim.begin(), dim.end()), p);
}
template <typename T> static inline int merged_momentum(Context* ctx, const std::vector<T*>& param_list,
        const std::vector<T*>& velocity_list,
        const std::vector<T*>& grad_list,
        std::vector<T*>& param_out_list,
        std::vector<T*>& velocity_out_list,
        const std::vector<float>& l2_weight_decay,
        const std::vector<int>& sizes, const float* lr,
        float mu, int64_t use_nesterov) {
    return merged_momentum(ctx, param_list, velocity_list, grad_list, param_out_list, velocity_out_list,
            l2_weight_decay, std::vector<int64_t>(sizes.begin(), sizes.end()), lr, mu, use_nesterov);
}
template <typename T> static inline int lars_momentum(Context* ctx, const std::vector<T*>& param_list,
        const std::vector<T*>& grad_list,
        const std::vector<float*>& velocity_list, const std::vector<float*>& lrs,
        const std::vector<float*>& master_param_list,
        const std::vector<T*>& param_out_list, const std::vector<float*>& velocity_out_list,
        const std::vector<float*>& master_param_out_list, const std::vector<float>& lars_weight_decay,
        const std::vector<int>& param_sizes, float mu, float lars_coeff, float epsilon,
        float rescale_grad) {
    return lars_momentum(ctx, param_list, grad_list, velocity_list, lrs, master_param_list, param_out_list,
            velocity_out_list, master_param_out_list, lars_weight_decay, std::vector<int64_t>(param_sizes.begin(),
            param_sizes.end()), mu, lars_coeff, epsilon, rescale_grad);
}
template<typename T> static inline int density_prior_box(Context* ctx, T* boxes,
        int64_t img_h, int64_t img_w, int64_t feature_h, int64_t feature_w,
        const std::vector<float>& fixed_sizes, const std::vector<float>& fixed_ratios,
        const std::vector<int>& densities, float step_w, float step_h, float offset, bool is_clip) {
    return density_prior_box(ctx, boxes, img_h, img_w, feature_h, feature_w, fixed_sizes, fixed_ratios,
            std::vector<int64_t>(densities.begin(), densities.end()), step_w, step_h, offset, is_clip);
}
template<typename T> static inline int yolo_box(Context* ctx, const T* input, const int* img_size, T* boxes_data,
        T* scores_data, int64_t n, int64_t h, int64_t w, const std::vector<int>& anchors, int64_t anchor_num,
        int64_t class_num, float conf_thresh, int64_t downsample_ratio, float scale = 1.0f, float bias = 0.0f,
        bool score_transpose = false, bool iou_aware = false, const float iou_aware_factor = 0.5f) {
    return yolo_box(ctx, input, img_size, boxes_data, scores_data, n, h, w, std::vector<int64_t>(anchors.begin(),
            anchors.end()), anchor_num, class_num, conf_thresh, downsample_ratio, scale, bias, score_transpose,
            iou_aware, iou_aware_factor);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int gru_core(Context* ctx,
        const TX* x, const TX* hidden_prev, const TW* weight, TY* y,
        const std::vector<uint64_t>& seq_len_lod, int64_t hdim,
        const float* x_maxptr, const float* hidden_prev_maxptr, const float* w_maxptr, float* y_maxptr,
        const float* bias,
        const Activation_t& act = Activation_t::TANH, const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false, bool is_reverse = false) {
    return gru_core<TX, TW, TY, TGEMM>(ctx, x, hidden_prev, weight, y,
        std::vector<int64_t>(seq_len_lod.begin(), seq_len_lod.end()), hdim,
        x_maxptr, hidden_prev_maxptr, w_maxptr, y_maxptr, bias, act, gate_act, origin_mode, is_reverse);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int bigru_core(Context* ctx,
        const TX* fw_x, const TX* bw_x, const TX* fw_hidden_prev, const TX* bw_hidden_prev,
        const TW* fw_weight, const TW* bw_weight, TY* fw_y, TY* bw_y,
        const std::vector<uint64_t>& seq_len_lod, int64_t hdim, const float* fw_x_maxptr, const float* bw_x_maxptr,
        const float* fw_hidden_prev_maxptr, const float* bw_hidden_prev_maxptr,
        const float* fw_w_maxptr, const float* bw_w_maxptr, float* fw_y_maxptr, float* bw_y_maxptr,
        const float* fw_bias, const float* bw_bias,
        const Activation_t& act = Activation_t::TANH, const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false) {
    return bigru_core<TX, TW, TY, TGEMM>(ctx, fw_x, bw_x, fw_hidden_prev, bw_hidden_prev, fw_weight, bw_weight,
        fw_y, bw_y, std::vector<int64_t>(seq_len_lod.begin(), seq_len_lod.end()), hdim, fw_x_maxptr, bw_x_maxptr,
        fw_hidden_prev_maxptr, bw_hidden_prev_maxptr, fw_w_maxptr, bw_w_maxptr, fw_y_maxptr, bw_y_maxptr,
        fw_bias, bw_bias, act, gate_act, origin_mode);
}
template <typename T> static inline int sequence_concat(Context* ctx, const std::vector<const T*>& x_list,
        const std::vector<std::vector<int>>& seqlens_list, T* y, int64_t dim) {
    return sequence_concat(ctx, x_list, vvi32_to_vvi64(seqlens_list), y, dim);
}
template <typename T, typename TID> static inline int sequence_context_projection(Context* ctx,
        const T* x, T* y, const T* padding_data, const VectorParam<TID>& lodx, int64_t dim,
        int64_t context_start, int64_t context_len, int64_t context_stride, const std::vector<int>& pad) {
    return sequence_context_projection(ctx, x, y, padding_data, lodx, dim, context_start, context_len, context_stride,
            std::vector<int64_t>(pad.begin(), pad.end()));
}
template <typename T, typename TID> static inline int sequence_context_projection_grad(Context* ctx,
        T* x, const T* y, const T* padding_data, const VectorParam<TID>& lodx, int64_t dim,
        int64_t context_start, int64_t context_len, int64_t context_stride, const std::vector<int>& pad) {
    return sequence_context_projection_grad(ctx, x, y, padding_data, lodx, dim, context_start, context_len,
            context_stride, std::vector<int64_t>(pad.begin(), pad.end()));
}
template<typename T> static inline int sequence_softmax(Context* ctx, const T* x, T* y,
        const std::vector<int>& xshape, int64_t axis, const VectorParam<int>& lod) {
    return sequence_softmax(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            axis, lod);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv2d(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* x_maxptr,
        const float* weight_maxptr, float* y_maxptr, bool is_nchw) {
    return conv2d<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, h, w, f,
            std::vector<int64_t>(ksize.begin(), ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, x_maxptr, weight_maxptr, y_maxptr, is_nchw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv2d_grad(Context* ctx,
        const TX* x, const TW* weight, const TY* dy, TX* dx, TW* dweight, int64_t n, int64_t c, int64_t h, int64_t w,
        int64_t f, const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* x_maxptr, const float* w_maxptr,
        const float* dy_maxptr, float* dx_maxptr, float* dw_maxptr, bool is_nchw) {
    return conv2d_grad<TX, TW, TY, TGEMM>(ctx, x, weight, dy, dx, dweight, n, c, h, w, f, std::vector<int64_t>(ksize.begin(),
            ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            std::vector<int64_t>(dilation.begin(), dilation.end()), group, x_maxptr, w_maxptr, dy_maxptr, dx_maxptr,
            dw_maxptr, is_nchw);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline int conv2d_transpose(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t yh, int64_t yw, int64_t xc,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* y_maxptr,
        const float* weight_maxptr, float* x_maxptr, bool is_nchw) {
    return conv2d_transpose<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, yh, yw, xc,
            std::vector<int64_t>(ksize.begin(), ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, y_maxptr, weight_maxptr, x_maxptr, is_nchw);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline int conv2d_transpose_v2(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t xh, int64_t xw, int64_t xc,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* y_maxptr,
        const float* weight_maxptr, float* x_maxptr, bool is_nchw) {
    return conv2d_transpose_v2<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, xh, xw, xc,
            std::vector<int64_t>(ksize.begin(), ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, y_maxptr, weight_maxptr, x_maxptr, is_nchw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv3d_grad(Context* ctx,
        const TX* x, const TW* weight, const TY* dy, TX* dx, TW* dweight, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w,
        int64_t f, const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* x_maxptr, const float* w_maxptr,
        const float* dy_maxptr, float* dx_maxptr, float* dw_maxptr, bool is_ncdhw) {
    return conv3d_grad<TX, TW, TY, TGEMM>(ctx, x, weight, dy, dx, dweight, n, c, d, h, w, f,
            std::vector<int64_t>(ksize.begin(), ksize.end()), std::vector<int64_t>(stride.begin(),
            stride.end()), std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(),
            dilation.end()), group, x_maxptr, w_maxptr, dy_maxptr, dx_maxptr, dw_maxptr, is_ncdhw);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline int conv3d_transpose(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t yd, int64_t yh, int64_t yw, int64_t xc,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* y_maxptr,
        const float* weight_maxptr, float* x_maxptr, bool is_ndhwc) {
    return conv3d_transpose<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, yd, yh, yw, xc, std::vector<int64_t>(ksize.begin(),
            ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(), pad.end()),
            std::vector<int64_t>(dilation.begin(), dilation.end()), group, y_maxptr, weight_maxptr, x_maxptr, is_ndhwc);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline int conv2d_transpose_grad(
        Context* ctx, const TY* y, const TW* weight, const TX* dx, TY* dy, TW* dweight,
        int64_t n, int64_t yc, int64_t yh, int64_t yw, int64_t xc, int64_t xh, int64_t xw,
        const std::vector<int>& _ksize, const std::vector<int>& _stride, const std::vector<int>& _pad,
        const std::vector<int>& _dilation, int64_t group, const float* y_maxptr, const float* w_maxptr,
        const float* dx_maxptr, float* dy_maxptr, float* dw_maxptr, bool is_nchw) {
    return conv2d_transpose_grad<TY, TW, TX, TGEMM>(ctx, y, weight, dx, dy, dweight, n, yc, yh, yw, xc, xh, xw,
            std::vector<int64_t>(_ksize.begin(), _ksize.end()), std::vector<int64_t>(_stride.begin(), _stride.end()),
            std::vector<int64_t>(_pad.begin(), _pad.end()), std::vector<int64_t>(_dilation.begin(), _dilation.end()),
            group, y_maxptr, w_maxptr, dx_maxptr, dy_maxptr, dw_maxptr, is_nchw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv3d(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w, int64_t f,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* x_maxptr, const float* weight_maxptr,
        float* y_maxptr, bool is_ncdhw) {
    return conv3d<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, d, h, w, f, std::vector<int64_t>(ksize.begin(),
            ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, x_maxptr, weight_maxptr, y_maxptr, is_ncdhw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int deformable_conv(
        Context* ctx, const TX* x, const TW* weight, const float* offset, const float* mask,
        TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, int64_t deformable_group,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr, bool is_nchw) {
    return deformable_conv<TX, TW, TY, TGEMM>(ctx, x, weight, offset, mask, y, n, c, h, w, f,
            std::vector<int64_t>(ksize.begin(), ksize.end()), std::vector<int64_t>(stride.begin(),
            stride.end()), std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(),
            dilation.end()), group, deformable_group, x_maxptr, w_maxptr, y_maxptr, is_nchw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int deformable_conv_grad(
        Context* ctx, const TX* x, const TW* weight, const float* offset, const float* mask,
        const TY* dy, TX* dx, TW* dw, float* doffset, float* dmask, int64_t n, int64_t c, int64_t h, int64_t w,
        int64_t f, const std::vector<int>& ksize, const std::vector<int>& stride,
        const std::vector<int>& pad, const std::vector<int>& dilation, int64_t group, int64_t deformable_group,
        const float* x_maxptr, const float* w_maxptr, float* dy_maxptr, float* dx_maxptr, float* dw_maxptr,
        bool is_nchw) {
    return deformable_conv_grad<TX, TW, TY, TGEMM>(ctx, x, weight, offset, mask, dy, dx, dw, doffset, dmask, n, c, h,
            w, f, std::vector<int64_t>(ksize.begin(), ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, deformable_group, x_maxptr, w_maxptr, dy_maxptr, dx_maxptr, dw_maxptr, is_nchw);
}

template<typename T> static inline int im2col1d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xw,
        int64_t ksize, int64_t stride, const std::initializer_list<int64_t>& pad, int64_t dilation, bool is_ncw) {
    return im2col1d(ctx, x, y, n, c, xw, ksize, stride, std::vector<int64_t>(pad), dilation, is_ncw);
}
template<typename T> static inline int im2col(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad,
        const std::initializer_list<int64_t>& dilation, bool is_nchw) {
    return im2col(ctx, x, y, n, c, h, w, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), std::vector<int64_t>(dilation), is_nchw);
}
template<typename T> static inline int im2col3d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::initializer_list<int64_t>& _ksize,
        const std::initializer_list<int64_t>& _stride, const std::initializer_list<int64_t>& _pad,
        const std::initializer_list<int64_t>& _dilation, bool is_ncdhw) {
    return im2col3d(ctx, x, y, n, c, xd, xh, xw, std::vector<int64_t>(_ksize),
            std::vector<int64_t>(_stride), std::vector<int64_t>(_pad),
            std::vector<int64_t>(_dilation), is_ncdhw);
}
template<typename T> static inline int im2im(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation, bool is_nchw) {
    return im2im(ctx, x, y, n, c, h, w, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), std::vector<int64_t>(dilation), is_nchw);
}
template<typename T> static inline int col2im1d(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xw,
        int64_t ksize, int64_t stride, const std::initializer_list<int64_t>& pad, int64_t dilation, bool is_ncw) {
    return col2im1d(ctx, y, x, n, c, xw, ksize, stride, std::vector<int64_t>(pad), dilation, is_ncw);
}
template<typename T> static inline int col2im(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xh,
        int64_t xw, const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation, bool is_nchw) {
    return col2im(ctx, y, x, n, c, xh, xw, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), std::vector<int64_t>(dilation), is_nchw);
}
template<typename T> static inline int col2im3d(Context* ctx, const T* y, T* x, int64_t n, int64_t c, int64_t xd,
        int64_t xh, int64_t xw, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad,
        const std::initializer_list<int64_t>& dilation, bool is_ndhwc) {
    return col2im3d(ctx, y, x, n, c, xd, xh, xw, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), std::vector<int64_t>(dilation), is_ndhwc);
}
template<typename T> static inline int pad2d(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::initializer_list<int64_t>& pad, const char* mode, T value = 0, bool is_nchw = true) {
    return pad2d(ctx, x, y, n, c, h, w, std::vector<int64_t>(pad), mode, value, is_nchw);
}
template<typename T> static inline int deformable_im2col(Context* ctx, const T* x, const float* offset,
        const float* mask, T* y, int64_t n, int64_t c, int64_t xh, int64_t xw,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        const int64_t deformable_group, bool is_nchw) {
    return deformable_im2col(ctx, x, offset, mask, y, n, c, xh, xw, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), std::vector<int64_t>(dilation),
            deformable_group, is_nchw);
}
template<typename T> static inline int l2_norm(Context* ctx,
        const T* x, T* y, T* norm, const std::initializer_list<int64_t>& xshape, int64_t axis, float eps) {
    return l2_norm(ctx, x, y, norm, std::vector<int64_t>(xshape), axis, eps);
}
template<typename T> static inline int clip_by_norm(Context* ctx, const T* x, T* y, float max_norm,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& rdims) {
    return clip_by_norm(ctx, x, y, max_norm, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template <typename T> static inline int norm(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& x_shape, const std::initializer_list<int64_t>& dim, float p) {
    return norm(ctx, x, y, std::vector<int64_t>(x_shape), std::vector<int64_t>(dim), p);
}
template <typename T> static inline int merged_momentum(Context* ctx, const std::vector<T*>& param_list,
        const std::vector<T*>& velocity_list,
        const std::vector<T*>& grad_list,
        std::vector<T*>& param_out_list,
        std::vector<T*>& velocity_out_list,
        const std::vector<float>& l2_weight_decay,
        const std::initializer_list<int64_t>& sizes, const float* lr,
        float mu, int64_t use_nesterov) {
    return merged_momentum(ctx, param_list, velocity_list, grad_list, param_out_list, velocity_out_list,
            l2_weight_decay, std::vector<int64_t>(sizes), lr, mu, use_nesterov);
}
template <typename T> static inline int lars_momentum(Context* ctx, const std::vector<T*>& param_list,
        const std::vector<T*>& grad_list,
        const std::vector<float*>& velocity_list, const std::vector<float*>& lrs,
        const std::vector<float*>& master_param_list,
        const std::vector<T*>& param_out_list, const std::vector<float*>& velocity_out_list,
        const std::vector<float*>& master_param_out_list, const std::vector<float>& lars_weight_decay,
        const std::initializer_list<int64_t>& param_sizes, float mu, float lars_coeff, float epsilon,
        float rescale_grad) {
    return lars_momentum(ctx, param_list, grad_list, velocity_list, lrs, master_param_list, param_out_list,
            velocity_out_list, master_param_out_list, lars_weight_decay, std::vector<int64_t>(param_sizes),
            mu, lars_coeff, epsilon, rescale_grad);
}
template<typename T> static inline int density_prior_box(Context* ctx, T* boxes,
        int64_t img_h, int64_t img_w, int64_t feature_h, int64_t feature_w,
        const std::vector<float>& fixed_sizes, const std::vector<float>& fixed_ratios,
        const std::initializer_list<int64_t>& densities, float step_w, float step_h, float offset, bool is_clip) {
    return density_prior_box(ctx, boxes, img_h, img_w, feature_h, feature_w, fixed_sizes, fixed_ratios,
            std::vector<int64_t>(densities), step_w, step_h, offset, is_clip);
}
template<typename T> static inline int yolo_box(Context* ctx, const T* input, const int* img_size, T* boxes_data,
        T* scores_data, int64_t n, int64_t h, int64_t w, const std::initializer_list<int64_t>& anchors,
        int64_t anchor_num, int64_t class_num, float conf_thresh, int64_t downsample_ratio, float scale = 1.0f,
        float bias = 0.0f, bool score_transpose = false) {
    return yolo_box(ctx, input, img_size, boxes_data, scores_data, n, h, w, std::vector<int64_t>(anchors),
            anchor_num, class_num, conf_thresh, downsample_ratio, scale, bias, score_transpose);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int gru_core(Context* ctx,
        const TX* x, const TX* hidden_prev, const TW* weight, TY* y,
        const std::initializer_list<int64_t>& seq_len_lod, int64_t hdim,
        const float* x_maxptr, const float* hidden_prev_maxptr, const float* w_maxptr, float* y_maxptr,
        const float* bias,
        const Activation_t& act = Activation_t::TANH, const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false, bool is_reverse = false) {
    return gru_core<TX, TW, TY, TGEMM>(ctx, x, hidden_prev, weight, y, std::vector<int64_t>(seq_len_lod), hdim,
        x_maxptr, hidden_prev_maxptr, w_maxptr, y_maxptr, bias, act, gate_act, origin_mode, is_reverse);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int bigru_core(Context* ctx,
        const TX* fw_x, const TX* bw_x, const TX* fw_hidden_prev, const TX* bw_hidden_prev,
        const TW* fw_weight, const TW* bw_weight, TY* fw_y, TY* bw_y,
        const std::initializer_list<int64_t>& seq_len_lod, int64_t hdim,
        const float* fw_x_maxptr, const float* bw_x_maxptr,
        const float* fw_hidden_prev_maxptr, const float* bw_hidden_prev_maxptr,
        const float* fw_w_maxptr, const float* bw_w_maxptr, float* fw_y_maxptr, float* bw_y_maxptr,
        const float* fw_bias, const float* bw_bias,
        const Activation_t& act = Activation_t::TANH, const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false) {
    return bigru_core<TX, TW, TY, TGEMM>(ctx, fw_x, bw_x, fw_hidden_prev, bw_hidden_prev, fw_weight, bw_weight,
        fw_y, bw_y, std::vector<int64_t>(seq_len_lod), hdim, fw_x_maxptr, bw_x_maxptr,
        fw_hidden_prev_maxptr, bw_hidden_prev_maxptr, fw_w_maxptr, bw_w_maxptr, fw_y_maxptr, bw_y_maxptr,
        fw_bias, bw_bias, act, gate_act, origin_mode);
}
template <typename T> static inline int sequence_concat(Context* ctx, const std::vector<const T*>& x_list,
        const std::initializer_list<std::initializer_list<int>>& seqlens_list, T* y, int64_t dim) {
    return sequence_concat(ctx, x_list, std::vector<std::vector<int64_t>>(seqlens_list.begin(),
            seqlens_list.end()), y, dim);
}
template <typename T, typename TID> static inline int sequence_context_projection(Context* ctx,
        const T* x, T* y, const T* padding_data, const VectorParam<TID>& lodx, int64_t dim,
        int64_t context_start, int64_t context_len, int64_t context_stride,
        const std::initializer_list<int64_t>& pad) {
    return sequence_context_projection(ctx, x, y, padding_data, lodx, dim, context_start, context_len, context_stride,
            std::vector<int64_t>(pad));
}
template <typename T, typename TID> static inline int sequence_context_projection_grad(Context* ctx,
        T* x, const T* y, const T* padding_data, const VectorParam<TID>& lodx, int64_t dim,
        int64_t context_start, int64_t context_len, int64_t context_stride,
        const std::initializer_list<int64_t>& pad) {
    return sequence_context_projection_grad(ctx, x, y, padding_data, lodx, dim, context_start, context_len,
            context_stride, std::vector<int64_t>(pad));
}
template<typename T> static inline int sequence_softmax(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, int64_t axis, const VectorParam<int>& lod) {
    return sequence_softmax(ctx, x, y, std::vector<int64_t>(xshape), axis, lod);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv2d(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        int64_t group, const float* x_maxptr, const float* weight_maxptr, float* y_maxptr, bool is_nchw) {
    return conv2d<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, h, w, f,
            std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), std::vector<int64_t>(dilation),
            group, x_maxptr, weight_maxptr, y_maxptr, is_nchw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv2d_grad(Context* ctx,
        const TX* x, const TW* weight, const TY* dy, TX* dx, TW* dweight, int64_t n, int64_t c, int64_t h, int64_t w,
        int64_t f, const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation, int64_t group,
        const float* x_maxptr, const float* w_maxptr, const float* dy_maxptr, float* dx_maxptr, float* dw_maxptr,
        bool is_nchw) {
    return conv2d_grad<TX, TW, TY, TGEMM>(ctx, x, weight, dy, dx, dweight, n, c, h, w, f, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), std::vector<int64_t>(dilation), group, x_maxptr,
            w_maxptr, dy_maxptr, dx_maxptr, dw_maxptr, is_nchw);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline int conv2d_transpose(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t yh, int64_t yw, int64_t xc,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation, int64_t group,
        const float* y_maxptr, const float* weight_maxptr, float* x_maxptr, bool is_nchw) {
    return conv2d_transpose<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, yh, yw, xc, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), std::vector<int64_t>(dilation), group,
            y_maxptr, weight_maxptr, x_maxptr, is_nchw);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline int conv2d_transpose_v2(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t xh, int64_t xw, int64_t xc,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation, int64_t group,
        const float* y_maxptr, const float* weight_maxptr, float* x_maxptr, bool is_nchw) {
    return conv2d_transpose_v2<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, xh, xw, xc, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), std::vector<int64_t>(dilation), group, y_maxptr,
            weight_maxptr, x_maxptr, is_nchw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv3d_grad(Context* ctx,
        const TX* x, const TW* weight, const TY* dy, TX* dx, TW* dweight, int64_t n, int64_t c, int64_t d, int64_t h,
        int64_t w, int64_t f, const std::initializer_list<int64_t>& ksize,
        const std::initializer_list<int64_t>& stride, const std::initializer_list<int64_t>& pad,
        const std::initializer_list<int64_t>& dilation, int64_t group, const float* x_maxptr, const float* w_maxptr,
        const float* dy_maxptr, float* dx_maxptr, float* dw_maxptr, bool is_ncdhw) {
    return conv3d_grad<TX, TW, TY, TGEMM>(ctx, x, weight, dy, dx, dweight, n, c, d, h, w, f,
            std::vector<int64_t>(ksize), std::vector<int64_t>(stride), std::vector<int64_t>(pad),
            std::vector<int64_t>(dilation), group, x_maxptr, w_maxptr, dy_maxptr, dx_maxptr, dw_maxptr, is_ncdhw);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline int conv3d_transpose(
        Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc, int64_t yd, int64_t yh, int64_t yw,
        int64_t xc, const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation, int64_t group,
        const float* y_maxptr, const float* weight_maxptr, float* x_maxptr, bool is_ndhwc) {
    return conv3d_transpose<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, yd, yh, yw, xc, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad),
            std::vector<int64_t>(dilation), group, y_maxptr, weight_maxptr, x_maxptr, is_ndhwc);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline int conv2d_transpose_grad(
        Context* ctx, const TY* y, const TW* weight, const TX* dx, TY* dy, TW* dweight,
        int64_t n, int64_t yc, int64_t yh, int64_t yw, int64_t xc, int64_t xh, int64_t xw,
        const std::initializer_list<int64_t>& _ksize, const std::initializer_list<int64_t>& _stride,
        const std::initializer_list<int64_t>& _pad,
        const std::initializer_list<int64_t>& _dilation, int64_t group, const float* y_maxptr, const float* w_maxptr,
        const float* dx_maxptr, float* dy_maxptr, float* dw_maxptr, bool is_nchw) {
    return conv2d_transpose_grad<TY, TW, TX, TGEMM>(ctx, y, weight, dx, dy, dweight, n, yc, yh, yw, xc, xh, xw,
            std::vector<int64_t>(_ksize), std::vector<int64_t>(_stride),
            std::vector<int64_t>(_pad), std::vector<int64_t>(_dilation),
            group, y_maxptr, w_maxptr, dx_maxptr, dy_maxptr, dw_maxptr, is_nchw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv3d(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w, int64_t f,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        int64_t group, const float* x_maxptr, const float* weight_maxptr, float* y_maxptr, bool is_ncdhw) {
    return conv3d<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, d, h, w, f, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), std::vector<int64_t>(dilation),
            group, x_maxptr, weight_maxptr, y_maxptr, is_ncdhw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int deformable_conv(
        Context* ctx, const TX* x, const TW* weight, const float* offset, const float* mask,
        TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        int64_t group, int64_t deformable_group, const float* x_maxptr, const float* w_maxptr,
        float* y_maxptr, bool is_nchw) {
    return deformable_conv<TX, TW, TY, TGEMM>(ctx, x, weight, offset, mask, y, n, c, h, w, f,
            std::vector<int64_t>(ksize), std::vector<int64_t>(stride), std::vector<int64_t>(pad),
            std::vector<int64_t>(dilation), group, deformable_group, x_maxptr, w_maxptr, y_maxptr, is_nchw);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int deformable_conv_grad(
        Context* ctx, const TX* x, const TW* weight, const float* offset, const float* mask,
        const TY* dy, TX* dx, TW* dw, float* doffset, float* dmask, int64_t n, int64_t c, int64_t h, int64_t w,
        int64_t f, const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation, int64_t group,
        int64_t deformable_group, const float* x_maxptr, const float* w_maxptr, float* dy_maxptr, float* dx_maxptr,
        float* dw_maxptr, bool is_nchw) {
    return deformable_conv_grad<TX, TW, TY, TGEMM>(ctx, x, weight, offset, mask, dy, dx, dw, doffset, dmask, n, c, h,
            w, f, std::vector<int64_t>(ksize), std::vector<int64_t>(stride),
            std::vector<int64_t>(pad), std::vector<int64_t>(dilation),
            group, deformable_group, x_maxptr, w_maxptr, dy_maxptr, dx_maxptr, dw_maxptr, is_nchw);
}

template<typename T> static inline int sorted_nms(Context* ctx, const T* boxes, int* index, int& n_keep,
        int64_t n_boxes, float iou_thres, bool pixel_offset = true) {
    int ret = 0;
    int64_t n_keep_i64 = n_keep;
    ret = sorted_nms<T, int>(ctx, boxes, index, n_keep_i64, n_boxes, iou_thres, pixel_offset);
    n_keep = n_keep_i64;
    return ret;
}

template<typename T> static inline int nms(Context* ctx, const T* boxes, const T* scores, int* keep,
        int64_t n_boxes, int64_t nms_topk, float iou_thres, float score_thres, int& keep_num, bool pixel_offset = true){
    int ret = 0;
    int64_t keep_num_i64 = keep_num;
    ret = nms<T, int>(ctx, boxes, scores, keep, n_boxes, nms_topk, iou_thres, score_thres, keep_num_i64, pixel_offset);
    keep_num = keep_num_i64;
    return ret;
}

template<typename T, typename TID = int> static inline int multiclass_nms(Context* ctx, const T* bboxes, 
        const T* scores, const std::vector<int>& rois_num, std::vector<T>& out, std::vector<TID>& out_index,
        std::vector<size_t>& accumlated_det_num, int n, int b, int class_num, int out_dim,
        int nms_topk, float score_threshold, int keep_top_k, float nms_threshold,
        int background_label, bool normalized, float nms_eta, bool return_index, bool is_lod){
    return multiclass_nms<T, TID>(ctx, bboxes, scores, std::vector<int64_t>(rois_num.begin(), rois_num.end()),
            out, out_index, accumlated_det_num, n, b, class_num, out_dim, nms_topk, score_threshold, keep_top_k,
            nms_threshold, background_label, normalized, nms_eta, return_index, is_lod);
}

// to be del
template<typename T, typename TID> static inline int multiclass_nms2(Context* ctx, const T* bboxes,
        const T* scores, std::vector<T>& out, std::vector<TID>& out_index, std::vector<size_t>& accumlated_det_num,
        int n, int b, int class_num, int out_dim, int nms_topk,
        float score_threshold, int keep_top_k, float nms_threshold,
        int background_label, bool normalized, float nms_eta, bool return_index){
    return multiclass_nms2<T, TID>(ctx, bboxes,scores, out, out_index, accumlated_det_num, static_cast<int64_t>(n),
            static_cast<int64_t>(b), static_cast<int64_t>(class_num), static_cast<int64_t>(out_dim),
            static_cast<int64_t>(nms_topk), score_threshold, static_cast<int64_t>(keep_top_k), nms_threshold, 
            static_cast<int64_t>(background_label), normalized, nms_eta, return_index);
}

template<typename TX, typename TW, typename TY, typename TID = int> DLL_EXPORT int fc_fusion_norm(Context* ctx,
        const TX* x, const TW* w, TY* y, int64_t m, int64_t n, int64_t k, bool x_trans, bool w_trans,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr, int64_t ldx, int64_t ldw,
        int64_t ldy, float alpha, float beta, const float* bias, const Activation_t& act,
        float normal_value, int64_t batch_size, std::vector<TID>& lod);

}
}
}
#endif
