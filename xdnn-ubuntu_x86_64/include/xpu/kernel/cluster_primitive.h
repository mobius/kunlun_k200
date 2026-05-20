#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_PRIMITIVE_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_PRIMITIVE_H
#include "xpu/kernel/cluster_simd.h"
#include "xpu/kernel/cluster_simd_with_template.h"

template<typename T>
static __device__ void primitive_constant(typename simd_type_trait<T>::IntOrFloat x, T* y, int len) {
    using TM = typename simd_type_trait<T>::IntOrFloat;
    auto tmp = vzero<TM>();
    tmp = svadd<TM>(x, tmp);
    int len_rounddown32 = rounddown32(len);
    for (int i = 0; i < len_rounddown32; i += 32) {
        vstore2_lm(y + i, tmp, tmp);
    }
    if (len_rounddown32 < len){
        __simd__ T lm[32];
        vstore2_lm(lm, tmp, tmp);
        mfence_lm();
        for (int i = len_rounddown32; i < len; i++) {
            y[i] = lm[i - len_rounddown32];
        }
    }
    mfence_lm();
}

// Part-1: elementwise operation
#define PRIMITIVE_DECLARE_BINARY(name) \
template<typename TX, typename TY, typename TZ>\
static __device__ void primitive_##name(const TX* x, const TY* y, TZ* z, int len) {\
    using TM = typename simd_type_trait<TX>::IntOrFloat;\
    VType<TM> vx0;\
    VType<TM> vy0;\
    VType<TM> vx1;\
    VType<TM> vy1;\
    int len_rounddown32 = rounddown32(len);\
    for (int i = 0; i < len_rounddown32; i += 32) {\
        vload2_lm(x + i, vx0, vx1);\
        vload2_lm(y + i, vy0, vy1);\
        vy0 = vv##name<TM>(vx0, vy0);\
        vy1 = vv##name<TM>(vx1, vy1);\
        vstore2_lm(z + i, vy0, vy1);\
    }\
    if (len_rounddown32 < len){\
        __simd__ TZ tmp[32];\
        vload2_lm(x + len_rounddown32, vx0, vx1);\
        vload2_lm(y + len_rounddown32, vy0, vy1);\
        vy0 = vv##name<TM>(vx0, vy0);\
        vy1 = vv##name<TM>(vx1, vy1);\
        vstore2_lm(tmp, vy0, vy1);\
        mfence_lm();\
        for (int i = len_rounddown32; i < len; i++) {\
            z[i] = tmp[i - len_rounddown32];\
        }\
    }\
    mfence_lm();\
}\
template<typename TY, typename TZ>\
static __device__ void primitive_##name(typename simd_type_trait<TY>::IntOrFloat x, const TY* y, TZ* z, int len) {\
    using TM = typename simd_type_trait<TY>::IntOrFloat;\
    VType<TM> vy0;\
    VType<TM> vy1;\
    int len_rounddown32 = rounddown32(len);\
    for (int i = 0; i < len_rounddown32; i += 32) {\
        vload2_lm(y + i, vy0, vy1);\
        vy0 = sv##name<TM>(x, vy0);\
        vy1 = sv##name<TM>(x, vy1);\
        vstore2_lm(z + i, vy0, vy1);\
    }\
    if (len_rounddown32 < len){\
        __simd__ TZ tmp[32];\
        vload2_lm(y + len_rounddown32, vy0, vy1);\
        vy0 = sv##name<TM>(x, vy0);\
        vy1 = sv##name<TM>(x, vy1);\
        vstore2_lm(tmp, vy0, vy1);\
        mfence_lm();\
        for (int i = len_rounddown32; i < len; i++) {\
            z[i] = tmp[i - len_rounddown32];\
        }\
    }\
    mfence_lm();\
}

#ifdef __XPU3__
#define PRIMITIVE_DECLARE_BFLOAT16(name) \
static __device__ void primitive_##name(const bfloat16* x, const bfloat16* y, bfloat16* z, int len) {\
    using TM = typename simd_type_trait<bfloat16>::IntOrFloat;\
    VType<TM> vx0;\
    VType<TM> vy0;\
    VType<TM> vx1;\
    VType<TM> vy1;\
    int len_rounddown32 = rounddown32(len);\
    for (int i = 0; i < len_rounddown32; i += 32) {\
        vload2_lm_unordered(x + i, vx0, vx1);\
        vload2_lm_unordered(y + i, vy0, vy1);\
        vy0 = vv##name<TM>(vx0, vy0);\
        vy1 = vv##name<TM>(vx1, vy1);\
        vstore2_lm_unordered(z + i, vy0, vy1);\
    }\
    if (len_rounddown32 < len){\
        __simd__ bfloat16 tmp[32];\
        vload2_lm_unordered(x + len_rounddown32, vx0, vx1);\
        vload2_lm_unordered(y + len_rounddown32, vy0, vy1);\
        vy0 = vv##name<TM>(vx0, vy0);\
        vy1 = vv##name<TM>(vx1, vy1);\
        vstore2_lm_unordered(tmp, vy0, vy1);\
        mfence_lm();\
        for (int i = len_rounddown32; i < len; i++) {\
            z[i] = tmp[i - len_rounddown32];\
        }\
    }\
    mfence_lm();\
}\
static __device__ void primitive_##name(float x, const bfloat16* y, bfloat16* z, int len) {\
    using TM = typename simd_type_trait<bfloat16>::IntOrFloat;\
    VType<TM> vy0;\
    VType<TM> vy1;\
    int len_rounddown32 = rounddown32(len);\
    for (int i = 0; i < len_rounddown32; i += 32) {\
        vload2_lm_unordered(y + i, vy0, vy1);\
        vy0 = sv##name<TM>(x, vy0);\
        vy1 = sv##name<TM>(x, vy1);\
        vstore2_lm_unordered(z + i, vy0, vy1);\
    }\
    if (len_rounddown32 < len){\
        __simd__ bfloat16 tmp[32];\
        vload2_lm_unordered(y + len_rounddown32, vy0, vy1);\
        vy0 = sv##name<TM>(x, vy0);\
        vy1 = sv##name<TM>(x, vy1);\
        vstore2_lm_unordered(tmp, vy0, vy1);\
        mfence_lm();\
        for (int i = len_rounddown32; i < len; i++) {\
            z[i] = tmp[i - len_rounddown32];\
        }\
    }\
    mfence_lm();\
}

PRIMITIVE_DECLARE_BFLOAT16(add);
PRIMITIVE_DECLARE_BFLOAT16(sub);
PRIMITIVE_DECLARE_BFLOAT16(mul);
PRIMITIVE_DECLARE_BFLOAT16(min);
PRIMITIVE_DECLARE_BFLOAT16(max);
#endif

PRIMITIVE_DECLARE_BINARY(add);
PRIMITIVE_DECLARE_BINARY(sub);
PRIMITIVE_DECLARE_BINARY(mul);
PRIMITIVE_DECLARE_BINARY(min);
PRIMITIVE_DECLARE_BINARY(max);

#ifdef __XPU3__
#define PRIMITIVE_DECLARE_BINARY_FP16(name) \
template<> __device__ void primitive_##name<float16, float16, float16>(const float16* x, const float16* y,\
        float16* z, int len) {\
    float16x32_t vx0;\
    float16x32_t vy0;\
    float16x32_t vx1;\
    float16x32_t vy1;\
    float16x32_t vx2;\
    float16x32_t vy2;\
    float16x32_t vx3;\
    float16x32_t vy3;\
    float16x32_t vx4;\
    float16x32_t vy4;\
    float16x32_t vx5;\
    float16x32_t vy5;\
    constexpr int step = 32 * 6;\
    int len_rounddown_step = len / step * step;\
    int len_rounddown32 = rounddown32(len);\
    for (int i = 0; i < len_rounddown_step; i += step) {\
        vx0 = vload_lm_float16x32(x + i);\
        vx1 = vload_lm_float16x32(x + i + 32);\
        vx2 = vload_lm_float16x32(x + i + 32 * 2);\
        vx3 = vload_lm_float16x32(x + i + 32 * 3);\
        vx4 = vload_lm_float16x32(x + i + 32 * 4);\
        vx5 = vload_lm_float16x32(x + i + 32 * 5);\
        vy0 = vload_lm_float16x32(y + i);\
        vy1 = vload_lm_float16x32(y + i + 32);\
        vy2 = vload_lm_float16x32(y + i + 32 * 2);\
        vy3 = vload_lm_float16x32(y + i + 32 * 3);\
        vy4 = vload_lm_float16x32(y + i + 32 * 4);\
        vy5 = vload_lm_float16x32(y + i + 32 * 5);\
        vy0 = vv##name##_float16x32(vx0, vy0);\
        vy1 = vv##name##_float16x32(vx1, vy1);\
        vy2 = vv##name##_float16x32(vx2, vy2);\
        vy3 = vv##name##_float16x32(vx3, vy3);\
        vy4 = vv##name##_float16x32(vx4, vy4);\
        vy5 = vv##name##_float16x32(vx5, vy5);\
        vstore_lm_float16x32(z + i, vy0);\
        vstore_lm_float16x32(z + i + 32, vy1);\
        vstore_lm_float16x32(z + i + 32 * 2, vy2);\
        vstore_lm_float16x32(z + i + 32 * 3, vy3);\
        vstore_lm_float16x32(z + i + 32 * 4, vy4);\
        vstore_lm_float16x32(z + i + 32 * 5, vy5);\
    }\
    for (int i = len_rounddown_step; i < len_rounddown32; i += 32) {\
        vx0 = vload_lm_float16x32(x + i);\
        vy0 = vload_lm_float16x32(y + i);\
        vy0 = vv##name##_float16x32(vx0, vy0);\
        vstore_lm_float16x32(z + i, vy0);\
    }\
    if (len_rounddown32 < len){\
        __simd__ float16 tmp[32];\
        vx0 = vload_lm_float16x32(x + len_rounddown32);\
        vy0 = vload_lm_float16x32(y + len_rounddown32);\
        vy0 = vv##name##_float16x32(vx0, vy0);\
        vstore_lm_float16x32(tmp, vy0);\
        mfence_lm();\
        for (int i = len_rounddown32; i < len; ++i) {\
            z[i] = tmp[i - len_rounddown32];\
        }\
    }\
    mfence_lm();\
}

