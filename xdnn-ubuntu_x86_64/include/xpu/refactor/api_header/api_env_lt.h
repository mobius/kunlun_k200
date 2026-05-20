#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_API_HEADER_API_ENV_LT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_API_HEADER_API_ENV_LT_H
#include "xpu/dll_export.h"
#include <unordered_map>
#include <mutex>
namespace baidu {
namespace xpu {
namespace api {
class DLL_EXPORT EnvLT {
public:
    EnvLT() = delete;
    EnvLT(const EnvLT&) = delete;
    static std::unordered_map<std::string, std::string> env_caches;
    static std::mutex env_mutex;
    static const char* get_env(const char* name) {
        if (name == nullptr) {
            return nullptr;
        }
        std::string n(name);
        std::string value;
        {
            std::lock_guard<std::mutex> lock(env_mutex);
            auto res = env_caches.find(n);
            if (res == env_caches.end()) {
                // retry
                char* value_ptr = std::getenv(name);
                if (value_ptr != nullptr) {
                    // add to cache and return
                    value = std::string(value_ptr);
                    env_caches.emplace(n, value);
                } else {
                    return nullptr;
                }
            }
        }
        return env_caches[n].empty() ? nullptr : env_caches[n].c_str();
    }
};
} // namespace api
} // namespace xpu
} // namespace baidu
#endif // BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_API_HEADER_API_ENV_LT_H