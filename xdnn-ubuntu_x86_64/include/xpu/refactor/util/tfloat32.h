#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_TFLOAT32_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_TFLOAT32_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <xpu/dll_export.h>

#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_clz __lzcnt
#define TFLOAT32_ALIGN_ATTR
#else
#define TFLOAT32_ALIGN_ATTR __attribute__((aligned(4)))
#endif

/* ref to https://github.com/NVIDIA/cutlass/blob/main/include/cutlass/tfloat32.h */
/*
     * Extend the tensor floating point number to 32 bits and shift to the upper part of the 32-bit word:
     *      +--+----------+------------+----------------+
     *      | S|EEE EEEE E|MMM MMMM MMM|0 0000 0000 0000|
     *      +--+----------+------------+----------------+
     * Bits  31    23-30      13-22           0-12
*/

// inline float fp32_from_bits(uint32_t w) {
//     union {
//         uint32_t as_bits;
//         float as_value;
//     } fp32 = {w};
//     return fp32.as_value;
// }

// inline uint32_t fp32_to_bits(float f) {
//     union {
//         float as_value;
//         uint32_t as_bits;
//     } fp32 = {f};
//     return fp32.as_bits;
// }

inline uint32_t tf32_to_fp32_bits(uint32_t h) {
    return h;
}

inline float tf32_to_fp32_value(uint32_t h) {
    return fp32_from_bits(h);
}

inline uint32_t tf32_from_fp32_value(float f) {
    uint32_t ret = fp32_to_bits(f);
    ret = ret & 0xffffe000;
    return ret;
}

struct DLL_EXPORT TFLOAT32_ALIGN_ATTR tfloat32 {
    unsigned int x;

    struct from_bits_t {};
    static constexpr from_bits_t from_bits() {
        return from_bits_t();
    }

    tfloat32() = default;

    constexpr tfloat32(unsigned int bits, from_bits_t) : x(bits) {};
    inline tfloat32(float value);
    inline operator float() const;
};

/// Constructors
inline tfloat32::tfloat32(float value) {
    x = tf32_from_fp32_value(value);
}

/// Implicit conversions
inline tfloat32::operator float() const {
    return tf32_to_fp32_value(x);
}

/// Arithmetic
inline tfloat32 operator+(const tfloat32& a, const tfloat32& b) {
    return static_cast<float>(a) + static_cast<float>(b);
}

inline tfloat32 operator-(const tfloat32& a, const tfloat32& b) {
    return static_cast<float>(a) - static_cast<float>(b);
}

inline float operator*(const tfloat32& a, const tfloat32& b) {
    return static_cast<float>(a) * static_cast<float>(b);
}

inline tfloat32 operator/(const tfloat32& a, const tfloat32& b) {
    return static_cast<float>(a) / static_cast<float>(b);
}

inline tfloat32 operator-(const tfloat32& a) {
    return -static_cast<float>(a);
}

inline tfloat32& operator+=(tfloat32& a, const tfloat32& b) {
    a = a + b;
    return a;
}

inline tfloat32& operator-=(tfloat32& a, const tfloat32& b) {
    a = a - b;
    return a;
}

inline tfloat32& operator*=(tfloat32& a, const tfloat32& b) {
    a = a * b;
    return a;
}

inline tfloat32& operator/=(tfloat32& a, const tfloat32& b) {
    a = a / b;
    return a;
}

/// Arithmetic with floats

inline float operator+(tfloat32 a, float b) {
    return static_cast<float>(a) + b;
}
inline float operator-(tfloat32 a, float b) {
    return static_cast<float>(a) - b;
}
inline float operator*(tfloat32 a, float b) {
    return static_cast<float>(a) * b;
}
inline float operator/(tfloat32 a, float b) {
    return static_cast<float>(a) / b;
}

inline float operator+(float a, tfloat32 b) {
    return a + static_cast<float>(b);
}
inline float operator-(float a, tfloat32 b) {
    return a - static_cast<float>(b);
}
inline float operator*(float a, tfloat32 b) {
    return a * static_cast<float>(b);
}
inline float operator/(float a, tfloat32 b) {
    return a / static_cast<float>(b);
}

inline float& operator+=(float& a, const tfloat32& b) {
    return a += static_cast<float>(b);
}
inline float& operator-=(float& a, const tfloat32& b) {
    return a -= static_cast<float>(b);
}
inline float& operator*=(float& a, const tfloat32& b) {
    return a *= static_cast<float>(b);
}
inline float& operator/=(float& a, const tfloat32& b) {
    return a /= static_cast<float>(b);
}