PRIMITIVE_DECLARE_BINARY_FP16(add);
PRIMITIVE_DECLARE_BINARY_FP16(mul);
PRIMITIVE_DECLARE_BINARY_FP16(min);
PRIMITIVE_DECLARE_BINARY_FP16(max);

#define PRIMITIVE_DECLARE_BINARY_BF16(name) \
template<> __device__ void primitive_##name<bfloat16, bfloat16, bfloat16>(const bfloat16* x, const bfloat16* y,\
        bfloat16* z, int len) {\
    float32x16_t vx0;\
    float32x16_t vy0;\
    float32x16_t vx1;\
    float32x16_t vy1;\
    float32x16_t vx2;\
    float32x16_t vy2;\
    float32x16_t vx3;\
    float32x16_t vy3;\
    float32x16_t vx4;\
    float32x16_t vy4;\
    float32x16_t vx5;\
    float32x16_t vy5;\
    constexpr int step = 16 * 6;\
    int len_rounddown_step = len / step * step;\
    int len_rounddown32 = rounddown32(len);\
    for (int i = 0; i < len_rounddown_step; i += step) {\
        vload2_lm_unordered(x + i, vx0, vx1);\
        vload2_lm_unordered(x + i + 16 * 2, vx2, vx3);\
        vload2_lm_unordered(x + i + 16 * 4, vx4, vx5);\
        vload2_lm_unordered(y + i, vy0, vy1);\
        vload2_lm_unordered(y + i + 16 * 2, vy2, vy3);\
        vload2_lm_unordered(y + i + 16 * 4, vy4, vy5);\
        vy0 = vv##name##_float32x16(vx0, vy0);\
        vy1 = vv##name##_float32x16(vx1, vy1);\
        vy2 = vv##name##_float32x16(vx2, vy2);\
        vy3 = vv##name##_float32x16(vx3, vy3);\
        vy4 = vv##name##_float32x16(vx4, vy4);\
        vy5 = vv##name##_float32x16(vx5, vy5);\
        vstore2_lm_unordered(z + i, vy0, vy1);\
        vstore2_lm_unordered(z + i + 16 * 2, vy2, vy3);\
        vstore2_lm_unordered(z + i + 16 * 4, vy4, vy5);\
    }\
    for (int i = len_rounddown_step; i < len_rounddown32; i += 32) {\
        vload2_lm_unordered(x + i, vx0, vx1);\
        vload2_lm_unordered(y + i, vy0, vy1);\
        vy0 = vv##name##_float32x16(vx0, vy0);\
        vy1 = vv##name##_float32x16(vx1, vy1);\
        vstore2_lm_unordered(z + i, vy0, vy1);\
    }\
    if (len_rounddown32 < len){\
        __simd__ bfloat16 tmp[32];\
        vload2_lm_unordered(x + len_rounddown32, vx0, vx1);\
        vload2_lm_unordered(y + len_rounddown32, vy0, vy1);\
        vy0 = vv##name##_float32x16(vx0, vy0);\
        vy1 = vv##name##_float32x16(vx1, vy1);\
        vstore2_lm_unordered(tmp, vy0, vy1);\
        mfence_lm();\
        for (int i = len_rounddown32; i < len; ++i) {\
            z[i] = tmp[i - len_rounddown32];\
        }\
    }\
    mfence_lm();\
}

PRIMITIVE_DECLARE_BINARY_BF16(add);
PRIMITIVE_DECLARE_BINARY_BF16(mul);
PRIMITIVE_DECLARE_BINARY_BF16(sub);
#endif

template<typename T>
static __device__ void primitive_div(const T* x, const T* y, T* z, int len) {
    int len_rounddown4 = rounddown4(len);
    for (int i = 0; i < len_rounddown4; i += 4) {
        T x0 = x[i + 0];
        T x1 = x[i + 1];
        T x2 = x[i + 2];
        T x3 = x[i + 3];
        T y0 = y[i + 0];
        T y1 = y[i + 1];
        T y2 = y[i + 2];
        T y3 = y[i + 3];
        x0 = x0 / y0;
        x1 = x1 / y1;
        x2 = x2 / y2;
        x3 = x3 / y3;
        z[i + 0] = x0;
        z[i + 1] = x1;
        z[i + 2] = x2;
        z[i + 3] = x3;
    }
    for (int i = len_rounddown4; i < len; i++) {
        T x0 = x[i + 0];
        T y0 = y[i + 0];
        x0 = x0 / y0;
        z[i + 0] = x0;
    }
    mfence_lm();
}
template<> __device__ void primitive_div<float16>(const float16* x, const float16* y, float16* z, int len) {
}
template<typename T>
static __device__ void primitive_div(float x, const T* y, T* z, int len) {
    int len_rounddown4 = rounddown4(len);
    for (int i = 0; i < len_rounddown4; i += 4) {
        T y0 = y[i + 0];
        T y1 = y[i + 1];
        T y2 = y[i + 2];
        T y3 = y[i + 3];
        y0 = x / y0;
        y1 = x / y1;
        y2 = x / y2;
        y3 = x / y3;
        z[i + 0] = y0;
        z[i + 1] = y1;
        z[i + 2] = y2;
        z[i + 3] = y3;
    }
    for (int i = len_rounddown4; i < len; i++) {
        T y0 = y[i + 0];
        y0 = x / y0;
        z[i + 0] = y0;
    }
    mfence_lm();
}
template<> __device__ void primitive_div<float16>(float x, const float16* y, float16* z, int len) {
}

// sencond part: reduce operation
/**
 * @brief    reduce sum, y = x0 + x1 + .. xN, output inf when overflow
 *
 * @param   [in] x  : operand x, vector
 * @param   [out] y : result
 *
 * @return  void
*/
template<typename T>
static __device__ void primitive_reduce_sum(const T* x, float* y, int len) {
    __simd__ float lm_sum[16];
    __simd__ float lm_sum_remain[32];
    float32x16_t vx0;
    float32x16_t vx1;
    float32x16_t vec_sum = vset_zero();
    float res = 0.f;

    int roundsize32 = rounddown32(len);
    int remain = len - roundsize32;                      // 32 is for compatibility of fp32 and fp16
    for (int i = 0; i < roundsize32; i += 32) {
        vload2_lm(x + i, vx0, vx1);
        vec_sum = vvadd_float32x16(vec_sum, vx0);
        vec_sum = vvadd_float32x16(vec_sum, vx1);
    }
    vstore_lm_float32x16(lm_sum, vec_sum);
    // deal with remaining part
    if (remain > 0) {
        vload2_lm(x + roundsize32, vx0, vx1);
        vstore2_lm(lm_sum_remain, vx0, vx1);
    }
    mfence_lm();
    // add lm_sm and lm_sm_remain together
    for (int i = 0; i < 16; i += 4) {
        float tmp0 = lm_sum[i];
        float tmp1 = lm_sum[i + 1];
        float tmp2 = lm_sum[i + 2];
        float tmp3 = lm_sum[i + 3];
        tmp0 = tmp0 + tmp1;
        tmp2 = tmp2 + tmp3;
        tmp0 = tmp0 + tmp2;
        res += tmp0;
    }
    for (int i = 0; i < remain; i++) {
        res += lm_sum_remain[i];
    }
    *y = res;
    mfence_lm();
}

