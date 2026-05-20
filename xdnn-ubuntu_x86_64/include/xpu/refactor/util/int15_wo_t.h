#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_INT15_WO_T_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_INT15_WO_T_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <xpu/dll_export.h>

struct DLL_EXPORT int15_wo_t {
    uint16_t x;
    int15_wo_t(): x(0) {};
    int15_wo_t(uint16_t x_) : x(x_) {};
    inline operator double() const {
        return double(x);
    };
}; // special type for TGEMM

#endif
