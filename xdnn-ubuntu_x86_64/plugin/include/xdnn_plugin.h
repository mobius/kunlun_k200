#ifndef XDNN_PLUGIN_H
#define XDNN_PLUGIN_H
#include "xpu/xdnn.h"

namespace plugin {
// y[i] = x[i] + 2
DLL_EXPORT int add2(baidu::xpu::api::Context* ctx, const float* x, float* y, int len);
// y[i] = x[i] - 2
DLL_EXPORT int sub2(baidu::xpu::api::Context* ctx, const float* x, float* y, int len);

}
#endif

