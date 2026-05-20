#include "xpu/xdnn.h"
#include "xpu/refactor/gtest/xdnn_gtest.h"
#include "xdnn_plugin.h"
template<typename T> static void gtest_add(api::DeviceType dev, const std::string& x_pos,
        const std::string& y_pos, int len, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
    api::Dtype dt = api::CPPTypeToDtype<T>();
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    float minv = -2.f;
    float maxv = 2.f;
    float maxdiff = 1e-5f;
    auto x0 = api::randfloat(minv, maxv, len).astype(dt);
    auto y0 = api::randfloat(minv, maxv, len).astype(dt);
    GTEST_DEFINE_PTR(T, x_pos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR(T, y_pos, y0, y1, y0ptr, y1ptr);

    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, plugin::add2(&ctx_xpu, x1ptr, y1ptr, len));
    //plugin::add2(&ctx_xpu, x1ptr, y1ptr, len);
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, len * 2 * sizeof(T), "add");

    GTEST_CPU_START(&ctx_cpu);
    //plugin::add2(&ctx_cpu, x0ptr, y0ptr, len);
    ASSERT_EQ(0, plugin::add2(&ctx_cpu, x0ptr, y0ptr, len));
    TENSOR_ALLCLOSE(y0, y1, maxdiff, maxdiff);
    GTEST_CPU_END(&ctx_cpu);
}

TEST(test_add, function_xpu2) {
    gtest_add<float>(api::kXPU2, GM, GM, 12345);
}


