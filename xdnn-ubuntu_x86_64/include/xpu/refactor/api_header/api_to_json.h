#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_API_HEADER_API_TO_JSON_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_API_HEADER_API_TO_JSON_H

#include "xpu/refactor/impl_public/xdnn_to_str.h"
#include "xpu/refactor/impl_public/json.h"
#include "xpu/xdnn_types.h"
#include <sstream>
#include <string>
#include <vector>
namespace baidu {
namespace xpu {
namespace api {
///////////////////////////////// api_param_to_json /////////////////////////////////////////
template <typename T>
inline json::Json api_param_to_json(DeviceType dev, T v) { return json::Json(v); }
inline json::Json api_param_to_json(DeviceType dev, Activation_t& v) {
    return v.to_json();
}

template <> inline json::Json api_param_to_json<float>(DeviceType dev, float v) {
    std::stringstream sstream;
    bool issue = false;
    if (std::isinf(v)) {
        if (v > 0) {
            sstream << "INFINITY";
        } else {
            sstream << "-INFINITY";
        }
        issue = true;
    } else if (std::isnan(v)) {
        sstream << "NAN";
        issue = true;
    } else {
        sstream << v;
    }
    if (!issue) {
        return json::Json(v);
    }
    return json::Json(sstream.str());
}

template <typename T> inline json::Json api_param_to_json(DeviceType dev, T* ptr) {
    auto ktype = pointer_type(dev, (const void*)ptr);
    std::string res;
    if (ktype == kNIL) {
        res = "NULL";
    } else if (ktype == kL3) {
        res = "L3";
    } else if (ktype == kGM) {
        res = "GM";
    } else if (ktype == kHOST) {
        res ="CPU";
    } else {
        res = "INVALID";
    }
    return api_param_to_json(dev, res);
}

template <typename T> inline json::Json api_param_to_json(DeviceType dev, const T* ptr) {
    return api_param_to_json(dev, const_cast<T*>(ptr));
}
template <typename T> inline json::Json api_param_to_json(DeviceType dev, VectorParam<T> vp) {
    std::string ret = "{";
    if (vp.cpu != nullptr) {
        if (vp.len > 0) {
            ret = ret + std::to_string(vp.cpu[0]);
        }
        for (int i = 1; i < vp.len; i++) {
            ret = ret + ", " + std::to_string(vp.cpu[i]);
        }
        ret = ret + "}";
    } else {
        // due to the cpu info is too long, use this branch
        // to support more in future
        ret = "\"cpu_nullptr\"";
    }
    return api_param_to_json(dev, ret);
}
} // namespace api
} // namespace xpu
} // namespace baidu
#endif
