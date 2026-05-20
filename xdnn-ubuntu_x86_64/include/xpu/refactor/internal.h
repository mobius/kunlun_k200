#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_INTERNAL_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_INTERNAL_H
#include "xpu/dll_export.h"
#include "xpu/refactor/context/newcontext.h"
#include "xpu/refactor/fusion_cv_param.h"
#include "xpu/refactor/batchnorm.h"
#include <vector>
namespace baidu {
namespace xpu {
namespace api {

inline bool is_gm(const void* ptr) {
    return (((long long int)(ptr) >> 32) != 0);
}

DLL_EXPORT int do_host2device(Context* ctx, const void* src, void* dst, int64_t bytes);
DLL_EXPORT int do_device2host(Context* ctx, const void* src, void* dst, int64_t bytes);

template <typename T> int from_fp32(Context* ctx, const float* x, T* y, int64_t len, float* maxptr);
template <typename T> int to_fp32(Context* ctx, const T* x, float* y, int64_t len, const float* maxptr);
template <typename T> int from_fp16(Context* ctx, const float16* x, T* y, int64_t len, float* maxptr);
template <typename T> int to_fp16(Context* ctx, const T* x, float16* y, int64_t len, const float* maxptr);
template <typename T> int from_nchw(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, bool is_nchw);
template <typename T> int to_nchw(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, bool is_nchw);
template <typename T> int from_nhwc(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, bool is_nchw);
template <typename T> int to_nhwc(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w, bool is_nchw);

template <typename T> const float* to_fp32_if_necessary(api::ctx_guard& RAII_GUARD, Context* ctx,
        const T* x, int64_t len, const float* maxptr);
template<typename T> const float* do_findmax_if_necessary(api::ctx_guard& RAII_GUARD, Context* ctx,
        const T* x, const float* maxptr, int64_t len);
template<typename T, int MAX_PTR_TYPE = 0>
const float* do_findmax_if_necessary_batched(api::ctx_guard& RAII_GUARD, Context* ctx, const T* data,
            const float* ptr, int batch_size, int64_t size);
template <typename T> const T* to_nchw_if_necessary(api::ctx_guard& RAII_GUARD, Context* ctx,
        const T* x, int64_t n, int64_t c, int64_t h, int64_t w, bool is_nchw);
template<typename T> const float* do_findmax_copy_if_necessary(api::ctx_guard& RAII_GUARD,
        Context* ctx, const T* data, const float* ptr, T* data_copy, int64_t size, bool copy_or_not);
template<typename T> const float* try_move_to_l3_findmax_fusion(api::ctx_guard& RAII_GUARD,
        Context* ctx, bool use_l3, const float* maxptr, int64_t len, const T** data_ptr);
template<typename T> float* enable_maxptr_merge(float* y_maxptr);

// extern from nn/im2col
bool ds_win2vec_limit(Context* ctx, const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride,
        const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation, bool is_nchw, int64_t TGEMM_size);

template<typename T>
int _internal_xpu_sdnn_activation(Context* ctx, const T* x, T* y, int64_t len, const Activation_t& act);
template<typename T>
int _internal_xpu_sdnn_activation_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len, const Activation_t& act);

template <typename TX, typename TW, typename TY, typename TY_INNER, typename TBIAS, typename TGEMM_I, typename TGEMM_O>
DLL_EXPORT int internal_fc_charm(Context* ctx,
             int m,
             int n,
             int k,
             bool a_trans,
             bool b_trans,
             const TX* x,
             const TW* w,
             TY* y,
             TY* noact_y,
             int ldx,
             int ldw,
             int ldy,
             const float* x_maxptr,
             const float* w_maxptr,
             float* y_maxptr,
             float* noact_y_maxtptr,
             float alpha,
             float beta,
             const TBIAS* bias,
             const float* scale,
             const TY* branch_add,
             const TY* branch_mul,
             const float* branch_add_maxptr,
             const float* branch_mul_maxptr,
             const Activation_t& act);

template <typename T, typename T_INNER, typename TGEMM_I, typename TGEMM_O>
DLL_EXPORT int internal_fc_fuse_reduce_charm(Context* ctx,
             int m,
             int n,
             int k,
             bool a_trans,
             bool b_trans,
             const T* x,
             const T* w,
             T* y,
             T* reduce_out,
             int ldx,
             int ldw,
             int ldy,
             const float* x_maxptr,
             const float* w_maxptr,
             bool reduce_out_shape_is_m);

template<typename T>
DLL_EXPORT int swish_after_bn(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t hw,
        int64_t out_c_start, int64_t out_c_end, int64_t ldc, float* max_y_ptr);
template<typename T>
DLL_EXPORT int swish_grad_before_bn(Context* ctx, const T* x, const T* dy, T* dx, int64_t n,
        int64_t c, int64_t hw, int64_t c_start, int64_t c_end, T* dx_l3);

template<typename TGEMM> DLL_EXPORT std::vector<void*> resnet_unit_fusion_get_reserve_space_ptrs(
        Context* ctx, void* reserve_space, const std::vector<const float*>& x_maxlist,
        const std::vector<const float*>& w_maxlist, const Activation_t& act, bool has_shortcut,
        bool is_train = true);

template <typename TX, typename TW, typename TY, typename TGEMM, typename BN_PARAM = CVFusionBatchNormParam>
DLL_EXPORT std::vector<void*> yolo_unit_fusion_get_reserve_space_ptrs(Context* ctx, void* reserve_space,
        const std::vector<std::vector<int64_t>>& x_shape_list, const std::vector<CVFusionConv2dParam>& conv_param_list,
        const std::vector<BN_PARAM>& bn_param_list, const std::vector<const float*>& x_maxlist,
        const std::vector<const float*>& w_maxlist, const Activation_t& act, bool is_nchw, bool is_train);

template<typename TW>
DLL_EXPORT int conv2d_filter_change_group(Context* ctx,
        const TW* old_filter, TW* new_filter, int64_t f, int64_t c,
        const std::vector<int64_t>& ksize, int64_t oldg, int64_t newg, bool is_nchw);
template<typename TW> TW* conv2d_filter_change_group(api::ctx_guard& RAII_GUARD, Context* ctx,
        const TW* old_filter, int64_t f, int64_t c, const std::vector<int64_t>& ksize,
        int64_t oldg, int64_t newg, bool is_nchw);

int im2col_param_check(Context* ctx, int64_t n, int64_t c, int64_t h, int64_t w,
        const std::vector<int64_t>& ksize_extend, const std::vector<int64_t>& stride_extend,
        const std::vector<int64_t>& pad_extend, const std::vector<int64_t>& dilation_extend,
        bool allow_pad_ge_ksize = false);
int im2col3d_param_check(Context* ctx, int64_t n, int64_t c, int64_t d, int64_t h, int64_t w,
        const std::vector<int64_t>& ksize_extend, const std::vector<int64_t>& stride_extend,
        const std::vector<int64_t>& pad_extend, const std::vector<int64_t>& dilation_extend);

int get_reduce_id(const std::vector<int64_t>& large_dims, const std::vector<int64_t>& small_dims,
        int64_t large_id, bool* is_first_ele_ptr);

template <typename T>
int get_reduce_execution_plan(
        api::ctx_guard& RAII_GUARD, Context* ctx,
        const std::vector<int64_t>& large_dims, const std::vector<int64_t>& small_dims,
        T* large_ptr, T* small_ptr,
        std::vector<std::tuple<int64_t, int64_t, int64_t, T*, T*>>& ret);

template <typename T> int get_pad_execution_plan(api::ctx_guard& RAII_GUARD, Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& pad_left,
        const std::vector<int64_t>& pad_right, std::vector<std::tuple<const T*, T*, int64_t,
        int64_t, int64_t, int64_t> >& ret, bool pad_exec = true);

template <typename T> int get_strided_slice_execution_plan(
        api::ctx_guard& RAII_GUARD, Context* ctx, const T* in, T* out,
        const std::vector<int64_t>& in_dims, const std::vector<int64_t>& out_dims,
        const std::vector<int64_t>& starts, const std::vector<int64_t>& ends,
        const std::vector<int64_t>& strides,
        std::vector<std::tuple<const T*, T*, int64_t, int64_t, int64_t, int64_t,
        int64_t, int64_t, int64_t>>& ret);

template<typename T> int im2col_ddim(Context* ctx, const T* x, T* y, int64_t n, int64_t c,
        int64_t xd, int64_t xh, int64_t xw, int64_t kd, int64_t sd, int64_t pad_d0,
        int64_t pad_d1, int64_t dila_d, bool is_ncdhw);

template<typename T> int image_pad2d_if_necessary(api::ctx_guard& RAII_GUARD, Context* ctx, const T** x,
        int64_t n, int64_t c, int64_t& xh, int64_t& xw, const std::vector<int64_t>& ksize, const std::vector<int64_t>& dilation,
        bool is_nchw, std::vector<int64_t>& pad);

template<typename T>
int box_inter_union(Context* ctx, const T* boxes1, const T* boxes2, T* _inter, T* _union, int64_t m, int64_t n);

template<typename T>
int box_diou_iou(Context* ctx, const T* boxes1, const T* boxes2, float* diou, float* iou,
        int64_t m, int64_t n, float eps);

void add_front_ones(std::vector<int64_t>* xshape, std::vector<int64_t>* yshape);

std::vector<int64_t> get_zshape(const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);

int get_mnk_for_broadcast_ops(const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape,
        std::vector<int64_t>* xshape_after_cmp, std::vector<int64_t>* yshape_after_cmp);

template <typename T>
DLL_EXPORT int kl3_warmup_cache(Context* ctx, const T* x, T* y, int64_t len, api::WARMUP_CACHE_MODE mode);

template <typename T>
DLL_EXPORT int bi_batch_norm_fusion(Context* ctx, const std::vector<const T*>& x_list, T* y, int64_t n, int64_t c,
        int64_t h, int64_t w, float eps, float momentum, const std::vector<const float*>& scale_list,
        const std::vector<const float*>& bias_list, const std::vector<float*>& batch_mean_list,
        const std::vector<float*>& batch_inv_std_list, const std::vector<float*>& global_mean_list,
        const std::vector<float*>& global_var_list, bool is_nchw, const Activation_t& act, float* bitmask,
        float* max_ptr, bool use_frozen_bn = false);

template <typename T>
DLL_EXPORT int bn_grad_phase1(Context* ctx, int64_t n, int64_t c_start, int64_t c_end, int64_t c, int64_t hw,
                                  const T* x, const T* y, const T* dy, T* dbranch, const float* scale,
                                  const float* saved_mean, const float* saved_var, float* dscale, float* dbias,
                                  float* aa_gm, float* bb_gm, float* cc_gm, const Activation_t& act,
                                  const float* bitmask_gm, int copy_to_l3, bool is_nchw = true);

template <typename T>
DLL_EXPORT int bn_grad_phase2(Context *ctx, int64_t n, int64_t c, int64_t hw, const T *x, const T *y, const T *dy,
                                  const T *dbranch, T *dx, const float *x_aa_gm,
                                  const float *x_bb_gm, const float *x_cc_gm, float *max_ptr,
                                  const Activation_t &act);

template <typename T>
DLL_EXPORT int bn_grad_phase2_with_mask(Context* ctx, int64_t n, int64_t c, int64_t hw, const T* x, const T* y, const T* dy,
        const T* dbranch, T* dx, const float* x_aa_gm, const float* x_bb_gm, const float* x_cc_gm, float* max_ptr,
        const Activation_t& act, const unsigned int* mask);

template <typename T>
DLL_EXPORT int bn_grad_phase2_nhwc(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t n, int64_t c,
                                   int64_t hw, const float* scale, const float* batch_mean, const float* batch_inv_std,
                                   float* dscale, float* dbias, T* dbranch, const Activation_t& act,
                                   const uint32_t* bitmask, float* max_ptr, const float* dscale_dbais_cluster_gm,
                                   int skip_reduce, int64_t total_nhw);

template <typename T>
DLL_EXPORT int bn_fwd_phase0(Context* ctx, int64_t n, int64_t c_start, int64_t c_end, int64_t c, int64_t hw,
        const T* x, float* sum, float* squaresum);

template <typename T>
DLL_EXPORT int bn_fwd_phase1_nhwc(Context* ctx, const T* x, float* y, int64_t n, int64_t c, int64_t h, int64_t w);

template <typename T>
DLL_EXPORT int bn_fwd_phase1(Context * ctx, int64_t n, int64_t c_start, int64_t c_end, int64_t c, int64_t hw,
            float epsilon, float momentum, const T* x, const float* scale, const float* bias, float* run_mean,
            float* run_var, float* saved_mean, float* saved_var, float* fusion_scale, float* fusion_bias);

template <typename T>
DLL_EXPORT int bn_fwd_phase2(Context* ctx, int64_t n, int64_t c_start, int64_t c_end, int64_t c, int64_t hw,
        float epsilon, float momentum, const T* x, T* y, const float* scale, const float* bias, float* run_mean,
        float* run_var, float* saved_mean, float* saved_var, const T* branch, int copytol3_bitmap, float* bitmask,
        float* max, float alpha, const Activation_t& act, const float* fusion_scale, const float* fusion_bias);

template <typename T>
DLL_EXPORT int bn_fwd_partial(Context* ctx, int64_t n, int64_t c_start, int64_t c_end, int64_t c, int64_t hw, const float* sum,
        const float* square_sum, float epsilon, float momentum, const float* scale, const float* bias, float* run_mean,
        float* run_var, float* saved_mean, float* saved_var, float* fusion_scale, float* fusion_bias);

template <typename T>
DLL_EXPORT int partial_bn_fusion(Context* ctx, const T* x, T* y, const float* sum, const float* square_sum, int64_t n,
        int64_t c, int64_t h, int64_t w, float eps, float momentum, const float* scale, const float* bias,
        float* batch_mean, float* batch_inv_std, float* global_mean, float* global_var, const T* branch,
        const Activation_t& act, uint32_t* bitmask, float* max_ptr);
template <typename T>
DLL_EXPORT int bi_bn_grad_phase1(Context* ctx, int64_t n, int64_t c_start, int64_t c_end, int64_t c, int64_t hw,
                                 const T* x, const T* z, const T* y, const T* dy, T* dbranch, const float* scale_x,
                                 const float* saved_mean_x, const float* saved_var_x, const float* scale_z,
                                 const float* saved_mean_z, const float* saved_var_z, float* dscale_x, float* dbias_x,
                                 float* dscale_z, float* dbias_z, float* x_aa_gm, float* x_bb_gm, float* x_cc_gm,
                                 float* z_aa_gm, float* z_bb_gm, float* z_cc_gm, const Activation_t& act,
                                 const float* bitmask_gm, bool is_nchw = true);

template <typename TX, typename TW, typename TY, typename TGEMM>
int conv2d_wrapper_xpu2_or_xpu3(Context* ctx, int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t f,
        const std::vector<int64_t>& ksize, const std::vector<int64_t>& stride, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const TX* x, const TW* weight, TY* y,
        const float* x_maxptr, const float* w_maxptr, float* y_maxptr, const float* scale, const float* bias,
        const TY* branch, const Activation_t& act, bool is_nchw, const float* branch_maxptr, float* x_maxptr_wo = nullptr, 
        float* w_maxptr_wo = nullptr, float* sum = nullptr, float* sum_of_square = nullptr, int64_t ld_out_f = -1);

template <typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int conv2d_with_maxptr_writeback(Context* ctx, const TX* x, const TW* weight, TY* y, int64_t n, int64_t c,
        int64_t xh, int64_t xw, int64_t f, const std::vector<int64_t>& _ksize, const std::vector<int64_t>& _stride,
        const std::vector<int64_t>& _pad, const std::vector<int64_t>& _dilation, int64_t group, const float* x_maxptr,
        const float* w_maxptr, float* y_maxptr, bool is_nchw, float* x_maxptr_wo, float* w_maxptr_wo);

template <typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int conv2d_with_sum(Context* ctx, const TX* x, const TW* weight, TY* y, float* sum, float* sum_of_square,
        int64_t n, int64_t c, int64_t xh, int64_t xw, int64_t f, const std::vector<int64_t>& _ksize,
        const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad, const std::vector<int64_t>& _dilation,
        const float* x_maxptr, const float* w_maxptr, bool is_nchw, float* x_maxptr_wo, float* w_maxptr_wo);

template <typename T>
DLL_EXPORT int batch_norm_swish_fusion(Context* ctx, const T* x, T* y, int64_t n, int64_t c, int64_t h, int64_t w,
        float eps, float momentum, const float* scale, const float* bias, float* batch_mean, float* batch_inv_std,
        float* global_mean, float* global_var, bool is_nchw, const T* branch, const Activation_t& act, T* swish_input,
        float* y_maxptr);

template <typename T>
DLL_EXPORT int batch_norm_swish_grad_fusion(Context* ctx, const T* x, const T* y, const T* dy, const T* swish_x, T* dx,
        int64_t n, int64_t c, int64_t h, int64_t w, const float* scale, const float* batch_mean,
        const float* batch_inv_std, float* dscale, float* dbias, bool is_nchw, const Activation_t& act,
        float* dx_maxptr);

template <typename T>
DLL_EXPORT int bi_bn_fusion_partial(Context* ctx,
                                    float eps,
                                    float momentum,
                                    int64_t n,
                                    int64_t c,
                                    int64_t h,
                                    int64_t w,
                                    const std::vector<const T*>& x_list, //[x, z]
                                    T* y,
                                    const std::vector<const float*>& sum_list,
                                    const std::vector<const float*>& ss_list,
                                    const std::vector<const float*>& scale_list,
                                    const std::vector<const float*>& bias_list,
                                    const std::vector<float*>& global_mean_list,
                                    const std::vector<float*>& global_var_list,
                                    const std::vector<float*>& batch_mean_list,
                                    const std::vector<float*>& batch_inv_std_list,
                                    bool is_nchw,
                                    const Activation_t& act,
                                    unsigned int* bitmask,
                                    float* max_ptr);
}
}
}

#endif // BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_INTERNAL_H
