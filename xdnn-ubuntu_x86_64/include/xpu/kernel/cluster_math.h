#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_MATH_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_MATH_H
#include "xpu/kernel/xtdk_math.h"

// int min(int a, int b)
// int max(int a, int b)
// float fmin(float a, float b)
// float fmax(float a, float b)
// float sqrt(float input)
// float log(float input)
// float expf(float input)
// float exp(float input)
//
// int roundup(int n, int k)
// int roundup_div(int n, int k)
// int rounddown(int n, int k)
// int rounddown_div(int n, int k)
// int div2(int n)
//      div4,div8,div16,div32,div64,div128
// int mod2(int n)
//      mod4,mod8,mod16,mod32,mod64,mod128,mod256
// int roundup2(int n)
//      roundup4,...,roundup256
// int roundup_div2(int n)
//      roundup_div4,...,roundup_div256
// int rounddown2(int n)
//      rounddown4,...,rounddown256
//
// float rint(float input)
// float trunc(float input)
// float ceil(float input)
// float floor(float input)
// int float2fix(float input)
// int float2fix_rz(float input)
// int float2fix_ru(float input)
// int float2fix_rd(float input)
// float fix2float(int input)
// float fix2float_rz(int input)
// float fix2float_ru(int input)
// float fix2float_rd(int input)
// float pow(float input1, float input2)
// float log_rz(float input)
// float log_ru(float input)
// float log_rd(float input)
// float exp_rz(float input)
// float exp_ru(float input)
// float exp_rd(float input)
// float pow_rz(float input1, float input2)
// float pow_ru(float input1, float input2)
// float pow_rd(float input1, float input2)
//
// float fabs(float t)
// float fast_inverse_sqrt(float number, float epsilon)
// float fast_erf(float x)
template<typename T>
static __device__ inline T min(T a, T b) {
    if (a > b) {
        return b;
    } else {
        return a;
    }
}
template<typename T>
static __device__ inline T max(T a, T b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

template <typename TX, typename TB>
static __device__ inline TX roundup_div(TX x, TB base) {
    if (base < static_cast<TB>(1)) {
        return 0;
    }
    return (x + static_cast<TX>(base) - 1) / static_cast<TX>(base);
}
template <typename TX, typename TB>
static __device__ inline TX roundup(TX x, TB base) {
    return roundup_div(x, base) * static_cast<TX>(base);
}
template <typename TX, typename TB>
static __device__ inline TX rounddown_div(TX x, TB base) {
    if (x < static_cast<TX>(1) || base < static_cast<TB>(1)) {
        return 0;
    }
    return x / static_cast<TX>(base);
}
template <typename TX, typename TB>
static __device__ inline TX rounddown(TX x, TB base) {
    return rounddown_div(x, base) * static_cast<TX>(base);
}
template <typename T>
static __device__ inline T roundup512(T n) {
    return roundup(n, 512);
}
template <typename T>
static __device__ inline T roundup64(T n) {
    return roundup(n, 64);
}
template <typename T>
static __device__ inline T roundup32(T n) {
    return roundup(n, 32);
}
template <typename T>
static __device__ inline T roundup16(T n) {
    return roundup(n, 16);
}
#endif