/**
 * @brief    reduce product, y = x0 * x1 * .. xN, output inf when overflow
 *
 * @param   [in] x  : operand x, vector
 * @param   [out] y : result
 *
 * @return  void
*/
template<typename T>
static __device__ void primitive_reduce_prod(const T* x, float* y, int len) {
    __simd__ float lm_prod[16];
    __simd__ float lm_prod_remain[32];
    float32x16_t vx0;
    float32x16_t vx1;
    float32x16_t vec_prod = vset_one();                  // initialize vec_prod with 1
    float res = 1.0f;

    int roundsize32 = rounddown32(len);
    int remain = len - roundsize32;                     // 32 is for compatibility of fp32 and fp16
    for (int i = 0; i < roundsize32; i += 32) {
        vload2_lm(x + i, vx0, vx1);
        vec_prod = vvmul_float32x16(vec_prod, vx0);
        vec_prod = vvmul_float32x16(vec_prod, vx1);
    }
    vstore_lm_float32x16(lm_prod, vec_prod);
    // deal with remaining part
    if (remain > 0) {
        vload2_lm(x + roundsize32, vx0, vx1);
        vstore2_lm(lm_prod_remain, vx0, vx1);
    }
    mfence_lm();
    // mul prod together
    for (int i = 0; i < 16; i++) {
        res *= lm_prod[i];
    }
    for (int i = 0; i < remain; i++) {
        res *= lm_prod_remain[i];
    }
    *y = res;
    mfence_lm();
}

/**
 * @brief    reduce min, y = the minimum value of vector x
 *
 * @param   [in] x  : operand x, vector
 * @param   [out] y : result
 *
 * @return  void
*/
template<typename T>
static __device__ void primitive_reduce_min(const T* x, float* y, int len) {
    __simd__ float lm_min[16];
    __simd__ float lm_min_remain[32];

    float32x16_t vx0;
    float32x16_t vx1;
    float thresh = 1e38f;
    float32x16_t vec_min = svadd_float32x16(thresh, vset_zero());   // x may be all positive

    int roundsize32 = rounddown32(len);
    int remain = len - roundsize32;                        // 32 is for compatibility of fp32 and fp16
    for (int i = 0; i < roundsize32; i += 32) {
        vload2_lm(x + i, vx0, vx1);
        vec_min = vvmin_float32x16(vec_min, vx0);
        vec_min = vvmin_float32x16(vec_min, vx1);
    }
    vstore_lm_float32x16(lm_min, vec_min);
    // deal with remaining part
    if (remain > 0) {
        vload2_lm(x + roundsize32, vx0, vx1);
        vstore2_lm(lm_min_remain, vx0, vx1);
    }
    mfence_lm();
    // get global min
    for (int i = 0; i < 16; i++) {
        thresh = fmin(thresh, lm_min[i]);
    }
    for (int i = 0; i < remain; i++) {
        thresh = fmin(thresh, lm_min_remain[i]);
    }
    *y = thresh;
    mfence_lm();
}

/**
 * @brief    reduce max, y = the maximum value of vector x
 *
 * @param   [in] x  : operand x, vector
 * @param   [out] y : result
 *
 * @return  void
*/
template<typename T>
static __device__ void primitive_reduce_max(const T* x, float* y, int len) {
    __simd__ float lm_max[16];
    __simd__ float lm_max_remain[32];

    float32x16_t vx0;
    float32x16_t vx1;
    float thresh = -1e38f;
    float32x16_t vec_max = svadd_float32x16(thresh, vset_zero());        // x may be all negative

    int roundsize32 = rounddown32(len);
    int remain = len - roundsize32;                        // 32 is for compatibility of fp32 and fp16
    for (int i = 0; i < roundsize32; i += 32) {
        vload2_lm(x + i, vx0, vx1);
        vec_max = vvmax_float32x16(vec_max, vx0);
        vec_max = vvmax_float32x16(vec_max, vx1);
    }
    vstore_lm_float32x16(lm_max, vec_max);
    // deal with remaining part
    if (remain > 0) {
        vload2_lm(x + roundsize32, vx0, vx1);
        vstore2_lm(lm_max_remain, vx0, vx1);
    }
    mfence_lm();
    // get global max value
    for (int i = 0; i < 16; i += 4) {
        float tmp0 = lm_max[i];
        float tmp1 = lm_max[i + 1];
        float tmp2 = lm_max[i + 2];
        float tmp3 = lm_max[i + 3];
        tmp0 = fmax(tmp0, tmp1);
        tmp2 = fmax(tmp2, tmp3);
        tmp0 = fmax(tmp0, tmp2);
        thresh = fmax(thresh, tmp0);
    }
    for (int i = 0; i < remain; i++) {
        thresh = fmax(thresh, lm_max_remain[i]);
    }
    *y = thresh;
    mfence_lm();
}

/**
 * @brief    calc exp, y[i] = exp(x[i])
 *
 * @param   [in] x  : operand x, vector
 * @param   [out] y : result
 *
 * @return  void
*/
template<typename T>
static __device__ void primitive_exp(const T* x, T* y, int len) {
    return;
}
template<>
__device__ void primitive_exp(const float* x, float* y, int len) {
    int len_rounddown4 = rounddown4(len);
    for (int i = 0; i < len_rounddown4; i += 4) {
        float v0 = x[i + 0];
        float v1 = x[i + 1];
        float v2 = x[i + 2];
        float v3 = x[i + 3];
        y[i + 0] = exp(v0);
        y[i + 1] = exp(v1);
        y[i + 2] = exp(v2);
        y[i + 3] = exp(v3);
    }
    for (int i = len_rounddown4; i < len; i++) {
        float v0 = x[i];
        y[i] = exp(v0);
    }
    mfence_lm();
}

template<typename T>
static __device__ void primitive_exp_with_clip(const T* x, T* y, int len) {
    return;
}
template<>
__device__ void primitive_exp_with_clip(const float* x, float* y, int len) {
    int len_rounddown4 = rounddown4(len);
    for (int i = 0; i < len_rounddown4; i += 4) {
        float v0 = fmax(x[i + 0], -64.0f);
        float v1 = fmax(x[i + 1], -64.0f);
        float v2 = fmax(x[i + 2], -64.0f);
        float v3 = fmax(x[i + 3], -64.0f);
        y[i + 0] = exp(v0);
        y[i + 1] = exp(v1);
        y[i + 2] = exp(v2);
        y[i + 3] = exp(v3);
    }
    for (int i = len_rounddown4; i < len; i++) {
        float v0 = fmax(x[i], -64.0f);
        y[i] = exp(v0);
    }
    mfence_lm();
}
/**
 * @brief    calc log, y[i] = log(x[i])
 *
 * @param   [in] x  : operand x, vector
 * @param   [out] y : result
 *
 * @return  void
*/
template<typename T>
static __device__ void primitive_log(const T* x, T* y, int len) {
    return;
}
template<>
__device__ void primitive_log(const float* x, float* y, int len) {
    int len_rounddown8 = rounddown8(len);
    for (int i = 0; i < len_rounddown8; i += 8) {
        float v0 = log(x[i + 0]);
        float v1 = log(x[i + 1]);
        float v2 = log(x[i + 2]);
        float v3 = log(x[i + 3]);
        float v4 = log(x[i + 4]);
        float v5 = log(x[i + 5]);
        float v6 = log(x[i + 6]);
        float v7 = log(x[i + 7]);
        y[i + 0] = v0;
        y[i + 1] = v1;
        y[i + 2] = v2;
        y[i + 3] = v3;
        y[i + 4] = v4;
        y[i + 5] = v5;
        y[i + 6] = v6;
        y[i + 7] = v7;
    }
    for (int i = len_rounddown8; i < len; i++) {
        y[i] = log(x[i]);
    }
    mfence_lm();
}

/**
 * @brief    calc sqrt, y[i] = sqrt(x[i])
 *
 * @param   [in] x  : operand x, vector
 * @param   [out] y : result
 *
 * @return  void
*/
template<typename T>
static __device__ void primitive_sqrt(const T* x, T* y, int len) {
    return;
}
template<>
__device__ void primitive_sqrt(const float* x, float* y, int len) {
    int len_rounddown4 = rounddown4(len);
    for (int i = 0; i < len_rounddown4; i += 4) {
        float v0 = sqrt(x[i + 0]);
        float v1 = sqrt(x[i + 1]);
        float v2 = sqrt(x[i + 2]);
        float v3 = sqrt(x[i + 3]);
        y[i + 0] = v0;
        y[i + 1] = v1;
        y[i + 2] = v2;
        y[i + 3] = v3;
    }
    for (int i = len_rounddown4; i < len; i++) {
        y[i] = sqrt(x[i]);
    }
    mfence_lm();
}

