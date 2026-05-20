#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_ACTIVATION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_ACTIVATION_H

#include "xpu/xdnn.h"
#include "xpu/refactor/gtest/xdnn_gtest.h"

template<typename T> static void gtest_common_activation(std::string funcname,
        std::function<int(api::Context*, const T*, T*, int, const float*, float*)> func,
        float diff, api::DeviceType dev, std::string xpos, std::string ypos, int64_t len,
        std::string xmax_pos, std::string ymax_pos, int l3size) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM6(xpos, ypos, len, xmax_pos, ymax_pos, l3size);
    GTEST_GEN_HASH_END();
    GTEST_INIT_TENSOR(api::kQINPUT, x, float, len, api::randfloat, -16.0f, 16.0f);
    GTEST_INIT_TENSOR(api::kQOUTPUT, y, float, len, api::randfloat, -16.0f, 16.0f);
    GTEST_INIT_QUANT_TENSOR(x, T);
    GTEST_INIT_QUANT_TENSOR(y, T);
    GTEST_REUSE_PTR_DEFINE();
    GTEST_DEFINE_QUANT_PTR_WO(T, ypos, ymax_pos, qy0, qy1, qy0ptr, qy1ptr, y0max, y1max);
    GTEST_DEFINE_QUANT_PTR_RO(T, xpos, xmax_pos, qx0, qx1, qx0ptr, qx1ptr, x0max, x1max);

    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func(&ctx_xpu, qx1ptr, qy1ptr, len, x1max, y1max));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, 2 * len * sizeof(T), "%s,len(%ld)", funcname.c_str(), len);

    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, qx0ptr, qy0ptr, len, x0max, y0max));
    GTEST_CPU_END(&ctx_cpu);

    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(qy0.val(), qy1.val(), diff, diff);
    GTEST_QUANT_TENSOR_MAXCLOSE(qy0.max(), qy1.max(), diff, diff);
    GTEST_CHECK_END();
}

template<typename T> static void gtest_common_activation_param(
        std::function<int(api::Context*, const T*, T*, int, const float*, float*)> func,
        api::DeviceType dev) {
    api::Context ctx(dev);
    api::Dtype dt = api::CPPTypeToDtype<T>();
    auto x = api::tensor(1.0f);
    auto qx = Quant(x, dt).to(dev);
    auto x_ptr = qx.val().data<T>();
    auto xmax_ptr = qx.max().data<float>();
    int64_t len = qx.val().numel();
    // nullptr
    ASSERT_NE(0, func(nullptr, x_ptr, x_ptr, len, xmax_ptr, xmax_ptr));
    ASSERT_NE(0, func(&ctx, x_ptr, nullptr, len, xmax_ptr, xmax_ptr));
    ASSERT_NE(0, func(&ctx, nullptr, x_ptr, len, xmax_ptr, xmax_ptr));
    if (!api::is_fp32_or_fp16_or_bfp16<T>()) {
        ASSERT_NE(0, func(&ctx, x_ptr, x_ptr, len, xmax_ptr, nullptr));
        ASSERT_NE(0, func(&ctx, x_ptr, x_ptr, len, nullptr, xmax_ptr));
    }
    // invalid len
    ASSERT_NE(0, func(&ctx, x_ptr, x_ptr, 0, xmax_ptr, xmax_ptr));
    ASSERT_NE(0, func(&ctx, x_ptr, x_ptr, -1, xmax_ptr, xmax_ptr));
}

template<typename T> static void gtest_common_param0_activation_grad(std::string funcname,
        std::function<int(api::Context*, const T*, T*, int, const float*, float*)> func,
        std::function<int(api::Context*, const T*, const T*, const T*, T*, int)> func_grad,
        float diff, api::DeviceType dev, std::string xpos, std::string ypos, std::string dypos, std::string dxpos,
        int64_t len, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM6(xpos, ypos, dypos, dxpos, len, l3size);
    GTEST_GEN_HASH_END();
    GTEST_INIT_TENSOR(api::kINPUT, x, T, len, api::randfloat, -16.0f, 16.0f);
    GTEST_INIT_TENSOR(api::kINPUT, y, T, len, api::randfloat, -16.0f, 16.0f);
    GTEST_INIT_TENSOR(api::kINPUT, dy, T, len, api::randfloat, -16.0f, 16.0f);
    GTEST_INIT_TENSOR(api::kOUTPUT, dx, T, len, api::randfloat, -16.0f, 16.0f);
    // init y0 using func
    GTEST_CPU_RUN_ONE_TIME_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0.data<T>(), y0.data<T>(), len, nullptr, nullptr));
    GTEST_CPU_RUN_ONE_TIME_END(&ctx_cpu);
    GTEST_REUSE_PTR_DEFINE();
    GTEST_DEFINE_PTR_WO(T, dxpos, dx0, dx1, dx0ptr, dx1ptr);
    GTEST_DEFINE_PTR_RO(T, dypos, dy0, dy1, dy0ptr, dy1ptr);
    GTEST_DEFINE_PTR(T, xpos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR(T, ypos, y0, y1, y0ptr, y1ptr);
    int dma_cnt = 4;
    if (xpos == "NULL") {
        dma_cnt--;
    }
    if (ypos == "NULL") {
        dma_cnt--;
    }
    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func_grad(&ctx_xpu, x1ptr, y1ptr, dy1ptr, dx1ptr, len));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, dma_cnt * len * sizeof(T), "%s,len(%ld)", funcname.c_str(), len);
    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func_grad(&ctx_cpu, x0ptr, y0ptr, dy0ptr, dx0ptr, len));
    GTEST_CPU_END(&ctx_cpu);
    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(dx0, dx1, diff, diff);
    GTEST_CHECK_END();
}

