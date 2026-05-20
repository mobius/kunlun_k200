#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_QUANT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_QUANT_H

#include "xpu/refactor/util/float16.h"
#include "xpu/refactor/util/tfloat32.h"
#include "xpu/refactor/util/bfloat16.h"
#include "xpu/refactor/util/int_with_ll_t.h"
#include "xpu/refactor/util/int8_wo_t.h"
#include "xpu/refactor/util/int15_wo_t.h"
#include "xpu/refactor/util/int4.h"
#include "xpu/refactor/util/int4_wo_int8.h"
#include "xpu/refactor/util/int4_wo_int15.h"
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <vector>

namespace baidu {
namespace xpu {
namespace api {

static inline long __round_half_to_even(const float src) {
    long ret = llround(src);
    if (fabs(fabs(std::round(src) - src) - 0.5) > 0) {
        return ret;
    } else {
        if (std::abs(ret) % 2 == 0) {
            return ret;
        } else {
            return ret + (ret > 0 ? -1 : 1);
        }
    }
}

static float __ieee_compliance_0(float f) {
    uint32_t sign = (*(uint32_t*)&f) & 0x80000000;
    uint32_t uf = 0;
    // nan -> inf
    if (std::isnan(f)) {
        uf = (sign | 0x7F800000);
        return *(reinterpret_cast<float*>(&uf));
    } else if (std::isnormal(f) || (std::isinf(f)) || (f == 0)) {
        return f;
    }
    // denormal -> +-0
    else {
        uf = 0x0;
        return *(reinterpret_cast<float*>(&uf));
    }
}

template <typename T, int RMAX>
static inline T __fp32_to_intx(const float f, float max) {
    max = __ieee_compliance_0(max);
    float input = __ieee_compliance_0(f);
    if (input == 0) { // +0 and -0 -> +0
        input = 0.0f;
    }

    float tmp = float(RMAX) / max;
    if (std::isinf(tmp)) {
        if ((*(uint32_t*)&input) >> 31 & 1) {
            return T(-RMAX);
        } else {
            return T(RMAX);
        }
    }

    tmp = input * tmp;
    if (std::isnan(tmp)) {
        return T(RMAX);
    }

    tmp = __ieee_compliance_0(tmp);
    // early check to avoid INF or big value get into convertor func.
    if (tmp > float(RMAX)) {
        return T(RMAX);
    }
    if (tmp < -float(RMAX)) {
        return T(-RMAX);
    }
    T ret = (T)__round_half_to_even(tmp);
    if (ret > RMAX) {
        ret = T(RMAX);
    }
    if (ret < -RMAX) {
        ret = T(-RMAX);
    }
    return ret;
}

template <typename T> struct QuantMax;
template<> struct QuantMax<int4_t> {
    const static int MAX = 7;
};
template<> struct QuantMax<int4_wo_int8> {
    const static int MAX = 7;
};
template<> struct QuantMax<int4_wo_int15> {
    const static int MAX = 7;
};
template<> struct QuantMax<int8_t> {
    const static int MAX = 127;
};
template<> struct QuantMax<int16_t> {
    const static int MAX = 32767;
};
template<> struct QuantMax<int> { // actually it's int31
    const static int MAX = 1073741823;
};
template<> struct QuantMax<int_with_ll_t> { // actually it's int31
    const static int MAX = 1073741823;
};

template <typename T> struct accumType;
template<> struct accumType<float> {
    using type = float;
};
// xpu mac input bf16->fp32
template<> struct accumType<bfloat16> {
    using type = float;
};
template<> struct accumType<float16> {
    using type = float;
};
template<> struct accumType<tfloat32> {
    using type = float;
};
template<> struct accumType<int16_t> {
    using type = int64_t;
};
template<> struct accumType<int8_t> {
    using type = int64_t;
};
template<> struct accumType<int> {
    using type = double;
};
template<> struct accumType<int_with_ll_t> {
    using type = double;
};
template<> struct accumType<int8_wo_t> {
    using type = int64_t;
};
template<> struct accumType<int4_wo_int8> {
    using type = int64_t;
};
template<> struct accumType<int4_wo_int15> {
    using type = int64_t;
};

template<typename TGEMM>
struct mockTGEMM {
    using type = TGEMM;
};
template<>
struct mockTGEMM<int_with_ll_t> {
    using type = int;
};
template<>
struct mockTGEMM<tfloat32> {
    using type = float;
};
//static inline int8_t fp32_to_int4(const float f, float max) {
//    int8_t v1 = __fp32_to_intx<int8_t, 7>(f, max);
//    return v1;
//}

template <typename T>
static inline T quant_fp32(float f, float max) {
    return __fp32_to_intx<T, QuantMax<T>::MAX>(f, max);
}
template <>
inline float quant_fp32<float>(float f, float max) {
    return f;
}
template <>
inline tfloat32 quant_fp32<tfloat32>(float f, float max) {
    return tfloat32(f);
}
template <>
inline bfloat16 quant_fp32<bfloat16>(float f, float max) {
    return bfloat16(f);
}
template <>
inline float16 quant_fp32<float16>(float f, float max) {
    return float16(f);
}

template <typename TSRC, typename TDST>
inline std::vector<TDST> quant_vector(const TSRC* src, int64_t size, float maxval) {
    std::vector<TDST> ret;
    ret.resize(size);
    for (size_t i = 0; i < size; i++) {
        ret[i] = quant_fp32<TDST>(src[i], maxval);
    }
    return ret;
}
template<>
inline std::vector<int16_t> quant_vector<int16_t, int16_t>(const int16_t* src, int64_t size, float maxval) {
    std::vector<int16_t> ret;
    ret.resize(size);
    for (size_t i = 0; i < size; i++) {
        ret[i] = src[i];
    }
    return ret;
}
template<>
inline std::vector<float> quant_vector<int16_t, float>(const int16_t* src, int64_t size, float maxval) {
    std::vector<float> ret;
    ret.resize(size);
    for (size_t i = 0; i < size; i++) {
        ret[i] = static_cast<float>(src[i]) * (maxval / 32767.0f);
    }
    return ret;
}
template<>
inline std::vector<int8_t> quant_vector<int8_t, int8_t>(const int8_t* src, int64_t size, float maxval) {
    std::vector<int8_t> ret;
    ret.resize(size);
    for (size_t i = 0; i < size; i++) {
        ret[i] = src[i];
    }
    return ret;
}
template<>
inline std::vector<float> quant_vector<int8_t, float>(const int8_t* src, int64_t size, float maxval) {
    std::vector<float> ret;
    ret.resize(size);
    for (size_t i = 0; i < size; i++) {
        ret[i] = static_cast<float>(src[i]) * (maxval / 127.0f);
    }
    return ret;
}
template<>
inline std::vector<float16> quant_vector<int8_t, float16>(const int8_t* src, int64_t size, float maxval) {
    std::vector<float16> ret;
    ret.resize(size);
    for (size_t i = 0; i < size; i++) {
        ret[i] = static_cast<float>(src[i]) * (maxval / 127.0f);
    }
    return ret;
}
template<>
inline std::vector<bfloat16> quant_vector<int8_t, bfloat16>(const int8_t* src, int64_t size, float maxval) {
    std::vector<bfloat16> ret;
    ret.resize(size);
    for (size_t i = 0; i < size; i++) {
        ret[i] = static_cast<float>(src[i]) * (maxval / 127.0f);
    }
    return ret;
}
template<>
inline std::vector<int8_wo_t> quant_vector<int15_wo_t, int8_wo_t>(const int15_wo_t* src, int64_t size, float maxval) {
    std::vector<int8_wo_t> ret;
    // TODO: code will not reach here, this function is used to pass template substitution
    return ret;
}
template<>
inline std::vector<int8_wo_t> quant_vector<int8_t, int8_wo_t>(const int8_t* src, int64_t size, float maxval) {
    std::vector<int8_wo_t> ret;
    // TODO: code will not reach here, this is used to pass template substitution
    return ret;
}
template<>
inline std::vector<int4_wo_int8> quant_vector<int8_t, int4_wo_int8>(const int8_t* src, int64_t size, float maxval) {
    std::vector<int4_wo_int8> ret;
    ret.resize(size);
    for (size_t i = 0; i < size; i++) {
        ret[i] = src[i];
    }
    return ret;
}

template <typename T> inline float dequant_scale(float max_a, float max_b) {
    return 1.0f;
}
template <> inline float dequant_scale<int8_t>(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x38820610;  // 1/(2^7-1)^2
    return max_a * max_b * magic_number;
}
template <> inline float dequant_scale<int16_t>(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x30800200;  // 1/(2^15-1)^2
    return max_a * max_b * magic_number;
}
template <> inline float dequant_scale<int>(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x21800000;  // 1/(2^30-1)^2
    return max_a * max_b * magic_number;
}
template <> inline float dequant_scale<int_with_ll_t>(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x21800000;  // 1/(2^30-1)^2
    return max_a * max_b * magic_number;
}
template <> inline float dequant_scale<int4_wo_int8>(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x3a937005; // 1/((2^7-1)*(2^3-1))
    return max_a * max_b * magic_number;
}
template <> inline float dequant_scale<int4_wo_int15>(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x37124b6e; // 1/((2^14-1)*(2^3-1))
    return max_a * max_b * magic_number;
}

template <typename T>
inline float quant_findmax(const T* src, int size, const float* maxptr) {
    return *maxptr;
}
template <>
inline float quant_findmax<float>(const float* src, int size, const float* maxptr) {
    if (maxptr != nullptr) {
        return *maxptr;
    }
    float absmax = 1e-30f;
    for (int i = 0; i < size; i++) {
        absmax = std::max<float>(absmax, fabs(src[i]));
    }
    return absmax;
}
template <>
inline float quant_findmax<float16>(const float16* src, int size, const float* maxptr) {
    if (maxptr != nullptr) {
        return *maxptr;
    }
    float absmax = 1e-30f;
    for (int i = 0; i < size; i++) {
        absmax = std::max<float>(absmax, fabs(static_cast<float>(src[i])));
    }
    return absmax;
}
template <>
inline float quant_findmax<bfloat16>(const bfloat16* src, int size, const float* maxptr) {
    if (maxptr != nullptr) {
        return *maxptr;
    }
    float absmax = 1e-30f;
    for (int i = 0; i < size; i++) {
        absmax = std::max<float>(absmax, fabs(static_cast<float>(src[i])));
    }
    return absmax;
}
template <typename T>
inline T quant_findmax_2d(const T* src, const float* maxptr, int row, int col, int ld) {
    return *maxptr;
}

template <>
inline float quant_findmax_2d(const float* src, const float* maxptr, int row, int col, int ld) {
    if (maxptr != nullptr) {
        return *maxptr;
    }
    float absmax = 1e-30f;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            absmax = std::max<float>(absmax, fabs(src[i * ld + j]));
        }
    }
    return absmax;
}

template <>
inline float16 quant_findmax_2d(const float16* src, const float* maxptr, int row, int col, int ld) {
    if (maxptr != nullptr) {
        return *maxptr;
    }
    float absmax = 1e-30f;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            absmax = std::max<float>(absmax, fabs(static_cast<float>(src[i * ld + j])));
        }
    }
    return absmax;
}


}
}
}
#endif