template<typename TX, typename TY>
static __device__ void primitive_cast_sm2lm(const _shared_ptr_ TX* x, TY* y, int len) {
    return;
}
template<>
__device__ void primitive_cast_sm2lm(const _shared_ptr_ float* x, float* y, int len) {            // just copy
    float32x16_t vec_x_0;
    float32x16_t vec_x_1;
    for (int i = 0; i < len; i += 32) {
        vload2_sm(x + i, vec_x_0, vec_x_1);
        vstore2_lm(y + i, vec_x_0, vec_x_1);
    }
    mfence();
}

template<>
__device__ void primitive_cast_sm2lm(const _shared_ptr_ float16* x, float16* y, int len) {        // just copy
    float32x16_t vec_x_0;
    float32x16_t vec_x_1;
    for (int i = 0; i < len; i += 32) {
        vload2_sm(x + i, vec_x_0, vec_x_1);
        vstore2_lm(y + i, vec_x_0, vec_x_1);
    }
    mfence();
}

template<>
__device__ void primitive_cast_sm2lm(const _shared_ptr_ float* x, float16* y, int len) {
    for (int i = 0; i < len; i += 32) {
        float32x16_t Y_h = vload_sm_float32x16(x + 16);
        float32x16_t Y_l = vload_sm_float32x16(x);
        __asm__ __volatile__("vfloat2fp16_l.rn vr0, %0\t\n"
                "vfloat2fp16_h.rn vr0, %1\t\n"
                "vstore.mz vr0{mr1}, 0(%2)"
                ::"v"(Y_l), "v"(Y_h), "r"(y):"vr0");
        x += 32;
        y += 32;
    }
    mfence();
}
template<>
__device__ void primitive_cast_sm2lm(const _shared_ptr_ float16* x, float* y, int len) {
    int start = (len - 1) / 32 * 32;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 32) {
        bfloat16x32_t X;
        float32x16_t X_l;
        float32x16_t X_h;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vfp162float_l.rn %0, %1":"=&v"(X_l):"v"(X));
        __asm__ __volatile__("vfp162float_h.rn %0, %1":"=&v"(X_h):"v"(X));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_h), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_l), "r"(y));
        x -= 32;
        y -= 32;
    }
    mfence();
}



template<typename TX, typename TY>
static __device__ void primitive_cast(const TX* x, TY* y, int len) {
    return;
}
template<>
__device__ void primitive_cast(const float* x, float* y, int len) {
    if (x == y) {
        return;
    } else {            // just copy
        float32x16_t vec_x_0;
        float32x16_t vec_x_1;
        for (int i = 0; i < len; i += 32) {
            vload2_lm(x + i, vec_x_0, vec_x_1);
            vstore2_lm(y + i, vec_x_0, vec_x_1);
        }
        mfence_lm();
    }
}

template<>
__device__ void primitive_cast(const float16* x, float16* y, int len) {
    if (x == y) {
        return;
    } else {            // just copy
        float32x16_t vec_x_0;
        float32x16_t vec_x_1;
        for (int i = 0; i < len; i += 32) {
            vload2_lm(x + i, vec_x_0, vec_x_1);
            vstore2_lm(y + i, vec_x_0, vec_x_1);
        }
        mfence_lm();
    }
}

template<>
__device__ void primitive_cast(const float* x, float16* y, int len) {
    for (int i = 0; i < len; i += 32) {
        float32x16_t Y_h = vload_lm_float32x16(x + 16);
        float32x16_t Y_l = vload_lm_float32x16(x);
        __asm__ __volatile__("vfloat2fp16_l.rn vr0, %0\t\n"
                "vfloat2fp16_h.rn vr0, %1\t\n"
                "vstore.mz vr0{mr1}, 0(%2)"
                ::"v"(Y_l), "v"(Y_h), "r"(y):"vr0");
        x += 32;
        y += 32;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const float16* x, float* y, int len) {
    int start = (len - 1) / 32 * 32;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 32) {
        bfloat16x32_t X;
        float32x16_t X_l;
        float32x16_t X_h;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vfp162float_l.rn %0, %1":"=&v"(X_l):"v"(X));
        __asm__ __volatile__("vfp162float_h.rn %0, %1":"=&v"(X_h):"v"(X));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_h), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_l), "r"(y));
        x -= 32;
        y -= 32;
    }
    mfence_lm();
}
#ifdef __XPU2__
template<>
__device__ void primitive_cast(const float* x, bfloat16* y, int len) {
    for (int i = 0; i < len; i += 32) {
        float32x16_t Y_h = vload_lm_float32x16(x + 16);
        float32x16_t Y_l = vload_lm_float32x16(x);
        __asm__ __volatile__("vfloat2bf16_l vr0, %0\t\n"
                "vfloat2bf16_h vr0, %1\t\n"
                "vstore.mz vr0{mr1}, 0(%2)"
                ::"v"(Y_l), "v"(Y_h), "r"(y):"vr0");
        x += 32;
        y += 32;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const bfloat16* x, float* y, int len) {
    int start = (len - 1) / 32 * 32;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 32) {
        bfloat16x32_t X;
        float32x16_t X_l;
        float32x16_t X_h;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vbf162float_l %0, %1":"=&v"(X_l):"v"(X));
        __asm__ __volatile__("vbf162float_h %0, %1":"=&v"(X_h):"v"(X));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_h), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_l), "r"(y));
        x -= 32;
        y -= 32;
    }
    mfence_lm();
}
#endif

