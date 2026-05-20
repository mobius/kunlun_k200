#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_BFLOAT16_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_BFLOAT16_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <xpu/dll_export.h>
#ifdef __BF16C__
#include <immintrin.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_clz __lzcnt
#define BFLOAT16_ALIGN_ATTR
#else
#define BFLOAT16_ALIGN_ATTR __attribute__((aligned(2)))
#endif

// bfloat16 = 1-bit sign + 8-bits exponent + 7-bits mantissa
inline float f32_from_bits(uint16_t src) {
    float res = 0;
    uint32_t tmp = src;
    tmp <<= 16;
    std::memcpy(&res, &tmp, sizeof(tmp));
    return res;
}

inline uint16_t bits_from_f32(float src) {
    uint32_t res = 0;
    std::memcpy(&res, &src, sizeof(res));
    return res >> 16;
}

inline uint16_t round_to_nearest_even(float src) {
    if (std::isnan(src)) {
        return UINT16_C(0x7FC0);
    } else {
        union {
            uint32_t U32;
            float F32;
        };

        F32 = src;
        uint32_t rounding_bias = ((U32 >> 16) & 1) + UINT32_C(0x7FFF);
        return static_cast<uint16_t>((U32 + rounding_bias) >> 16);
    }
}

inline uint16_t round_to_zero(float src) {
    if (std::isnan(src)) {
        return UINT16_C(0x7FC0);
    } else {
        union {
            uint32_t U32;
            float F32;
        };

        F32 = src;
        return static_cast<uint16_t>(U32 >> 16);
    }
}

struct DLL_EXPORT BFLOAT16_ALIGN_ATTR bfloat16 {
    uint16_t x;
    bfloat16() = default;

    struct from_bits_t {};
    static constexpr from_bits_t from_bits() {
        return from_bits_t();
    }

    constexpr bfloat16(unsigned short bits, from_bits_t) : x(bits) {};
    inline bfloat16(float value, bool useRoundToZero = false);
    inline operator float() const;
};

/// Constructors
inline bfloat16::bfloat16(float value, bool useRoundToZero) {
    // RNE by default
    if (useRoundToZero) {
        x = round_to_zero(value);
    } else {
        x = round_to_nearest_even(value);
    }
}

/// Implicit conversions
inline bfloat16::operator float() const {
    return f32_from_bits(x);
}

/// Arithmetic

inline bfloat16 operator+(const bfloat16& a, const bfloat16& b) {
    return static_cast<float>(a) + static_cast<float>(b);
}

inline bfloat16 operator-(const bfloat16& a, const bfloat16& b) {
    return static_cast<float>(a) - static_cast<float>(b);
}

inline bfloat16 operator*(const bfloat16& a, const bfloat16& b) {
    return static_cast<float>(a) * static_cast<float>(b);
}

inline bfloat16 operator/(const bfloat16& a, const bfloat16& b) {
    return static_cast<float>(a) / static_cast<float>(b);
}

inline bfloat16 operator-(const bfloat16& a) {
    return -static_cast<float>(a);
}

inline bfloat16& operator+=(bfloat16& a, const bfloat16& b) {
    a = a + b;
    return a;
}

inline bfloat16& operator-=(bfloat16& a, const bfloat16& b) {
    a = a - b;
    return a;
}

inline bfloat16& operator*=(bfloat16& a, const bfloat16& b) {
    a = a * b;
    return a;
}

inline bfloat16& operator/=(bfloat16& a, const bfloat16& b) {
    a = a / b;
    return a;
}

inline bfloat16& operator|(bfloat16& a, const bfloat16& b) {
    a.x = a.x | b.x;
    return a;
}

inline bfloat16& operator^(bfloat16& a, const bfloat16& b) {
    a.x = a.x ^ b.x;
    return a;
}

inline bfloat16& operator&(bfloat16& a, const bfloat16& b) {
    a.x = a.x & b.x;
    return a;
}

/// Arithmetic with floats

inline float operator+(bfloat16 a, float b) {
    return static_cast<float>(a) + b;
}
inline float operator-(bfloat16 a, float b) {
    return static_cast<float>(a) - b;
}
inline float operator*(bfloat16 a, float b) {
    return static_cast<float>(a) * b;
}
inline float operator/(bfloat16 a, float b) {
    return static_cast<float>(a) / b;
}

inline float operator+(float a, bfloat16 b) {
    return a + static_cast<float>(b);
}
inline float operator-(float a, bfloat16 b) {
    return a - static_cast<float>(b);
}
inline float operator*(float a, bfloat16 b) {
    return a * static_cast<float>(b);
}
inline float operator/(float a, bfloat16 b) {
    return a / static_cast<float>(b);
}

inline float& operator+=(float& a, const bfloat16& b) {
    return a += static_cast<float>(b);
}
inline float& operator-=(float& a, const bfloat16& b) {
    return a -= static_cast<float>(b);
}
inline float& operator*=(float& a, const bfloat16& b) {
    return a *= static_cast<float>(b);
}
inline float& operator/=(float& a, const bfloat16& b) {
    return a /= static_cast<float>(b);
}

/// Arithmetic with doubles

