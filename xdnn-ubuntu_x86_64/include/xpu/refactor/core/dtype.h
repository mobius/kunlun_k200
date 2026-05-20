#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_DTYPE_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_DTYPE_H

#include "xpu/refactor/util/float16.h"
#include "xpu/refactor/util/bfloat16.h"
#include "xpu/refactor/util/tfloat32.h"
#include "xpu/refactor/util/bit16.h"
#include "xpu/refactor/util/int_with_ll_t.h"
#include "xpu/refactor/util/int4.h"
#include "xpu/refactor/util/double_int4.h"
#include "xpu/refactor/util/int4_wo_int8.h"
#include "xpu/refactor/util/int4_wo_int15.h"
#include "xpu/refactor/core/device.h"
#include <cstdint>
#include <string>

namespace baidu {
namespace xpu {
namespace api {

enum class Dtype {
    FLOAT32 = 0,
    FLOAT16 = 1,
    INT32 = 2,
    INT16 = 3,
    INT8 = 4,
    INT64 = 5,
    BOOL = 6,
    UINT8 = 7,
    BFLOAT16 = 8,
    UINT32 = 9,
    DOUBLE = 10,
    INT_WITH_LL = 11,
    INT4 = 12,
    INT4_WO_INT8 = 13,
    INT4_WO_INT15 = 14,
};

constexpr Dtype kFLOAT32 = Dtype::FLOAT32;
constexpr Dtype kFLOAT16 = Dtype::FLOAT16;
constexpr Dtype kINT32 = Dtype::INT32;
constexpr Dtype kINT16 = Dtype::INT16;
constexpr Dtype kINT8 = Dtype::INT8;
constexpr Dtype kINT64 = Dtype::INT64;
constexpr Dtype kBOOL = Dtype::BOOL;
constexpr Dtype kUINT8 = Dtype::UINT8;
constexpr Dtype kBFLOAT16 = Dtype::BFLOAT16;
constexpr Dtype kUINT32 = Dtype::UINT32;
constexpr Dtype kDOUBLE = Dtype::DOUBLE;
constexpr Dtype kINT_WITH_LL = Dtype::INT_WITH_LL;
constexpr Dtype kINT4 = Dtype::INT4;
constexpr Dtype kINT4_WO_INT8 = Dtype::INT4_WO_INT8;
constexpr Dtype kINT4_WO_INT15 = Dtype::INT4_WO_INT15;

inline std::string to_string(Dtype dt) {
    std::string arr[15] = {
        std::string("kFLOAT32"),
        std::string("kFLOAT16"),
        std::string("kINT32"),
        std::string("kINT16"),
        std::string("kINT8"),
        std::string("kINT64"),
        std::string("kBOOL"),
        std::string("kUINT8"),
        std::string("kBFLOAT16"),
        std::string("kUINT32"),
        std::string("kDOUBLE"),
        std::string("kINT_WITH_LL"),
        std::string("kINT4"),
        std::string("kINT4_WO_INT8"),
        std::string("kINT4_WO_INT15"),
    };
    return arr[static_cast<int>(dt)];
}

template <Dtype T> struct DtypeToCPPType;
template <typename T> inline constexpr Dtype CPPTypeToDtype() {
    return Dtype::FLOAT32;
};
#define SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(cpp_type, _dtype) \
template<> struct DtypeToCPPType<_dtype> {                          \
    using type = cpp_type;                                          \
};                                                                  \
template<> inline constexpr Dtype CPPTypeToDtype<cpp_type>() {      \
    return _dtype;                                                  \
};

SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(float, Dtype::FLOAT32);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(float16, Dtype::FLOAT16);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(int, Dtype::INT32);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(int16_t, Dtype::INT16);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(int8_t, Dtype::INT8);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(int64_t, Dtype::INT64);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(bool, Dtype::BOOL);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(uint8_t, Dtype::UINT8);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(bfloat16, Dtype::BFLOAT16);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(uint32_t, Dtype::UINT32);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(double, Dtype::DOUBLE);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(int_with_ll_t, Dtype::INT_WITH_LL);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(int4_t, Dtype::INT4);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(int4_wo_int8, Dtype::INT4_WO_INT8);
SPECIALIZE_CAST_BETWEEN_DTYPE_AND_CPPTYPE(int4_wo_int15, Dtype::INT4_WO_INT15);

#define DTYPE_CAST_READ(dtype, atype, aval, btype, bval)                        \
if (src_type == dtype) {                                                        \
    auto ptr = reinterpret_cast<const DtypeToCPPType<dtype>::type*>(src_ptr);   \
    aval = static_cast<atype>(*ptr);                                            \
    bval = static_cast<btype>(aval);                                            \
}

#define DTYPE_CAST_WRITE(dtype, inval)                                  \
if (dst_type == dtype) {                                                \
    auto ptr = reinterpret_cast<DtypeToCPPType<dtype>::type*>(dst_ptr); \
    *ptr = inval;                                                       \
}

#define DTYPE_CAST_WRITE_BF16(dtype, inval)                             \
if (dst_type == dtype) {                                                \
    auto ptr = reinterpret_cast<DtypeToCPPType<dtype>::type*>(dst_ptr); \
    if (compared_to_cpu == kXPU3) {                                     \
        *ptr = bfloat16(inval, true);                                   \
    } else {                                                            \
        *ptr = inval;                                                   \
    }                                                                   \
}

#define DTYPE_CAST_WRITE_INT8(dtype, inval)                             \
if (dst_type == dtype) {                                                \
    auto ptr = reinterpret_cast<DtypeToCPPType<dtype>::type*>(dst_ptr); \
    if (compared_to_cpu == kXPU3) {                                     \
        *ptr = inval > 127 ? 127 : (inval < -128 ? -128 : inval);                            \
    } else {                                                            \
        *ptr = inval;                                                   \
    }                                                                   \
}                                                                       \

#define DTYPE_CAST_WRITE_UINT8(dtype, inval)                            \
if (dst_type == dtype) {                                                \
    auto ptr = reinterpret_cast<DtypeToCPPType<dtype>::type*>(dst_ptr); \
    if (compared_to_cpu == kXPU3) {                                     \
        *ptr = inval > 255 ? 255 : (inval < 0 ? 0 : inval);             \
    } else {                                                            \
        *ptr = inval;                                                   \
    }                                                                   \
}

inline void Dtype_cast(void* dst_ptr, const void* src_ptr, Dtype dst_type, Dtype src_type, DeviceType compared_to_cpu = kXPU2) {
    float fv = 0.0f;
    int iv = 0;
    // load
    DTYPE_CAST_READ(kFLOAT32, float, fv, int, iv);
    DTYPE_CAST_READ(kFLOAT16, float, fv, int, iv);
    DTYPE_CAST_READ(kINT32, int, iv, float, fv);
    DTYPE_CAST_READ(kINT16, int, iv, float, fv);
    DTYPE_CAST_READ(kINT8, int, iv, float, fv);
    DTYPE_CAST_READ(kINT64, int, iv, float, fv);
    DTYPE_CAST_READ(kBOOL, int, iv, float, fv);
    DTYPE_CAST_READ(kUINT8, int, iv, float, fv);
    DTYPE_CAST_READ(kBFLOAT16, float, fv, int, iv);
    DTYPE_CAST_READ(kUINT32, int, iv, float, fv);
    DTYPE_CAST_READ(kDOUBLE, float, fv, int, iv);
    DTYPE_CAST_READ(kINT4, int, iv, float, fv);
    DTYPE_CAST_READ(kINT4_WO_INT8, int, iv, float, fv);
    DTYPE_CAST_READ(kINT4_WO_INT15, int, iv, float, fv);
    // store
    DTYPE_CAST_WRITE(kFLOAT32, fv);
    DTYPE_CAST_WRITE(kFLOAT16, fv);
    DTYPE_CAST_WRITE(kINT32, iv);
    DTYPE_CAST_WRITE(kINT16, iv);
    DTYPE_CAST_WRITE_INT8(kINT8, iv);
    DTYPE_CAST_WRITE(kINT64, iv);
    DTYPE_CAST_WRITE(kBOOL, fv);
    DTYPE_CAST_WRITE_UINT8(kUINT8, iv);
    DTYPE_CAST_WRITE_BF16(kBFLOAT16, fv);
    DTYPE_CAST_WRITE(kUINT32, iv);
    DTYPE_CAST_WRITE(kDOUBLE, fv);
    DTYPE_CAST_WRITE(kINT4, iv);
    DTYPE_CAST_WRITE(kINT4_WO_INT8, iv);
    DTYPE_CAST_WRITE(kINT4_WO_INT15, iv);
}

inline int Dtype_size(Dtype dtype) {
    int arr[15] = {4, 2, 4, 2, 1, 8, 1, 1, 2, 4, 8, 4, 1, 1, 1};
    return arr[static_cast<int>(dtype)];
}

// some op implemented by sdnn only support fp32 for in/out_data_type
template <typename T> bool constexpr is_fp32() {
    return false;
}
template <> bool constexpr is_fp32<float>() {
    return true;
}

template <typename T> bool constexpr is_tf32() {
    return false;
}
template <> bool constexpr is_tf32<tfloat32>() {
    return true;
}

template <typename T> bool constexpr is_double() {
    return false;
}
template <> bool constexpr is_double<double>() {
    return true;
}

// some op implemented by sdnn only support int8 for mac_data_type
template <typename T> bool constexpr is_int8() {
    return false;
}
template <> bool constexpr is_int8<int8_t>() {
    return true;
}

// some op implemented by sdnn only support int16 for mac_data_type
template <typename T> bool constexpr is_int16() {
    return false;
}
template <> bool constexpr is_int16<int16_t>() {
    return true;
}

// some op implemented by sdnn only support int16/int8_t for mac_data_type
template <typename T> bool constexpr is_int16_or_int8() {
    return false;
}
template <> bool constexpr is_int16_or_int8<int16_t>() {
    return true;
}
template <> bool constexpr is_int16_or_int8<int8_t>() {
    return true;
}

// some op implemented by sdnn only support int32 for mac_data_type
template <typename T> bool constexpr is_int() {
    return false;
}
template <> bool constexpr is_int<int>() {
    return true;
}

// some op implemented by sdnn only support int32 for mac_data_type
template <typename T> bool constexpr is_int_with_ll() {
    return false;
}
template <> bool constexpr is_int_with_ll<int_with_ll_t>() {
    return true;
}
// some op implemented by sdnn only support int32 for mac_data_type
template <typename T> bool constexpr is_int64() {
    return false;
}
template <> bool constexpr is_int64<int64_t>() {
    return true;
}

// some op implemented by sdnn only support fp32
template <typename T> bool constexpr is_fp16() {
    return false;
}
template <> bool constexpr is_fp16<float16>() {
    return true;
}
template <typename T> bool constexpr is_bfp16() {
    return false;
}
template <> bool constexpr is_bfp16<bfloat16>() {
    return true;
}
template <typename T> bool constexpr is_fp16_or_bfp16() {
    return false;
}
template <> bool constexpr is_fp16_or_bfp16<float16>() {
    return true;
}
template <> bool constexpr is_fp16_or_bfp16<bfloat16>() {
    return true;
}
// some op implemented by sdnn only support fp32 and fp16
template <typename T> bool constexpr is_fp32_or_fp16() {
    return false;
}
template <> bool constexpr is_fp32_or_fp16<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16<float16>() {
    return true;
}
template <typename T> bool constexpr is_fp32_or_fp16_or_tf32() {
    return is_fp32_or_fp16<T>();
}
template <> bool constexpr is_fp32_or_fp16_or_tf32<tfloat32>() {
    return true;
}
template <typename T> bool constexpr is_fp32_or_fp16_or_bfp16() {
    return is_fp32_or_fp16<T>();
}
template <> bool constexpr is_fp32_or_fp16_or_bfp16<bfloat16>() {
    return true;
}
// some op implemented by cluster only support fp32 and int32
template <typename T> bool constexpr is_fp32_or_int32() {
    return false;
}
template <> bool constexpr is_fp32_or_int32<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_int32<int>() {
    return true;
}

// some op implemented by cluster only support fp32 and int32 and int8_t
template <typename T> bool constexpr is_fp32_or_int32_or_int8() {
    return false;
}
template <> bool constexpr is_fp32_or_int32_or_int8<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_int32_or_int8<int>() {
    return true;
}
template <> bool constexpr is_fp32_or_int32_or_int8<int8_t>() {
    return true;
}

// some op implemented by cluster only support fp32, int32 and bool
template <typename T> bool constexpr is_fp32_or_int32_or_bool() {
    return false;
}
template <> bool constexpr is_fp32_or_int32_or_bool<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_int32_or_bool<int>() {
    return true;
}
template <> bool constexpr is_fp32_or_int32_or_bool<bool>() {
    return true;
}

// some op implemented by cluster only support int32 and int64
template <typename T> bool constexpr is_int32_or_int64() {
    return false;
}
template <> bool constexpr is_int32_or_int64<int>() {
    return true;
}
template <> bool constexpr is_int32_or_int64<int64_t>() {
    return true;
}

// some op implemented by cluster only support integer
template <typename T> bool constexpr is_integer() {
    return false;
}
template <> bool constexpr is_integer<int>() {
    return true;
}
template <> bool constexpr is_integer<int64_t>() {
    return true;
}
template <> bool constexpr is_integer<int8_t>() {
    return true;
}
template <> bool constexpr is_integer<int16_t>() {
    return true;
}
template <> bool constexpr is_integer<uint8_t>() {
    return true;
}

// some op implemented by cluster only support fp32, fp16, int8, int16, int32
template <typename T> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32() {
    return false;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32<float16>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32<int8_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32<int16_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32<int32_t>() {
    return true;
}

// some op implemented by cluster only support fp32, fp16, int8, int16, int32, int64
template <typename T> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64() {
    return false;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64<float16>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64<int8_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64<int16_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64<int32_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64<int64_t>() {
    return true;
}

// some op implemented by cluster only support fp32, fp16, int8, int16, int32, int64, bool
template <typename T> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64_or_bool() {
    return false;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64_or_bool<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64_or_bool<float16>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64_or_bool<int8_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64_or_bool<int16_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64_or_bool<int32_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64_or_bool<int64_t>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_int8_or_int16_or_int32_or_int64_or_bool<bool>() {
    return true;
}

// some op implemented by sdnn only support fp16 and fp32 and int32
template <typename T> bool constexpr is_fp32_or_fp16_or_int32() {
    return false;
}
template <> bool constexpr is_fp32_or_fp16_or_int32<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_int32<float16>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_int32<int>() {
    return true;
}

// some op implemented by cluster only support fp16, fp32, int32, int64 and bool
template <typename T> bool constexpr is_fp16_or_fp32_or_int32_or_int64_or_bool() {
    return false;
}
template <> bool constexpr is_fp16_or_fp32_or_int32_or_int64_or_bool<float>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_int32_or_int64_or_bool<float16>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_int32_or_int64_or_bool<int>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_int32_or_int64_or_bool<int64_t>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_int32_or_int64_or_bool<bool>() {
    return true;
}

//some op implemented by cluster only support fp32, fp16, int32 and int64
template <typename T> bool constexpr is_fp32_or_fp16_or_int32_or_int64() {
    return false;
}
template <> bool constexpr is_fp32_or_fp16_or_int32_or_int64<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_int32_or_int64<float16>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_int32_or_int64<int>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_int32_or_int64<int64_t>() {
    return true;
}

template <typename T> bool constexpr is_fp32_or_fp16_or_bf16_int32_or_int64() {
    return false;
}
template <> bool constexpr is_fp32_or_fp16_or_bf16_int32_or_int64<float>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_bf16_int32_or_int64<float16>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_bf16_int32_or_int64<bfloat16>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_bf16_int32_or_int64<int>() {
    return true;
}
template <> bool constexpr is_fp32_or_fp16_or_bf16_int32_or_int64<int64_t>() {
    return true;
}

// some op implemented by cluster only support fp16, fp32, uint8, int32, int64 and bool
template <typename T> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool() {
    return false;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool<float>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool<float16>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool<uint8_t>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool<int>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool<int64_t>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool<bool>() {
    return true;
}

// nonzero count op implemented by cluster only support fp16, fp32, uint8, int32, int64, bool and bfloat16
template <typename T> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool_or_bfloat16() {
    return false;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool_or_bfloat16<float>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool_or_bfloat16<float16>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool_or_bfloat16<uint8_t>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool_or_bfloat16<int>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool_or_bfloat16<int64_t>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool_or_bfloat16<bool>() {
    return true;
}
template <> bool constexpr is_fp16_or_fp32_or_uint8_or_int32_or_int64_or_bool_or_bfloat16<bfloat16>() {
    return true;
}

// At 64-bit Host, int64_t == long
// At 32-bit XPU, int64_t == long long
template <typename T> struct XPUIndexType {
    using type = T;
};
template <> struct XPUIndexType<int64_t> {
    using type = long long;
};
}
}
}

// At 64-bit Host, int64_t == long
// At 32-bit XPU, int64_t == long long
// for the situation that wrapper link kernel when int64_t types are different
namespace xpu2 {
typedef long long int		int64_t;
}

namespace xpu3 {
typedef long long int		int64_t;
}

namespace xpu4 {
typedef long long int		int64_t;
}
#endif