#ifdef __XPU3__
template<>
__device__ void primitive_cast(const float* x, bfloat16* y, int len) {
    float32x16_t vec_x_0;
    float32x16_t vec_x_1;
    for (int i = 0; i < len; i += 32) {
        vload2_lm(x + i, vec_x_0, vec_x_1);
        vstore2_lm(y + i, vec_x_0, vec_x_1);
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const bfloat16* x, float* y, int len) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    constexpr int pose = 16;
    int start = (len - 1) / 32 * 32;
    for (int i = start; i >= 0; i -= 32) {
        // The same as vload2_lm
        float32x16_t veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + i, mask));
        float32x16_t vodd  = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + i, (~mask)));
        __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
        float32x16_t vec_x_0 = vvmerge_l_float32x16(vodd, veven);
        float32x16_t vec_x_1 = vvmerge_h_float32x16(vodd, veven);
        vstore2_lm(y + i, vec_x_0, vec_x_1);
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const float* x, int* y, int len) {
    for (int i = 0; i < len; i += 16) {
        float32x16_t Y = vload_lm_float32x16(x);
        __asm__ __volatile__("vfloat2fix.rz vr0, %0\t\n"
                "vstore_mask16.mz vr0{mr1}, 0(%1)"
                ::"v"(Y), "r"(y):"vr0");
        x += 16;
        y += 16;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int* x, float* y, int len) {
    for (int i = 0; i < len; i += 16) {
        int32x16_t Y = vload_lm_int32x16(x);
        __asm__ __volatile__("vfix2float.rn vr0, %0\t\n"
                "vstore_mask16.mz vr0{mr1}, 0(%1)"
                ::"v"(Y), "r"(y):"vr0");
        x += 16;
        y += 16;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const float16* x, int* y, int len) {
    int start = (len - 1) / 32 * 32;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 32) {
        float16x32_t X;
        float32x16_t X_l;
        float32x16_t X_h;
        int32x16_t tmp_l;
        int32x16_t tmp_h;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vfp162float_l.rn %0, %1":"=&v"(X_l):"v"(X));
        __asm__ __volatile__("vfp162float_h.rn %0, %1":"=&v"(X_h):"v"(X));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(tmp_l):"v"(X_l));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(tmp_h):"v"(X_h));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(tmp_h), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(tmp_l), "r"(y));
        x -= 32;
        y -= 32;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int* x, float16* y, int len) {
    for (int i = 0; i < len; i += 32) {
        int32x16_t Y_h = vload_lm_int32x16(x + 16);
        int32x16_t Y_l = vload_lm_int32x16(x);
        int32x16_t tmp_l;
        int32x16_t tmp_h;
        __asm__ __volatile__("vfix2float.rn %0, %1":"=&v"(tmp_l):"v"(Y_l));
        __asm__ __volatile__("vfix2float.rn %0, %1":"=&v"(tmp_h):"v"(Y_h));
        __asm__ __volatile__("vfloat2fp16_l.rn vr0, %0\t\n"
                "vfloat2fp16_h.rn vr0, %1\t\n"
                "vstore.mz vr0{mr1}, 0(%2)"
                ::"v"(tmp_l), "v"(tmp_h), "r"(y):"vr0");
        x += 32;
        y += 32;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const float* x, uint8_t* y, int len) {
    for (int i = 0; i < len; i += 64) {
        float32x16_t Y_hh = vload_lm_float32x16(x + 48);
        float32x16_t Y_hl = vload_lm_float32x16(x + 32);
        float32x16_t Y_lh = vload_lm_float32x16(x + 16);
        float32x16_t Y_ll = vload_lm_float32x16(x);
        __asm__ __volatile__("vfloat2ufix8_ll.rz vr0, %0\t\n"
                "vfloat2ufix8_lh.rz vr0, %1\t\n"
                "vfloat2ufix8_hl.rz vr0, %2\t\n"
                "vfloat2ufix8_hh.rz vr0, %3\t\n"
                "vstore.mz vr0{mr1}, 0(%4)"
                ::"v"(Y_ll), "v"(Y_lh), "v"(Y_hl), "v"(Y_hh), "r"(y):"vr0");
        x += 64;
        y += 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const uint8_t* x, float* y, int len) {
    int start = (len - 1) / 64 * 64;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 64) {
        float16x32_t X;
        float32x16_t X_ll;
        float32x16_t X_lh;
        float32x16_t X_hl;
        float32x16_t X_hh;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vufix82float_ll.rn %0, %1":"=&v"(X_ll):"v"(X));
        __asm__ __volatile__("vufix82float_lh.rn %0, %1":"=&v"(X_lh):"v"(X));
        __asm__ __volatile__("vufix82float_hl.rn %0, %1":"=&v"(X_hl):"v"(X));
        __asm__ __volatile__("vufix82float_hh.rn %0, %1":"=&v"(X_hh):"v"(X));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_hh), "r"(y + 48));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_hl), "r"(y + 32));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_lh), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_ll), "r"(y));
        x -= 64;
        y -= 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int* x, uint8_t* y, int len) {
    for (int i = 0; i < len; i += 64) {
        int32x16_t Y_hh_ = vload_lm_int32x16(x + 48);
        int32x16_t Y_hl_ = vload_lm_int32x16(x + 32);
        int32x16_t Y_lh_ = vload_lm_int32x16(x + 16);
        int32x16_t Y_ll_ = vload_lm_int32x16(x);
        float32x16_t Y_ll;
        float32x16_t Y_lh;
        float32x16_t Y_hl;
        float32x16_t Y_hh;
        __asm__ __volatile__("vfix2float.rn %0, %1":"=&v"(Y_ll):"v"(Y_ll_));
        __asm__ __volatile__("vfix2float.rn %0, %1":"=&v"(Y_lh):"v"(Y_lh_));
        __asm__ __volatile__("vfix2float.rn %0, %1":"=&v"(Y_hl):"v"(Y_hl_));
        __asm__ __volatile__("vfix2float.rn %0, %1":"=&v"(Y_hh):"v"(Y_hh_));
        __asm__ __volatile__("vfloat2ufix8_ll.rz vr0, %0\t\n"
                "vfloat2ufix8_lh.rz vr0, %1\t\n"
                "vfloat2ufix8_hl.rz vr0, %2\t\n"
                "vfloat2ufix8_hh.rz vr0, %3\t\n"
                "vstore.mz vr0{mr1}, 0(%4)"
                ::"v"(Y_ll), "v"(Y_lh), "v"(Y_hl), "v"(Y_hh), "r"(y):"vr0");
        x += 64;
        y += 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const uint8_t* x, int* y, int len) {
    int start = (len - 1) / 64 * 64;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 64) {
        float16x32_t X;
        float32x16_t X_ll_;
        float32x16_t X_lh_;
        float32x16_t X_hl_;
        float32x16_t X_hh_;
        int32x16_t X_ll;
        int32x16_t X_lh;
        int32x16_t X_hl;
        int32x16_t X_hh;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vufix82float_ll.rn %0, %1":"=&v"(X_ll_):"v"(X));
        __asm__ __volatile__("vufix82float_lh.rn %0, %1":"=&v"(X_lh_):"v"(X));
        __asm__ __volatile__("vufix82float_hl.rn %0, %1":"=&v"(X_hl_):"v"(X));
        __asm__ __volatile__("vufix82float_hh.rn %0, %1":"=&v"(X_hh_):"v"(X));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(X_ll):"v"(X_ll_));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(X_lh):"v"(X_lh_));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(X_hl):"v"(X_hl_));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(X_hh):"v"(X_hh_));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_hh), "r"(y + 48));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_hl), "r"(y + 32));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_lh), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_ll), "r"(y));
        x -= 64;
        y -= 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int* x, int8_t* y, int len) {
    uint32x16_t min_val = svadd_uint32x16(0x80808080, reinterpret_cast<uint32x16_t>(vset_zero_int()));
    for (int i = 0; i < len; i += 64) {
        int32x16_t Y_hh_ = vload_lm_int32x16(x + 48);
        int32x16_t Y_hl_ = vload_lm_int32x16(x + 32);
        int32x16_t Y_lh_ = vload_lm_int32x16(x + 16);
        int32x16_t Y_ll_ = vload_lm_int32x16(x);
        int mask_hh_ = ~svle_int32x16(-127, Y_hh_);
        int mask_hl_ = ~svle_int32x16(-127, Y_hl_);
        int mask_lh_ = ~svle_int32x16(-127, Y_lh_);
        int mask_ll_ = ~svle_int32x16(-127, Y_ll_);
        uint32_t mask_l = 
            (static_cast<uint32_t>(mask_ll_ & 0xFFFF)) |
            (static_cast<uint32_t>(mask_lh_ & 0xFFFF) << 16);
        uint32_t mask_h = 
            (static_cast<uint32_t>(mask_hl_ & 0xFFFF)) |
            (static_cast<uint32_t>(mask_hh_ & 0xFFFF) << 16);
        float32x16_t Y_ll = vfix2float(Y_ll_);
        float32x16_t Y_lh = vfix2float(Y_lh_);
        float32x16_t Y_hl = vfix2float(Y_hl_);
        float32x16_t Y_hh = vfix2float(Y_hh_);
        __asm__ __volatile__("vfloat2fix8_ll.rz vr0, %0\t\n"
                "vfloat2fix8_lh.rz vr0, %1\t\n"
                "vfloat2fix8_hl.rz vr0, %2\t\n"
                "vfloat2fix8_hh.rz vr0, %3\t\n"
                "vstore.mz vr0{mr1}, 0(%4)"
                ::"v"(Y_ll), "v"(Y_lh), "v"(Y_hl), "v"(Y_hh), "r"(y):"vr0");
        // Due to a bug in vstore_lm_int8x64_mh() in the current version of XTDK, 
        // the following code is provided for compiler debugging.
        // uint64_t mask = 
        //     (static_cast<uint64_t>(mask_ll_ & 0xFFFF)) |
        //     (static_cast<uint64_t>(mask_lh_ & 0xFFFF) << 16) |
        //     (static_cast<uint64_t>(mask_hl_ & 0xFFFF) << 32) |
        //     (static_cast<uint64_t>(mask_hh_ & 0xFFFF) << 48); 
        // vstore_lm_int8x64_mh(y, reinterpret_cast<int8x64_t>(min_val), mask);
        // printf("mask_high: 0x%x, mask_low: 0x%x\n", uint32_t(mask >> 32), uint32_t(mask));
        __asm__ __volatile__("vmmov mr0, %1\t\n"
                             "vmmovh mr0, %2\t\n"
                             "vstore_mask64.mh %0{mr0}, 0(%3)"::"v"(min_val), "r"(mask_l), "r"(mask_h), "r"(y):"mr0");
        x += 64;
        y += 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int8_t* x, int* y, int len) {
    int start = (len - 1) / 64 * 64;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 64) {
        float16x32_t X;
        float32x16_t X_ll_;
        float32x16_t X_lh_;
        float32x16_t X_hl_;
        float32x16_t X_hh_;
        int32x16_t X_ll;
        int32x16_t X_lh;
        int32x16_t X_hl;
        int32x16_t X_hh;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vfix82float_ll.rn %0, %1":"=&v"(X_ll_):"v"(X));
        __asm__ __volatile__("vfix82float_lh.rn %0, %1":"=&v"(X_lh_):"v"(X));
        __asm__ __volatile__("vfix82float_hl.rn %0, %1":"=&v"(X_hl_):"v"(X));
        __asm__ __volatile__("vfix82float_hh.rn %0, %1":"=&v"(X_hh_):"v"(X));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(X_ll):"v"(X_ll_));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(X_lh):"v"(X_lh_));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(X_hl):"v"(X_hl_));
        __asm__ __volatile__("vfloat2fix.rz %0, %1":"=&v"(X_hh):"v"(X_hh_));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_hh), "r"(y + 48));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_hl), "r"(y + 32));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_lh), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_ll), "r"(y));
        x -= 64;
        y -= 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int8_t* x, float* y, int len) {
    int start = (len - 1) / 64 * 64;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 64) {
        float16x32_t X;
        float32x16_t X_ll;
        float32x16_t X_lh;
        float32x16_t X_hl;
        float32x16_t X_hh;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vfix82float_ll.rn %0, %1":"=&v"(X_ll):"v"(X));
        __asm__ __volatile__("vfix82float_lh.rn %0, %1":"=&v"(X_lh):"v"(X));
        __asm__ __volatile__("vfix82float_hl.rn %0, %1":"=&v"(X_hl):"v"(X));
        __asm__ __volatile__("vfix82float_hh.rn %0, %1":"=&v"(X_hh):"v"(X));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_hh), "r"(y + 48));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_hl), "r"(y + 32));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_lh), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_ll), "r"(y));
        x -= 64;
        y -= 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const float16* x, uint8_t* y, int len) {
    for (int i = 0; i < len; i += 64) {
        float16x32_t Y_h = vload_lm_float16x32(x + 32);
        float16x32_t Y_l = vload_lm_float16x32(x);
        float32x16_t Y_hh = vfp162float_h(Y_h);
        float32x16_t Y_hl = vfp162float_l(Y_h);
        float32x16_t Y_lh = vfp162float_h(Y_l);
        float32x16_t Y_ll = vfp162float_l(Y_l);
        __asm__ __volatile__("vfloat2ufix8_ll.rz vr0, %0\t\n"
                "vfloat2ufix8_lh.rz vr0, %1\t\n"
                "vfloat2ufix8_hl.rz vr0, %2\t\n"
                "vfloat2ufix8_hh.rz vr0, %3\t\n"
                "vstore.mz vr0{mr1}, 0(%4)"
                ::"v"(Y_ll), "v"(Y_lh), "v"(Y_hl), "v"(Y_hh), "r"(y):"vr0");
        x += 64;
        y += 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const uint8_t* x, float16* y, int len) {
    int start = (len - 1) / 64 * 64;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 64) {
        float16x32_t X_l;
        float16x32_t X_h;
        float32x16_t X_ll;
        float32x16_t X_lh;
        float32x16_t X_hl;
        float32x16_t X_hh;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X_l):"r"(x));
        __asm__ __volatile__("vufix82float_ll.rn %0, %1":"=&v"(X_ll):"v"(X_l));
        __asm__ __volatile__("vufix82float_lh.rn %0, %1":"=&v"(X_lh):"v"(X_l));
        __asm__ __volatile__("vufix82float_hl.rn %0, %1":"=&v"(X_hl):"v"(X_l));
        __asm__ __volatile__("vufix82float_hh.rn %0, %1":"=&v"(X_hh):"v"(X_l));
        X_l = vfloat2fp16_lh(X_ll, X_lh);
        X_h = vfloat2fp16_lh(X_hl, X_hh);
        __asm__ __volatile__("vstore.mz %0{mr1}, 0(%1)"::"v"(X_h), "r"(y + 32));
        __asm__ __volatile__("vstore.mz %0{mr1}, 0(%1)"::"v"(X_l), "r"(y));
        x -= 64;
        y -= 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const float16* x, int8_t* y, int len) {
    uint32x16_t min_val = svadd_uint32x16(0x80808080, reinterpret_cast<uint32x16_t>(vset_zero_int()));
    for (int i = 0; i < len; i += 64) {
        float16x32_t Y_h = vload_lm_float16x32(x + 32);
        float16x32_t Y_l = vload_lm_float16x32(x);
        float32x16_t Y_hh = vfp162float_h(Y_h);
        float32x16_t Y_hl = vfp162float_l(Y_h);
        float32x16_t Y_lh = vfp162float_h(Y_l);
        float32x16_t Y_ll = vfp162float_l(Y_l);
        int mask_hh = ~svlt_float32x16(-128, Y_hh);
        int mask_hl = ~svlt_float32x16(-128, Y_hl);
        int mask_lh = ~svlt_float32x16(-128, Y_lh);
        int mask_ll = ~svlt_float32x16(-128, Y_ll);
        uint32_t mask_l = 
            (static_cast<uint32_t>(mask_ll & 0xFFFF)) |
            (static_cast<uint32_t>(mask_lh & 0xFFFF) << 16);
        uint32_t mask_h = 
            (static_cast<uint32_t>(mask_hl & 0xFFFF)) |
            (static_cast<uint32_t>(mask_hh & 0xFFFF) << 16);
        __asm__ __volatile__("vfloat2fix8_ll.rz vr0, %0\t\n"
                "vfloat2fix8_lh.rz vr0, %1\t\n"
                "vfloat2fix8_hl.rz vr0, %2\t\n"
                "vfloat2fix8_hh.rz vr0, %3\t\n"
                "vstore.mz vr0{mr1}, 0(%4)"
                ::"v"(Y_ll), "v"(Y_lh), "v"(Y_hl), "v"(Y_hh), "r"(y):"vr0");
        __asm__ __volatile__("vmmov mr0, %1\t\n"
                             "vmmovh mr0, %2\t\n"
                             "vstore_mask64.mh %0{mr0}, 0(%3)"::"v"(min_val), "r"(mask_l), "r"(mask_h), "r"(y):"mr0");
        x += 64;
        y += 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int8_t* x, float16* y, int len) {
    int start = (len - 1) / 64 * 64;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 64) {
        float16x32_t X_l;
        float16x32_t X_h;
        float32x16_t X_ll;
        float32x16_t X_lh;
        float32x16_t X_hl;
        float32x16_t X_hh;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X_l):"r"(x));
        __asm__ __volatile__("vfix82float_ll.rn %0, %1":"=&v"(X_ll):"v"(X_l));
        __asm__ __volatile__("vfix82float_lh.rn %0, %1":"=&v"(X_lh):"v"(X_l));
        __asm__ __volatile__("vfix82float_hl.rn %0, %1":"=&v"(X_hl):"v"(X_l));
        __asm__ __volatile__("vfix82float_hh.rn %0, %1":"=&v"(X_hh):"v"(X_l));
        X_l = vfloat2fp16_lh(X_ll, X_lh);
        X_h = vfloat2fp16_lh(X_hl, X_hh);
        __asm__ __volatile__("vstore.mz %0{mr1}, 0(%1)"::"v"(X_h), "r"(y + 32));
        __asm__ __volatile__("vstore.mz %0{mr1}, 0(%1)"::"v"(X_l), "r"(y));
        x -= 64;
        y -= 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int64_t* x, int* y, int len) {
    int32x16_t offset_v = {0,  8,  16, 24, 32,  40,  48,  56, 
                           64, 72, 80, 88, 96, 104, 112, 120};
    int* x_ptr;
    int* y_ptr;
    int32x16_t Y_0;
    int32x16_t Y_1;
    int32x16_t Y_2;
    int32x16_t Y_3;
    int32x16_t Y_4;
    int32x16_t Y_5;
    int32x16_t Y_6;
    int32x16_t Y_7;               
    for (int i = 0; i < len; i += 128) {
        x_ptr = (int*)(x + i);
        y_ptr = y + i;
        Y_0 = vgather_lm_int32x16(x_ptr, offset_v);
        Y_1 = vgather_lm_int32x16(x_ptr + 32, offset_v);
        Y_2 = vgather_lm_int32x16(x_ptr + 64, offset_v);
        Y_3 = vgather_lm_int32x16(x_ptr + 96, offset_v);
        Y_4 = vgather_lm_int32x16(x_ptr + 128, offset_v);
        Y_5 = vgather_lm_int32x16(x_ptr + 160, offset_v);
        Y_6 = vgather_lm_int32x16(x_ptr + 192, offset_v);
        Y_7 = vgather_lm_int32x16(x_ptr + 224, offset_v);

        vstore_lm_int32x16(y_ptr, Y_0);
        vstore_lm_int32x16(y_ptr + 16, Y_1);
        vstore_lm_int32x16(y_ptr + 32, Y_2);
        vstore_lm_int32x16(y_ptr + 48, Y_3);
        vstore_lm_int32x16(y_ptr + 64, Y_4);
        vstore_lm_int32x16(y_ptr + 80, Y_5);
        vstore_lm_int32x16(y_ptr + 96, Y_6);
        vstore_lm_int32x16(y_ptr + 112, Y_7);
    }
    mfence_lm();
}

