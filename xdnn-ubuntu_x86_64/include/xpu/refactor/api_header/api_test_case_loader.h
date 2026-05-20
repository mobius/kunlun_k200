#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_API_HEADER_API_TEST_CASE_LOADER_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_API_HEADER_API_TEST_CASE_LOADER_H
#include "xpu/xdnn_types.h"
#include "xpu/refactor/impl_public/json.h"
#include "xpu/refactor/api_header/api_env_lt.h"
#include "xpu/refactor/api_header/api_logging_lt.h"
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
namespace baidu {
namespace xpu {
namespace api {
// 用来声明单测支持的类型
#define DECLARE_API_JSON_TESTS            static void _api_json_tests(json::Json& obj)
// 辅助函数
#define TYPE_COMPARE_HELPER(T) obj["types"][_inner_idx++].string_value() == DECLARE_API_TYPE_TO_STRING_NEW(T) &&
#define TYPE_COMPARE_HELPER_TAIL(T) obj["types"][_inner_idx].string_value() == DECLARE_API_TYPE_TO_STRING_NEW(T)
// 辅助多 type 类型的 op 的测试函数
#define API_DO_0_TYPE_JSON_TEST(func)                                                                                  \
    { func(obj); }
#define API_DO_1_TYPE_JSON_TEST(T1, func)                                                                              \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (TYPE_COMPARE_HELPER_TAIL(T1)) {                                                                            \
            func<T1>(obj);                                                                                             \
        }                                                                                                              \
    }
#define API_DO_2_TYPE_JSON_TEST(T1, T2, func)                                                                          \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (WRAPPER_NESTED_DEFINE(TYPE_COMPARE_HELPER, T1) TYPE_COMPARE_HELPER_TAIL(T2)) {                             \
            func<T1, T2>(obj);                                                                                         \
        }                                                                                                              \
    }
#define API_DO_3_TYPE_JSON_TEST(T1, T2, T3, func)                                                                      \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (WRAPPER_NESTED_DEFINE(TYPE_COMPARE_HELPER, T1, T2) TYPE_COMPARE_HELPER_TAIL(T3)) {                         \
            func<T1, T2, T3>(obj);                                                                                     \
        }                                                                                                              \
    }
#define API_DO_4_TYPE_JSON_TEST(T1, T2, T3, T4, func)                                                                  \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (WRAPPER_NESTED_DEFINE(TYPE_COMPARE_HELPER, T1, T2, T3) TYPE_COMPARE_HELPER_TAIL(T4)) {                     \
            func<T1, T2, T3, T4>(obj);                                                                                 \
        }                                                                                                              \
    }
#define API_DO_5_TYPE_JSON_TEST(T1, T2, T3, T4, T5, func)                                                              \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (WRAPPER_NESTED_DEFINE(TYPE_COMPARE_HELPER, T1, T2, T3, T4) TYPE_COMPARE_HELPER_TAIL(T5)) {                 \
            func<T1, T2, T3, T4, T5>(obj);                                                                             \
        }                                                                                                              \
    }
#define API_DO_6_TYPE_JSON_TEST(T1, T2, T3, T4, T5, T6, func)                                                          \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (WRAPPER_NESTED_DEFINE(TYPE_COMPARE_HELPER, T1, T2, T3, T4, T5) TYPE_COMPARE_HELPER_TAIL(T6)) {             \
            func<T1, T2, T3, T4, T5, T6>(obj);                                                                         \
        }                                                                                                              \
    }

#define API_DO_7_TYPE_JSON_TEST(T1, T2, T3, T4, T5, T6, T7, func)                                                      \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (WRAPPER_NESTED_DEFINE(TYPE_COMPARE_HELPER, T1, T2, T3, T4, T5, T6) TYPE_COMPARE_HELPER_TAIL(T7)) {         \
            func<T1, T2, T3, T4, T5, T6, T7>(obj);                                                                     \
        }                                                                                                              \
    }

#define API_DO_8_TYPE_JSON_TEST(T1, T2, T3, T4, T5, T6, T7, T8, func)                                                  \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (WRAPPER_NESTED_DEFINE(TYPE_COMPARE_HELPER, T1, T2, T3, T4, T5, T6, T7) TYPE_COMPARE_HELPER_TAIL(T8)) {     \
            func<T1, T2, T3, T4, T5, T6, T7, T8>(obj);                                                                 \
        }                                                                                                              \
    }

#define API_DO_9_TYPE_JSON_TEST(T1, T2, T3, T4, T5, T6, T7, T8, T9, func)                                              \
    {                                                                                                                  \
        int _inner_idx = 0;                                                                                            \
        if (WRAPPER_NESTED_DEFINE(TYPE_COMPARE_HELPER, T1, T2, T3, T4, T5, T6, T7, T8) TYPE_COMPARE_HELPER_TAIL(T9)) { \
            func<T1, T2, T3, T4, T5, T6, T7, T8, T9>(obj);                                                             \
        }                                                                                                              \
    }

std::string get_pure_file_name(const std::string& full_path);

/// @brief 加载json测试用例, 实际使用请调用 API_DYNAMIC_JSON_LOADER
/// @param target_file 指定json文件名，不填则自动以 当前文件名.json 作为文件名，也可使用环境变量 XPUAPI_TEST_JSON_PATH
/// 指定文件名
#define API_DYNAMIC_JSON_LOADER(target_file)                                                                           \
    {                                                                                                                  \
        std::string target_file_str = target_file;                                                                     \
        std::string path = "./json_cases/";                                                                            \
        const char* overwrite_path = api::EnvLT::get_env("XPUAPI_TEST_JSON_PATH");                                     \
        if (overwrite_path != nullptr) {                                                                               \
            path = overwrite_path;                                                                                     \
            LOGI("[API JSON LOADER] 通过环境变量指定目标文件: %s\n", path.c_str());                                    \
        } else if (target_file_str != "") {                                                                            \
            path += target_file_str;                                                                                   \
        } else {                                                                                                       \
            std::string full_path = __FILE__;                                                                          \
            path += api::get_pure_file_name(full_path);                                                                \
            path += ".json";                                                                                           \
        }                                                                                                              \
        std::ifstream file(path);                                                                                      \
        if (file.is_open()) {                                                                                          \
            std::string line;                                                                                          \
            while (std::getline(file, line)) {                                                                         \
                std::string error = "";                                                                                \
                json::Json obj = json::Json::parse(line, error);                                                       \
                if (error != "") {                                                                                     \
                    continue;                                                                                          \
                }                                                                                                      \
                _api_json_tests(obj);                                                                                  \
            }                                                                                                          \
            file.close();                                                                                              \
        } else {                                                                                                       \
            LOGE("无法打开文件 %s\n", path.c_str());                                                                   \
        }                                                                                                              \
    }

#define API_DYNAMIC_JSON_LOADER_DEFAULT()                                                                              \
    do {                                                                                                               \
        API_DYNAMIC_JSON_LOADER("");                                                                                  \
    } while (0)

/// @brief 从 dump 的 json obj 中读取 device_type 并返回对应的 api DeviceType
/// @param obj 
/// @return api::DeviceType(kXPU1, kXPU2, kXPU3, kCPU)
api::DeviceType load_dev(json::Json obj);
} // namespace api
} // namespace xpu
} // namespace baidu
#endif