inline double operator+(bfloat16 a, double b) {
    return static_cast<double>(a) + b;
}
inline double operator-(bfloat16 a, double b) {
    return static_cast<double>(a) - b;
}
inline double operator*(bfloat16 a, double b) {
    return static_cast<double>(a) * b;
}
inline double operator/(bfloat16 a, double b) {
    return static_cast<double>(a) / b;
}

inline double operator+(double a, bfloat16 b) {
    return a + static_cast<double>(b);
}
inline double operator-(double a, bfloat16 b) {
    return a - static_cast<double>(b);
}
inline double operator*(double a, bfloat16 b) {
    return a * static_cast<double>(b);
}
inline double operator/(double a, bfloat16 b) {
    return a / static_cast<double>(b);
}

/// Arithmetic with ints

inline bfloat16 operator+(bfloat16 a, int b) {
    return a + static_cast<bfloat16>(b);
}
inline bfloat16 operator-(bfloat16 a, int b) {
    return a - static_cast<bfloat16>(b);
}
inline bfloat16 operator*(bfloat16 a, int b) {
    return a * static_cast<bfloat16>(b);
}
inline bfloat16 operator/(bfloat16 a, int b) {
    return a / static_cast<bfloat16>(b);
}

inline bfloat16 operator+(int a, bfloat16 b) {
    return static_cast<bfloat16>(a) + b;
}
inline bfloat16 operator-(int a, bfloat16 b) {
    return static_cast<bfloat16>(a) - b;
}
inline bfloat16 operator*(int a, bfloat16 b) {
    return static_cast<bfloat16>(a) * b;
}
inline bfloat16 operator/(int a, bfloat16 b) {
    return static_cast<bfloat16>(a) / b;
}

//// Arithmetic with int64_t
inline bfloat16 operator+(bfloat16 a, int64_t b) {
    return a + static_cast<bfloat16>(b);
}
inline bfloat16 operator-(bfloat16 a, int64_t b) {
    return a - static_cast<bfloat16>(b);
}
inline bfloat16 operator*(bfloat16 a, int64_t b) {
    return a * static_cast<bfloat16>(b);
}
inline bfloat16 operator/(bfloat16 a, int64_t b) {
    return a / static_cast<bfloat16>(b);
}

inline bfloat16 operator+(int64_t a, bfloat16 b) {
    return static_cast<bfloat16>(a) + b;
}
inline bfloat16 operator-(int64_t a, bfloat16 b) {
    return static_cast<bfloat16>(a) - b;
}
inline bfloat16 operator*(int64_t a, bfloat16 b) {
    return static_cast<bfloat16>(a) * b;
}
inline bfloat16 operator/(int64_t a, bfloat16 b) {
    return static_cast<bfloat16>(a) / b;
}

namespace std {

template <>
class numeric_limits<::bfloat16> {
public:
    static constexpr bool is_signed = true;
    static constexpr bool is_specialized = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = false;
    static constexpr bool has_infinity = true;
    static constexpr bool has_quiet_NaN = true;
    static constexpr bool has_signaling_NaN = true;
    static constexpr auto has_denorm = numeric_limits<float>::has_denorm;
    static constexpr auto has_denorm_loss =
            numeric_limits<float>::has_denorm_loss;
    static constexpr auto round_style = numeric_limits<float>::round_style;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr int digits = 8;
    static constexpr int digits10 = 2;
    static constexpr int max_digits10 = 4;
    static constexpr int radix = 2;
    static constexpr int min_exponent = -125;
    static constexpr int min_exponent10 = -37;
    static constexpr int max_exponent = 128;
    static constexpr int max_exponent10 = 38;
    static constexpr auto traps = numeric_limits<float>::traps;
    static constexpr auto tinyness_before =
            numeric_limits<float>::tinyness_before;

    static constexpr ::bfloat16 min() {
        return ::bfloat16(0x0080, bfloat16::from_bits());
    }
    static constexpr ::bfloat16 lowest() {
        return ::bfloat16(0xFF7F, bfloat16::from_bits());
    }
    static constexpr ::bfloat16 max() {
        return ::bfloat16(0x7F7F, bfloat16::from_bits());
    }
    static constexpr ::bfloat16 epsilon() {
        return ::bfloat16(0x3C00, bfloat16::from_bits());
    }
    static constexpr ::bfloat16 round_error() {
        return ::bfloat16(0x3F00, bfloat16::from_bits());
    }
    static constexpr ::bfloat16 infinity() {
        return ::bfloat16(0x7F80, bfloat16::from_bits());
    }
    static constexpr ::bfloat16 quiet_NaN() {
        return ::bfloat16(0x7FC0, bfloat16::from_bits());
    }
    static constexpr ::bfloat16 signaling_NaN() {
        return ::bfloat16(0x7F80, bfloat16::from_bits());
    }
    static constexpr ::bfloat16 denorm_min() {
        return ::bfloat16(0x0001, bfloat16::from_bits());
    }
};

inline ::bfloat16 exp(bfloat16 a) {
    return std::exp(float(a));
}
inline ::bfloat16 log(bfloat16 a) {
    return std::log(float(a));
}

} // namespace std

#endif