template<>
__device__ void primitive_cast(const int* x, int64_t* y, int len) {
    int32x16_t pad_0(0);
    int32x16_t nums_pad_0(-1);
    int mask;
    int start = (len - 1) / 16 * 16;
    for (int i = start; i >= 0; i -= 16) {
        // The same as vload2_lm
        int32x16_t X = vload_lm_int32x16(x + i);
        mask = ~svle_int32x16(0, X);
        nums_pad_0 = vvadd_int32x16_mz(nums_pad_0, pad_0, mask);
        int32x16_t Y_l = vvmerge_l_int32x16(X, nums_pad_0);
        int32x16_t Y_h = vvmerge_h_int32x16(X, nums_pad_0);
        vstore_lm_int32x16((int*)(y + i), Y_l);
        vstore_lm_int32x16((int*)(y + i) + 16, Y_h);
    }
    mfence_lm();
}

template<>
__device__ void primitive_cast(const uint8_t* x, int8_t* y, int len) {
    for (int i = 0; i < len; i += 64) {
        uint8x64_t X;
        float32x16_t X_ll;
        float32x16_t X_lh;
        float32x16_t X_hl;
        float32x16_t X_hh;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vufix82float_ll.rn %0, %1":"=&v"(X_ll):"v"(X));
        __asm__ __volatile__("vufix82float_lh.rn %0, %1":"=&v"(X_lh):"v"(X));
        __asm__ __volatile__("vufix82float_hl.rn %0, %1":"=&v"(X_hl):"v"(X));
        __asm__ __volatile__("vufix82float_hh.rn %0, %1":"=&v"(X_hh):"v"(X));
        __asm__ __volatile__("vfloat2fix8_ll.rz vr0, %0\t\n"
                             "vfloat2fix8_lh.rz vr0, %1\t\n"
                             "vfloat2fix8_hl.rz vr0, %2\t\n"
                             "vfloat2fix8_hh.rz vr0, %3\t\n"
                             "vstore.mz vr0{mr1}, 0(%4)"
                             ::"v"(X_ll), "v"(X_lh), "v"(X_hl), "v"(X_hh), "r"(y):"vr0");
        x += 64;
        y += 64;
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast(const int8_t* x, uint8_t* y, int len) {
    for (int i = 0; i < len; i += 64) {
        int8x64_t X;
        float32x16_t X_ll;
        float32x16_t X_lh;
        float32x16_t X_hl;
        float32x16_t X_hh;
        __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vfix82float_ll.rn %0, %1":"=&v"(X_ll):"v"(X));
        __asm__ __volatile__("vfix82float_lh.rn %0, %1":"=&v"(X_lh):"v"(X));
        __asm__ __volatile__("vfix82float_hl.rn %0, %1":"=&v"(X_hl):"v"(X));
        __asm__ __volatile__("vfix82float_hh.rn %0, %1":"=&v"(X_hh):"v"(X));
        __asm__ __volatile__("vfloat2ufix8_ll.rz vr0, %0\t\n"
                             "vfloat2ufix8_lh.rz vr0, %1\t\n"
                             "vfloat2ufix8_hl.rz vr0, %2\t\n"
                             "vfloat2ufix8_hh.rz vr0, %3\t\n"
                             "vstore.mz vr0{mr1}, 0(%4)"
                             ::"v"(X_ll), "v"(X_lh), "v"(X_hl), "v"(X_hh), "r"(y):"vr0");
        x += 64;
        y += 64;
    }
    mfence_lm();

}

template<typename TX, typename TY>
static __device__ void primitive_cast_gsm(_group_shared_ptr_ const TX* x, _group_shared_ptr_ TY* y, int len) {
    return;
}

template<>
__device__ void primitive_cast_gsm(_group_shared_ptr_ const float16* x, _group_shared_ptr_ float* y, int len) {
    int start = (len - 1) / 32 * 32;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 32) {
        float16x32_t X;
        float32x16_t X_l;
        float32x16_t X_h;
        X = vload_gsm_float16x32(x);
        __asm__ __volatile__("vfp162float_l.rn %0, %1":"=&v"(X_l):"v"(X));
        __asm__ __volatile__("vfp162float_h.rn %0, %1":"=&v"(X_h):"v"(X));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_h), "r"(y + 16));
        __asm__ __volatile__("vstore_mask16.mz %0{mr1}, 0(%1)"::"v"(X_l), "r"(y));
        x -= 32;
        y -= 32;
    }
    mfence_lm();
}

