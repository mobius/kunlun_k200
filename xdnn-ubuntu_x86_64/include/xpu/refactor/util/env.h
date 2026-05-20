#ifndef BAIDU_XPU_API_INCLUDE_XPU_ENV_H
#define BAIDU_XPU_API_INCLUDE_XPU_ENV_H

#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <xpu/runtime.h>
#include "xpu/xdnn_error.h"
#include <iostream>

namespace baidu {
namespace xpu {
namespace api {
/*
 * ------------ key ------------ -------------- value ---------------
 * XPUSIM_DEVICE_MODEL          | string  |  [KUNLUN1, KUNLUN2]
 * XPUAPI_DEBUG                 | int     |  [0, 1]
 * XPU_CONV_AUTOTUNE_FILE       | string  |  "AnyNameYouLike"
 * XPU_CONV_AUTOTUNE            | int     |  [1, 2, 3, 4, 5, ...]
 * XPU_CONV_AUTOTUNE_L3_ALIGN   | int     |  [1, 2, 3, 4, 5, ...]
 * XPUAPI_DEFAULT_SIZE          | size_t  |  [1, 2, 3, 4, 5, ...]
 * XPU_FC_INFERENCE_ACCELERATE  | int     |  [0, 1]
 * ----------------------------- ------------------------------------
 */
class XPUEnv {
public:
    XPUEnv() : kl2_env("KUNLUN2") {
        init_env();
    }
    const char* get_env(const char* name) {
        if (name == nullptr) {
            return nullptr;
        }
        std::string n(name);
        auto res = env_caches.find(n);
        if (res == env_caches.end()) {
            return nullptr;
        } else {
            return env_caches[n].empty() ? nullptr : env_caches[n].c_str();
        }
    }
    int set_env(const char* name, const char* value) {
        if (name == nullptr || value == nullptr) {
            return api::INVALID_PARAM;
        }
        std::string n(name);
        auto res = env_caches.find(n);
        if (res != env_caches.end()) {
            env_caches[n] = std::string(value);
            return api::SUCCESS;
        }
        return api::NOT_IMPLEMENT;
    }
    
private:
    void init_env() {
        std::vector<const char*> env_names = {
            // please add the name of xpu environment variable here if necessary
            "XPUSIM_DEVICE_MODEL",
            "XPUAPI_DEBUG",
            "XPUAPI_DEBUG_CUSTOM_OP_LEVEL",
            "XPU_CONV_AUTOTUNE_FILE",
            "XPU_CONV_AUTOTUNE",
            "XPU_CONV_AUTOTUNE_L3_ALIGN",
            "XPU_CONV_AUTOTUNE_KERNEL_CNT",
            "XPUAPI_DEFAULT_SIZE",
            "XPU_AUTOTUNE_WRITEBACK",
            "XDNN_LOG_FILE",
            "XPU_FC_AUTOTUNE",
            "XPU_FC_AUTOTUNE_FILE",
            "XPU_FC_AUTOTUNE_WRITEBACK",
            "XPU_FC_INFERENCE_ACCELERATE",
            "XPU_NPY_OP_FILTER",
            "XPU_PRECISION_MODE",
            "XPU_SOFTMAX_OPT"
        };

        for (auto name : env_names) {
            std::string n(name);
            std::string value;
            char* value_ptr = std::getenv(name);
            if (value_ptr != nullptr) {
                value = std::string(value_ptr);
            }
            env_caches.emplace(n, value);
        }

#ifndef _MSC_VER
        // TODO: Remove All pre-refactor wrapper in Paddle-Lite
        auto dev_env = env_caches["XPUSIM_DEVICE_MODEL"];
        if (dev_env.empty() || std::strcmp(dev_env.c_str(), "KUNLUN2") != 0) {
            int cur_dev_idx = 0;
            uint64_t cur_dev_attr = 0;
            int ret = xpu_current_device(&cur_dev_idx);
            if (ret != 0) {
                fprintf(stderr, "[INFO][XPUAPI][No XPU Device Found]");
            }
            ret = xpu_device_get_attr(&cur_dev_attr, XPUATTR_MODEL, cur_dev_idx);
            if (ret != 0) {
                fprintf(stderr, "[INFO][XPUAPI][Invalid Device Attr 'XPUATTR_MODEL']");
            }
            if (cur_dev_attr == R200) {
                env_caches["XPUSIM_DEVICE_MODEL"] = kl2_env;
            }
        }
#endif
    }

    std::unordered_map<std::string, std::string> env_caches;
    const std::string kl2_env;
};

}
}
}

#endif // BAIDU_XPU_API_INCLUDE_XPU_ENV_H
