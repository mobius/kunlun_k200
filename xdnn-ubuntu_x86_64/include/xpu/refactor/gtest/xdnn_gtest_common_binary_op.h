#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_BINARY_OP_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_BINARY_OP_H

#include "xpu/xdnn.h"
#include "xpu/refactor/gtest/xdnn_gtest.h"

static int get_sign(float v) {
    if (v >= 0) {
        return 1;
    } else {
        return -1;
    }
}

template<typename T> static void binary_param(api::DeviceType dev,
        std::function<int(api::Context*, const T*, const T*, T*, int)> func) {
    api::Context ctx(dev);
    api::Dtype dt = api::CPPTypeToDtype<T>();
    auto x = api::tensor(1.0f).astype(dt).to(dev);
    ASSERT_NE(0, func(nullptr, x.data<T>(), x.data<T>(), x.data<T>(), x.numel()));
    ASSERT_NE(0, func(&ctx, x.data<T>(), x.data<T>(), nullptr, x.numel()));
    ASSERT_NE(0, func(&ctx, x.data<T>(), x.data<T>(), x.data<T>(), -1));
}

template<typename T> static void gtest_common_binary_op(std::string funcname,
        std::function<int(api::Context*, const T*, const T*, T*, int)> func,
        api::DeviceType dev, const std::string& x_pos, const std::string& y_pos,
        const std::string& z_pos, int64_t len, float minv, float maxv, float maxdiff, float eps,
        int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM4(x_pos, y_pos, z_pos, len);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();

    auto f = [eps](float minval, float maxval, int64_t len) {
        api::Tensor ret = api::randfloat(minval, maxval, len);
        for (size_t i = 0; i < ret.numel(); ++i) {
            ret.data<float>()[i] += get_sign(ret.data<float>()[i]) * eps;
        }
        return ret;
    };
    GTEST_INIT_TENSOR(api::kINPUT, x, T, len, f, minv, maxv);
    GTEST_INIT_TENSOR(api::kINPUT, y, T, len, f, minv, maxv);
    GTEST_INIT_TENSOR(api::kOUTPUT, z, T, len, api::randfloat, minv, maxv);

    GTEST_REUSE_PTR_DEFINE();
    GTEST_DEFINE_PTR_WO(T, z_pos, z0, z1, z0ptr, z1ptr);
    GTEST_DEFINE_PTR_RO(T, x_pos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR_RO(T, y_pos, y0, y1, y0ptr, y1ptr);

    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func(&ctx_xpu, x1ptr, y1ptr, z1ptr, len));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, len * 3 * sizeof(T), "%s profiling", funcname.c_str());

    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0ptr, y0ptr, z0ptr, len));
    GTEST_CPU_END(&ctx_cpu);

    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(z0, z1, maxdiff, maxdiff);
    GTEST_CHECK_END();
}

#endif