template<typename T> static void gtest_common_param0_activation_grad_param(
        std::function<int(api::Context*, const T*, const T*, const T*, T*, int)> func_grad,
        api::DeviceType dev, bool allow_x_null = false, bool allow_y_null = false) {
    api::Context ctx(dev);
    api::Dtype dt = api::CPPTypeToDtype<T>();
    auto x = api::tensor(1.0f).astype(dt).to(dev);
    // nullptr
    ASSERT_NE(0, func_grad(nullptr, x.data<T>(), x.data<T>(), x.data<T>(), x.data<T>(), x.numel()));
    if (!allow_x_null) {
        ASSERT_NE(0, func_grad(&ctx, nullptr, x.data<T>(), x.data<T>(), x.data<T>(), x.numel()));
    }
    if (!allow_y_null) {
        ASSERT_NE(0, func_grad(&ctx, x.data<T>(), nullptr, x.data<T>(), x.data<T>(), x.numel()));
    }
    ASSERT_NE(0, func_grad(&ctx, x.data<T>(), x.data<T>(), nullptr, x.data<T>(), x.numel()));
    ASSERT_NE(0, func_grad(&ctx, x.data<T>(), x.data<T>(), x.data<T>(), nullptr, x.numel()));
    // invalid len
    ASSERT_NE(0, func_grad(&ctx, x.data<T>(), x.data<T>(), x.data<T>(), x.data<T>(), 0));
    ASSERT_NE(0, func_grad(&ctx, x.data<T>(), x.data<T>(), x.data<T>(), x.data<T>(), -1));
}

template<typename T> static void gtest_common_param1_activation_grad(std::string funcname,
        std::function<int(api::Context*, const T*, T*, int, float, const float*, float*)> func,
        std::function<int(api::Context*, const T*, const T*, const T*, T*, int, float)> func_grad,
        float diff, api::DeviceType dev, std::string xpos, std::string ypos, std::string dypos, std::string dxpos,
        int64_t len, float param, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM6(xpos, ypos, dypos, dxpos, len, param);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();
    GTEST_INIT_TENSOR(api::kINPUT, x, T, len, api::randfloat, -16.0f, 16.0f);
    GTEST_INIT_TENSOR(api::kINPUT, y, T, len, api::randfloat, -16.0f, 16.0f);
    GTEST_INIT_TENSOR(api::kINPUT, dy, T, len, api::randfloat, -16.0f, 16.0f);
    GTEST_INIT_TENSOR(api::kOUTPUT, dx, T, len, api::randfloat, -16.0f, 16.0f);
    // init y0 using func
    GTEST_CPU_RUN_ONE_TIME_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0.data<T>(), y0.data<T>(), len, param, nullptr, nullptr));
    GTEST_CPU_RUN_ONE_TIME_END(&ctx_cpu);

    GTEST_REUSE_PTR_DEFINE();
    GTEST_DEFINE_PTR_WO(T, dxpos, dx0, dx1, dx0ptr, dx1ptr);
    GTEST_DEFINE_PTR_RO(T, dypos, dy0, dy1, dy0ptr, dy1ptr);
    GTEST_DEFINE_PTR(T, xpos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR(T, ypos, y0, y1, y0ptr, y1ptr);

    int dma_cnt = 4;
    if (xpos == "NULL") {
        dma_cnt--;
    }
    if (ypos == "NULL") {
        dma_cnt--;
    }
    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func_grad(&ctx_xpu, x1ptr, y1ptr, dy1ptr, dx1ptr, len, param));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, dma_cnt * len * sizeof(T), "%s,len(%ld)", funcname.c_str(), len);

    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func_grad(&ctx_cpu, x0ptr, y0ptr, dy0ptr, dx0ptr, len, param));
    GTEST_CPU_END(&ctx_cpu);

    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(dx0, dx1, diff, diff);
    GTEST_CHECK_END();
}

#endif
