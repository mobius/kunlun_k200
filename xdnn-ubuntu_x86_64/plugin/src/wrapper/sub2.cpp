#include "xdnn_plugin.h"
#include "xpu/refactor/impl_public/wrapper_check.h"

namespace xpu2 {
__attribute__((global)) void sub1(const float* x, float* y, int len);
}
namespace api = baidu::xpu::api;

namespace plugin {

static int cpu_wrapper(api::Context* ctx, const float* x, float* y, int len) {
    for (int i = 0; i < len; i++) {
        y[i] = x[i] - 2.0f;
    }
    return api::SUCCESS;
}

static int xpu2_wrapper(api::Context* ctx, const float* x, float* y, int len) {
    api::ctx_guard RAII_GUARD(ctx);
    float* tensor_one = RAII_GUARD.alloc<float>(len);
    WRAPPER_ASSERT_WORKSPACE(ctx, tensor_one);
    int ret = api::constant<float>(ctx, tensor_one, len, 1.0f);
    WRAPPER_ASSERT_SUCCESS(ctx, ret);
    ret = api::sub<float>(ctx, x, tensor_one, y, len);
    WRAPPER_ASSERT_SUCCESS(ctx, ret);
    xpu2::sub1<<<ctx->ncluster(), 64, ctx->xpu_stream>>>(y, y, len);
    return api::SUCCESS;
}

int sub2(api::Context* ctx, const float* x, float* y, int len) {
    WRAPPER_CHECK_CTX(ctx);
    WRAPPER_DUMP_FUNCTION_T1(ctx, "sub2", float);
    WRAPPER_DUMP_PARAM4(ctx, x, y, len, ctx->_l3_mgr.get_size());
    WRAPPER_DUMP(ctx);
    WRAPPER_ASSERT_GT(ctx, len, 0);
    WRAPPER_CHECK_2PTRS(ctx, float, len, x, y);

    if (ctx->dev().type() == api::kCPU) {
        return cpu_wrapper(ctx, x, y, len);
    }
    if (ctx->dev().type() == api::kXPU2) {
        return xpu2_wrapper(ctx, x, y, len);
    }
    return api::NOT_IMPLEMENT;
}

}