/// Arithmetic with doubles

inline double operator+(tfloat32 a, double b) {
    return static_cast<double>(a) + b;
}
inline double operator-(tfloat32 a, double b) {
    return static_cast<double>(a) - b;
}
inline double operator*(tfloat32 a, double b) {
    return static_cast<double>(a) * b;
}
inline double operator/(tfloat32 a, double b) {
    return static_cast<double>(a) / b;
}

inline double operator+(double a, tfloat32 b) {
    return a + static_cast<double>(b);
}
inline double operator-(double a, tfloat32 b) {
    return a - static_cast<double>(b);
}
inline double operator*(double a, tfloat32 b) {
    return a * static_cast<double>(b);
}
inline double operator/(double a, tfloat32 b) {
    return a / static_cast<double>(b);
}

/// Arithmetic with ints

inline tfloat32 operator+(tfloat32 a, int b) {
    return a + static_cast<tfloat32>(b);
}
inline tfloat32 operator-(tfloat32 a, int b) {
    return a - static_cast<tfloat32>(b);
}
inline tfloat32 operator*(tfloat32 a, int b) {
    return a * static_cast<tfloat32>(b);
}
inline tfloat32 operator/(tfloat32 a, int b) {
    return a / static_cast<tfloat32>(b);
}

inline tfloat32 operator+(int a, tfloat32 b) {
    return static_cast<tfloat32>(a) + b;
}
inline tfloat32 operator-(int a, tfloat32 b) {
    return static_cast<tfloat32>(a) - b;
}
inline tfloat32 operator*(int a, tfloat32 b) {
    return static_cast<tfloat32>(a) * b;
}
inline tfloat32 operator/(int a, tfloat32 b) {
    return static_cast<tfloat32>(a) / b;
}

//// Arithmetic with int64_t

inline tfloat32 operator+(tfloat32 a, int64_t b) {
    return a + static_cast<tfloat32>(b);
}
inline tfloat32 operator-(tfloat32 a, int64_t b) {
    return a - static_cast<tfloat32>(b);
}
inline tfloat32 operator*(tfloat32 a, int64_t b) {
    return a * static_cast<tfloat32>(b);
}
inline tfloat32 operator/(tfloat32 a, int64_t b) {
    return a / static_cast<tfloat32>(b);
}

inline tfloat32 operator+(int64_t a, tfloat32 b) {
    return static_cast<tfloat32>(a) + b;
}
inline tfloat32 operator-(int64_t a, tfloat32 b) {
    return static_cast<tfloat32>(a) - b;
}
inline tfloat32 operator*(int64_t a, tfloat32 b) {
    return static_cast<tfloat32>(a) * b;
}
inline tfloat32 operator/(int64_t a, tfloat32 b) {
    return static_cast<tfloat32>(a) / b;
}

namespace std {
template <>
class numeric_limits<tfloat32> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = false;
    static constexpr bool has_infinity = true;
    static constexpr bool has_quiet_NaN = true;
    static constexpr bool has_signaling_NaN = true;
    static constexpr auto has_denorm = std::denorm_present;
    static constexpr auto has_denorm_loss = true;
    static constexpr auto round_style = std::round_to_nearest;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr int digits = 19;
    static constexpr auto traps = numeric_limits<float>::traps;
    static constexpr auto tinyness_before =
            numeric_limits<float>::tinyness_before;
    static constexpr tfloat32 min() {
        return tfloat32(0x01, tfloat32::from_bits());
    }
    static constexpr tfloat32 lowest() {
        return tfloat32(0xff7fffff, tfloat32::from_bits());
    }
    static constexpr tfloat32 max() {
        return tfloat32(0x7f7fffff, tfloat32::from_bits());
    }
    static constexpr tfloat32 epsilon() {
        return tfloat32(0x1000, tfloat32::from_bits());
    }
    static constexpr tfloat32 round_error() {
        return tfloat32(0x3F000000, tfloat32::from_bits());
    }
    static constexpr tfloat32 infinity() {
        return tfloat32(0x7f800000, tfloat32::from_bits());
    }
    static constexpr tfloat32 quiet_NaN() {
        return tfloat32(0x7fffffff, tfloat32::from_bits());
    }
    static constexpr tfloat32 signaling_NaN() {
        return tfloat32(0x7fffffff, tfloat32::from_bits());
    }
    static constexpr tfloat32 denorm_min() {
        return tfloat32(0x1, tfloat32::from_bits());
    }
};

} // namespace std
#endif
