#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_MEMORY_REGION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_MEMORY_REGION_H

#include <string>
#include <vector>
#include <chrono>
#include "xpu/refactor/context/newcontext.h"
#include "xpu/refactor/core/dtype.h"
#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>   // std::replace
#include <direct.h>    // _getcwd
#include <fileapi.h>   // GetFileAttributesA, CreateDirectoryA
#else
#include <unistd.h>    // getcwd, access, F_OK
#include <sys/stat.h>  // mkdir, S_IRUSR
#include <sys/types.h> // mkdir
#endif
#include "xpu/refactor/impl_public/npy_dump.h"

namespace baidu {
namespace xpu {
namespace api {

template<typename T>
struct type_trans {
    using type = T;
};

template<>
struct type_trans<bool> {
    using type = int8_t;
};

bool DLL_EXPORT create_dirs(const std::string& path);
std::string DLL_EXPORT short_file_path(const char* file, int line);
std::string DLL_EXPORT npy_short_file_path(const char* file, int line);
template<typename T> DLL_EXPORT std::string data_string(const std::vector<T>& data);

static constexpr int _average_grow_rate_debug_threshold_ = 10;
static constexpr int _max_debug_threshold_ = 5;
static constexpr int _bytewise_checksum_threshold_ = 2;

struct memory_region_detail {
    Dtype dtype;
    std::string name;
    int op_count;
    std::string file_path_log;
    std::string npy_file_path_log;
    std::vector<char> buffer;
    //checksum
    double mean;
    double standard_deviation;
    uint64_t bytesum;
    double average_grow_rate;
    float max;
    int max_idx;
    std::string data_overview;
    template <typename T> void compute(int debug_level) {
        const T* x = (const T*) buffer.data();
        size_t length = buffer.size() / sizeof(T);
        /////compute sum/////
        double sum = 0;
        for (size_t i = 0; i < length; ++i) {
            sum += x[i];
        }
        mean = sum / length;
        /////compute standard_deviation/////
        double variance = 0;
        const double eps = 1e-5;
        for (size_t i = 0; i < length; ++i) {
            variance += (x[i] - mean) * (x[i] - mean);
        }
        variance /= length;
        standard_deviation = std::sqrt(variance);
        /////bytewise checksum(detect small differences between floats)/////
        if (debug_level >= _bytewise_checksum_threshold_) {
            bytesum = 0;
            for (size_t i = 0; i < buffer.size(); ++i) {
                bytesum += (uint8_t)buffer[i];
            }
        }

        if (debug_level >= _average_grow_rate_debug_threshold_) {
            /////compute average_grow_rate/////
            average_grow_rate = 0;
            for (size_t i = 1; i < length; ++i) {
                average_grow_rate += (x[i] - x[i - 1]) / (x[i - 1] + eps);
            }
            average_grow_rate /= length;
        }
        /////input_detail/////
        std::vector<T> data_vec(x, x + length);
        data_overview = data_string<T>(data_vec);
        if (debug_level >= _max_debug_threshold_) {
            /////compute max/////
            T local_max = x[0] > 0 ? x[0] : -x[0];
            int local_idx = 0;
            for (size_t i = 1; i < length; ++i) {
                T temp_x = x[i] > 0 ? x[i] : -x[i];
                if (temp_x > local_max) {
                    local_max = temp_x;
                    local_idx = i;
                }
            }
            max = local_max;
            max_idx = local_idx;
        }
    }
    template <typename T> void save_to_npy(Context* ctx) {
        using TEMP_T = typename type_trans<T>::type; // use int8_t to replace bool for vector init
        const TEMP_T* x = (const TEMP_T*) buffer.data();
        size_t length = buffer.size() / sizeof(TEMP_T);
        std::string dump_path = "./npydump/" + std::to_string(op_count) + "/";
        bool iscreate = create_dirs(dump_path);
        if (!iscreate) {
            return;
        }
        std::string dev_type = (ctx->dev().type() == api::kCPU) ? "-cpu" : "-xpu";
        std::string filename = dump_path + npy_file_path_log + "-" + name + dev_type +".npy";
        const std::vector<int> shape = {static_cast<int>(length)};
        std::vector<unsigned long int> data_shape(shape.begin(), shape.end());
        npy::SaveArrayAsNumpy(filename, false, data_shape, x);
        npy_file_path_log = filename;
    }
};
// ld > 0 : 2d
// ld <=0 : 1d
struct memory_region {
    const void* ptr;
    int64_t m;
    int64_t n;
    int64_t ld;
    bool is_input;
    int debug_level;
    memory_region_detail* detail;
    memory_region(): detail(nullptr) {
    }
    memory_region(bool _is_input, const void *_ptr, int64_t len, int _debug_level)
        : ptr(_ptr), m(1), n(len), ld(len), is_input(_is_input), debug_level(_debug_level),
          detail(nullptr){};
    memory_region(bool _is_input, const void *_ptr, int64_t _m, int64_t _n, int64_t _ld,
                  int _debug_level)
        : ptr(_ptr), m(_m), n(_n), ld(_ld), is_input(_is_input), debug_level(_debug_level),
          detail(nullptr) {}
    memory_region(const memory_region& obj) {
        m = obj.m;
        n = obj.n;
        ld = obj.ld;
        ptr = obj.ptr;
        detail = nullptr;
        debug_level = obj.debug_level;
        if (obj.detail != nullptr) {
            detail = new memory_region_detail();
            *detail = *(obj.detail);
        }
    }
    memory_region& operator=(const memory_region& obj) = delete;
    ~memory_region() {
        if (detail != nullptr) {
            delete detail;
        }
    }
    template<typename T> bool is_valid(Context* ctx) {
        const T* tmpptr = (const T*)(ptr);
        if (tmpptr == nullptr) {
            return false;
        }
        bool _ld_valid = ((0 < n) && (n <= ld));
        int64_t len = (m - 1) * ld + n;
        if ((!_ld_valid) || (len <= 0)) {
            return false;
        }
        if (ctx->dev().type() == api::kCPU) {
            return true;
        }
        unsigned int first_high = (((unsigned long long) tmpptr) >> 32);
        unsigned int last_high = (((unsigned long long)(tmpptr + len) - 1) >> 32);
        auto first_type = pointer_type(ctx, tmpptr);
        auto last_type = pointer_type(ctx, (const char*)(tmpptr + len) - 1);
        if ((ctx->dev().type() == api::kXPU1 || ctx->dev().type() == api::kXPU2) && first_high != last_high) {
            return false;
        }
        if ((ctx->dev().type() == api::kXPU3 || ctx->dev().type() == api::kXPU4) && first_high > last_high) {
            return false;
        }
        if (first_type != last_type) {
            return false;
        }
        if (first_type == kNIL || first_type == kINVALID) {
            return false;
        }
        return true;
    }
    void copy_data(Context* ctx) {
        if (!detail) {
            return;
        }
        if (detail->buffer.size() == 0) {
            int64_t dst_col_bytes = Dtype_size(detail->dtype) * n;
            int64_t src_col_bytes = Dtype_size(detail->dtype) * ld;
            detail->buffer.resize(m * dst_col_bytes);
            for (int _m = 0; _m < m; _m++) {
                if (ctx->dev().type() == api::kCPU) {
                    std::memcpy(&(detail->buffer[_m * dst_col_bytes]),
                            ((const char*)ptr) + _m * src_col_bytes, dst_col_bytes);
                } else {
                    xpu_wait(ctx->xpu_stream);
                    xpu_memcpy(&(detail->buffer[_m * dst_col_bytes]),
                            ((const char*)ptr) + _m * src_col_bytes, dst_col_bytes, XPUMemcpyKind::XPU_DEVICE_TO_HOST);
                }
            }
        }
    }
    template <typename T>
    void init_detail(bool need_detail, int _op_count, const char* file, int line, const char* name) {
        if (need_detail) {
            detail = new memory_region_detail();
            detail->name = std::string(name);
            detail->op_count = _op_count;
            detail->dtype = api::CPPTypeToDtype<T>();
            detail->file_path_log = short_file_path(file, line);
            detail->npy_file_path_log = npy_short_file_path(file, line);
        }
    }
    int64_t bytes() {
        if (detail) {
            int64_t dst_col_bytes = Dtype_size(detail->dtype) * n;
            return m * dst_col_bytes;
        } else {
            return 0;
        }
    }
    std::string header() {
        if (detail) {
            char tmp[128];
            sprintf(tmp, "%p", ptr);
            return std::string("[") + detail->name + "," + to_string(detail->dtype) + "["
                    + std::to_string(m * n) + "]," + std::string(tmp) + "]";
        } else {
            return "";
        }
    }
    std::string data_indicators(Context* ctx, bool need_data_overview) {
        if (detail) {
            copy_data(ctx);
            if (detail->dtype == api::kFLOAT32) {
                detail->compute<float>(debug_level);
            } else if (detail->dtype == api::kFLOAT16) {
                detail->compute<float16>(debug_level);
            } else if (detail->dtype == api::kBFLOAT16) {
                detail->compute<bfloat16>(debug_level);
            } else if (detail->dtype == api::kINT32) {
                detail->compute<int32_t>(debug_level);
            } else if (detail->dtype == api::kINT16) {
                detail->compute<int16_t>(debug_level);
            } else if (detail->dtype == api::kINT8) {
                detail->compute<int8_t>(debug_level);
            } else if (detail->dtype == api::kINT64) {
                detail->compute<int64_t>(debug_level);
            } else if (detail->dtype == api::kBOOL) {
                detail->compute<bool>(debug_level);
            } else {
                return std::string("[unknown data type to get checksum]");
            }
            std::string r = std::string("[(");
            // mean
            r += "[mean]" + std::to_string(detail->mean);
            // std
            r += ", [std]" + std::to_string(detail->standard_deviation);
            // bytewise checksum
            if (debug_level >= _bytewise_checksum_threshold_) {
                r += ", [bytesum]" + std::to_string(detail->bytesum);
            }
            // agr
            if (debug_level >= _average_grow_rate_debug_threshold_) {
                r += ", [agr]" + std::to_string(detail->average_grow_rate);
            }
            // max
            if (debug_level >= _max_debug_threshold_) {
                r += ", [max]" + std::to_string(detail->max);
                r += ", [max_idx]" + std::to_string(detail->max_idx);
            }
            r += ")]";
            // data overview
            if (need_data_overview) {
                r += std::string("[data_overview=(") + detail->data_overview + ")]";
            }
            return r;
        } else {
            return "";
        }
    }
    std::string npy_dump(Context* ctx) {
        if (detail) {
            copy_data(ctx);
            if (detail->dtype == api::kFLOAT32) {
                detail->save_to_npy<float>(ctx);
            } else if (detail->dtype == api::kFLOAT16) {
                detail->save_to_npy<float16>(ctx);
            } else if (detail->dtype == api::kINT32) {
                detail->save_to_npy<int32_t>(ctx);
            } else if (detail->dtype == api::kINT16) {
                detail->save_to_npy<int16_t>(ctx);
            } else if (detail->dtype == api::kINT8) {
                detail->save_to_npy<int8_t>(ctx);
            } else if (detail->dtype == api::kINT64) {
                detail->save_to_npy<int64_t>(ctx);
            } else if (detail->dtype == api::kBOOL) {
                detail->save_to_npy<bool>(ctx);
            } else if (detail->dtype == api::kBFLOAT16) {
                detail->save_to_npy<bfloat16>(ctx);
            } else {
                return std::string("[unknown data type to dump as npy]");
            }
            return std::string("[FilePath: ") + detail->npy_file_path_log + "]";
        } else {
            return "";
        }
    }
    std::string path() {
        if (detail) {
            return detail->file_path_log;
        } else {
            return "";
        }
    }
};

}
}
}
#endif
