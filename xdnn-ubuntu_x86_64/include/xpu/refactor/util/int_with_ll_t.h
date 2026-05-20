#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_INT_WITH_LL_T_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_INT_WITH_LL_T_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <xpu/dll_export.h>

struct DLL_EXPORT int_with_ll_t {
    int x;
    int_with_ll_t(): x(0) {};
    int_with_ll_t(int x) : x(x) {};
    inline operator double() const {
        return double(x);
    };
}; // special type for TGEMM

#endif
