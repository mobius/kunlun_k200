#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_FUSION_CV_PARAM_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_FUSION_CV_PARAM_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"

#ifdef _MSC_VER
#include <algorithm>
#endif

namespace baidu {
namespace xpu {
namespace api {

struct DLL_EXPORT ResnetExtraParam {
    // Pad algo of max pooling is *SAME*(defined the same as tensorflow), otherwise, use *DEFAULT* padding({1})
    // With *SAME* padding, yh and yw is computed as:
    // yh = ceil(xh / stride_h), yw = ceil(xw / stride_w)
    // Padding is computed as follows, take pad_up, pad_down as an example:
    // pad_h = max(filter_h - (xh % stride_h == 0 ? stride_h : xh % stride_h), 0)
    // pad_up = pad_h / 2
    // pad_down = pad_h - pad_up
    bool max_pool_pad_algo_use_tf_same;
};

struct DLL_EXPORT CVFusionConv2dParam {
    std::vector<int64_t> ksize;
    std::vector<int64_t> stride;
    std::vector<int64_t> pad;
    std::vector<int64_t> dilation;
    int64_t f;
    int64_t c;
    int64_t group;
    CVFusionConv2dParam(int64_t f, int64_t c, int64_t group, const std::vector<int64_t>& ksize,
            const std::vector<int64_t>& stride, const std::vector<int64_t>& pad, const std::vector<int64_t>& dilation)
        : ksize(ksize), stride(stride), pad(pad), dilation(dilation), f(f), c(c), group(group) {
    }
};

struct DLL_EXPORT CVFusionBatchNormParam {
    float eps;
    float momentum;
    const float* scale;
    const float* bias;
    float* batch_mean;
    float* batch_inv_std;
    float* global_mean;
    float* global_var;
    std::vector<std::string> pos_list;

    CVFusionBatchNormParam(float eps, float momentum, const float* scale, const float* bias, float* batch_mean,
            float* batch_inv_std, float* global_mean, float* global_var)
        : eps(eps), momentum(momentum), scale(scale), bias(bias), batch_mean(batch_mean), batch_inv_std(batch_inv_std),
          global_mean(global_mean), global_var(global_var) {}
    CVFusionBatchNormParam(float eps, float momentum, const std::string& scale_pos, const std::string& bias_pos,
            const std::string& batch_mean_pos, const std::string& batch_inv_std_pos, const std::string& global_mean_pos,
            const std::string& global_var_pos) : eps(eps), momentum(momentum), scale(nullptr), bias(nullptr),
            batch_mean(nullptr), batch_inv_std(nullptr), global_mean(nullptr), global_var(nullptr) {
        pos_list = {scale_pos, bias_pos, batch_mean_pos, batch_inv_std_pos, global_mean_pos, global_var_pos};
    }
    void init(const std::vector<float*>& ptr_list) {
        scale = ptr_list[0];
        bias = ptr_list[1];
        batch_mean = ptr_list[2];
        batch_inv_std = ptr_list[3];
        global_mean = ptr_list[4];
        global_var = ptr_list[5];
    }
};

struct DLL_EXPORT CVFusionBatchNormGradParam {
    const float* scale;
    const float* batch_mean;
    const float* batch_inv_std;
    float* dscale;
    float* dbias;
    std::vector<std::string> pos_list;
    CVFusionBatchNormGradParam(
            const float* scale, const float* batch_mean, const float* batch_inv_std, float* dscale, float* dbias)
        : scale(scale), batch_mean(batch_mean), batch_inv_std(batch_inv_std), dscale(dscale), dbias(dbias) {}
    CVFusionBatchNormGradParam(const std::string& scale_pos, const std::string& batch_mean_pos,
            const std::string& batch_inv_std_pos, const std::string& dscale_pos, const std::string& dbias_pos)
        : scale(nullptr), batch_mean(nullptr), batch_inv_std(nullptr), dscale(nullptr), dbias(nullptr) {
        pos_list = {scale_pos, batch_mean_pos, batch_inv_std_pos, dscale_pos, dbias_pos};
    }
    void init(const std::vector<float*>& ptr_list) {
        scale = ptr_list[0];
        batch_mean = ptr_list[1];
        batch_inv_std = ptr_list[2];
        dscale = ptr_list[3];
        dbias = ptr_list[4];
    }
};

} // namespace api
} // namespace xpu
} // namespace baidu
#endif
