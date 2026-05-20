#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_FUSION_VEC_INT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_FUSION_VEC_INT_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include "xpu/refactor/deprecation/deprecated.h"
#include "xpu/refactor/util/vector_util.h"

namespace baidu {
namespace xpu {
namespace api {

template<typename T, typename TW, typename TGEMM> static inline int lstm_train(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* w_x, const TW* w_h, const TW* b_x,
        const TW* b_h, T* y, T* last_h, T* last_c, int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len,
        const std::vector<int>& seq_len_tensor, bool is_reverse, const float* x_maxptr, const float* h_maxptr,
        const float* wx_maxptr, const float* wh_maxptr, T* i_f_g_o, T* c, const Activation_t& act,
        const Activation_t& recurrent_act) {
    return lstm_train<T, TW, TGEMM>(ctx, x, init_h, init_c, w_x, w_h, b_x, b_h, y, last_h, last_c,
        batch_size, xdim, hdim, seq_len, std::vector<int64_t>(seq_len_tensor.begin(), seq_len_tensor.end()),
        is_reverse, x_maxptr, h_maxptr, wx_maxptr, wh_maxptr, i_f_g_o, c, act, recurrent_act);
}
template<typename T, typename TW, typename TGEMM> static inline int lstm_grad(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* w_x, const TW* w_h, const T* y, const T* y_grad, const T* last_h_grad,
        const T* last_c_grad, T* x_grad, T* init_h_grad, T* init_c_grad, TW* w_x_grad, TW* w_h_grad, TW* b_x_grad,
        TW* b_h_grad, int batch_size, int xdim, int hdim, int seq_len, const std::vector<int>& seq_len_tensor,
        const float* x_maxptr, const float* h_maxptr, const float* wx_maxptr, const float* wh_maxptr,
        const T* i_f_g_o, const T* c) {
    return lstm_grad<T, TW, TGEMM>(ctx, x, init_h, init_c, w_x, w_h, y, y_grad, last_h_grad, last_c_grad, x_grad,
        init_h_grad, init_c_grad, w_x_grad, w_h_grad, b_x_grad, b_h_grad, batch_size, xdim, hdim, seq_len,
        std::vector<int64_t>(seq_len_tensor.begin(), seq_len_tensor.end()), x_maxptr, h_maxptr, wx_maxptr, wh_maxptr,
        i_f_g_o, c);
}
template<typename T, typename TW, typename TGEMM> static inline int bilstm_train(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* forward_w_x, const TW* forward_w_h,
        const TW* forward_b_x, const TW* forward_b_h, const TW* backward_w_x, const TW* backward_w_h,
        const TW* backward_b_x, const TW* backward_b_h, T* y, T* last_h, T* last_c,
        int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len, const std::vector<int>& seq_len_tensor,
        int64_t layer_num, const float* x_maxptr, const float* h_maxptr,
        const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, T* i_f_g_o, T* c, const Activation_t& act,
        const Activation_t& recurrent_act) {
    return bilstm_train<T, TW, TGEMM>(ctx, x, init_h, init_c, forward_w_x, forward_w_h, forward_b_x, forward_b_h,
        backward_w_x, backward_w_h, backward_b_x, backward_b_h, y, last_h, last_c, batch_size, xdim, hdim, seq_len,
        std::vector<int64_t>(seq_len_tensor.begin(), seq_len_tensor.end()),
        layer_num, x_maxptr, h_maxptr, forward_wx_maxptr, forward_wh_maxptr, backward_wx_maxptr, backward_wh_maxptr,
        i_f_g_o, c, act, recurrent_act);
}
template<typename T, typename TW, typename TGEMM> static inline int bilstm_grad(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* backward_w_x,
        const TW* backward_w_h, const T* y, const T* y_grad, const T* last_h_grad, const T* last_c_grad, T* x_grad,
        T* init_h_grad, T* init_c_grad, TW* forward_w_x_grad, TW* forward_w_h_grad,
        TW* forward_b_x_grad, TW* forward_b_h_grad, TW* backward_w_x_grad, TW* backward_w_h_grad,
        TW* backward_b_x_grad, TW* backward_b_h_grad, int batch_size, int xdim, int hdim, int seq_len,
        const std::vector<int>& seq_len_tensor, int layer_num, const float* x_maxptr, const float* h_maxptr,
        const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, const T* i_f_g_o, const T* c,
        const Activation_t& act, const Activation_t& recurrent_act) {
    return bilstm_grad<T, TW, TGEMM>(ctx, x, init_h, init_c, forward_w_x, forward_w_h, backward_w_x, backward_w_h, y,
        y_grad, last_h_grad, last_c_grad, x_grad, init_h_grad, init_c_grad,
        forward_w_x_grad, forward_w_h_grad, forward_b_x_grad, forward_b_h_grad,
        backward_w_x_grad, backward_w_h_grad, backward_b_x_grad, backward_b_h_grad, batch_size, xdim, hdim, seq_len,
        std::vector<int64_t>(seq_len_tensor.begin(), seq_len_tensor.end()),
        layer_num, x_maxptr, h_maxptr, forward_wx_maxptr, forward_wh_maxptr, backward_wx_maxptr, backward_wh_maxptr,
        i_f_g_o, c, act, recurrent_act);
}
template<typename T, typename TW, typename TGEMM> static inline int bilstm_inference(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* forward_w_x, const TW* forward_w_h,
        const TW* forward_b_x, const TW* forward_b_h, const TW* backward_w_x, const TW* backward_w_h,
        const TW* backward_b_x, const TW* backward_b_h, T* y, T* last_h, T* last_c,
        int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len, const std::vector<int>& seq_len_tensor,
        int64_t layer_num, const float* x_maxptr, const float* h_maxptr,
        const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr,
        const Activation_t& act, const Activation_t& recurrent_act) {
    return bilstm_inference<T, TW, TGEMM>(ctx, x, init_h, init_c, forward_w_x, forward_w_h, forward_b_x, forward_b_h,
        backward_w_x, backward_w_h, backward_b_x, backward_b_h, y, last_h, last_c, batch_size, xdim, hdim, seq_len,
        std::vector<int64_t>(seq_len_tensor.begin(), seq_len_tensor.end()),
        layer_num, x_maxptr, h_maxptr, forward_wx_maxptr, forward_wh_maxptr, backward_wx_maxptr, backward_wh_maxptr,
        act, recurrent_act);
}
template<typename T, typename TID> static inline int sorted_softmax_topk(Context* ctx, const T* x, T* y, TID* index,
        const std::vector<int>& xshape, int64_t axis, int64_t k) {
    return sorted_softmax_topk(ctx, x, y, index, std::vector<int64_t>(xshape.begin(), xshape.end()), axis, k);
}
template<typename TX, typename TY, typename TZ, typename TGEMM> int attention_fusion(
        Context* ctx, const TX* x, const TY* y, const float* mask, TZ* z,
        int64_t batch_size, int64_t head_num, int64_t m, int64_t n, int64_t k,
        const std::vector<int>& mask_shape,
        bool x_trans, bool y_trans, float alpha, float beta,
        const float* max_x, const float* max_y, float* max_z,
        bool fuse_x_transpose0213, bool fuse_y_transpose0213, bool fuse_z_transpose0213,
        bool fuse_softmax) {
    return attention_fusion(ctx, x, y, mask, z, batch_size, head_num, m, n, k, std::vector<int64_t>(mask_shape.begin(), mask_shape.end()), 
                x_trans, y_trans, alpha, beta, max_x, max_y, max_z, fuse_x_transpose0213, fuse_y_transpose0213, 
                fuse_z_transpose0213, fuse_softmax);
}

template<typename T> static inline int yolo_box_coord(Context* ctx,
        const T* x, T* y,
        const std::vector<int>& x_shape,
        const float* grid,
        const float* stride,
        const float* anchor_grid,
        const std::vector<int>& grid_shape,
        const std::vector<int>& stride_shape,
        const std::vector<int>& anchor_grid_shape,
        float offset,
        float* x_max,
        float* y_max) {
    return yolo_box_coord(ctx, x, y, std::vector<int64_t>(x_shape.begin(), x_shape.end()), grid, stride, anchor_grid,
            std::vector<int64_t>(grid_shape.begin(), grid_shape.end()), std::vector<int64_t>(stride_shape.begin(),
            stride_shape.end()), std::vector<int64_t>(anchor_grid_shape.begin(), anchor_grid_shape.end()), offset, x_max,
            y_max);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv2d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* x_maxptr, const float* weight_maxptr,
        float* y_maxptr, bool is_nchw, const float* bias, const TY* branch, const Activation_t& act,
        const float* branch_maxptr = nullptr, const float* scale = nullptr) {
    return conv2d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, h, w, f,
            std::vector<int64_t>(ksize.begin(), ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, x_maxptr, weight_maxptr, y_maxptr, is_nchw, bias, branch, act, branch_maxptr, scale);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv3d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w, int64_t f,
        const std::vector<int>& ksize, const std::vector<int>& stride, const std::vector<int>& pad,
        const std::vector<int>& dilation, int64_t group, const float* x_maxptr, const float* weight_maxptr,
        float* y_maxptr, bool is_ncdhw, const float* bias, const TY* branch, const Activation_t& act,
        const float* branch_maxptr = nullptr) {
    return conv3d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, d, h, w, f, std::vector<int64_t>(ksize.begin(),
            ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()), std::vector<int64_t>(pad.begin(),
            pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()), group, x_maxptr, weight_maxptr,
            y_maxptr, is_ncdhw, bias, branch, act, branch_maxptr);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv2d_with_pooling(
        Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::vector<int>& _conv_ksize, const std::vector<int>& _conv_stride,
        const std::vector<int>& _conv_pad, const std::vector<int>& _conv_dilation,
        const std::vector<int>& _pool_ksize, const std::vector<int>& _pool_stride,
        const std::vector<int>& _pool_pad, bool count_include_pad, bool is_avg, bool is_nchw,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        const float* bias, const Activation_t& act) {
    return conv2d_with_pooling<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, h, w, f,
            std::vector<int64_t>(_conv_ksize.begin(), _conv_ksize.end()),
            std::vector<int64_t>(_conv_stride.begin(), _conv_stride.end()),
            std::vector<int64_t>(_conv_pad.begin(), _conv_pad.end()),
            std::vector<int64_t>(_conv_dilation.begin(), _conv_dilation.end()),
            std::vector<int64_t>(_pool_ksize.begin(), _pool_ksize.end()),
            std::vector<int64_t>(_pool_stride.begin(), _pool_stride.end()),
            std::vector<int64_t>(_pool_pad.begin(), _pool_pad.end()),
            count_include_pad, is_avg, is_nchw, x_maxptr, w_maxptr, y_maxptr, bias, act);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline
int conv2d_transpose_fusion(Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc,
        int64_t yh, int64_t yw, int64_t xc, const std::vector<int>& _ksize,
        const std::vector<int>& _stride, const std::vector<int>& _pad,
        const std::vector<int>& _dilation, int64_t group,
        const float* y_maxptr, const float* w_maxptr, float* x_maxptr,
        const float* bias, const Activation_t& act, bool is_nchw, const float* scale = nullptr) {
    return conv2d_transpose_fusion<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, yh, yw, xc,
            std::vector<int64_t>(_ksize.begin(), _ksize.end()), std::vector<int64_t>(_stride.begin(),
            _stride.end()), std::vector<int64_t>(_pad.begin(), _pad.end()), std::vector<int64_t>(_dilation.begin(),
            _dilation.end()), group, y_maxptr, w_maxptr, x_maxptr, bias, act, is_nchw, scale);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline
int conv2d_transpose_fusion_v2(Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc,
        int64_t xh, int64_t xw, int64_t xc, const std::vector<int>& _ksize, const std::vector<int>& _stride,
        const std::vector<int>& _pad, const std::vector<int>& _dilation, int64_t group,
        const float* y_maxptr, const float* w_maxptr, float* x_maxptr,
        const float* bias, const Activation_t& act, bool is_nchw, const float* scale = nullptr) {
    return conv2d_transpose_fusion_v2<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, xh, xw, xc,
            std::vector<int64_t>(_ksize.begin(), _ksize.end()), std::vector<int64_t>(_stride.begin(),
            _stride.end()), std::vector<int64_t>(_pad.begin(), _pad.end()),
            std::vector<int64_t>(_dilation.begin(), _dilation.end()), group, y_maxptr, w_maxptr,
            x_maxptr, bias, act, is_nchw, scale);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv1d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t w, int64_t f, int64_t ksize_w,
        int64_t stride_w, const std::vector<int>& pad, int64_t dilation_w, int64_t group, const float* x_maxptr,
        const float* weight_maxptr, float* y_maxptr, bool is_nchw, const float* bias, const TY* branch,
        const Activation_t& act, const float* branch_maxptr = nullptr) {
    return conv1d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, w, f, ksize_w, stride_w,
            std::vector<int64_t>(pad.begin(), pad.end()),
            dilation_w, group, x_maxptr, weight_maxptr, y_maxptr, is_nchw, bias, branch, act, branch_maxptr);
}
template<typename TX, typename TW, typename TY, typename TGEMM>
static inline int var_conv2d_fusion(Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c,
        const VectorParam<int>& xh_lod, const VectorParam<int>& xw_lod, int64_t f, const std::vector<int>& ksize,
        const std::vector<int>& stride, const std::vector<int>& pad, const std::vector<int>& dilation,
        int64_t group, const float* x_maxptr, const float* weight_maxptr, float* y_maxptr, bool is_nchw,
        const float* bias, const Activation_t& act) {
    return var_conv2d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, xh_lod, xw_lod, f,
            std::vector<int64_t>(ksize.begin(), ksize.end()), std::vector<int64_t>(stride.begin(), stride.end()),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, x_maxptr, weight_maxptr, y_maxptr, is_nchw, bias, act);
}
template<typename TX, typename TW, typename TY, typename TGEMM>
static inline int var_conv1d_fusion(Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c,
        const VectorParam<int>& xw_lod, int64_t f, int64_t ksize_w, int64_t stride_w, const std::vector<int>& _pad,
        int64_t dilation_w, int64_t group, const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        bool is_nchw, const float* bias, const Activation_t& act) {
    return var_conv1d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, xw_lod,
            f, ksize_w, stride_w, std::vector<int64_t>(_pad.begin(), _pad.end()), dilation_w,
            group, x_maxptr, w_maxptr, y_maxptr, is_nchw, bias, act);
}
template<typename TX, typename TW, typename TY, typename TGEMM>
static inline size_t resnet_unit_fusion_get_reserve_space_size(Context* ctx,
        const std::vector<std::vector<int>>& x_shape_list, int64_t f, const std::vector<std::vector<int>>& ksize_list,
        const std::vector<std::vector<int>>& stride_list, const std::vector<int>& pad, const std::vector<int>& dilation,
        int64_t group, const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const Activation_t& act, bool is_nchw, bool has_shortcut, bool fused_add) {
    auto x_shape_list_i64 = vector2d_to_i64(x_shape_list);
    auto ksize_list_i64 = vector2d_to_i64(ksize_list);
    auto stride_list_i64 = vector2d_to_i64(stride_list);
    return resnet_unit_fusion_get_reserve_space_size<TX, TW, TY, TGEMM>(ctx,
            x_shape_list_i64, f, ksize_list_i64, stride_list_i64,
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, x_maxlist, w_maxlist, act, is_nchw, has_shortcut, fused_add);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int resnet_unit_fusion(
        Context* ctx, const std::vector<const TX*>& x_list, const std::vector<const TW*>& w_list,
        const std::vector<TY*>& conv_y_list, TY* y, const std::vector<std::vector<int>>& x_shape_list,
        int64_t f, const std::vector<std::vector<int>>& ksize_list, const std::vector<std::vector<int>>& stride_list,
        const std::vector<int>& pad, const std::vector<int>& dilation, int64_t group, float eps, float momentum,
        const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const std::vector<const float*>& scale_list, const std::vector<const float*>& bias_list,
        const std::vector<float*>& batch_mean_list, const std::vector<float*>& batch_inv_std_list,
        const std::vector<float*>& global_mean_list, const std::vector<float*>& global_var_list,
        const Activation_t& act, bool is_nchw, bool has_shortcut, bool fused_add, bool is_train,
        void* reserve_space = nullptr) {
    auto x_shape_list_i64 = vector2d_to_i64(x_shape_list);
    auto ksize_list_i64 = vector2d_to_i64(ksize_list);
    auto stride_list_i64 = vector2d_to_i64(stride_list);
    return resnet_unit_fusion<TX, TW, TY, TGEMM>(ctx,
            x_list, w_list, conv_y_list, y, x_shape_list_i64, f, ksize_list_i64,
            stride_list_i64, std::vector<int64_t>(pad.begin(), pad.end()),
            std::vector<int64_t>(dilation.begin(), dilation.end()), group, eps, momentum,
            x_maxlist, w_maxlist, scale_list, bias_list, batch_mean_list, batch_inv_std_list,
            global_mean_list, global_var_list, act, is_nchw, has_shortcut, fused_add, is_train,
            reserve_space);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int resnet_unit_grad_fusion(Context* ctx,
        const std::vector<const TX*>& x_list, const std::vector<const TW*>& w_list, const TY* dy,
        const TY* y, const std::vector<const TY*>& conv_y_list, std::vector<TX*>& dx_list,
        std::vector<TW*>& dw_list, const std::vector<std::vector<int>>& x_shape_list, int64_t f,
        const std::vector<std::vector<int>>& ksize_list, const std::vector<std::vector<int>>& stride_list,
        const std::vector<int>& pad, const std::vector<int>& dilation, int64_t group,
        const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const std::vector<const float*>& scale_list, const std::vector<const float*>& batch_mean_list,
        const std::vector<const float*>& batch_inv_std_list, std::vector<float*>& dscale_list,
        std::vector<float*>& dbias_list, const Activation_t& act, float eps,
        bool is_nchw, bool has_shortcut, bool fused_add, void* reserve_space = nullptr) {
    return resnet_unit_grad_fusion<TX, TW, TY, TGEMM>(ctx, x_list, w_list, dy, y, conv_y_list, dx_list, dw_list,
            vvi32_to_vvi64(x_shape_list), f, vvi32_to_vvi64(ksize_list), vvi32_to_vvi64(stride_list),
            std::vector<int64_t>(pad.begin(), pad.end()), std::vector<int64_t>(dilation.begin(), dilation.end()),
            group, x_maxlist, w_maxlist, scale_list, batch_mean_list, batch_inv_std_list, dscale_list, dbias_list,
            act, eps, is_nchw, has_shortcut, fused_add, reserve_space);
}

template<typename T, typename TW, typename TGEMM> static inline int lstm_train(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* w_x, const TW* w_h, const TW* b_x,
        const TW* b_h, T* y, T* last_h, T* last_c, int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len,
        const std::initializer_list<int64_t>& seq_len_tensor, bool is_reverse, const float* x_maxptr,
        const float* h_maxptr, const float* wx_maxptr, const float* wh_maxptr, T* i_f_g_o, T* c,
        const Activation_t& act, const Activation_t& recurrent_act) {
    return lstm_train<T, TW, TGEMM>(ctx, y, x, init_h, init_c, w_x, w_h, b_x, b_h, y, last_h, last_c,
        batch_size, xdim, hdim, seq_len, std::vector<int64_t>(seq_len_tensor), is_reverse, x_maxptr, h_maxptr,
        wx_maxptr, wh_maxptr, i_f_g_o, c, act, recurrent_act);
}
template<typename T, typename TW, typename TGEMM> static inline int lstm_grad(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* w_x, const TW* w_h, const T* y, const T* y_grad, const T* last_h_grad,
        const T* last_c_grad, T* x_grad, T* init_h_grad, T* init_c_grad, TW* w_x_grad, TW* w_h_grad, TW* b_x_grad,
        TW* b_h_grad, int batch_size, int xdim, int hdim, int seq_len,
        const std::initializer_list<int64_t>& seq_len_tensor, const float* x_maxptr, const float* h_maxptr,
        const float* wx_maxptr, const float* wh_maxptr, const T* i_f_g_o, const T* c) {
    return lstm_grad<T, TW, TGEMM>(ctx, x, init_h, init_c, w_x, w_h, y, y_grad, last_h_grad, last_c_grad, x_grad,
        init_h_grad, init_c_grad, w_x_grad, w_h_grad, b_x_grad, b_h_grad, batch_size, xdim, hdim, seq_len,
        std::vector<int64_t>(seq_len_tensor), x_maxptr, h_maxptr, wx_maxptr, wh_maxptr, i_f_g_o, c);
}
template<typename T, typename TW, typename TGEMM> static inline int bilstm_train(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* forward_w_x, const TW* forward_w_h,
        const TW* forward_b_x, const TW* forward_b_h, const TW* backward_w_x, const TW* backward_w_h,
        const TW* backward_b_x, const TW* backward_b_h, T* y, T* last_h, T* last_c,
        int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len,
        const std::initializer_list<int64_t>& seq_len_tensor,
        int64_t layer_num, const float* x_maxptr, const float* h_maxptr,
        const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, T* i_f_g_o, T* c, const Activation_t& act,
        const Activation_t& recurrent_act) {
    return bilstm_train<T, TW, TGEMM>(ctx, x, init_h, init_c, forward_w_x, forward_w_h, forward_b_x, forward_b_h,
        backward_w_x, backward_w_h, backward_b_x, backward_b_h, y, last_h, last_c, batch_size, xdim, hdim, seq_len,
        std::vector<int64_t>(seq_len_tensor), layer_num, x_maxptr, h_maxptr, forward_wx_maxptr, forward_wh_maxptr,
        backward_wx_maxptr, backward_wh_maxptr, i_f_g_o, c, act, recurrent_act);
}
template<typename T, typename TW, typename TGEMM> static inline int bilstm_grad(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* backward_w_x,
        const TW* backward_w_h, const T* y, const T* y_grad, const T* last_h_grad, const T* last_c_grad, T* x_grad,
        T* init_h_grad, T* init_c_grad, TW* forward_w_x_grad, TW* forward_w_h_grad,
        TW* forward_b_x_grad, TW* forward_b_h_grad, TW* backward_w_x_grad, TW* backward_w_h_grad,
        TW* backward_b_x_grad, TW* backward_b_h_grad, int batch_size, int xdim, int hdim, int seq_len,
        const std::initializer_list<int64_t>& seq_len_tensor, int layer_num,
        const float* x_maxptr, const float* h_maxptr, const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, const T* i_f_g_o, const T* c,
        const Activation_t& act, const Activation_t& recurrent_act) {
    return bilstm_grad<T, TW, TGEMM>(ctx, x, init_h, init_c, forward_w_x, forward_w_h, backward_w_x, backward_w_h, y,
        y_grad, last_h_grad, last_c_grad, x_grad, init_h_grad, init_c_grad,
        forward_w_x_grad, forward_w_h_grad, forward_b_x_grad, forward_b_h_grad,
        backward_w_x_grad, backward_w_h_grad, backward_b_x_grad, backward_b_h_grad, batch_size, xdim, hdim, seq_len,
        std::vector<int64_t>(seq_len_tensor), layer_num, x_maxptr, h_maxptr, forward_wx_maxptr, forward_wh_maxptr,
        backward_wx_maxptr, backward_wh_maxptr, i_f_g_o, c, act, recurrent_act);
}
template<typename T, typename TW, typename TGEMM> static inline int bilstm_inference(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* forward_w_x, const TW* forward_w_h,
        const TW* forward_b_x, const TW* forward_b_h, const TW* backward_w_x, const TW* backward_w_h,
        const TW* backward_b_x, const TW* backward_b_h, T* y, T* last_h, T* last_c,
        int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len,
        const std::initializer_list<int64_t>& seq_len_tensor,
        int64_t layer_num, const float* x_maxptr, const float* h_maxptr,
        const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr,
        const Activation_t& act, const Activation_t& recurrent_act) {
    return bilstm_inference<T, TW, TGEMM>(ctx, x, init_h, init_c, forward_w_x, forward_w_h, forward_b_x, forward_b_h,
        backward_w_x, backward_w_h, backward_b_x, backward_b_h, y, last_h, last_c, batch_size, xdim, hdim, seq_len,
        std::vector<int64_t>(seq_len_tensor), layer_num, x_maxptr, h_maxptr, forward_wx_maxptr, forward_wh_maxptr,
        backward_wx_maxptr, backward_wh_maxptr, act, recurrent_act);
}
template<typename T, typename TID> static inline int sorted_softmax_topk(Context* ctx, const T* x, T* y, TID* index,
        const std::initializer_list<int64_t>& xshape, int64_t axis, int64_t k) {
    return sorted_softmax_topk(ctx, x, y, index, std::vector<int64_t>(xshape), axis, k);
}
template<typename T> static inline int yolo_box_coord(Context* ctx,
        const T* x, T* y,
        const std::initializer_list<int64_t>& x_shape,
        const float* grid,
        const float* stride,
        const float* anchor_grid,
        const std::initializer_list<int64_t>& grid_shape,
        const std::initializer_list<int64_t>& stride_shape,
        const std::initializer_list<int64_t>& anchor_grid_shape,
        float offset,
        float* x_max,
        float* y_max) {
    return yolo_box_coord(ctx, x, y, std::vector<int64_t>(x_shape), grid, stride, anchor_grid,
            std::vector<int64_t>(grid_shape), std::vector<int64_t>(stride_shape),
            std::vector<int64_t>(anchor_grid_shape), offset, x_max, y_max);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv2d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        int64_t group, const float* x_maxptr, const float* weight_maxptr, float* y_maxptr,
        bool is_nchw, const float* bias, const TY* branch, const Activation_t& act,
        const float* branch_maxptr = nullptr, const float* scale = nullptr) {
    return conv2d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, h, w, f, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), std::vector<int64_t>(dilation),
            group, x_maxptr, weight_maxptr, y_maxptr, is_nchw, bias, branch, act, branch_maxptr, scale);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv3d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w, int64_t f,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        int64_t group, const float* x_maxptr, const float* weight_maxptr,
        float* y_maxptr, bool is_ncdhw, const float* bias, const TY* branch, const Activation_t& act,
        const float* branch_maxptr = nullptr) {
    return conv3d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, d, h, w, f, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), std::vector<int64_t>(dilation),
            group, x_maxptr, weight_maxptr, y_maxptr, is_ncdhw, bias, branch, act, branch_maxptr);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv2d_with_pooling(
        Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::initializer_list<int64_t>& _conv_ksize, const std::initializer_list<int64_t>& _conv_stride,
        const std::initializer_list<int64_t>& _conv_pad, const std::initializer_list<int64_t>& _conv_dilation,
        const std::initializer_list<int64_t>& _pool_ksize, const std::initializer_list<int64_t>& _pool_stride,
        const std::initializer_list<int64_t>& _pool_pad, bool count_include_pad, bool is_avg, bool is_nchw,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        const float* bias, const Activation_t& act) {
    return conv2d_with_pooling<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, h, w, f,
            std::vector<int64_t>(_conv_ksize), std::vector<int64_t>(_conv_stride),
            std::vector<int64_t>(_conv_pad), std::vector<int64_t>(_conv_dilation),
            std::vector<int64_t>(_pool_ksize), std::vector<int64_t>(_pool_stride),
            std::vector<int64_t>(_pool_pad), count_include_pad, is_avg, is_nchw, x_maxptr, w_maxptr, y_maxptr,
            bias, act);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline
