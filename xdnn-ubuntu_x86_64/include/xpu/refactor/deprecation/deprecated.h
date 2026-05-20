#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_DEPRECATED_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_DEPRECATED_H

#include "xpu/refactor/util/float16.h"
#include "xpu/refactor/context/newcontext.h"
#include <vector>
namespace baidu {
namespace xpu {
namespace api {

// Deprecated@2.6
template<typename T> DLL_EXPORT int prelu(Context* ctx, const T* x, const T* alpha, T* y, int m, int t, int n);

// Deprecated@v2.5
template<typename TX, typename TY> DLL_EXPORT int cast_v2(Context* ctx, const TX* x, TY* y, int64_t len);
template<typename T> DLL_EXPORT int clip_v2(Context* ctx, const T* x, T* y, int64_t len, T min_val, T max_val);

// Deprecated@2.1.1.1
// for paddle before version 1.8 only
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int lstm_train_for_old_paddle(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* w_x, const TW* w_h, const TW* b_x, const TW* b_h, T* y, T* last_h,
        T* last_c, int batch_size, int xdim, int hdim, int seq_len, const std::vector<int>& seq_len_tensor,
        const float* x_maxptr, const float* h_maxptr, const float* wx_maxptr, const float* wh_maxptr, T* i_f_g_o, T* c);

// Deprecated@2.0.2.1
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int lstm_train(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* w_x, const TW* w_h, const TW* b_x, const TW* b_h, T* y, T* last_h, T* last_c,
        int batch_size, int xdim, int hdim, int seq_len, const std::vector<int>& seq_len_tensor, const float* x_maxptr,
        const float* h_maxptr, const float* wx_maxptr, const float* wh_maxptr, T* i_f_g_o, T* c);

// Deprecated@2.4.0.1
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int lstm_train(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* w_x, const TW* w_h, const TW* b_x, const TW* b_h, T* y, T* last_h, T* last_c,
        int batch_size, int xdim, int hdim, int seq_len, const std::vector<int>& seq_len_tensor, const float* x_maxptr,
        const float* h_maxptr, const float* wx_maxptr, const float* wh_maxptr, T* i_f_g_o, T* c,
        const Activation_t& act, const Activation_t& recurrent_act);

// Deprecated@2.0.2.1
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int bilstm_train(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* forward_b_x, const TW* forward_b_h,
        const TW* backward_w_x, const TW* backward_w_h, const TW* backward_b_x, const TW* backward_b_h, T* y, T* last_h,
        T* last_c, int batch_size, int xdim, int hdim, int seq_len, const std::vector<int>& seq_len_tensor,
        const float* x_maxptr, const float* h_maxptr, const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, T* i_f_g_o, T* c);

// Deprecated@2.4.0.1
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int bilstm_train(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* forward_b_x, const TW* forward_b_h,
        const TW* backward_w_x, const TW* backward_w_h, const TW* backward_b_x, const TW* backward_b_h, T* y, T* last_h,
        T* last_c, int batch_size, int xdim, int hdim, int seq_len, const std::vector<int>& seq_len_tensor,
        const float* x_maxptr, const float* h_maxptr, const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, T* i_f_g_o, T* c, const Activation_t& act,
        const Activation_t& recurrent_act);

// Deprecated@2.4.0.1
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int bilstm_grad(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* backward_w_x, const TW* backward_w_h,
        const T* y, const T* y_grad, const T* last_h_grad, const T* last_c_grad, T* x_grad, T* init_h_grad,
        T* init_c_grad, TW* forward_w_x_grad, TW* forward_w_h_grad, TW* forward_b_x_grad, TW* forward_b_h_grad,
        TW* backward_w_x_grad, TW* backward_w_h_grad, TW* backward_b_x_grad, TW* backward_b_h_grad, int batch_size,
        int xdim, int hdim, int seq_len, const std::vector<int>& seq_len_tensor, const float* x_maxptr,
        const float* h_maxptr, const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, const T* i_f_g_o, const T* c);

// Deprecated@2.4.0.1
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int bilstm_inference(Context* ctx, const T* x,
        const T* init_h,
        const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* forward_b_x, const TW* forward_b_h,
        const TW* backward_w_x, const TW* backward_w_h, const TW* backward_b_x, const TW* backward_b_h, T* y, T* last_h,
        T* last_c, int batch_size, int xdim, int hdim, int seq_len, const std::vector<int>& seq_len_tensor,
        const float* x_maxptr, const float* h_maxptr, const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, const Activation_t& act,
        const Activation_t& recurrent_act);

//**DEPRECATED**@paddle-lite-release v2.8
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int squeeze_excitation_block(Context* ctx,
        const T* x, const TW* weight1, const TW* weight2, T* y, int n, int c, int h, int w, int r,
        const float* w1_maxptr, const float* w2_maxptr, const T* branch,
        const Activation_t& excitation_act1, const Activation_t& excitation_act2, const Activation_t& block_act);

// Deprecated@2.1.1.1
template<typename T> DLL_EXPORT int masked_select_grad(Context* ctx, const T* y, const bool* mask, T* x,
        const std::vector<int>& x_shape, const std::vector<int>& mask_shape);

// Deprecated@2.1.1.1
template<typename T, typename TID> DLL_EXPORT int sequence_topk_avg_pooling(Context* ctx, const T* x, T* y, TID* y_pos,
        int channel_num,
        const VectorParam<TID>& in_lod,
        const VectorParam<TID>& row_lod,
        const VectorParam<TID>& col_lod,
        const VectorParam<int>& topks);

// Deprecated@2.1.1.1
template<typename T> DLL_EXPORT int masked_select(Context* ctx, const T* x, const bool* mask, T* y,
        const std::vector<int>& x_shape,
        const std::vector<int>& mask_shape);

// Deprecated@2.1.1.1
template <typename T, typename TID> DLL_EXPORT int sequence_reverse(Context* ctx, const T* x, const TID* lod, T* y,
        int batch, int dim);

template<typename T, typename TID> DLL_EXPORT int sequence_max_pool(Context* ctx, const T* x,
        const TID* lod, T* y,
        int batch, int dim, float pad_value, TID* max_index);
template<typename T, typename TID> DLL_EXPORT int sequence_first_pool(Context* ctx, const T* x, const TID* lod, T* y,
        int batch, int dim, float pad_value);
template<typename T, typename TID> DLL_EXPORT int sequence_last_pool(Context* ctx, const T* x, const TID* lod, T* y,
        int batch, int dim, float pad_value);
template<typename T, typename TID> DLL_EXPORT int sequence_sum_pool(Context* ctx, const T* x, const TID* lod, T* y,
        int batch, int dim, float pad_value);

// Deprecated@2.2.19.1
template<typename T, typename TID>
DLL_EXPORT int multiclass_nms2(Context* ctx, const T* bboxes, const T* scores,
        T* out, TID* out_index, std::vector<size_t>* accumlated_det_num,
        int n, int b, int class_num, int out_dim, int nms_topk,
        float score_threshold, int keep_top_k, float nms_threshold,
        int background_label, bool normalized, float nms_eta, bool return_index);
}
}
}
#endif
