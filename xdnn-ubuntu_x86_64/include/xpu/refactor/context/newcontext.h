#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CONTEXT_NEWCONTEXT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CONTEXT_NEWCONTEXT_H
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <xpu/runtime.h>
#include "xpu/dll_export.h"
#include "xpu/refactor/core/device.h"
#include "xpu/refactor/core/dtype.h"
#include "xpu/refactor/context/buffer_mgr.h"
#include "xpu/refactor/util/env.h"
#include "xpu/refactor/context/xpu_act_type.h"
#include "xpu/refactor/context/launch_cfg.h"

namespace baidu {
namespace xpu {
namespace api {

using GMAllocFunc = std::function<void*(size_t)>;
using GMSaveFunc = std::function<void(void)>;
using GMFreeFunc = std::function<void(void)>;

class ContextImpl;
struct DLL_EXPORT Context {
private:
    static constexpr int _nsdnn_list[5] = {-1, 4, 6, 12, 8};
    static constexpr int _ncluster_list[5] = {-1, 4, 8, 12, 8};
    static constexpr int _max_ptr_size_list[5] = {1, 4, 6, 12, 8};
    static constexpr int _default_gm_mb[5] = {0, 64, 128, 128, 128};
public:
    Context(Device in_dev);
    ~Context();
    Context(const Context& other) = delete;
    const char* get_option(const char* name);
    int set_option(const char* name, const char* value);
    int set_type_compared_to_cpu(DeviceType type) {
        return _dev.set_type_compared_to_cpu(type);
    }
    DeviceType type_compared_to_cpu_value() const {
        return _dev.type_compared_to_cpu_value();
    }
    int debug_level() const;
    int default_debug_level() const {
        return _debug_level;
    }
    Device dev() const {
        return _dev;
    }
    int set_debug_level(int lv);
    int max_debug_level() const {
        return _max_debug_level;
    }
    int ncluster() const {
        return _ncluster;
    }
    int set_ncluster(int n) {
        int list_index = (int)(_dev.type());
        int max_ncluster = _ncluster_list[list_index];
        if (n >= 1 && n <= max_ncluster) {
            _ncluster = n;
            return 0;
        } else {
            return -1;
        }
    }
    int nsdnn() const {
        return _nsdnn;
    }
    int set_nsdnn(int n) {
        int list_index = (int)(_dev.type());
        int max_nsdnn = _nsdnn_list[list_index];
        if (n >= 1 && n <= max_nsdnn) {
            _nsdnn = n;
            return 0;
        } else {
            return -1;
        }
    }
    int max_ptr_size() const {
        return _max_ptr_size;
    }
    void set_stream(XPUStream s, bool destroy_new_stream_at_destruction = false) {
        if (xpu_stream != nullptr && destroy_stream_at_destruction) {
            xpu_stream_destroy(xpu_stream);
        }
        xpu_stream = s;
        destroy_stream_at_destruction = destroy_new_stream_at_destruction;
    }
    XPUStream get_stream() {
        return xpu_stream;
    }
    int custom_debug_level(const std::string& func_name = "") const {
        if (func_name == "") {
            return _debug_level;
        }
        auto it = _custom_op_debug_level.find(func_name);
        if (it != _custom_op_debug_level.end()) {
            return it->second;
        } else {
            return _debug_level;
        }
    }
    void set_overload_alloc(GMAllocFunc overload_alloc, GMFreeFunc overload_free, GMSaveFunc overload_save=nullptr) {
        overload_alloc_gm = overload_alloc;
        overload_free_gm = overload_free;
        overload_save_gm = overload_save;
    }
    void unset_overload_alloc() {
        overload_alloc_gm = nullptr;
        overload_free_gm = nullptr;
        overload_save_gm = nullptr;
    }

