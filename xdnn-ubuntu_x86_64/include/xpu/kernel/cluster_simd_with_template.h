#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_SIMD_WITH_TEMPLATE_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER_SIMD_WITH_TEMPLATE_H
#include "xpu/kernel/cluster_simd.h"

template <typename T> struct simd_type_trait {};
template <> struct simd_type_trait<float> {
    typedef float IntOrFloat;
    typedef __attribute__((ext_vector_type(16))) float VT;
};
template <> struct simd_type_trait<float16> {
    typedef float IntOrFloat;
};
template <> struct simd_type_trait<bfloat16> {
    typedef float IntOrFloat;
};
template <> struct simd_type_trait<int> {
    typedef int IntOrFloat;
    typedef __attribute__((ext_vector_type(16))) int32_t VT;
};
template<typename T> using VType = typename simd_type_trait<T>::VT;

#define DECLARE_TEMPLATE_OP_SV_VV_FLOAT_INT(name)                                                  \
template<typename T> static __device__ inline VType<T> vv##name(VType<T> a, VType<T> b) {}         \
template<> __device__ VType<float> vv##name<float>(VType<float> a, VType<float> b) {               \
    return vv##name##_float32x16(a, b);                                                            \
}                                                                                                  \
template<> __device__ VType<int> vv##name<int>(VType<int> a, VType<int> b) {                       \
    return vv##name##_int32x16(a, b);                                                              \
}                                                                                                  \
template<typename T> static __device__ inline VType<T> sv##name(T a, VType<T> b) {}                \
template<> __device__ VType<float> sv##name<float>(float a, VType<float> b) {                      \
    return sv##name##_float32x16(a, b);                                                            \
}                                                                                                  \
template<> __device__ VType<int> sv##name<int>(int a, VType<int> b) {                              \
    return sv##name##_int32x16(a, b);                                                              \
}                                                                                                  

DECLARE_TEMPLATE_OP_SV_VV_FLOAT_INT(add);
DECLARE_TEMPLATE_OP_SV_VV_FLOAT_INT(sub);
DECLARE_TEMPLATE_OP_SV_VV_FLOAT_INT(mul);
DECLARE_TEMPLATE_OP_SV_VV_FLOAT_INT(min);
DECLARE_TEMPLATE_OP_SV_VV_FLOAT_INT(max);
// number of functions = [add,sub,mul,min,max] * [sv,vv] * [float,int] = 20
// VType<float> vvadd<float>(VType<float> a, VType<float> b);
// VType<float> vvadd<int>(VType<int> a, VType<int> b);
// VType<float> svadd<float>(float a, VType<float> b);
// VType<float> svadd<int>(int a, VType<int> b);
// VType<float> vvsub<float>(VType<float> a, VType<float> b);
// VType<float> vvsub<int>(VType<int> a, VType<int> b);
// VType<float> svsub<float>(float a, VType<float> b);
// VType<float> svsub<int>(int a, VType<int> b);
// VType<float> vvmul<float>(VType<float> a, VType<float> b);
// VType<float> vvmul<int>(VType<int> a, VType<int> b);
// VType<float> svmul<float>(float a, VType<float> b);
// VType<float> svmul<int>(int a, VType<int> b);
// VType<float> vvmin<float>(VType<float> a, VType<float> b);
// VType<float> vvmin<int>(VType<int> a, VType<int> b);
// VType<float> svmin<float>(float a, VType<float> b);
// VType<float> svmin<int>(int a, VType<int> b);
// VType<float> vvmax<float>(VType<float> a, VType<float> b);
// VType<float> vvmax<int>(VType<int> a, VType<int> b);
// VType<float> svmax<float>(float a, VType<float> b);
// VType<float> svmax<int>(int a, VType<int> b);

template<typename T> __device__ VType<T> vzero() {
    VType<T> ret;
    ret = __builtin_xpu2_vvxor_s_mr1(ret, ret);
    return ret;
}
template<typename T> __device__ VType<T> vone() {
    VType<T> ret = vzero<T>;
    T scalar = 1;
    return svadd<T>(scalar, ret);
}

#endif