#endif

#ifdef __XPU3__
template<typename TX, typename TY>
static __device__ void primitive_cast_unordered(const TX* x, TY* y, int len) {
    return;
}
template<>
__device__ void primitive_cast_unordered(const float* x, bfloat16* y, int len) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    constexpr int pose = 16;
    int len_rounddown32 = len / 32 * 32;
    for (int i = 0; i < len_rounddown32; i += 32) {
        float32x16_t veven;
        float32x16_t vodd;
        vload2_lm(x + i, vodd, veven);

        // The same as vstore2_lm_unordered
        vstore_lm_int16x32_mh(y + i, reinterpret_cast<int16x32_t>(veven), mask);
        __asm__ __volatile__("vsrl.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
        vstore_lm_int16x32_mh(y + i, reinterpret_cast<int16x32_t>(vodd), (~mask));
    }

    if (len_rounddown32 < len) {
        int16x32_t offset_v = {0, 0,  0, 2,  0, 4,  0, 6,
                               0, 8,  0, 10, 0, 12, 0, 14,
                               0, 16, 0, 18, 0, 20, 0, 22,
                               0, 24, 0, 26, 0, 28, 0, 30};
        float32x16_t vec_x_0;
        float32x16_t vec_x_1;
        vload2_lm(x + len_rounddown32, vec_x_0, vec_x_1);
        mfence_lm();
        // The same as vstore2_lm
        vscatter_lm_int16x32_mh(y + len_rounddown32, reinterpret_cast<int16x32_t>(vec_x_0), offset_v, mask);
        vscatter_lm_int16x32_mh(y + len_rounddown32 + 16, reinterpret_cast<int16x32_t>(vec_x_1), offset_v, mask);
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast_unordered(const bfloat16* x, float* y, int len) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    constexpr int pose = 16;
    int len_rounddown32 = len / 32 * 32;
    int start = (len_rounddown32 - 1) / 32 * 32;
    start = len_rounddown32 == 0 ? -1 : start;
    if (len_rounddown32 < len) {
        // The same as vload2_lm
        float32x16_t veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + len_rounddown32, mask));
        float32x16_t vodd  = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + len_rounddown32, (~mask)));
        __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
        float32x16_t vl = vvmerge_l_float32x16(vodd, veven);
        float32x16_t vh = vvmerge_h_float32x16(vodd, veven);

        vstore2_lm(y + len_rounddown32, vl, vh);
    }
    for (int i = start; i >= 0; i -= 32) {
        // The same as vload2_lm_unordered
        float32x16_t veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + i, mask));
        float32x16_t vodd = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + i, (~mask)));
        __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));

        vstore2_lm(y + i, vodd, veven);
    }
    mfence_lm();
}
#endif

#ifdef __XPU3__
template<typename TX, typename TY>
static __device__ void primitive_cast_unordered2(const TX* x, TY* y, int len, int buf_len) {
    return;
}
template<>
__device__ void primitive_cast_unordered2(const float* x, bfloat16* y, int len, int buf_len) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    constexpr int pose = 16;
    int len_rounddown32 = len / 32 * 32;
    for (int i = 0; i < len_rounddown32; i += 32) {
        float32x16_t vodd = __builtin_xpu2_vload_mask16_mr1(x + i/2, 0);
        float32x16_t veven = __builtin_xpu2_vload_mask16_mr1(x + buf_len/2 + i/2, 0);

        // The same as vstore2_lm_unordered
        vstore_lm_int16x32_mh(y + i, reinterpret_cast<int16x32_t>(veven), mask);
        __asm__ __volatile__("vsrl.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
        vstore_lm_int16x32_mh(y + i, reinterpret_cast<int16x32_t>(vodd), (~mask));
    }

    if (len_rounddown32 < len) {
        float32x16_t vodd = __builtin_xpu2_vload_mask16_mr1(x + len_rounddown32/2, 0);
        float32x16_t veven = __builtin_xpu2_vload_mask16_mr1(x + buf_len/2 + len_rounddown32/2, 0);

        // The same as vstore2_lm_unordered
        vstore_lm_int16x32_mh(y + len_rounddown32, reinterpret_cast<int16x32_t>(veven), mask);
        __asm__ __volatile__("vsrl.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
        vstore_lm_int16x32_mh(y + len_rounddown32, reinterpret_cast<int16x32_t>(vodd), (~mask));
    }
    mfence_lm();
}
template<>
__device__ void primitive_cast_unordered2(const bfloat16* x, float* y, int len, int buf_len) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    constexpr int pose = 16;
    int len_rounddown32 = len / 32 * 32;
    int start = (len_rounddown32 - 1) / 32 * 32;
    start = len_rounddown32 == 0 ? -1 : start;
    if (len_rounddown32 < len) {
        // The same as vload2_lm_unordered
        float32x16_t veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + len_rounddown32, mask));
        float32x16_t vodd  = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + len_rounddown32, (~mask)));
        __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));

        vstore_lm_float32x16(y + len_rounddown32/2, vodd);
        vstore_lm_float32x16(y + buf_len/2 + len_rounddown32/2, veven);
    }
    for (int i = start; i >= 0; i -= 32) {
        // The same as vload2_lm_unordered
        float32x16_t veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + i, mask));
        float32x16_t vodd = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(x + i, (~mask)));
        __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));

        vstore_lm_float32x16(y + i/2, vodd);
        vstore_lm_float32x16(y + buf_len/2 + i/2, veven);
    }
    mfence_lm();
}
#endif

