#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_FUSION_CV_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_FUSION_CV_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/refactor/fusion_cv_param.h"
#include "xpu/xdnn_types.h"

#ifdef _MSC_VER
#include <algorithm>
#endif

namespace baidu {
namespace xpu {
namespace api {

template <typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT size_t resnet_unit_fusion_get_reserve_space_size(Context* ctx,
        const std::vector<std::vector<int64_t>>& x_shape_list, int64_t f,
        const std::vector<std::vector<int64_t>>& ksize_list, const std::vector<std::vector<int64_t>>& stride_list,
        const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation, int64_t group,
        const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist, const Activation_t& act,
        bool is_nchw, bool has_shortcut, bool fused_add);

template <typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int resnet_unit_fusion(Context* ctx, const std::vector<const TX*>& x_list,
        const std::vector<const TW*>& w_list, const std::vector<TY*>& conv_y_list, TY* y,
        const std::vector<std::vector<int64_t>>& x_shape_list, int64_t f,
        const std::vector<std::vector<int64_t>>& ksize_list, const std::vector<std::vector<int64_t>>& stride_list,
        const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation, int64_t group, float eps, float momentum,
        const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const std::vector<const float*>& scale_list, const std::vector<const float*>& bias_list,
        const std::vector<float*>& batch_mean_list, const std::vector<float*>& batch_inv_std_list,
        const std::vector<float*>& global_mean_list, const std::vector<float*>& global_var_list,
        const Activation_t& act, bool is_nchw, bool has_shortcut, bool fused_add, bool is_train,
        void* reserve_space = nullptr, bool use_frozen_bn = false);

template <typename T, typename TW, typename TGEMM, typename TM>
DLL_EXPORT int resnet50(Context* ctx, const T* x, const std::vector<const TW*>& weight_list, T* y, int64_t n, int64_t c,
        int64_t h, int64_t w, const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const std::vector<float*>& y_maxlist, const std::vector<const float*>& blist,
        const std::vector<T*>& feature_list, bool is_nchw, const ResnetExtraParam& extra_params = {false});

template <typename T, typename TW, typename TGEMM, typename TM>
DLL_EXPORT int resnet50_v10(Context* ctx, const T* x, const std::vector<const TW*>& weight_list, T* y, int64_t n,
        int64_t c, int64_t h, int64_t w, const std::vector<const float*>& x_maxlist,
        const std::vector<const float*>& w_maxlist, const std::vector<float*>& y_maxlist,
        const std::vector<const float*>& blist, const std::vector<T*>& feature_list, bool is_nchw,
        const ResnetExtraParam& extra_params = {false});

template <typename T, typename TW, typename TGEMM, typename TM>
DLL_EXPORT int resnet101(Context* ctx, const T* x, const std::vector<const TW*>& weight_list, T* y, int64_t n,
        int64_t c, int64_t h, int64_t w, const float* x_maxptr, const std::vector<const float*>& w_maxptr_list,
        float* y_maxptr, const std::vector<const float*>& bias_list, const std::vector<T*>& feature_list, bool is_nchw,
        const ResnetExtraParam& extra_params = {false});

template <typename T, typename TW, typename TGEMM, typename TM>
DLL_EXPORT int resnet34(Context* ctx, const T* x, const std::vector<const TW*>& weight_list, T* y, int64_t n, int64_t c,
        int64_t h, int64_t w, const std::vector<const float*>& x_maxlist, const std::vector<const float*>& w_maxlist,
        const std::vector<float*>& y_maxlist, const std::vector<const float*>& blist,
        const std::vector<T*>& feature_list, bool is_nchw, const ResnetExtraParam& extra_params = {false});

template <typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int resnet_unit_grad_fusion(Context* ctx, const std::vector<const TX*>& x_list,
        const std::vector<const TW*>& w_list, const TY* dy, const TY* y, const std::vector<const TY*>& conv_y_list,
        std::vector<TX*>& dx_list, std::vector<TW*>& dw_list, const std::vector<std::vector<int64_t>>& x_shape_list,
        int64_t f, const std::vector<std::vector<int64_t>>& ksize_list,
        const std::vector<std::vector<int64_t>>& stride_list, const std::vector<int64_t>& pad,
        const std::vector<int64_t>& dilation, int64_t group, const std::vector<const float*>& x_maxlist,
        const std::vector<const float*>& w_maxlist, const std::vector<const float*>& scale_list,
        const std::vector<const float*>& batch_mean_list, const std::vector<const float*>& batch_inv_std_list,
        std::vector<float*>& dscale_list, std::vector<float*>& dbias_list, const Activation_t& act, float eps,
        bool is_nchw, bool has_shortcut, bool fused_add, void* reserve_space = nullptr, bool use_frozen_bn = false);

template <typename TX, typename TW, typename TY, typename TGEMM, typename BN_PARAM = CVFusionBatchNormParam>
DLL_EXPORT size_t yolo_unit_fusion_get_reserve_space_size(Context* ctx,
        const std::vector<std::vector<int64_t>>& x_shape_list, const std::vector<CVFusionConv2dParam>& conv_param_list,
        const std::vector<BN_PARAM>& bn_param_list, const std::vector<const float*>& x_maxlist,
        const std::vector<const float*>& w_maxlist, const Activation_t& act, bool is_nchw, bool is_train);

template <typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int yolo_unit_fusion(Context* ctx, const std::vector<const TX*>& x_list,
        const std::vector<const TW*>& w_list, TY* y, const std::vector<std::vector<int64_t>>& x_shape_list,
        const std::vector<CVFusionConv2dParam>& conv_param_list,
        const std::vector<CVFusionBatchNormParam>& bn_param_list, const std::vector<const float*>& x_maxlist,
        const std::vector<const float*>& w_maxlist, void* reserve_space, const Activation_t& act, bool is_nchw,
        bool is_train);

template <typename TX, typename TW, typename TY, typename TGEMM>
DLL_EXPORT int yolo_unit_grad_fusion(Context* ctx, const std::vector<const TX*>& x_list,
        const std::vector<const TW*>& w_list, const TY* dy, const TY* y, std::vector<TX*>& dx_list,
        std::vector<TW*>& dw_list, const std::vector<std::vector<int64_t>>& x_shape_list,
        const std::vector<CVFusionConv2dParam>& conv_param_list,
        const std::vector<CVFusionBatchNormGradParam>& bn_param_list, const std::vector<const float*>& x_maxlist,
        const std::vector<const float*>& w_maxlist, void* reserve_space, const Activation_t& act, bool is_nchw);

} // namespace api
} // namespace xpu
} // namespace baidu
#endif
