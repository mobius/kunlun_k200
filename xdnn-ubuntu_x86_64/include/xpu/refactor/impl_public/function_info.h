#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_FUNCTION_INFO_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_FUNCTION_INFO_H
#include <unordered_map>
#include <string>
#include <vector>
#include "xpu/refactor/context/newcontext.h"
#include "xpu/refactor/impl_public/xdnn_to_str.h"
#include "xpu/refactor/impl_public/json.h"
#include "xpu/refactor/core/xdnn_version.h"
#include "xpu/refactor/api_header/api_to_json.h"
namespace baidu {
namespace xpu {
namespace api {

struct function_info {
    json::Json::object param_raw_json_obj;
    json::Json::object extra_info_raw_json_obj;
    std::vector<std::string> type_raw_vec;
    std::unordered_map<void*, int> ptr_param_table;
    int ptr_param_idx;
    std::vector<std::string> param_vec;
    std::string fname;
    std::vector<std::string> template_vec;
    DeviceType cur_device;
    function_info() {
        ptr_param_idx = 0;
        param_raw_json_obj = json::Json::object({});
    }
    function_info(DeviceType dev) : function_info() { cur_device = dev; }
    template <typename T> inline void add_param(std::string key, T value) {
        param_raw_json_obj.push_back({key, api_param_to_json(cur_device, value)});
    }

    // 添加额外信息
    template <typename T> inline void add_extra_info(std::string key, T value) {
        extra_info_raw_json_obj.push_back({key, api_param_to_json(cur_device, value)});
    }

    template <typename T> inline void add_param(Context* _ctx, std::string key, T value) {
        param_raw_json_obj.push_back({key, api_param_to_json( _ctx->dev().type(), value)});
    }
    template <typename T> inline void add_param(Context* _ctx, T param) {
        param_vec.push_back(xdnn_param_to_str(_ctx, param));
    }
    inline void add_param(Context* _ctx, const BaseAttnParam& param) {
        param_vec.push_back(xdnn_param_to_str(_ctx, param));
    }
    template <typename T> inline void add_param(Context* _ctx, T* ptr) {
        void* _ptr = static_cast<void*>(ptr);
        if (ptr_param_table.find(_ptr) != ptr_param_table.end()) {
            int first_idx = ptr_param_table[_ptr];
            if (param_vec[first_idx].size() <= 4) {   // only occurs once before
                param_vec[first_idx] = param_vec[first_idx].substr(0, 3)
                        + "_" + std::to_string(ptr_param_idx++) + "\"";
            }
            param_vec.push_back(param_vec[first_idx]);
        } else {   // occurs for the first time
            param_vec.push_back(xdnn_param_to_str(_ctx, ptr));
            if (ptr != nullptr) {
                ptr_param_table[_ptr] = param_vec.size() - 1;
            }
        }
    }
    template <typename T> inline void add_param(Context* _ctx, std::vector<T*> ptr_vec) {
        std::vector<std::string> str_vec;
        param_vec.push_back("{");
        for (size_t i = 0; i < ptr_vec.size(); ++i) {
            add_param(_ctx, ptr_vec[i]);
        }
        param_vec.push_back("}");
    }
    template <typename T> inline void add_param(Context* _ctx, const T* ptr) {
        add_param<T>(_ctx, const_cast<T*>(ptr));
    }
    template <typename T> inline void add_param(Context* _ctx, char* ptr) {
        std::string char2string;
        char2string.push_back('"');
        for (int i = 0; i < strlen(ptr); i++) {
            char2string.push_back(*(ptr + i));
        }
        char2string.push_back('"');
        param_vec.push_back(char2string);
    }
    std::string get_trace(Context* _ctx) {
        std::string ret = "";
        if (template_vec.size() == 0) {
            template_vec.push_back("unknown");
        }
        ret += "gtest_";
        ret += fname;
        ret += "<";
        ret += template_vec[0];
        for (int i = 1; i < template_vec.size(); i++) {
            ret += ", ";
            ret += template_vec[i];
        }
        ret += ">(api::";
        ret += to_string(_ctx->dev().type());
        for (int i = 0; i < param_vec.size(); i++) {
            if ((i == 0 || param_vec[i - 1] != "{") && param_vec[i] != "}") {
                ret += ", ";
            }
            ret += param_vec[i];
        }
        ret += ");";
        return ret;
    }
    std::string get_trace_json(Context* _ctx) {
        json::Json::object raw_out;
        raw_out.push_back({"op", fname});
        raw_out.push_back({"types", type_raw_vec});
        raw_out.push_back({"params", param_raw_json_obj});
        raw_out.push_back({"dev", to_string(_ctx->dev().type())}); // add dev type
        raw_out.push_back({"l3_size", api_param_to_json(_ctx->dev().type(), _ctx->_l3_mgr.get_size())}); // add l3_size
        raw_out.push_back({"extra", extra_info_raw_json_obj}); // add extra info
        json::Json json_out = raw_out;
        return json_out.dump();    
    }
    std::string get_trace_json() {
        json::Json::object raw_out;
        raw_out.push_back({"op", fname});
        raw_out.push_back({"types", type_raw_vec});
        raw_out.push_back({"params", param_raw_json_obj});
        raw_out.push_back({"dev", to_string(cur_device)}); // add dev type
        raw_out.push_back({"extra", extra_info_raw_json_obj}); // add extra info
        json::Json json_out = raw_out;
        return json_out.dump();    
    }
};

}
}
}
#endif