    ContextImpl* impl;
    BufferMgr _gm_mgr;
    BufferMgr _l3_mgr;
private:
    Device _dev;
    XPUEnv* xpu_env;
    int update_env();
    int _debug_level;
    int _max_debug_level;
    int _nsdnn;
    int _ncluster;
    int _max_ptr_size;
    std::unordered_map<std::string, int> _custom_op_debug_level;
    bool destroy_stream_at_destruction;
public:
    XPUStream xpu_stream = nullptr;
    // deprecated fields
    bool enable_multi_stream = false;
    int batch_split_type = -1;
    bool qkv_fusion = false;
    GMAllocFunc overload_alloc_gm;
    GMFreeFunc overload_free_gm;
    GMSaveFunc overload_save_gm;
};

DLL_EXPORT Context* create_context();
DLL_EXPORT void destroy_context(Context* ctx);

class DLL_EXPORT ctx_guard {
private:
    Context* _ctx;
    template<class T> friend class VectorParam;
    std::vector<void*> cpu_vec;
    GMAllocFunc overload_alloc_gm;
    GMFreeFunc overload_free_gm;
    template <typename T> T* alloc_cpu(size_t cnt) {
        void* ptr = malloc(sizeof(T) * cnt);
        if (ptr != nullptr) {
            cpu_vec.push_back(ptr);
        } else {
            return nullptr;
            //throw std::bad_alloc();
        }
        return (T*)(ptr);
    }
public:
    void set_overload_alloc(GMAllocFunc overload_alloc, GMFreeFunc overload_free) {
        overload_alloc_gm = overload_alloc;
        overload_free_gm = overload_free;
    }
    void unset_overload_alloc() {
        overload_alloc_gm = nullptr;
        overload_free_gm = nullptr;
    }
    ctx_guard(const ctx_guard&) = delete;
    ctx_guard& operator=(const ctx_guard&) = delete;
    ctx_guard(Context* ctx) {
        _ctx = ctx;
        if (_ctx->overload_alloc_gm) {
            set_overload_alloc(_ctx->overload_alloc_gm, _ctx->overload_free_gm);
            if (_ctx->overload_save_gm) {
                _ctx->overload_save_gm();
            }
        }
        if (_ctx) {
            _ctx->_gm_mgr.save();
            _ctx->_l3_mgr.save();
        }
    }
    ~ctx_guard() {
        if (overload_free_gm) {
            overload_free_gm();
        }
        if (_ctx) {
            _ctx->_gm_mgr.restore(_ctx->xpu_stream);
            _ctx->_l3_mgr.restore(_ctx->xpu_stream);
        }
        for (auto ptr : cpu_vec) {
            free(ptr);
        }
    }
    template<typename T> T* alloc_l3(size_t cnt) {
        if (_ctx) {
            if (_ctx->dev().type() == api::kCPU) {
                return alloc_cpu<T>(cnt);
            }
            return static_cast<T*>(_ctx->_l3_mgr.alloc(_ctx->impl, sizeof(T) * cnt, _ctx->xpu_stream));
        }
        return nullptr;
    }
    template<typename T> T* alloc(size_t cnt) {
        if (_ctx) {
            if (_ctx->dev().type() == api::kCPU) {
                return alloc_cpu<T>(cnt);
            }
            if (overload_alloc_gm) {
                return static_cast<T*>(overload_alloc_gm(cnt * sizeof(T)));
            }
            return static_cast<T*>(_ctx->_gm_mgr.alloc(_ctx->impl, sizeof(T) * cnt, _ctx->xpu_stream));
        }
        return nullptr;
    }
    template<typename T> T* alloc_l3_or_gm(size_t cnt) {
        if (_ctx) {
            if (_ctx->dev().type() == api::kCPU) {
                return alloc_cpu<T>(cnt);
            }
            T* ptr = static_cast<T*>(_ctx->_l3_mgr.alloc(_ctx->impl, sizeof(T) * cnt, _ctx->xpu_stream));
            if (ptr != nullptr) {
                return ptr;
            } else {
                if (overload_alloc_gm) {
                    return static_cast<T*>(overload_alloc_gm(cnt * sizeof(T)));
                }
                return static_cast<T*>(_ctx->_gm_mgr.alloc(_ctx->impl, sizeof(T) * cnt, _ctx->xpu_stream));
            }
        }
        return nullptr;
    }
};

DeviceType get_dev_type();

}
}
}
#endif
