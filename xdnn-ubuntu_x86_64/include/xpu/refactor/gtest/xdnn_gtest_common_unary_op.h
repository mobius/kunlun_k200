#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_UNARY_OP_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_UNARY_OP_H

#include "xpu/xdnn.h"
#include "xpu/refactor/gtest/xdnn_gtest.h"

static int get_sign(float v) {
    if (v >= 0) {
        return 1;
    } else {
        return -1;
    }
}

template<typename T> static void unary_param(std::function<int(api::Context*, const T*, T*, int)> func,
        api::DeviceType dev) {
    api::Context ctx(dev);
    api::Dtype dt = api::CPPTypeToDtype<T>();
    auto x = api::tensor(1.0f).astype(dt).to(dev);
    ASSERT_NE(0, func(nullptr, x.data<T>(), x.data<T>(), x.numel()));
    ASSERT_NE(0, func(&ctx, x.data<T>(), nullptr, x.numel()));
    ASSERT_NE(0, func(&ctx, x.data<T>(), x.data<T>(), -1));
}

// not_check: 0 --> x, 1 --> y
template<typename T> static void unary_grad_param(
        std::function<int(api::Context*, const T*, const T*, const T*, T*, int)> func,
        api::DeviceType dev, int not_check) {
    api::Context ctx(dev);
    api::Dtype dt = api::CPPTypeToDtype<T>();
    auto x = api::tensor(1.0f).astype(dt).to(dev);
    ASSERT_NE(0, func(nullptr, x.data<T>(), x.data<T>(), x.data<T>(), x.data<T>(), x.numel()));
    ASSERT_NE(0, func(&ctx, x.data<T>(), x.data<T>(), nullptr, x.data<T>(), x.numel()));
    ASSERT_NE(0, func(&ctx, x.data<T>(), x.data<T>(), x.data<T>(), nullptr, x.numel()));
    ASSERT_NE(0, func(&ctx, x.data<T>(), x.data<T>(), x.data<T>(), x.data<T>(), -1));
    if (not_check != 0) {
        ASSERT_NE(0, func(&ctx, nullptr, x.data<T>(), x.data<T>(), x.data<T>(), x.numel()));
    }
    if (not_check != 1) {
        ASSERT_NE(0, func(&ctx, x.data<T>(), nullptr, x.data<T>(), x.data<T>(), x.numel()));
    }
}

template<typename T>
static void gtest_common_unary_op(std::string funcname,
        std::function<int(api::Context*, const T*, T*, int)> func,
        api::DeviceType dev, const std::string& xpos, const std::string& ypos, int64_t len,
        float minv, float maxv, float maxdiff, float eps = 0.0f, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM6(xpos, ypos, len, minv, maxv, maxdiff);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();
    GTEST_INIT_TENSOR(api::kINPUT, x, T, len, api::randfloat, minv, maxv);
    GTEST_INIT_TENSOR(api::kOUTPUT, y, T, len, api::randfloat, minv, maxv);
    GTEST_CPU_RUN_ONE_TIME_START(&ctx_cpu);
    for (int64_t j = 0; j < x0.numel(); ++j) {
        x0.data<T>()[j] += get_sign(x0.data<T>()[j]) * eps;
    }
    GTEST_CPU_RUN_ONE_TIME_END(&ctx_cpu);
    GTEST_REUSE_PTR_DEFINE();
    GTEST_DEFINE_PTR_WO(T, ypos, y0, y1, y0ptr, y1ptr);
    GTEST_DEFINE_PTR_RO(T, xpos, x0, x1, x0ptr, x1ptr);
    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func(&ctx_xpu, x1ptr, y1ptr, len));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, len * 2 * sizeof(T), "%s profiling", funcname.c_str());
    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0ptr, y0ptr, len));
    GTEST_CPU_END(&ctx_cpu);
    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(y0, y1, maxdiff, maxdiff);
    GTEST_CHECK_END();
}

template<typename T>
static void gtest_common_unary_op_grad(std::string funcname,
        std::function<int(api::Context*, const T*, const T*, const T*, T*, int)> func,
        api::DeviceType dev, const std::string& xpos, const std::string& ypos, const std::string& dxpos,
        const std::string& dypos,
        int64_t len, float minv, float maxv, float maxdiff, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM6(xpos, ypos, dxpos, dypos, len, minv);
    GTEST_GEN_HASH_PARAM1(maxv);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();
    GTEST_INIT_TENSOR(api::kINPUT, x, T, len, api::randfloat, minv, maxv);
    GTEST_INIT_TENSOR(api::kINPUT, y, T, len, api::randfloat, minv, maxv);
    GTEST_INIT_TENSOR(api::kINPUT, dy, T, len, api::randfloat, minv, maxv);
    GTEST_INIT_TENSOR(api::kOUTPUT, dx, T, len, api::randfloat, minv, maxv);
    GTEST_REUSE_PTR_DEFINE();
    GTEST_DEFINE_PTR_WO(T, dxpos, dx0, dx1, dx0ptr, dx1ptr);
    GTEST_DEFINE_PTR_RO(T, dypos, dy0, dy1, dy0ptr, dy1ptr);
    GTEST_DEFINE_PTR_RO(T, xpos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR_RO(T, ypos, y0, y1, y0ptr, y1ptr);
    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func(&ctx_xpu, x1ptr, y1ptr, dy1ptr, dx1ptr, len));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, len * 2 * sizeof(T), "%s profiling", funcname.c_str());
    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0ptr, y0ptr, dy0ptr, dx0ptr, len));
    GTEST_CPU_END(&ctx_cpu);
    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(dx0, dx1, maxdiff, maxdiff);
    GTEST_CHECK_END();
}

#endif