int conv2d_transpose_fusion(Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc,
        int64_t yh, int64_t yw, int64_t xc, const std::initializer_list<int64_t>& _ksize,
        const std::initializer_list<int64_t>& _stride, const std::initializer_list<int64_t>& _pad,
        const std::initializer_list<int64_t>& _dilation, int64_t group,
        const float* y_maxptr, const float* w_maxptr, float* x_maxptr,
        const float* bias, const Activation_t& act, bool is_nchw, const float* scale = nullptr) {
    return conv2d_transpose_fusion<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, yh, yw, xc,
            std::vector<int64_t>(_ksize), std::vector<int64_t>(_stride),std::vector<int64_t>(_pad),
            std::vector<int64_t>(_dilation), group, y_maxptr, w_maxptr, x_maxptr, bias, act, is_nchw, scale);
}
template<typename TY, typename TW, typename TX, typename TGEMM> static inline
int conv2d_transpose_fusion_v2(Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc,
        int64_t xh, int64_t xw, int64_t xc, const std::initializer_list<int64_t>& _ksize,
        const std::initializer_list<int64_t>& _stride, const std::initializer_list<int64_t>& _pad,
        const std::initializer_list<int64_t>& _dilation, int64_t group,
        const float* y_maxptr, const float* w_maxptr, float* x_maxptr,
        const float* bias, const Activation_t& act, bool is_nchw, const float* scale = nullptr) {
    return conv2d_transpose_fusion_v2<TY, TW, TX, TGEMM>(ctx, y, weight, x, n, yc, xh, xw, xc,
            std::vector<int64_t>(_ksize), std::vector<int64_t>(_stride), std::vector<int64_t>(_pad),
            std::vector<int64_t>(_dilation), group, y_maxptr, w_maxptr, x_maxptr, bias, act, is_nchw, scale);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int conv1d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t w, int64_t f, int64_t ksize_w,
        int64_t stride_w, const std::initializer_list<int64_t>& pad, int64_t dilation_w, int64_t group,
        const float* x_maxptr, const float* weight_maxptr, float* y_maxptr, bool is_nchw, const float* bias,
        const TY* branch, const Activation_t& act, const float* branch_maxptr = nullptr) {
    return conv1d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, w, f, ksize_w, stride_w,  std::vector<int64_t>(pad),
            dilation_w, group, x_maxptr, weight_maxptr, y_maxptr, is_nchw, bias, branch, act, branch_maxptr);
}
template<typename TX, typename TW, typename TY, typename TGEMM>
static inline int var_conv2d_fusion(Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c,
        const VectorParam<int>& xh_lod, const VectorParam<int>& xw_lod, int64_t f,
        const std::initializer_list<int64_t>& ksize, const std::initializer_list<int64_t>& stride,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        int64_t group, const float* x_maxptr, const float* weight_maxptr, float* y_maxptr, bool is_nchw,
        const float* bias, const Activation_t& act) {
    return var_conv2d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, xh_lod, xw_lod, f, std::vector<int64_t>(ksize),
            std::vector<int64_t>(stride), std::vector<int64_t>(pad), std::vector<int64_t>(dilation),
            group, x_maxptr, weight_maxptr, y_maxptr, is_nchw, bias, act);
}
template<typename TX, typename TW, typename TY, typename TGEMM>
static inline int var_conv1d_fusion(Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c,
        const VectorParam<int>& xw_lod, int64_t f, int64_t ksize_w, int64_t stride_w,
        const std::initializer_list<int64_t>& _pad, int64_t dilation_w, int64_t group, const float* x_maxptr,
        const float* w_maxptr, float* y_maxptr, bool is_nchw, const float* bias, const Activation_t& act) {
    return var_conv1d_fusion<TX, TW, TY, TGEMM>(ctx, x, weight, y, n, c, xw_lod,
            f, ksize_w, stride_w, std::vector<int64_t>(_pad), dilation_w,
            group, x_maxptr, w_maxptr, y_maxptr, is_nchw, bias, act);
}
template<typename TX, typename TW, typename TY, typename TGEMM>
static inline size_t resnet_unit_fusion_get_reserve_space_size(Context* ctx,
        const std::vector<std::initializer_list<int64_t>>& x_shape_list, int64_t f,
        const std::vector<std::initializer_list<int64_t>>& ksize_list,
        const std::vector<std::initializer_list<int64_t>>& stride_list,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        int64_t group, const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const Activation_t& act, bool is_nchw, bool has_shortcut, bool fused_add) {
    return resnet_unit_fusion_get_reserve_space_size<TX, TW, TY, TGEMM>(ctx,
            x_shape_list, f, ksize_list, stride_list,
            std::vector<int64_t>(pad), std::vector<int64_t>(dilation), group, x_maxlist, w_maxlist,
            act, is_nchw, has_shortcut, fused_add);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int resnet_unit_fusion(
        Context* ctx, const std::vector<const TX*>& x_list, const std::vector<const TW*>& w_list,
        const std::vector<TY*>& conv_y_list, TY* y, const std::vector<std::initializer_list<int64_t>>& x_shape_list,
        int64_t f, const std::vector<std::initializer_list<int64_t>>& ksize_list,
        const std::vector<std::initializer_list<int64_t>>& stride_list,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation,
        int64_t group, float eps, float momentum,
        const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const std::vector<const float*>& scale_list, const std::vector<const float*>& bias_list,
        const std::vector<float*>& batch_mean_list, const std::vector<float*>& batch_inv_std_list,
        const std::vector<float*>& global_mean_list, const std::vector<float*>& global_var_list,
        const Activation_t& act, bool is_nchw, bool has_shortcut, bool fused_add, bool is_train,
        void* reserve_space = nullptr) {
    return resnet_unit_fusion<TX, TW, TY, TGEMM>(ctx,
            x_list, w_list, conv_y_list, y, x_shape_list, f, ksize_list,
            stride_list, std::vector<int64_t>(pad), std::vector<int64_t>(dilation), group, eps, momentum,
            x_maxlist, w_maxlist, scale_list, bias_list, batch_mean_list, batch_inv_std_list,
            global_mean_list, global_var_list, act, is_nchw, has_shortcut, fused_add, is_train,
            reserve_space);
}
template<typename TX, typename TW, typename TY, typename TGEMM> static inline int resnet_unit_grad_fusion(Context* ctx,
        const std::vector<const TX*>& x_list, const std::vector<const TW*>& w_list, const TY* dy,
        const TY* y, const std::vector<const TY*>& conv_y_list, std::vector<TX*>& dx_list,
        std::vector<TW*>& dw_list, const std::initializer_list<std::initializer_list<int64_t>>& x_shape_list,
        int64_t f, const std::initializer_list<std::initializer_list<int64_t>>& ksize_list,
        const std::initializer_list<std::initializer_list<int64_t>>& stride_list,
        const std::initializer_list<int64_t>& pad, const std::initializer_list<int64_t>& dilation, int64_t group,
        const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const std::vector<const float*>& scale_list, const std::vector<const float*>& batch_mean_list,
        const std::vector<const float*>& batch_inv_std_list, std::vector<float*>& dscale_list,
        std::vector<float*>& dbias_list, const Activation_t& act, float eps,
        bool is_nchw, bool has_shortcut, bool fused_add, void* reserve_space = nullptr) {
    return resnet_unit_grad_fusion<TX, TW, TY, TGEMM>(ctx, x_list, w_list, dy, y, conv_y_list, dx_list, dw_list,
            std::vector<std::vector<int64_t> >(x_shape_list.begin(), x_shape_list.end()), f,
            std::vector<std::vector<int64_t> >(ksize_list.begin(), ksize_list.end()),
            std::vector<std::vector<int64_t> >(stride_list.begin(), stride_list.end()),
            std::vector<int64_t>(pad), std::vector<int64_t>(dilation),
            group, x_maxlist, w_maxlist, scale_list, batch_mean_list, batch_inv_std_list, dscale_list, dbias_list,
            act, eps, is_nchw, has_shortcut, fused_add, reserve_space);
}
template<typename TX, typename TY, typename TZ, typename TGEMM> int attention_fusion(
        Context* ctx, const TX* x, const TY* y, const float* mask, TZ* z,
        int64_t batch_size, int64_t head_num, int64_t m, int64_t n, int64_t k,
        const std::initializer_list<int64_t>& mask_shape,
        bool x_trans, bool y_trans, float alpha, float beta,
        const float* max_x, const float* max_y, float* max_z,
        bool fuse_x_transpose0213, bool fuse_y_transpose0213, bool fuse_z_transpose0213,
        bool fuse_softmax) {
    return attention_fusion(ctx, x, y, mask, z, batch_size, head_num, m, n, k, std::vector<int64_t>(mask_shape), 
                x_trans, y_trans, alpha, beta, max_x, max_y, max_z, fuse_x_transpose0213, fuse_y_transpose0213, 
                fuse_z_transpose0213, fuse_softmax);
}

}
}
}
#endif
