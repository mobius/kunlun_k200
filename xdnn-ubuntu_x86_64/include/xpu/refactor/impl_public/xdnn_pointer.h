#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_POINTER_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_POINTER_H

#include "xpu/refactor/context/newcontext.h"

namespace baidu {
namespace xpu {
namespace api {

enum class pointer_enum {
    INVALID = 0,
    GM = 1,
    L3 = 2,
    NIL = 3,
    HOST = 4,
};

constexpr pointer_enum kINVALID = pointer_enum::INVALID;
constexpr pointer_enum kGM = pointer_enum::GM;
constexpr pointer_enum kL3 = pointer_enum::L3;
constexpr pointer_enum kNIL = pointer_enum::NIL;
constexpr pointer_enum kHOST = pointer_enum::HOST;

static pointer_enum pointer_type(DeviceType dev, const void* ptr) {
    unsigned int ptr_high = (((unsigned long long) ptr) >> 32);
    unsigned int ptr_low = (((unsigned long long) ptr));
    if (ptr_high == 0 && ptr_low == 0) {
        return kNIL;
    }
    if (dev == api::kCPU) {
        return kHOST;
    }
    if (dev == api::kXPU1) {
        if (ptr_high == 0 || ptr_high == 4) {
            if (ptr_low >= 0xC0000000 && ptr_low <= 0xC0FFFFFF) {
                return kL3;
            }
        }
        if (ptr_high == 2 || ptr_high == 3 || ptr_high == 6 || ptr_high == 7) {
            return kGM;
        }
    }
    if (dev == api::kXPU2) {
        if (ptr_high == 0 && ptr_low >= 0xC0000000 && ptr_low <= 0xC3FFFFFF) {
            return kL3;
        }
        if (ptr_high >= 8 && ptr_high <= 15) {
            return kGM;
        }
    }
    if (dev == api::kXPU3 || dev == api::kXPU4) { // todo: check xpu4
        if (ptr_high == 0 && ptr_low >= 0x90000000 && ptr_low <= 0x95FFFFFF) {
            return kL3;
        }
        unsigned int ptr_high_filtered = (ptr_high & 0x7f);     // address: 0~38bit
        if (ptr_high_filtered >= 0x40 && ptr_high_filtered < 0x80) {
            return kGM;
        }
    }
    return kINVALID;
}

static pointer_enum pointer_type(Context* ctx, const void* ptr) {
    DeviceType dev = ctx->dev().type();
    return pointer_type(dev, ptr);
}

}
}
}
#endif
