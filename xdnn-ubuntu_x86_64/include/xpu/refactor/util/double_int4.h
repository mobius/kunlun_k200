#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_DOUBLE_INT4_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_DOUBLE_INT4_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <xpu/dll_export.h>
#ifdef __INT4C__
#include <immintrin.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_clz __lzcnt
#define INT4_ALIGN_ATTR
#else
#define INT4_ALIGN_ATTR __attribute__((aligned(1)))
#endif

struct DLL_EXPORT INT4_ALIGN_ATTR double_int4_t {
    int8_t x_;
    double_int4_t(): x_(0) {};
    double_int4_t(int8_t x, int8_t y) {
        int8_t tmp_high = y * 16;
        int8_t tmp_low = x * 16;
        tmp_low = tmp_low >> 4 & 0xf;
        x_ = tmp_high | tmp_low;
    };
    inline operator double() const {
        return double(x_);
    };
}; // special type for input

#endif