#ifdef __XPU3__
template<typename TX, typename TY>
static __device__ void primitive_cast_unordered_sm(const _shared_ptr_ TX* x, _shared_ptr_ TY* y, int len) {
    return;
}
template<>
__device__ void primitive_cast_unordered_sm(const _shared_ptr_ float* x, _shared_ptr_ bfloat16* y, int len) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    constexpr int pose = 16;
    int len_rounddown32 = len / 32 * 32;
    for (int i = 0; i < len_rounddown32; i += 32) {
        float32x16_t veven;
        float32x16_t vodd;
        vload2_sm(x + i, vodd, veven);

        // The same as vstore2_lm_unordered
        vstore_sm_int16x32_mh(y + i, reinterpret_cast<int16x32_t>(veven), mask);
        __asm__ __volatile__("vsrl.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
        vstore_sm_int16x32_mh(y + i, reinterpret_cast<int16x32_t>(vodd), (~mask));
    }

    if (len_rounddown32 < len) {
        int16x32_t offset_v = {0, 0,  0, 2,  0, 4,  0, 6,
                               0, 8,  0, 10, 0, 12, 0, 14,
                               0, 16, 0, 18, 0, 20, 0, 22,
                               0, 24, 0, 26, 0, 28, 0, 30};
        float32x16_t vec_x_0;
        float32x16_t vec_x_1;
        vload2_sm(x + len_rounddown32, vec_x_0, vec_x_1);

        mfence_sm();

        // The same as vstore2_lm
        vscatter_sm_int16x32_mh(y + len_rounddown32, reinterpret_cast<int16x32_t>(vec_x_0), offset_v, mask);
        vscatter_sm_int16x32_mh(y + len_rounddown32 + 16, reinterpret_cast<int16x32_t>(vec_x_1), offset_v, mask);
    }
    mfence_sm();
}
template<>
__device__ void primitive_cast_unordered_sm(const _shared_ptr_ bfloat16* x, _shared_ptr_ float* y, int len) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    constexpr int pose = 16;
    int len_rounddown32 = len / 32 * 32;
    int start = (len_rounddown32 - 1) / 32 * 32;
    start = len_rounddown32 == 0 ? -1 : start;
    if (len_rounddown32 < len) {
        // The same as vload2_lm
        float32x16_t veven = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(x + len_rounddown32, mask));
        float32x16_t vodd  = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(x + len_rounddown32, (~mask)));
        __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
        float32x16_t vl = vvmerge_l_float32x16(vodd, veven);
        float32x16_t vh = vvmerge_h_float32x16(vodd, veven);

        vstore2_sm(y + len_rounddown32, vl, vh);
    }
    for (int i = start; i >= 0; i -= 32) {
        // The same as vload2_lm_unordered
        float32x16_t veven = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(x + i, mask));
        float32x16_t vodd = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(x + i, (~mask)));
        __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));

        vstore2_sm(y + i, vodd, veven);
    }
    mfence_sm();
}
#endif

template<typename TX, typename TY>
static __device__ void primitive_cast_sm(const _shared_ptr_ TX* x, _shared_ptr_ TY* y, int len) {
    return;
}
template<>
__device__ void primitive_cast_sm(const _shared_ptr_ float* x, _shared_ptr_ float16* y, int len) {
    for (int i = 0; i < len; i += 32) {
        float32x16_t Y_h = vload_sm_float32x16(x + 16);
        float32x16_t Y_l = vload_sm_float32x16(x);
        __asm__ __volatile__("vfloat2fp16_l.rn vr0, %0\t\n"
                "vfloat2fp16_h.rn vr0, %1\t\n"
                "vstores.mz vr0{mr1}, 0(%2)"::"v"(Y_l), "v"(Y_h), "r"(y):"vr0");
        x += 32;
        y += 32;
    }
    mfence_sm();
}
template<>
__device__ void primitive_cast_sm(const _shared_ptr_ float16* x, _shared_ptr_ float* y, int len) {
    int start = (len - 1) / 32 * 32;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 32) {
        bfloat16x32_t X;
        float32x16_t X_l;
        float32x16_t X_h;
        __asm__ __volatile__("vloads.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vfp162float_l.rn %0, %1":"=&v"(X_l):"v"(X));
        __asm__ __volatile__("vfp162float_h.rn %0, %1":"=&v"(X_h):"v"(X));
        __asm__ __volatile__("vstores_mask16.mz %0{mr1}, 0(%1)"::"v"(X_h), "r"(y + 16));
        __asm__ __volatile__("vstores_mask16.mz %0{mr1}, 0(%1)"::"v"(X_l), "r"(y));
        x -= 32;
        y -= 32;
    }
    mfence_sm();
}

#ifdef __XPU2__
template<>
__device__ void primitive_cast_sm(const _shared_ptr_ float* x, _shared_ptr_ bfloat16* y, int len) {
    for (int i = 0; i < len; i += 32) {
        float32x16_t Y_h = vload_sm_float32x16(x + 16);
        float32x16_t Y_l = vload_sm_float32x16(x);
        __asm__ __volatile__("vfloat2bf16_l vr0, %0\t\n"
                "vfloat2bf16_h vr0, %1\t\n"
                "vstores.mz vr0{mr1}, 0(%2)"::"v"(Y_l), "v"(Y_h), "r"(y):"vr0");
        x += 32;
        y += 32;
    }
    mfence_sm();
}

template<>
__device__ void primitive_cast_sm(const _shared_ptr_ bfloat16* x, _shared_ptr_ float* y, int len) {
    int start = (len - 1) / 32 * 32;
    x = x + start;
    y = y + start;
    for (int i = start; i >= 0; i -= 32) {
        bfloat16x32_t X;
        float32x16_t X_l;
        float32x16_t X_h;
        __asm__ __volatile__("vloads.mz %0{mr1}, 0(%1)":"=&v"(X):"r"(x));
        __asm__ __volatile__("vbf162float_l %0, %1":"=&v"(X_l):"v"(X));
        __asm__ __volatile__("vbf162float_h %0, %1":"=&v"(X_h):"v"(X));
        __asm__ __volatile__("vstores_mask16.mz %0{mr1}, 0(%1)"::"v"(X_h), "r"(y + 16));
        __asm__ __volatile__("vstores_mask16.mz %0{mr1}, 0(%1)"::"v"(X_l), "r"(y));
        x -= 32;
        y -= 32;
    }
    mfence_sm();
}
#endif

#ifdef __XPU3__
template<>
__device__ void primitive_cast_sm(const _shared_ptr_ bfloat16* x, _shared_ptr_ float* y, int len) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    constexpr int pose = 16;
    int start = (len - 1) / 32 * 32;
    for (int i = start; i >= 0; i -= 32) {
        // The same as vload2_lm
        float32x16_t veven = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(x + i, mask));
        float32x16_t vodd  = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(x + i, (~mask)));
        __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
        float32x16_t vec_x_0 = vvmerge_l_float32x16(vodd, veven);
        float32x16_t vec_x_1 = vvmerge_h_float32x16(vodd, veven);

        vstore2_sm(y + i, vec_x_0, vec_x_1);
    }
    mfence_sm();
}

template<>
__device__ void primitive_cast_sm(const _shared_ptr_ float* x, _shared_ptr_ bfloat16* y, int len) {
    float32x16_t vec_x_0;
    float32x16_t vec_x_1;
    for (int i = 0; i < len; i += 32) {
        vload2_sm(x + i, vec_x_0, vec_x_1);
        vstore2_sm(y + i, vec_x_0, vec_x_1);
    }
    mfence_sm();
}
#endif

#endif // BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_PRIMITIVE_H


