#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_INT8_WO_T_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_INT8_WO_T_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <xpu/dll_export.h>

struct DLL_EXPORT int8_wo_t {
    int8_t x;
    int8_wo_t(): x(0) {};
    int8_wo_t(int8_t x_) : x(x_) {};
    inline operator double() const {
        return double(x);
    };
}; // special type for TGEMM

#endif
