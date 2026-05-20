#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_INT4_WO_INT8_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_INT4_WO_INT8_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <xpu/dll_export.h>
#ifdef __INT4_WO_INT8C__
#include <immintrin.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_clz __lzcnt
#define INT4_WO_INT8_ALIGN_ATTR
#else
#define INT4_WO_INT8_ALIGN_ATTR __attribute__((aligned(1)))
#endif

struct DLL_EXPORT INT4_WO_INT8_ALIGN_ATTR int4_wo_int8 {
    int8_t x;
    int4_wo_int8(): x(0) {};
    int4_wo_int8(int8_t x) : x(x) {};
    inline operator double() const {
        return double(x);
    };
}; // special type for TGEMM

#endif