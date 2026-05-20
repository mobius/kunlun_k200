#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_FUSION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_FUSION_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include "xpu/refactor/deprecation/deprecated.h"
#include "xpu/refactor/attention.h"
#include "xpu/refactor/fusion_cv.h"

#ifdef _MSC_VER
#include <algorithm>
#endif

namespace baidu {
namespace xpu {
namespace api {

template<typename T> DLL_EXPORT int findmax_copy_fusion(Context* ctx, const T* x, float* maxptr, T* y, int64_t len);
template<typename T> DLL_EXPORT int findmax2d_copy_fusion(Context* ctx, const T* x, float* maxptr, T* y,
        int64_t m, int64_t ldx, int64_t ldy, int64_t n, const float* prev_maxptr = nullptr);
template <typename T, typename TID>
DLL_EXPORT int findmax2d_copycache_fusion(Context* ctx, const T* x, T* y, const VectorParam<TID>& x_lods,
        const VectorParam<TID>& y_offset, int batch, int cache_len, int head_num, int head_dim, int ldx,
        float* max_ptrs, float* maxptr, std::string cache_kv_layout = "BLHD");

template<typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int fc_fusion(Context* ctx, const TX* x, const TW* w, TY* y, int64_t m, int64_t n, int64_t k,
        bool x_trans, bool w_trans, const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        int64_t ldx, int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias,
        const Activation_t& act, const float*  scale = nullptr, const float* scale_x = nullptr, const float* scale_w = nullptr);

template<typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int fc_fusion_stable(Context* ctx, const TX* x, const TW* w, TY* y, int64_t m, int64_t n, int64_t k,
        bool x_trans, bool w_trans, const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        int64_t ldx, int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias,
        const Activation_t& act, const float* scale = nullptr, const int64_t m_config = -1);

template<typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int fc_fusion_scale(Context* ctx, const TX* x, const TW* w, TY* y, int64_t m, int64_t n, int64_t k, bool x_trans,
        bool w_trans, const float* x_maxptr, const float* w_maxptr, float* y_maxptr, int64_t ldx,
        int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias,
        const Activation_t& act, const float* scale,
        const float* inv_scale_x, const float* inv_scale_w, const float* history_max_y,
        float* inv_scale_y, float* max_y);

/**
 * @brief fc fusion all_reduce op
 *  requirements:
 *                1. libbkcl.so verison is 1.20.1 or 1.20.2
 *                2. export BKCL_CLUSTERS_PER_RING=2
 * @param[in] bkcl_ctx: the BKCLContext_t of xccl init_rank
 */
template<typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int fc_fusion_all_reduce(Context* ctx, const TX* x, const TW* w, TY* y, int64_t m, int64_t n, int64_t k,
        bool x_trans, bool w_trans, const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        int64_t ldx, int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias,
        const Activation_t& act, const float*  scale = nullptr, void* bkcl_ctx = nullptr);

template<typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int fc_ny_fusion(Context* ctx, const TX* x, const TW* w, std::vector<TY*> &y_list, int64_t m,
        int64_t n, int64_t k, bool x_trans, bool w_trans, const float* x_maxptr, const float* w_maxptr,
        float* y_maxptr, int64_t ldx, int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias,
        const Activation_t& act, const float* scale = nullptr);

template<typename TX, typename TW, typename TY, typename TGEMM, typename TID = int> DLL_EXPORT int fc_fusion_pb(Context* ctx,
        const TX* x, const TW* w, TY* y, int64_t m, int64_t n, int64_t k, bool x_trans, bool w_trans,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr, int64_t ldx, int64_t ldw,
        int64_t ldy, float alpha, float beta, const float* bias, const Activation_t& act,
        int64_t batch_size, std::vector<TID>& lod);

template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int fc_fusion_pc(Context* ctx,
        const TX* x, const TW* w, TY* y, int64_t m, int64_t n, int64_t k, bool x_trans, bool w_trans,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        int64_t ldx, int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias,
        const float* scale, const Activation_t& act);

template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int fc_fusion_multi_y(Context* ctx,
        const TX* x, const TW* w, std::vector<TY*>& y_list, int64_t m, int64_t n, int64_t k,
        bool x_trans, bool w_trans, const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        int64_t ldx, int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias, const Activation_t& act,
        const float* scale = nullptr);

template<typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int fc_fusion_branch_mul(Context* ctx, const TX* x, const TW* w, TY* y, TY* noact_y, int64_t m, int64_t n, int64_t k, bool x_trans,
        bool w_trans, const float* x_maxptr, const float* w_maxptr, float* y_maxptr, float* noact_y_maxptr,
        int64_t ldx, int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias,
        const TY* branch_mul, const float* branch_mul_maxptr,
        const Activation_t& act, const float* scale);

template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int fc_limit_softmax_fusion(Context* ctx,
        const TX* x, const TW* w, TY* y, int64_t m, int64_t n, int64_t k, bool x_trans,
        bool w_trans, const float* x_maxptr, const float* w_maxptr, int64_t ldx, int64_t ldw, int64_t ldy,
        float alpha, int64_t sqrt_cnt = 2);

// only support TX=float16, TW=int8, TY=float16
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int gpt_fc_fusion(Context* ctx,
        const TX* x, const TW* w, TY* y,
        int64_t m, int64_t n, int64_t k, bool x_trans, bool w_trans,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        int64_t ldx, int64_t ldw, int64_t ldy, float alpha, float beta, const float* bias,
        const Activation_t& act, const float* sacle = nullptr);

template<typename TX, typename TY, typename TZ, typename TID = float> DLL_EXPORT int slice_x_mul_act_fusion(Context* ctx,
        const TX* x, const TY* y, TZ* z, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape,
        const std::vector<int64_t>& startshape, const std::vector<int64_t>& endshape, const float* max_x,
        const float* max_y, float* max_z, const Activation_t& act);

template<typename TX, typename TY, typename TZ, typename TID = float> DLL_EXPORT int slice_x_rotate_half_mul_act_fusion(Context* ctx,
        const TX* x, const TY* y, TZ* z, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape,
        const std::vector<int64_t>& startshape, const std::vector<int64_t>& endshape, const float* max_x,
        const float* max_y, float* max_z, const Activation_t& act);

template<typename TX, typename TY, typename TZ, typename TID = float> DLL_EXPORT int slice_x_llama_fusion(Context* ctx,
        const TX* x, const TY* y1, const TY* y2, TZ* z, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape,
        const std::vector<int64_t>& startshape, const std::vector<int64_t>& endshape, const float* max_x,
        const float* max_y1, const float* max_y2, float* max_z, const Activation_t& act);

template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv2d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const float* x_maxptr, const float* weight_maxptr,
        float* y_maxptr, bool is_nchw, const float* bias, const TY* branch, const Activation_t& act,
        const float* branch_maxptr = nullptr, const float* scale = nullptr, int64_t ld_out_f = -1);

template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv3d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w, int64_t f,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const float* x_maxptr, const float* weight_maxptr,
        float* y_maxptr, bool is_ncdhw, const float* bias, const TY* branch, const Activation_t& act,
        const float* branch_maxptr = nullptr);

template<typename TY, typename TW, typename TX, typename TGEMM> DLL_EXPORT
int conv2d_transpose_fusion(Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc,
        int64_t yh, int64_t yw, int64_t xc, const std::vector<int64_t>& _ksize,
        const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad,
        const std::vector<int64_t>& _dilation, int64_t group,
        const float* y_maxptr, const float* w_maxptr, float* x_maxptr,
        const float* bias, const Activation_t& act, bool is_nchw, const float* scale = nullptr);

template<typename TY, typename TW, typename TX, typename TGEMM> DLL_EXPORT
int conv2d_transpose_fusion_v2(Context* ctx, const TY* y, const TW* weight, TX* x, int64_t n, int64_t yc,
        int64_t xh, int64_t xw, int64_t xc, const std::vector<int64_t>& _ksize, const std::vector<int64_t>& _stride,
        const std::vector<int64_t>& _pad, const std::vector<int64_t>& _dilation, int64_t group,
        const float* y_maxptr, const float* w_maxptr, float* x_maxptr,
        const float* bias, const Activation_t& act, bool is_nchw, const float* scale = nullptr);

template<typename T, typename TID = int> DLL_EXPORT int sequence_sum_pool_cvm_concat(Context* ctx, std::vector<const T*>& x, T* y,
        std::vector<TID>& lods, int64_t batch, int64_t dim, float pad_value, int64_t slot_num, bool use_cvm);

template<typename T, typename TID = int> DLL_EXPORT int sequence_sum_pool_cvm_concat_grad(Context* ctx, T* dy, T* cvm,
        std::vector<const T*>& dx, std::vector<TID>& sequence,
        int64_t item_width, int64_t batch_size, int64_t slot_num, int64_t dy_offset, bool use_cvm);

template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv2d_with_pooling(
        Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t f,
        const std::vector<int64_t>& _conv_ksize, const std::vector<int64_t>& _conv_stride,
        const std::vector<int64_t>& _conv_pad, const std::vector<int64_t>& _conv_dilation,
        const std::vector<int64_t>& _pool_ksize, const std::vector<int64_t>& _pool_stride,
        const std::vector<int64_t>& _pool_pad, bool count_include_pad, bool is_avg, bool is_nchw,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        const float* bias, const Activation_t& act);

template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int conv1d_fusion(Context* ctx,
        const TX* x, const TW* weight, TY* y, int64_t n, int64_t c, int64_t w, int64_t f, int64_t ksize_w, int64_t stride_w,
        const std::vector<int64_t>& pad, int64_t dilation_w, int64_t group, const float* x_maxptr, const float* weight_maxptr,
        float* y_maxptr, bool is_nchw, const float* bias, const TY* branch, const Activation_t& act,
        const float* branch_maxptr = nullptr);

template<typename TX, typename TW, typename TY, typename TGEMM, typename TID = int>
DLL_EXPORT int var_conv2d_fusion(Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c,
        const VectorParam<TID>& xh_lod, const VectorParam<TID>& xw_lod, int64_t f, const std::vector<int64_t>& ksize,
        const std::vector<int64_t>& stride, const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation,
        int64_t group, const float* x_maxptr, const float* weight_maxptr, float* y_maxptr, bool is_nchw,
        const float* bias, const Activation_t& act);

template<typename TX, typename TW, typename TY, typename TGEMM, typename TID = int>
DLL_EXPORT int var_conv1d_fusion(Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c,
        const VectorParam<TID>& xw_lod, int64_t f, int64_t ksize_w, int64_t stride_w, const std::vector<int64_t>& _pad,
        int64_t dilation_w, int64_t group, const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        bool is_nchw, const float* bias, const Activation_t& act);

// lstm&bilstm fusion
template<typename T, typename TW, typename TGEMM> DLL_EXPORT int lstm_train(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* w_x, const TW* w_h, const TW* b_x, const TW* b_h, T* y, T* last_h, T* last_c,
        int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len, const std::vector<int64_t>& seq_len_tensor,
        bool is_reverse, const float* x_maxptr, const float* h_maxptr, const float* wx_maxptr, const float* wh_maxptr,
        T* i_f_g_o, T* c, const Activation_t& act, const Activation_t& recurrent_act);

template<typename T, typename TW, typename TGEMM> DLL_EXPORT int lstm_grad(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* w_x, const TW* w_h, const T* y, const T* y_grad, const T* last_h_grad,
        const T* last_c_grad, T* x_grad, T* init_h_grad, T* init_c_grad, TW* w_x_grad, TW* w_h_grad, TW* b_x_grad,
        TW* b_h_grad, int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len,
        const std::vector<int64_t>& seq_len_tensor, const float* x_maxptr, const float* h_maxptr,
        const float* wx_maxptr, const float* wh_maxptr, const T* i_f_g_o, const T* c);

DLL_EXPORT int lstm_inference(Context* ctx,
        const float* x,             // embedding_1.tmp_0 [batch_size, seq_len, xdim]
        bool x_need_transpose,      // true
        const float* init_h,        // nullptr
        const float* init_c,        // nullptr
        const int64_t* x_seq_len,   // seq_len [batch_size]
        const float* wx,            // lstm_cell_0.w_0 [4 * hdim, xdim]
        const float* wx_maxptr,     // nullptr
        const float* wh,            // lstm_cell_0.w_1 [4 * hdim, hdim]
        const float* wh_maxptr,     // nullptr
        const float* bx,            // lstm_cell_0.b_0 [4 * hdim]
        const float* bh,            // lstm_cell_0.b_1 [4 * hdim]
        float* last_h,              // lstm_0.tmp_1 [1, batch_size, hdim]
        int64_t batch_size,
        int64_t seq_len,
        int64_t xdim,
        int64_t hdim);

template<typename T, typename TW, typename TGEMM = int16_t> DLL_EXPORT int lstm_inference(Context* ctx,
        int64_t seq_len, int64_t batch_size, int64_t xdim, int64_t hdim, bool is_reverse,
        const T* x, const T* init_h, const T* init_c,
        const int64_t* x_seq_len, // can be nullptr
        const TW* wx, const float* wx_maxptr,
        const TW* wh, const float* wh_maxptr,
        const float* bx, const float* bh,
        T* y, T* last_h, T* last_c);

template<typename T, typename TW, typename TGEMM> DLL_EXPORT int bilstm_inference(Context* ctx, const T* x,
        const T* init_h, const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* forward_b_x,
        const TW* forward_b_h, const TW* backward_w_x, const TW* backward_w_h, const TW* backward_b_x,
        const TW* backward_b_h, T* y, T* last_h, T* last_c, int64_t batch_size, int64_t xdim, int64_t hdim,
        int64_t seq_len, const std::vector<int64_t>& seq_len_tensor, int64_t layer_num,
        const float* x_maxptr, const float* h_maxptr, const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr,
        const Activation_t& act, const Activation_t& recurrent_act);

template<typename T, typename TW, typename TGEMM> DLL_EXPORT int bilstm_train(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* forward_b_x, const TW* forward_b_h,
        const TW* backward_w_x, const TW* backward_w_h, const TW* backward_b_x, const TW* backward_b_h, T* y, T* last_h,
        T* last_c, int64_t batch_size, int64_t xdim, int64_t hdim, int64_t seq_len,
        const std::vector<int64_t>& seq_len_tensor, int64_t layer_num, const float* x_maxptr, const float* h_maxptr,
        const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, T* i_f_g_o, T* c,
        const Activation_t& act, const Activation_t& recurrent_act);

template<typename T, typename TW, typename TGEMM> DLL_EXPORT int bilstm_grad(Context* ctx, const T* x, const T* init_h,
        const T* init_c, const TW* forward_w_x, const TW* forward_w_h, const TW* backward_w_x, const TW* backward_w_h,
        const T* y, const T* y_grad, const T* last_h_grad, const T* last_c_grad, T* x_grad, T* init_h_grad,
        T* init_c_grad, TW* forward_w_x_grad, TW* forward_w_h_grad, TW* forward_b_x_grad, TW* forward_b_h_grad,
        TW* backward_w_x_grad, TW* backward_w_h_grad, TW* backward_b_x_grad, TW* backward_b_h_grad, int64_t batch_size,
        int64_t xdim, int64_t hdim, int64_t seq_len, const std::vector<int64_t>& seq_len_tensor, int64_t layer_num,
        const float* x_maxptr, const float* h_maxptr, const float* forward_wx_maxptr, const float* forward_wh_maxptr,
        const float* backward_wx_maxptr, const float* backward_wh_maxptr, const T* i_f_g_o, const T* c,
        const Activation_t& act, const Activation_t& recurrent_act);

template<typename T, typename TGEMM> DLL_EXPORT int lstmp_unit(api::Context* ctx, const int64_t hdim, const T* cifo, const T* wc_ifo,
            T* cp, T* m_out, const int64_t rnn_batch, const float scale_z);

// GRU
template<typename TX, typename TW, typename TY, typename TGEMM> DLL_EXPORT int gru_cell(Context* ctx,
        const TX* x, const TX* hidden_prev/*can be nullptr*/, const TW* weight_ih/*[3 * hdim, xdim]*/, const TW* weight_hh,
        TY* y,
        int64_t batch, int64_t seq_len, int64_t xdim, int64_t hdim,
        const float* x_maxptr, const float* hidden_prev_maxptr/*can be nullptr*/, const float* w_ih_maxptr,
        const float* w_hh_maxptr/*float[8]*/, float* y_maxptr/*can be nullptr*/,
        const float* bias_ih/*can be nullptr*/, const float* bias_hh/*can be nullptr*/,
        const Activation_t& act = Activation_t::TANH,
        const Activation_t& gate_act = Activation_t::SIGMOID,
        bool origin_mode = false,
        bool is_reverse = false,
        bool reset_after = false);

template <typename T, typename TW, typename TID, typename TGEMM> DLL_EXPORT
int grnn_cell(Context* ctx, const T* x, const T* h_prev, const std::vector<const TW*>& weight_x,
        const std::vector<const TW*>& weight_h, T* y, int64_t cap_e, int64_t cap_h, const VectorParam<TID>& lod,
        const float* x_maxptr, const float* h_prev_maxptr, const std::vector<const float*>& weight_x_max,
        const std::vector<const float*>& weight_h_max, float* y_maxptr);

template <typename T, typename TW, typename TID> DLL_EXPORT int match_matrix_tensor(Context* ctx, const T* x,
        const T* y, const TW* w, T* z, int64_t dim_in, int64_t dim_w, bool w_trans, VectorParam<TID> x_lod,
        VectorParam<TID> y_lod, const float* max_x, const float* max_y, const float* max_w,
        const Activation_t& act, T* x_dot_w = nullptr);

template<typename T, typename TID> DLL_EXPORT int sorted_softmax_topk(Context* ctx, const T* x, T* y, TID* index,
        const std::vector<int64_t>& xshape, int64_t axis, int64_t k);

template<typename T> DLL_EXPORT int spacial_attention_pool2d(Context* ctx,
        const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, bool is_nchw);

template<typename T, typename TW, typename TGEMM> DLL_EXPORT int squeeze_excitation_block(Context* ctx,
        const T* x, const TW* weight1, const TW* weight2, T* y, int64_t n, int64_t c, int64_t h, int64_t w, int64_t r,
        const float* w1_maxptr, const float* w2_maxptr, const float* bias1, const float* bias2, const T* branch,
        const Activation_t& excitation_act1, const Activation_t& excitation_act2, const Activation_t& block_act,
        bool fusion_self_add = false);

template <typename T>
DLL_EXPORT int batch_norm_fusion(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, float eps,
                                 float momentum, const float* scale, const float* bias, float* batch_mean,
                                 float* batch_inv_std, float* global_mean, float* global_var, bool is_nchw,
                                 const T* branch, const Activation_t& act, void* reserve_space,
                                 int64_t reserve_space_size, uint32_t* bitmask = nullptr, float* max = nullptr,
                                 bool split = false, bool use_frozen_bn = false, bool unbiased_global_var = false);

template<typename T> DLL_EXPORT int batch_norm_fusion_gen_bitmask(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h,
        int64_t w,
        float eps, float momentum, const float* scale, const float* bias,
        float* batch_mean, float* batch_inv_std, float* global_mean, float* global_var, bool is_nchw,
        const T* branch, const Activation_t& act, void* reserve_space, int64_t reserve_space_size, uint32_t* bitmask, float* max=nullptr);

template<typename T> DLL_EXPORT int batch_norm_grad_fusion(Context* ctx, const T* x, const T* y, const T* dy, T* dx,
        int64_t n, int64_t c, int64_t h, int64_t w, const float* scale, const float* batch_mean,
        const float* batch_inv_std, float* dscale, float* dbias, bool is_nchw,
        T* dbranch, const Activation_t& act, const void* reserve_space, int64_t reserve_space_size,
        const uint32_t* bitmask = nullptr, float* max_gm = nullptr);

size_t batch_norm_fusion_get_reserve_space_size(Context* ctx, int64_t n, int64_t c, int64_t h, int64_t w, bool is_nchw,
                                                bool enable_bitmask, bool enable_findmax);

std::vector<void*> batch_norm_fusion_get_reserve_ptrs(Context* ctx, int64_t n, int64_t c, int64_t h, int64_t w, void* reserve_space,
                                                      bool is_nchw, bool enable_bitmask, bool enable_findmax);

template<typename TX, typename TY, typename TZ> DLL_EXPORT int add_activation_fusion(Context* ctx, const TX* x,
        const TY* y, TZ* z, int64_t len, const float* max_x, const float* max_y, float* max_z, const Activation_t& act, int write_len = -1, int stride = -1);

template<typename TX, typename TY, typename TZ, typename TID = float> DLL_EXPORT int add_activation_fusion(Context* ctx, const TX* x,
        const TY* y, TZ* z, const std::vector<int64_t>& x_shape, const std::vector<int64_t>& y_shape, const float* max_x,
        const float* max_y, float* max_z, const Activation_t& act);

template<typename TX, typename TY, typename TZ> DLL_EXPORT int mul_activation_fusion(Context* ctx, const TX* x,
        const TY* y, TZ* z, int64_t len, const float* max_x, const float* max_y, float* max_z, const Activation_t& act);

template<typename TX, typename TY, typename TZ, typename TID = float> DLL_EXPORT int mul_activation_fusion(Context* ctx, const TX* x,
        const TY* y, TZ* z, const std::vector<int64_t>& x_shape, const std::vector<int64_t>& y_shape, const float* max_x,
        const float* max_y, float* max_z, const Activation_t& act);

template<typename T> DLL_EXPORT int add_activation_grad_fusion(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz,
        const T* inter_out, T* dinter_out, T* dy, T* dx, const Activation_t& act, int64_t len);

template <typename T>
DLL_EXPORT int add_layer_norm_fusion(Context* ctx,
                                     const T* x,
                                     const T* y,
                                     T* z,
                                     int64_t m,
                                     int64_t n,
                                     float eps,
                                     const float* scale,
                                     const float* bias,
                                     float* mean = nullptr,
                                     float* var = nullptr,
                                     T* z_add = nullptr,
                                     float* z_max = nullptr,
                                     const Activation_t& act = Activation_t::LINEAR);

template <typename T>
DLL_EXPORT int add_layer_norm_fusion_stable(Context* ctx,
                                            const T* x,
                                            const T* y,
                                            T* z,
                                            int64_t m,
                                            int64_t n,
                                            float eps,
                                            const float* scale,
                                            const float* bias,
                                            float* mean = nullptr,
                                            float* var = nullptr,
                                            T* z_add = nullptr,
                                            float* z_max = nullptr,
                                            const Activation_t& act = Activation_t::LINEAR);

template<typename T> DLL_EXPORT int mul_add_layer_norm_fusion(Context* ctx, const T* x,
        const T* y, T* z, const float16* w, int64_t m, int64_t n,
        float eps, const float* scale, const float* bias,
        float* mean = nullptr, float* var = nullptr);

template<typename T> DLL_EXPORT int add_rms_layer_norm_fusion(Context* ctx, const T* x,
        const T* y, T* z, int64_t m, int64_t n, float eps, const float* scale, const float* bias,
        float* var = nullptr, T* z_add = nullptr, float* z_max = nullptr);
DLL_EXPORT int add_layer_norm_quant_fusion(Context* ctx, const float16* x,
        const float16* y, int8_t* z, int64_t m, int64_t n, float eps, const float* scale, const float* bias,
        float* mean = nullptr, float* var = nullptr, float16* z_add = nullptr, float16* fp_result = nullptr, float* maxptr = nullptr);

template<typename T> DLL_EXPORT int add_layer_norm_grad_fusion(Context* ctx, const T* x, const T* y, const T* dz,
        T* dx, T* dy, int64_t m, int64_t n, float eps, const float* scale, const float* mean, const float* var, float* dscale, float* dbias);

template<typename T, typename TID = int> DLL_EXPORT int slice_add_layer_norm_fusion(Context* ctx, const T* x,
        const T* y, T* z, int64_t n, const VectorParam<TID>& mseqs, float eps, const float* scale, const float* bias);

template<typename T> DLL_EXPORT int greater_filter_fusion(Context* ctx, const T* x, T* y, float scale, int64_t len);

template<typename TT, typename TY, typename TID> DLL_EXPORT int multi_embedding_fusion(Context* ctx,
        const std::vector<const TT*>& table_list, TY* y, const std::vector<VectorParam<TID>>& idx_list,
        const std::vector<TID>& table_len_list, int64_t dim, const std::vector<float>& scale_list,
        const std::vector<TID>& padding_idx_list);

template<typename T> DLL_EXPORT int yolo_box_coord(Context* ctx,
        const T* x, T* y,
        const std::vector<int64_t>& x_shape,
        const float* grid,
        const float* stride,
        const float* anchor_grid,
        const std::vector<int64_t>& grid_shape,
        const std::vector<int64_t>& stride_shape,
        const std::vector<int64_t>& anchor_grid_shape,
        float offset,
        float* x_max,
        float* y_max);


// z = option_trans0213(option_softmax(alpha * batch_matmul(option_trans0213(x), option_trans0213(y)) + mask) * beta)
// the meaning of mask is same as that in qk_attention
template<typename TX, typename TY, typename TZ, typename TGEMM> DLL_EXPORT int attention_fusion(
        Context* ctx, const TX* x, const TY* y, const float* mask, TZ* z,
        int64_t batch_size, int64_t head_num, int64_t m, int64_t n, int64_t k,
        const std::vector<int64_t>& mask_shape,
        bool x_trans, bool y_trans, float alpha, float beta,
        const float* max_x, const float* max_y, float* max_z,
        bool fuse_x_transpose0213, bool fuse_y_transpose0213, bool fuse_z_transpose0213,
        bool fuse_softmax);

template<typename TQ, typename TP, typename TQP, typename TGEMM> DLL_EXPORT int q_pos_attention
        (Context* ctx, const TQ* q, const TP* pos, TQP* q_pos,
        int64_t batch_size, int64_t seqlen, int64_t head_num, int64_t head_dim, float alpha,
        const float* q_maxptr, const float* p_maxptr, float* qp_maxptr);

template<typename TQ, typename TK, typename TQK, typename TGEMM, typename TZ = float, typename TID = int, typename TEW = float>
DLL_EXPORT int qk_attention(
        Context* ctx, const TQ* q, const TK* k, TQK* qk, const float* max_q, const float* max_k, float* max_qk,
        const NewBaseAttnParam<TID>& att, const TZ* mask = nullptr);

template<typename TQK, typename TV, typename TQKV, typename TGEMM, typename TZ = float, typename TID = int> DLL_EXPORT int qk_v_attention(
        Context* ctx, const TQK* qk, const TV* v, TQKV* qkv,
        const float* max_qk, const float* max_v, float* max_qkv, const NewBaseAttnParam<TID>& att, const TZ* gate = nullptr,
        const TQKV* shift = nullptr, const TQKV* smooth = nullptr);

template<typename TQ, typename TK, typename TV, typename TQKV, typename TGEMM, typename TZ = float,
         typename TID = int, typename TEW = float, typename TGEMM_CTX = float>
DLL_EXPORT int qkv_attention(
        Context* ctx, const TQ* q, const TK* k, const TV* v, TQKV* qkv,
        const float* max_q, const float* max_k, const float* max_v, float* max_qkv, const NewBaseAttnParam<TID>& basep,
        const TZ* mask = nullptr, const float* max_qk = nullptr, TV* v_padding = nullptr, const TZ* gate = nullptr);

template<typename TQ, typename TK, typename TV, typename TQKV, typename TGEMM, typename TZ = float, typename TID = int, typename TGEMM_CTX = int16_t> DLL_EXPORT int
qkv_attention_gpt(
        Context* ctx, const TQ* q, const TK* k, const TV* v, TQKV* qkv,
        const float* max_q, const float* max_k, const float* max_v, float* max_qkv, const NewBaseAttnParam<TID>& basep,
        const TZ* mask = nullptr, const TZ* gate = nullptr, const TQKV* shift = nullptr, const TQKV* smooth = nullptr);

template<typename TQ, typename TK, typename TV, typename TQKV, typename TGEMM, typename TZ, typename TID = int>
DLL_EXPORT int qkv_paged_attention(
        Context* ctx, const TQ* q, const TK* k, const TV* v, const TID* block_tables, TQKV* qkv,
        const float* max_q, const float* max_k, const float* max_v, float* max_qkv,
        const NewBaseAttnParam<TID>& basep, const PageAttnParam<TID>& pagep,
        const TQ* k_cur = nullptr, const TQ* v_cur = nullptr,
        const float* scale_k = nullptr, const float* scale_v = nullptr,
        const TZ* mask = nullptr, const TZ* gate = nullptr);

template<typename TX, typename TY, typename TID> DLL_EXPORT int gather_cached_kv(Context* ctx, const TX* x_cached,
        TY* y, const TID* block_table, const api::VectorParam<int32_t>& kv_seq_lod, const api::VectorParam<int32_t>&  real_batch, int64_t head_num,
        int64_t head_dim, int64_t block_size, int64_t num_blocks, int64_t max_batch_size, int64_t max_num_blocks_per_seq, int64_t max_cache_seqlen, const std::string& page_layout,
        const std::string& qkv_layout, const TY* x_cur = nullptr, const float* dequant_scale = nullptr);

template<typename T, typename TW, typename TGEMM, typename TID = int, typename TEW = float> DLL_EXPORT int transformer_encoder(
        Context* ctx, const T* x, const std::vector<const TW*>& w_list, T* y,
        const std::vector<const float*>& max_xy_list, const std::vector<const float*>& max_w_list,
        const std::vector<const float*>& bias_list, const std::vector<const float*>& ln_scale_list,
        const std::vector<const float*>& ln_bias_list, const NewQKVAttnParam<TID>& att, const float* mask = nullptr,
        const std::vector<const float*>& relative_position_bias = {});

template <typename T, typename TW, typename TGEMM> DLL_EXPORT int conv_knrm(
        Context* ctx, const T* query, const T* doc, const T* mask, T* y,
        const float* query_max, const float* doc_max, float* y_max,
        std::vector<const TW*> conv_weight_list, std::vector<const float*> conv_bias_list,
        std::vector<const float*> conv_maxw_list, std::vector<const TW*> fc_weight_list,
        std::vector<const float*> fc_bias_list, std::vector<const float*> fc_maxw_list,
        int64_t batch, int64_t hidden_dim, int64_t query_len, int64_t doc_len, int64_t conv_num, int64_t conv_filters,
        int64_t out_dim, const VectorParam<float>& rbf_mu, const VectorParam<float>& rbf_sigma,
        const Activation_t& conv_act, const Activation_t& fc_act);

struct DLL_EXPORT FTUnifiedDecodingParam {
    const char* decoding_strategy{};
    int64_t beam_size;
    int64_t topk;
    float topp;
    int64_t n_head;
    int64_t size_per_head;
    int64_t num_layer;
    int64_t bos_id;
    int64_t eos_id;
    long max_len;
    float beam_search_diversity_rate;
    int64_t unk_id;
    int64_t mask_id;
    float temperature;
    float len_penalty;
    bool normalize_before;
    bool pos_bias;
    Activation_t hidden_act{Activation_t::RELU};
    bool rel_len;
    bool early_stopping;
    int64_t min_length;
    int64_t vocab_size;
};

template<typename T, typename TW, typename TGEMM, typename TID = int>
DLL_EXPORT int fasttransformer_unified_decoding(
        Context* ctx, const TID* input_ids, const T* attn_mask, const TID* mem_seq_len, const TID* type_id,
        const TID* decoder_type_id, const T* logits_mask, const T* word_embedding,
        std::vector<const float*>& self_ln_weight, std::vector<const float*>& self_ln_bias,
        std::vector<const TW*>& self_q_weight, std::vector<const float*>& self_q_maxquant,
        std::vector<const float*>& self_q_bias,
        std::vector<const TW*>& self_k_weight, std::vector<const float*>& self_k_maxquant,
        std::vector<const float*>& self_k_bias,
        std::vector<const TW*>& self_v_weight, std::vector<const float*>& self_v_maxquant,
        std::vector<const float*>& self_v_bias,
        std::vector<const TW*>& self_out_weight, std::vector<const float*>& self_out_maxquant,
        std::vector<const float*>& self_out_bias,
        std::vector<const float*>& ffn_ln_weight, std::vector<const float*>& ffn_ln_bias,
        std::vector<const TW*>& ffn_inter_weight, std::vector<const float*>& ffn_inter_maxquant,
        std::vector<const float*>& ffn_inter_bias,
        std::vector<const TW*>& ffn_out_weight, std::vector<const float*>& ffn_out_maxquant,
        std::vector<const float*>& ffn_out_bias,
        const float* decoder_ln_weight, const float* decoder_ln_bias,
        const TW* trans_weight, const float* trans_maxquant, const float* trans_bias,
        const float* lm_ln_weight, const float* lm_ln_bias,
        const TW* embedding_weight, const float* embedding_maxquant, const T* embedding_bias,
        const T* positional_embedding_weight, const T* type_embedding_weight,
        const TID* role_id, const TID* decoder_role_id, const T* role_embedding_table,
        const TID* position_ids, const TID* decoder_position_ids,
        TID* output_ids, float* output_scores, TID* parent_ids, TID* sequence_length,
        const int64_t batch_size, const int64_t mem_length, const FTUnifiedDecodingParam& fudparam);
#pragma pack (4)
struct DLL_EXPORT DropoutAddLayernormParam {
    bool is_test;
    bool is_upscale_in_train;
    float dropout_prob;
    int64_t seed_val;
    bool is_layernorm;
    float eps;
    int64_t m;
    int64_t n;
};
#pragma pack ()
#pragma pack (4)
struct DLL_EXPORT DropoutAddRMSLayernormParam {
    bool is_test;
    bool is_upscale_in_train;
    float dropout_prob;
    int64_t seed_val;
    float eps;
    int64_t m;
    int64_t n;
};
#pragma pack ()
template<typename T>
DLL_EXPORT int dropout_add_layernorm(Context* ctx, const T* x, const T* bias,
        const float* ln_scale, const float* ln_bias,
        T* dropout, T* dropout_mask, T* y, float* mean, float* var, const DropoutAddLayernormParam& param,
        const T* input_bias = nullptr);

template<typename T>
DLL_EXPORT int dropout_add_layernorm_grad(Context* ctx, const T* dropout,
        const T* dropmask, const T* dy, T* dx, T* d_dropout, const float* scale,
        const float* mean, const float* var, float* dscale, float* dbias, const DropoutAddLayernormParam& param);

template<typename T>
DLL_EXPORT int dropout_add_rms_layernorm(Context* ctx, const T* x, const T* bias,
        const float* ln_scale, T* dropout, T* dropout_mask, T* y, float* var, const DropoutAddRMSLayernormParam& param);

template<typename T>
DLL_EXPORT int dropout_add_rms_layernorm_grad(Context* ctx, const T* dropout,
        const T* dropmask, const T* dy, T* dx, T* d_dropout, const float* scale,
        const float* var, float* dscale, const DropoutAddRMSLayernormParam& param);

template <typename T, typename TMASK, typename T_ACC>
DLL_EXPORT int dropout_add_layernorm_grad_v2(Context* ctx,
                                             const T* dropout,
                                             const TMASK* dropmask,
                                             const T* dy,
                                             T* dx,
                                             T* d_dropout,
                                             const T* scale,
                                             const T_ACC* mean,
                                             const T_ACC* var,
                                             T* dscale,
                                             T* dbias,
                                             const DropoutAddLayernormParam& param);

template <typename T, typename TMASK, typename T_ACC>
DLL_EXPORT int dropout_add_layernorm_v2(Context* ctx,
                                        const T* x,
                                        const T* bias,
                                        const T* ln_scale,
                                        const T* ln_bias,
                                        T* dropout,
                                        TMASK* dropout_mask,
                                        T* y,
                                        T_ACC* mean,
                                        T_ACC* var,
                                        const DropoutAddLayernormParam& param);

// fused op: add bias (1, n) to matrix (m, n), then relu activation
template<typename T> DLL_EXPORT int add_bias_relu(Context* ctx, const T* x, const T* b, T* z, T* relu_out, int64_t m,
        int64_t n);

template <typename T>
DLL_EXPORT int multi_rbf_fusion(Context* ctx, const T* x, T* y, int64_t xlen,
        const VectorParam<float>& mu, const VectorParam<float>& sigma);
template<typename T, typename TID = int>
DLL_EXPORT int rope(Context* ctx, const T* x, T* y, const float* cos, const float* sin,
        int64_t batch, int64_t head_num, int64_t head_dim, int64_t offset, const std::vector<TID>& lod,
        int64_t max_pos_len = 512, bool is_vsl = false, bool trans = true);
template<typename T, typename TID = int>
DLL_EXPORT int rope(Context* ctx, const T* x, T* y, const float* cos, const float* sin,
        int64_t batch, int64_t head_num, int64_t head_dim, int64_t offset, const VectorParam<TID>& rope_lod,
        int64_t max_pos_len = 512, bool is_vsl = false, bool trans = true);
template<typename T, typename TR, typename TID> DLL_EXPORT int vsl_rotary_neox_embedding(Context* ctx, const T* q, const T* k, const TR* rotary_pos_emb, T* q_emb, T* k_emb, const VectorParam<TID>& seq_lod, int batch, int max_seqlen, int head_num, int head_dims, std::string rotary_embedding_layout = "BLHD", const VectorParam<TID>& pos_emb_offset = {nullptr, 0, nullptr}, std::string pos_emb_type = "NORMAL", int k_group_num = -1, int pack_size = 2);
template<typename T> DLL_EXPORT int group_norm_silu_fusion(Context* ctx, const T* x, T* y,
        int64_t n, int64_t c, int64_t h, int64_t w, int64_t groups,
        float eps, const float* scale, const float* bias, float* mean, float* var, bool is_nchw);

// dropout参数
#pragma pack (4)
struct DLL_EXPORT DropoutParam {
    bool is_upscale_in_train;
    bool is_fixed_seed;
    float dropout_rate;
    int seed_val;
};
#pragma pack ()
// fused op: softmax_dropout_grad
template<typename T, typename TID = int>
DLL_EXPORT int softmax_dropout_grad_fusion(Context* ctx,
                const T* y, const T* dy, T* dx, const T* dropout_mask,
                int64_t batch, int64_t head_num, int64_t max_q_seq, int64_t max_kv_seq,
                const VectorParam<TID>& qlod, const VectorParam<TID>& kvlod,
                bool is_vsl, const DropoutParam& drop_p);
// fused op: multihead_self-attention
template <typename T, typename TGEMM, typename TID = int>
DLL_EXPORT int mha_fusion(Context* ctx, const T* qkv, T* softmax_out, T* dropout_mask, T* dropout_out, T* mha_out,
        int64_t batch, int64_t head_num, int64_t head_dim, const VectorParam<TID>& qlod, const VectorParam<TID>& kvlod,
        const DropoutParam& drop_p, bool is_test, float* q_maxptr = nullptr, float* k_maxptr = nullptr,
        float* v_maxptr = nullptr, float* dropout_out_maxptr = nullptr, float* mha_out_maxptr = nullptr);
// fused op: multihead_self-attention grad
template <typename T, typename TGEMM, typename TID = int>
DLL_EXPORT int mha_fusion_grad(Context* ctx, const T* qkv, const T* softmax_out, const T* dropout_mask,
        const T* dropout_out, const T* dmha_out, T* dqkv, int64_t batch, int64_t head_num, int64_t head_dim,
        const VectorParam<TID>& qlod, const VectorParam<TID>& kvlod, const DropoutParam& drop_p, bool is_test,
        const float* dmha_maxptr = nullptr, const float* dropout_out_maxptr = nullptr, const float* q_maxptr = nullptr,
        const float* k_maxptr = nullptr, const float* v_maxptr = nullptr, float* dqkv_maxptr = nullptr);

template <typename T, typename TOUT = float>
DLL_EXPORT int square_reduce_sum(Context* ctx, const T* x, TOUT* y, int64_t len, bool is_sqrt=false);
template<typename T, typename TR> DLL_EXPORT int rotary_embedding(Context* ctx, const T* q, const T* k,
        const TR* rotary_pos_emb, T* q_emb, T* k_emb, int batch, int seq_len, int head_num, int head_dim, int k_group_num = -1);

template<typename T> DLL_EXPORT int check_finite_unscale(Context* ctx, const T* input, T* res, int64_t n, float _scale, bool* found_inf_data);

template<typename T, typename TR> DLL_EXPORT int rotary_embedding_grad(Context* ctx, const T* q_emb_grad, const T* k_emb_grad,
        const TR* rotary_pos_emb, T* q_grad_input, T* k_grad_input, int batch, int seq_len, int head_num, int head_dim, int k_group_num = -1);

template<typename T, typename TR> DLL_EXPORT int rotary_embedding_v3(Context* ctx, const T* q, const T* k, const TR* cos, const TR* sin,
        T* q_emb, T* k_emb, int batch, int seq_len, int head_num, int head_dim, const std::vector<int64_t>& q_stride, const std::vector<int64_t>& k_stride, int k_group_num = -1,
        std::string rotary_embedding_layout="BLHD", bool is_qk_contiguous=true);
template<typename T, typename TR> DLL_EXPORT int rotary_embedding_v3_grad(Context* ctx, const T* q_emb_grad, const T* k_emb_grad,
        const TR* cos, const TR* sin, T* q_grad_input, T* k_grad_input, int batch, int seq_len, int head_num, int head_dim, const std::vector<int64_t>& q_stride, const std::vector<int64_t>& k_stride,
        int k_group_num = -1, std::string rotary_embedding_layout="BLHD", bool is_qk_contiguous=true);

template<typename T, typename TR, typename TID> DLL_EXPORT int vsl_rotary_embedding(Context* ctx, const T* q, const T* k,
        const TR* rotary_pos_emb, T* q_emb, T* k_emb, const VectorParam<TID>& seq_lod, int batch, int max_seqlen, int head_num,
        int head_dims, std::string rotary_embedding_layout = "BLHD", const VectorParam<TID>& pos_emb_offset = {nullptr, 0, nullptr}, std::string pos_emb_type = "NORMAL", int k_group_num = -1);
template<typename T, typename TID> DLL_EXPORT int vsl_cache_transpose(Context* ctx, const T* input, T* output,
        const VectorParam<TID>& seq_lod, int batch, int max_seqlen, int head_num, int head_dims);
template<typename T> DLL_EXPORT int fusion_smooth_transform(Context* ctx, const T* x, const T* shift, const T* smooth,
        T* y, int m, int n);
// fused op: softmax with mask
template<typename T>
DLL_EXPORT int softmax_with_mask(Context* ctx, const T* x, const T* mask, T* y, const std::vector<int64_t>& x_shape, const
        std::vector<int64_t>& mask_shape, float input_scale=1);

template <typename T>
DLL_EXPORT int rotary_embedding_v2(
        Context* ctx, const T* t, const T* freqs, T* out, const std::vector<int64_t>& t_shape,
        const std::vector<int64_t>& freqs_shape,
        const std::vector<int64_t>& t_stride,
        const std::vector<int64_t>& freqs_stride);
template <typename T>
DLL_EXPORT int rotary_embedding_v2_grad(
        Context* ctx, const T* dy, const T* freqs, T* out, const std::vector<int64_t>& dy_shape,
        const std::vector<int64_t>& freqs_shape,
        const std::vector<int64_t>& dy_stride,
        const std::vector<int64_t>& freqs_stride);
template <typename T>
DLL_EXPORT int multi_copy2d_findxmax_fusion(Context* ctx,
                                            const T* x,
                                            std::vector<T*>& y_list,
                                            const std::vector<int64_t>& ldy_list,
                                            int64_t m,
                                            int64_t n,
                                            std::vector<float*>& ymax_list);
template <typename T>
DLL_EXPORT int multi_copy2d_findxmax_fusion_v2(Context* ctx,
                                            const T* x,
                                            std::vector<T*>& y_list,
                                            const std::vector<int64_t>& cache_len_list,
                                            const std::vector<int64_t>& ldy_list,
                                            int64_t m,
                                            int64_t ldx,
                                            std::vector<float*>& ymax_list);

template <typename T>
DLL_EXPORT int multi_copy2d_findxmax_max_fusion(Context* ctx,
                                            const T* x,
                                            std::vector<T*>& y_list,
                                            const std::vector<int64_t>& ldy_list,
                                            int64_t m,
                                            int64_t n,
                                            const std::vector<float*>& ymax_list,
                                            const std::vector<float*>& premax_list);

template <typename T>
DLL_EXPORT int precompute_rotary_embedding(Context* ctx, const T* input, T* output, const std::vector<int64_t>& input_shape, const bool grad);


template <typename TX, typename TY>
DLL_EXPORT int scale_cast_fusion(Context* ctx, const TX* x, TY* y, int64_t len, float scale);

template<typename T> DLL_EXPORT int swiglu_add_mul_fusion(Context* ctx, const T* x, T* y, int64_t m, int64_t n,
        const T* add_data = nullptr, const T* mul_data = nullptr,
        bool turn = false, const float* max_x = nullptr, float* max_y = nullptr);

template <typename T>
DLL_EXPORT int rotary_no_freqs_embedding_v2(
        Context* ctx, const T* t, const T* sin, const T* cos, T* out, const std::vector<int64_t>& t_shape,
        const std::vector<int64_t>& freqs_shape,
        const std::vector<int64_t>& t_stride,
        const std::vector<int64_t>& freqs_stride);
template <typename T>
DLL_EXPORT int rotary_no_freqs_embedding_v2_grad(
        Context* ctx, const T* dy, const T* sin, const T* cos, T* out, const std::vector<int64_t>& dy_shape,
        const std::vector<int64_t>& freqs_shape,
        const std::vector<int64_t>& dy_stride,
        const std::vector<int64_t>& freqs_stride);

template <typename T>
DLL_EXPORT int rotary_no_freqs_qk_embedding_v2(
        Context* ctx, const T* q, const T* k, const T* sin, const T* cos, T* q_emb, T* k_emb, const std::vector<int64_t>& t_shape,
        const std::vector<int64_t>& freqs_shape,
        const std::vector<int64_t>& t_stride,
        const std::vector<int64_t>& freqs_stride,
        int64_t k_group_num = -1);

template <typename T>
DLL_EXPORT int rotary_no_freqs_qk_embedding_v2_grad(
        Context* ctx, const T* dq_emb, const T* dk_emb, const T* sin, const T* cos, T* dq, T* dk, const std::vector<int64_t>& t_shape,
        const std::vector<int64_t>& freqs_shape,
        const std::vector<int64_t>& t_stride,
        const std::vector<int64_t>& freqs_stride,
        int64_t k_group_num = -1);

template<typename T_X, typename T_W, typename T_Y, typename T_BIAS, typename T_ACT, typename T_GEMM>
DLL_EXPORT int fused_dense_fusion(Context* ctx, const T_X* x, const T_W* w, T_Y* y, int64_t m, int64_t n, int64_t k,
        bool x_trans, bool w_trans, const float* x_maxptr, const float* w_maxptr, float* y_maxptr,
        int64_t x_ld, int64_t w_ld, int64_t y_ld, float alpha, float beta, const T_BIAS* bias, const Activation_t& act,
        T_ACT* act_in = nullptr);

template <typename T_X, typename T_W, typename T_Y, typename T_GRAD, typename T_ACT, typename T_GEMM>
DLL_EXPORT int fused_dense_fusion_grad(Context* ctx, const T_X* x, const T_W* w, const T_Y* dy, T_X* dx, T_GRAD* dw,
        int64_t m, int64_t n, int64_t k, bool x_trans, bool w_trans, const float* x_maxptr, const float* w_maxptr,
        const float* dy_maxptr, float* dx_maxptr, float* dw_maxptr, int64_t x_ld, int64_t w_ld, int64_t y_ld,
        float alpha, float beta, T_GRAD* dbias, const Activation_t& act, const T_ACT* act_in = nullptr);

template <typename T1, typename T2, typename T3>
DLL_EXPORT int elementwiseadd_with_cast(Context* ctx, const T1* x, const T2* y, T3* z, int64_t len);
template <typename T, typename TR>
DLL_EXPORT int rotary_embedding_bhld(api::Context* ctx, const T* q, const T* k, const TR* cos, const TR* sin, T* q_emb, T* k_emb,
                          int batch, int head_num, int seq_len, int head_dim);

template <typename T, typename TR>
DLL_EXPORT int rotary_embedding_bhld_grad(api::Context* ctx, const T* dqout, const T* dkout, const TR* cos, const TR* sin, T* dq,
                               T* dk, int batch, int head_num, int seq_len, int head_dim);

template<typename TI, typename TO, typename TID>
DLL_EXPORT int reshape_cached_kv(Context* ctx, const TI* x, TO* y, const TID* block_table,
                      const VectorParam<int32_t>& kv_seq_lod,
                      const VectorParam<int32_t>& start_tokens,
                      const VectorParam<int32_t>& real_batch,
                      int64_t batch_size, int64_t kv_head_num, int64_t head_dim,
                      int64_t max_seq_num, int64_t block_size, int64_t max_num_blocks_per_seq,
                      const std::string& qkv_layout,
                      const std::string& page_layout,
                      const float* scale = nullptr,
                      float* batch_max_ptrs = nullptr,
                      float* max_ptrs = nullptr);

template<typename TI, typename TO, typename TID>
DLL_EXPORT int reshape_cached_kv_zp(Context* ctx, const TI* x, TO* y, const TID* block_table,
                      const VectorParam<int32_t>& kv_seq_lod,
                      const VectorParam<int32_t>& start_tokens,
                      const VectorParam<int32_t>& real_batch,
                      int64_t batch_size, int64_t kv_head_num, int64_t head_dim,
                      int64_t max_seq_num, int64_t block_size, int64_t max_num_blocks_per_seq,
                      const std::string& qkv_layout,
                      const std::string& page_layout,
                      const TI* scale,
                      const TI* zero);

template<typename T> DLL_EXPORT int sine_pos_fusion(Context* ctx, const T* x, const T* x_mul, T* y, int64_t batch, int64_t n, int64_t dim);
}
}
}
#endif
