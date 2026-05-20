#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_DEBUG_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_DEBUG_H

#include "xpu/refactor/context/newcontext.h"

namespace baidu {
namespace xpu {
namespace api {

// trace, checksum, npy, profiling
static bool debug_any_enable(Context* ctx) {
    if ((ctx->dev().type() == api::kCPU) && (ctx->max_debug_level() & 0xF0000) == 0) {
        return false;
    }
    return (ctx->max_debug_level() & 0xF0FFFF) != 0;
}

static bool debug_trace_enable(Context* ctx) {
    if (!debug_any_enable(ctx)) {
        return false;
    }
    return (ctx->debug_level() & 0xF) != 0;
}
static int debug_trace_level(Context* ctx) {
    if (!debug_any_enable(ctx)) {
        return 0;
    }
    return (ctx->debug_level() & 0xF);
}

static int debug_checksum_level(Context* ctx) {
    if (!debug_any_enable(ctx)) {
        return 0;
    }
    return (ctx->debug_level() & 0xF0) >> 4;
}
static bool debug_npy_enable(Context* ctx) {
    if (!debug_any_enable(ctx)) {
        return false;
    }
    return (ctx->debug_level() & 0xF00) != 0;
}
static int debug_profiling_level(Context* ctx) {
    if (!debug_any_enable(ctx)) {
        return 0;
    }
    return (ctx->debug_level() & 0xF000) >> 12;
}
static int debug_max_profiling_level(Context* ctx) {
    if (!debug_any_enable(ctx)) {
        return 0;
    }
    return (ctx->max_debug_level() & 0xF000) >> 12;
}
static bool debug_plan_enable(Context* ctx) {
    if (!debug_any_enable(ctx)) {
        return false;
    }
    return (ctx->debug_level() & 0xF00000) != 0;
}

}
}
}
#endif
