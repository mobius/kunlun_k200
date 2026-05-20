#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_MATH_CUSTOMIZATION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_MATH_CUSTOMIZATION_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {

template<typename TW, typename TH = int> DLL_EXPORT int histogram(Context* ctx,
        const TH* x, const TW* weight, TH* y, TW* weight_y, int64_t xlen, int64_t ylen);

}
}
}
#endif

