#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_SIMD_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_SIMD_H
#include "xpu/kernel/xtdk_simd.h"

#ifndef __XPU_KERNEL_XTDK_SIMD_XPU3_H
typedef __attribute__((ext_vector_type(32))) int16_t float16x32_t;

__device__ float32x16_t vfp162float_l(float16x32_t& a) {
    float32x16_t ret;
    __asm__ __volatile__("vfp162float_l.rn %0, %1" : "=v"(ret) : "v"(a));
    return ret;
}

__device__ float32x16_t vfp162float_h(float16x32_t& a) {
    float32x16_t ret;
    __asm__ __volatile__("vfp162float_h.rn %0, %1" : "=v"(ret) : "v"(a));
    return ret;
}

__device__ float16x32_t vshuffle2_float16x32(float16x32_t& a) {
    float16x32_t ret;
    __asm__ __volatile__("vshuffle2.hf %0, %1" : "=&v"(ret) : "v"(a));
    return ret;
}


static __device__ inline float16x32_t vload_lm_float16x32(const float16* src_ptr) {
    float16x32_t ret;
    __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)":"=&v"(ret):"r"(src_ptr));
    return ret;
}

static __device__ inline float16x32_t vload_sm_float16x32(_shared_ptr_ const float16* src_ptr) {
    float16x32_t ret;
    __asm__ __volatile__("vloads.mz %0{mr1}, 0(%1)":"=&v"(ret):"r"(src_ptr));
    return ret;
}
static __device__ void vstore_sm_float16x32(_shared_ptr_ const float16* dst_ptr, float16x32_t& a) {
    __asm__ __volatile__("vstores.mz %0{mr1}, 0(%1)" ::"v"(a), "r"(dst_ptr));
}

static __device__ inline void vstore_lm_float16x32(float16* dst_ptr, float16x32_t src_data) {
    __asm__ __volatile__("vstore.mz %0{mr1}, 0(%1)"::"v"(src_data), "r"(dst_ptr));
}

static __device__ inline float16x32_t vload_lm_float16x32_mz(const float16* src_ptr, int mask) {
    float16x32_t ret;
    __asm__ __volatile__("vload.mz %0{%1}, 0(%2)":"=&v"(ret):"M"(mask), "r"(src_ptr));
    return ret;
}

static __device__ inline void vstore_lm_float16x32_mz(float16* dst_ptr, float16x32_t src_data, int mask) {
    __asm__ __volatile__("vstore.mz %0{%1}, 0(%2)"::"v"(src_data), "M"(mask), "r"(dst_ptr));
}

static __device__ float16x32_t vload_gsm_float16x32(_group_shared_ptr_ const float16* src_ptr) {
    float16x32_t ret;
    __asm__ __volatile__("vload.mz %0{mr1}, 0(%1)" : "=v"(ret) : "r"(src_ptr));
    return ret;
}

static __device__ void vstore_gsm_float16x32(_group_shared_ptr_ const float16* dst_ptr, float16x32_t& a) {
    __asm__ __volatile__("vstore.mz %0{mr1}, 0(%1)" ::"v"(a), "r"(dst_ptr));
}

static __device__ float16x32_t vvxor_float16x32(float16x32_t& a, float16x32_t& b) {
    float16x32_t ret;
    __asm__ __volatile__("vxor.hf.mz %0{mr1}, %1, %2" : "=v"(ret) : "v"(a), "v"(b));
    return ret;
}

static __device__ inline float16x32_t vvmul_float16x32(float16x32_t a, float16x32_t b) {
    float16x32_t ret;
    __asm__ __volatile__("vmul.hf.mz.rn %0{mr1}, %1, %2":"=&v"(ret):"v"(a), "v"(b));
    return ret;
}

static __device__ inline float16x32_t svmul_float16x32(float16 a, float16x32_t b) {
    float16x32_t ret;
    __asm__ __volatile__("vmul.hf.mz.rn %0{mr1}, %1, %2":"=&v"(ret):"r"(a), "v"(b));
    return ret;
}

static __device__ inline float16x32_t svmul_float16x32_mh(float16 a, float16x32_t b, int mask) {
    float16x32_t ret = b;
    __asm__ __volatile__("vmul.hf.mh.rn %0{%3}, %1, %2":"+v"(ret):"r"(a), "v"(b),"M"(mask));
    return ret;
}

static __device__ inline float16x32_t svmul_float16x32_mz(float16 a, float16x32_t b, int mask) {
    float16x32_t ret;
    __asm__ __volatile__("vmul.hf.mz.rn %0{%1}, %2, %3" : "=v"(ret) : "M"(mask), "r"(a), "v"(b));
    return ret;
}


static __device__ inline float16x32_t vvadd_float16x32(float16x32_t a, float16x32_t b) {
    float16x32_t ret;
    __asm__ __volatile__("vadd.hf.mz.rn %0{mr1}, %1, %2":"=&v"(ret):"v"(a), "v"(b));
    return ret;
}

static __device__ inline float16x32_t svadd_float16x32(float16 a, float16x32_t b) {
    float16x32_t ret;
    __asm__ __volatile__("vadd.hf.mz.rn %0{mr1}, %1, %2":"=&v"(ret):"r"(a), "v"(b));
    return ret;
}

static __device__ inline float16x32_t svadd_float16x32_mz(float16 a, float16x32_t b, int mask = -1) {
    float16x32_t ret;
    __asm__ __volatile__("vadd.hf.mz.rn %0{%3}, %1, %2":"=&v"(ret):"r"(a), "v"(b), "M"(mask));
    return ret;
}

static __device__ inline float16x32_t svadd_float16x32_mh(float16 a, float16x32_t b, int mask = -1) {
    float16x32_t ret = b;
    __asm__ __volatile__("vadd.hf.mh.rn %0{%3}, %1, %2":"+v"(ret):"r"(a), "v"(b), "M"(mask));
    return ret;
}

static __device__ inline float16x32_t svmac_float16x32(float16 a, float16x32_t b, float16x32_t c) {
    __asm__ __volatile__("vmac.hf.mz.rn %0{mr1}, %1, %2" : "+v"(c) : "r"(a), "v"(b));
    return c;
}


static __device__ inline float vrmax_float32x16(float32x16_t a) {
    float ret;
    __asm__ __volatile__("vrmax.f %0, %1" : "=&r"(ret) : "v"(a));
    return ret;
}

static __device__ inline float16 vrmax_float16x32(float16x32_t a) {
    float16 ret;
    __asm__ __volatile__("vrmax.hf %0, %1" : "=&r"(ret) : "v"(a));
    return ret;
}

static __device__ inline float16x32_t vvmax_float16x32(float16x32_t a, float16x32_t b) {
    float16x32_t ret;
    __asm__ __volatile__("vmax.hf.mz %0{mr1}, %1, %2":"=v"(ret):"v"(a), "v"(b));
    return ret;
}

static __device__ inline float16x32_t vvmax_float16x32_mz(float16x32_t a, float16x32_t b, int mask = -1) {
    float16x32_t ret;
    __asm__ __volatile__("vmax.hf.mz %0{%3}, %1, %2":"=v"(ret):"v"(a), "v"(b), "M"(mask));
    return ret;
}

static __device__ inline float16x32_t vvmax_float16x32_mh(float16x32_t a, float16x32_t b, float16x32_t hold, int mask = -1) {
    float16x32_t ret = hold;
    __asm__ __volatile__("vmax.hf.mh %0{%3}, %1, %2" : "+v"(ret) : "v"(a), "v"(b), "M"(mask));
    return ret;
}

/*!
 *\brief XPU3 float16x32_t two vector min. a and b are src vector, min result undermask put in dst vector
 *       'for (int i = 0; i < 16; i++) { if (mask[i]) ret[i] = a[i] < b[i] ? a : b[i] else ret[i] = 0 }'
 *\param src vector variable0
 *\param src vector variable1
 *\param each mask bit control if compute and put result in related element in dst vector
 */
static __device__ inline float16x32_t vvmin_float16x32(float16x32_t a, float16x32_t b) {
    float16x32_t ret;
    __asm__("vmin.hf.mz %0{mr1}, %1, %2":"=v"(ret):"v"(a), "v"(b));
    return ret;
}

/*!
 *\brief XPU3 float16x32_t two vector min. a and b are src vector, min result undermask put in dst vector
 *       unmask behaviour is mz(to zero)
 *       'for (int i = 0; i < 16; i++) { if (mask[i]) ret[i] = a[i] < b[i] ? a : b[i] else ret[i] = 0 }'
 *\param src vector variable0
 *\param src vector variable1
 *\param each mask bit control if compute and put result in related element in dst vector
 */
static __device__ inline float16x32_t vvmin_float16x32_mz(float16x32_t a, float16x32_t b, int mask = -1) {
    float16x32_t ret;
    __asm__("vmin.hf.mz %0{%3}, %1, %2":"=v"(ret):"v"(a), "v"(b), "M"(mask));
    return ret;
}

static __device__ inline float16x32_t vvmin_float16x32_mh(float16x32_t a, float16x32_t b, float16x32_t hold, int mask = -1) {
    float16x32_t ret = hold;
    __asm__("vmin.hf.mh %0{%3}, %1, %2":"+v"(ret):"v"(a), "v"(b), "M"(mask));
    return ret;
}

/*!
 *\brief XPU3 float16x32_t multiply two vector a and b, accumulating with c undermask, return mac result in c
 *       unmask behaviour is mz(to zero), round mode is rn(round to neareast)
 *       'for (int i = 0; i < 16; i++) { if (mask[i]) ret[i] = a[i] * b[i] + c[i] else ret[i] = 0 }'
 *\param src vector variable0
 *\param src vector variable1
 *\param src vector variable2
 *\param each mask bit control if compute and put result in related element in dst vector
 */
static __device__ inline float16x32_t vvmac_float16x32(float16x32_t a, float16x32_t b, float16x32_t c) {
    float16x32_t ret;
    __asm__("vmac.hf.mz.rn %0{mr1}, %1, %2":"+v"(c): "v"(a), "v"(b));
    ret = c;
    return ret;
}

/*!
 *\brief XPU3 float16x32_t multiply two vector a and b, accumulating with c undermask, return mac result in c
 *       unmask behaviour is mz(to zero), round mode is rn(round to neareast)
 *       'for (int i = 0; i < 16; i++) { if (mask[i]) c[i] = a[i] * b[i] + c[i] else c[i] = 0 }'
 *\param src vector variable0
 *\param src vector variable1
 *\param src vector variable2
 *\param each mask bit control if compute and put result in related element in dst vector
 */
static __device__ inline float16x32_t vvmac_float16x32_mz_rn(float16x32_t a, float16x32_t b, float16x32_t c, int mask = -1) {
    float16x32_t ret;
    __asm__("vmac.hf.mz.rn %0{%3}, %1, %2":"+v"(c): "v"(a), "v"(b), "M"(mask));
    ret = c;
    return ret;
}

/*!
 *\brief XPU2 single precision scalar - float16x32_t vector max. a and b are src operand, max result undermask put in dst vector
 *       unmask behaviour is mz(to zero)
 *       'for (int i = 0; i < 16; i++) { if (mask[i]) ret[i] = a > b[i] ? a : b[i] else ret[i] = 0 }'
 *\param src scalar variable0
 *\param src vector variable1
 *\param each mask bit control if compute and put result in related element in dst vector
 */
static __device__ inline float16x32_t svmax_float16x32_mz(float16 a, float16x32_t b, int mask = -1) {
    float16x32_t ret;
    __asm__("vmax.hf.mz %0{%3}, %1, %2":"=v"(ret):"r"(a), "v"(b), "M"(mask));
    return ret;
}

/*!
 *\brief XPU2 single precision scalar - float16x32_t vector max. a and b are src operand, max result undermask put in dst vector
 *       unmask behaviour is mz(to zero)
 *       'for (int i = 0; i < 16; i++) { if (mask[i]) ret[i] = a > b[i] ? a : b[i] else ret[i] = 0 }'
 *\param src scalar variable0
 *\param src vector variable1
 *\param each mask bit control if compute and put result in related element in dst vector
 */
static __device__ inline float16x32_t svmax_float16x32(float16 a, float16x32_t b) {
    float16x32_t ret;
    __asm__("vmax.hf.mz %0{mr1}, %1, %2":"=v"(ret):"r"(a), "v"(b));
    return ret;
}

#define TEMP_TYPE bfloat16x32
#define TEMP_TYPE_t bfloat16x32_t
#define vstore_lm_TEMP_TYPE vstore_lm_bfloat16x32
#define vstore_sm_TEMP_TYPE vstore_sm_bfloat16x32
#define vstore_lm_TEMP_TYPE_mz vstore_lm_bfloat16x32_mz
#endif

#ifndef TEMP_TYPE
#define TEMP_TYPE float16x32
#define TEMP_TYPE_t float16x32_t
#define vstore_lm_TEMP_TYPE vstore_lm_float16x32
#define vstore_sm_TEMP_TYPE vstore_sm_float16x32
#define vstore_lm_TEMP_TYPE_mz vstore_lm_float16x32_mz
#endif

// the same with xpu2
static __device__ inline float32x16_t vset_zero() {
    float32x16_t ret;
    ret = __builtin_xpu2_vvxor_s_mr1(ret, ret);
    return ret;
}

static __device__ inline float16x32_t vset_zero_fp16() {
    float16x32_t ret;
    __asm__("vxor.hf.mz %0{mr1}, %1, %2":"=v"(ret):"v"(ret), "v"(ret));
    return ret;
}

static __device__ inline float32x16_t vfp162fp32_l(float16x32_t& src) {
    return __builtin_xpu2_vfp162float_l_rn(src);
    // float32x16_t ret;
    // __asm__("vfp162float_l.rn %0, %1":"=v"(ret):"v"(src));
    // return ret;
}

static __device__ inline float32x16_t vfp162fp32_h(float16x32_t& src) {
    return __builtin_xpu2_vfp162float_h_rn(src);
    // float32x16_t ret;
    // __asm__("vfp162float_h.rn %0, %1":"=v"(ret):"v"(src));
    // return ret;
}

static __device__ inline int32x16_t vset_zero_int() {
    int32x16_t ret(0);
    return ret;
}

static __device__ inline float32x16_t vset_one() {
    float32x16_t ret;
    ret = svadd_float32x16(1.0f, vset_zero());
    return ret;
}

static __device__ inline void vload2_lm(const float* ptr, float32x16_t& vl, float32x16_t& vh) {
    vl = __builtin_xpu2_vload_mask16_mr1(ptr, 0);
    vh = __builtin_xpu2_vload_mask16_mr1(ptr + 16, 0);
}
static __device__ inline void vload2_lm(const float* ptr, float16x32_t& vl, float16x32_t& vh) {
    TEMP_TYPE_t vfp32_1 = __builtin_xpu2_vload_mr1(ptr, 0);
    TEMP_TYPE_t vfp32_2 = __builtin_xpu2_vload_mr1(ptr + 16, 0);
    TEMP_TYPE_t vfp32_3 = __builtin_xpu2_vload_mr1(ptr + 16 * 2, 0);
    TEMP_TYPE_t vfp32_4 = __builtin_xpu2_vload_mr1(ptr + 16 * 3, 0);
    vl = __builtin_xpu2_vfloat2fp16_lh_rn(vfp32_1, vfp32_2);
    vh = __builtin_xpu2_vfloat2fp16_lh_rn(vfp32_3, vfp32_4);
}
static __device__ inline void vload2_lm(const float16* ptr, float32x16_t& vl, float32x16_t& vh) {
    TEMP_TYPE_t vfp16 = __builtin_xpu2_vload_mr1(ptr, 0);
    vl = __builtin_xpu2_vfp162float_l_rn(vfp16);
    vh = __builtin_xpu2_vfp162float_h_rn(vfp16);
}
static __device__ inline void vload2_lm(const float16* ptr, float16x32_t& vl, float16x32_t& vh) {
    vl = vload_lm_float16x32(ptr);
    vh = vload_lm_float16x32(ptr + 32);
}
static __device__ inline void vload2_lm(const int* ptr, int32x16_t& vl, int32x16_t& vh) {
    vl = vload_lm_int32x16(ptr);
    vh = vload_lm_int32x16(ptr + 16);
}
static __device__ inline void vload2_lm(const bfloat16* ptr, float32x16_t& vl, float32x16_t& vh) {
    int16x32_t vbf16 = vload_lm_int16x32(ptr);
    int16x32_t pad_vbf16(0);
    __asm__("vmerge_l.hf %0, %1, %2":"=&v"(vl):"v"(pad_vbf16), "v"(vbf16));
    __asm__("vmerge_h.hf %0, %1, %2":"=&v"(vh):"v"(pad_vbf16), "v"(vbf16));
}
static __device__ inline void vload2_lm_unordered(const bfloat16* ptr, float32x16_t& veven, float32x16_t& vodd) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(ptr, mask));
    vodd = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(ptr, (~mask)));
    constexpr int pose = 16;
    __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
}
static __device__ inline void vload2_sm(_shared_ptr_ const float* ptr, float32x16_t& vl, float32x16_t& vh) {
    vl = __builtin_xpu2_vloads_mask16_mr1(ptr, 0);
    vh = __builtin_xpu2_vloads_mask16_mr1(ptr + 16, 0);
}
static __device__ inline void vload2_sm(_shared_ptr_ const int* ptr, int32x16_t& vl, int32x16_t& vh) {
    vl = __builtin_xpu2_vloads_mask16_mr1(ptr, 0);
    vh = __builtin_xpu2_vloads_mask16_mr1(ptr + 16, 0);
}
static __device__ inline void vload2_sm(_shared_ptr_ const float16* ptr, float32x16_t& vl, float32x16_t& vh) {
    TEMP_TYPE_t vfp16 = __builtin_xpu2_vloads_mr1(ptr, 0);
    vl = __builtin_xpu2_vfp162float_l_rn(vfp16);
    vh = __builtin_xpu2_vfp162float_h_rn(vfp16);
}
static __device__ inline void vload2_sm(_shared_ptr_ const bfloat16* ptr, float32x16_t& vl, float32x16_t& vh) {
    int16x32_t vbf16 = vload_sm_int16x32(ptr);
    int16x32_t pad_vbf16(0);
    __asm__("vmerge_l.hf %0, %1, %2":"=&v"(vl):"v"(pad_vbf16), "v"(vbf16));
    __asm__("vmerge_h.hf %0, %1, %2":"=&v"(vh):"v"(pad_vbf16), "v"(vbf16));
}
static __device__ inline void vload2_sm_unordered(_shared_ptr_ const bfloat16* ptr, float32x16_t& veven, float32x16_t& vodd) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    veven = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(ptr, mask));
    vodd = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(ptr, (~mask)));
    constexpr int pose = 16;
    __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
}

static __device__ inline void vload2_lm_mz(const float* ptr, float32x16_t& vl, float32x16_t& vh, int mask) {
    vl = __builtin_xpu2_vload_mask16_mz(mask, ptr, 0);
    vh = __builtin_xpu2_vload_mask16_mz((mask >> 16), ptr + 16, 0);
}
static __device__ inline void vload2_lm_mz(const int* ptr, int32x16_t& vl, int32x16_t& vh, int mask) {
    vl = __builtin_xpu2_vload_mask16_mz(mask, ptr, 0);
    vh = __builtin_xpu2_vload_mask16_mz((mask >> 16), ptr + 16, 0);
}
static __device__ inline void vload2_lm_mz(const float16* ptr, float32x16_t& vl, float32x16_t& vh, int mask) {
    TEMP_TYPE_t vfp16 = __builtin_xpu2_vload_mz(mask, ptr, 0);
    vl = __builtin_xpu2_vfp162float_l_rn(vfp16);
    vh = __builtin_xpu2_vfp162float_h_rn(vfp16);
}
static __device__ inline void vload2_lm_mz(const bfloat16* ptr, float32x16_t& vl, float32x16_t& vh, int mask) {
    int16x32_t vbf16 = vload_lm_int16x32_mz(ptr, mask);
    int16x32_t pad_vbf16(0);
    __asm__("vmerge_l.hf %0, %1, %2":"=&v"(vl):"v"(pad_vbf16), "v"(vbf16));
    __asm__("vmerge_h.hf %0, %1, %2":"=&v"(vh):"v"(pad_vbf16), "v"(vbf16));
}
static __device__ inline void vload2_lm_unordered_mz(const bfloat16* ptr, float32x16_t& veven, float32x16_t& vodd, int mz_mask) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(ptr, mask & mz_mask));
    vodd = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz(ptr, (~mask) & mz_mask));
    constexpr int pose = 16;
    __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
}
static __device__ void vload2_sm_mz(_shared_ptr_ const float* ptr, float32x16_t& vl, float32x16_t& vh,
        unsigned int mask) {
    vl = __builtin_xpu2_vloads_mask16_mz(mask, ptr, 0);
    vh = __builtin_xpu2_vloads_mask16_mz((mask >> 16), ptr + 16, 0);
}
static __device__ void vload2_sm_mz(_shared_ptr_ const int* ptr, int32x16_t& vl, int32x16_t& vh,
        unsigned int mask) {
    vl = __builtin_xpu2_vloads_mask16_mz(mask, ptr, 0);
    vh = __builtin_xpu2_vloads_mask16_mz((mask >> 16), ptr + 16, 0);
}
static __device__ void vload2_sm_mz(_shared_ptr_ const float16* ptr, float32x16_t& vl, float32x16_t& vh,
        unsigned int mask) {
    TEMP_TYPE_t vfp16 = __builtin_xpu2_vloads_mz(mask, ptr, 0);
    vl = __builtin_xpu2_vfp162float_l_rn(vfp16);
    vh = __builtin_xpu2_vfp162float_h_rn(vfp16);
}
static __device__ void vload2_sm_mz(_shared_ptr_ const bfloat16* ptr, float32x16_t& vl, float32x16_t& vh,
        unsigned int mask) {
    int16x32_t vbf16 = vload_sm_int16x32_mz(ptr, mask);
    int16x32_t pad_vbf16(0);
    __asm__("vmerge_l.hf %0, %1, %2":"=&v"(vl):"v"(pad_vbf16), "v"(vbf16));
    __asm__("vmerge_h.hf %0, %1, %2":"=&v"(vh):"v"(pad_vbf16), "v"(vbf16));
}
static __device__ inline void vload2_sm_unordered_mz(_shared_ptr_ const bfloat16* ptr, float32x16_t& veven, float32x16_t& vodd, unsigned int mz_mask) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    veven = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(ptr, mask & mz_mask));
    vodd = reinterpret_cast<float32x16_t>(vload_sm_int16x32_mz(ptr, (~mask) & mz_mask));
    constexpr int pose = 16;
    __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
}
static __device__ inline void vload2_gsm_unordered(_group_shared_ptr_ const bfloat16* ptr, float32x16_t& veven, float32x16_t& vodd) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz((const bfloat16*)ptr, mask));
    vodd = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz((const bfloat16*)ptr, (~mask)));
    constexpr int pose = 16;
    __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
}
static __device__ inline void vload2_gsm_unordered_mz(_group_shared_ptr_ const bfloat16* ptr, float32x16_t& veven, float32x16_t& vodd, int mz_mask) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    veven = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz((const bfloat16*)ptr, mask & mz_mask));
    vodd = reinterpret_cast<float32x16_t>(vload_lm_int16x32_mz((const bfloat16*)ptr, (~mask) & mz_mask));
    constexpr int pose = 16;
    __asm__ __volatile__("vsll.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
}
static __device__ inline void vstore2_lm(float* ptr, float32x16_t& vl, float32x16_t& vh) {
    vstore_lm_float32x16(ptr, vl);
    vstore_lm_float32x16(ptr + 16, vh);
}
static __device__ inline void vstore2_lm(float16* ptr, float32x16_t& vl, float32x16_t& vh) {
    TEMP_TYPE_t temp =  __builtin_xpu2_vfloat2fp16_lh_rn(vl, vh);
    vstore_lm_TEMP_TYPE(ptr, temp);
}
static __device__ inline void vstore2_lm(float16* ptr, float16x32_t& vl, float16x32_t& vh) {
    vstore_lm_float16x32(ptr, vl);
    vstore_lm_float16x32(ptr + 32, vh);
}
static __device__ inline void vstore2_lm(int* ptr, int32x16_t& vl, int32x16_t& vh) {
    vstore_lm_int32x16(ptr, vl);
    vstore_lm_int32x16(ptr + 16, vh);
}
static __device__ inline void vstore2_lm(bfloat16* ptr, float32x16_t vl, float32x16_t vh) {
    constexpr uint32_t one = 0x3f800000; // 0b00111111100000000000000000000000
    constexpr int32_t sin_exp_0 = 0x7fffff;// 0b00000000011111111111111111111111
    constexpr int32_t mantissa = 0x7f007f; // 0b00000000011111110000000001111111
    constexpr int pose10 = 10;
    constexpr int pose7 = 7;
    constexpr int pose3 = 3;
    uint32x16_t vl_sign_exp;
    uint32x16_t vh_sign_exp;
    // handle the exp 
    __asm__ __volatile__("vsrl.s.mz %0{mr1}, %1, %2":"=&v"(vl_sign_exp):"r"(pose10), "v"(vl));
    __asm__ __volatile__("vsrl.s.mz %0{mr1}, %1, %2":"=&v"(vh_sign_exp):"r"(pose10), "v"(vh));
    vl_sign_exp = svor_uint32x16(one, vl_sign_exp);
    vh_sign_exp = svor_uint32x16(one, vh_sign_exp);
    vl_sign_exp = reinterpret_cast<uint32x16_t>(vfloat2fp16_lh_rz(reinterpret_cast<float32x16_t>(vl_sign_exp), reinterpret_cast<float32x16_t>(vh_sign_exp)));
    vl_sign_exp = svsll_uint32x16(pose7, vl_sign_exp);
    vl_sign_exp = svand_uint32x16(0xff80ff80, vl_sign_exp);

    // handle the mantissa
    vl = reinterpret_cast<float32x16_t>(svand_uint32x16(sin_exp_0, reinterpret_cast<uint32x16_t>(vl)));
    vh = reinterpret_cast<float32x16_t>(svand_uint32x16(sin_exp_0, reinterpret_cast<uint32x16_t>(vh)));
    vl = reinterpret_cast<float32x16_t>(svor_uint32x16(one, reinterpret_cast<uint32x16_t>(vl)));
    vh = reinterpret_cast<float32x16_t>(svor_uint32x16(one, reinterpret_cast<uint32x16_t>(vh)));
    vl = reinterpret_cast<float32x16_t>(vfloat2fp16_lh_rz(vl, vh));
    vl = reinterpret_cast<float32x16_t>(svsrl_uint32x16(pose3, reinterpret_cast<uint32x16_t>(vl)));


    // merge
    vl = reinterpret_cast<float32x16_t>(svand_uint32x16(mantissa, reinterpret_cast<uint32x16_t>(vl)));
    vl = reinterpret_cast<float32x16_t>(vvor_uint32x16(reinterpret_cast<uint32x16_t>(vl), vl_sign_exp));
    vstore_lm_float16x32(ptr, reinterpret_cast<float16x32_t>(vl));
}
static __device__ inline void vstore2_lm_unordered(bfloat16* ptr, float32x16_t veven, float32x16_t vodd) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    vstore_lm_int16x32_mh(ptr, reinterpret_cast<int16x32_t>(veven), mask);
    constexpr int pose = 16;
    __asm__ __volatile__("vsrl.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
    vstore_lm_int16x32_mh(ptr, reinterpret_cast<int16x32_t>(vodd), (~mask));
}
static __device__ inline void vstore2_lm_mz(float* ptr, float32x16_t& vl, float32x16_t& vh, unsigned int mask1,
        unsigned int mask2) {
    vstore_lm_float32x16_mz(ptr, vl, mask1);
    vstore_lm_float32x16_mz(ptr + 16, vh, mask2);
}
static __device__ inline void vstore2_lm_mz(float16* ptr, float32x16_t& vl, float32x16_t& vh, unsigned int mask1,
        unsigned int mask2) {
    unsigned int bi_mask = 0;
    // bi_mask = (~(((unsigned int)-1) <<  16)) & mask1; // low 16 for mask1
    // bi_mask <<= 16; // hi 16 for mask1
    // bi_mask |= (~(((unsigned int)-1) <<  16)) & mask2; // low 16 for mask2
    mask1 &= 0xffff;
    mask2 &= 0xffff;
    bi_mask = (mask2 << 16) | mask1;
    TEMP_TYPE_t temp =  __builtin_xpu2_vfloat2fp16_lh_rn(vl, vh);
    vstore_lm_TEMP_TYPE_mz(ptr, temp, bi_mask);
}
static __device__ void vstore2_sm(_shared_ptr_ float* ptr, float32x16_t& vl, float32x16_t& vh) {
    vstore_sm_float32x16(ptr, vl);
    vstore_sm_float32x16(ptr + 16, vh);
}
static __device__ void vstore2_sm(_shared_ptr_ int* ptr, int32x16_t& vl, int32x16_t& vh) {
    vstore_sm_int32x16(ptr, vl);
    vstore_sm_int32x16(ptr + 16, vh);
}
static __device__ void vstore2_sm(_shared_ptr_ float16* ptr, float32x16_t& vl, float32x16_t& vh) {
    TEMP_TYPE_t temp =  __builtin_xpu2_vfloat2fp16_lh_rn(vl, vh);
    vstore_sm_TEMP_TYPE(ptr, temp);
}
static __device__ void vstore2_sm(_shared_ptr_ bfloat16* ptr, float32x16_t vl, float32x16_t vh) {
    constexpr uint32_t one = 0x3f800000; // 0b00111111100000000000000000000000
    constexpr int32_t sin_exp_0 = 0x7fffff;// 0b00000000011111111111111111111111
    constexpr int32_t mantissa = 0x7f007f; // 0b00000000011111110000000001111111
    constexpr int pose10 = 10;
    constexpr int pose7 = 7;
    constexpr int pose3 = 3;
    uint32x16_t vl_sign_exp;
    uint32x16_t vh_sign_exp;
    // handle the exp 
    __asm__ __volatile__("vsrl.s.mz %0{mr1}, %1, %2":"=&v"(vl_sign_exp):"r"(pose10), "v"(vl));
    __asm__ __volatile__("vsrl.s.mz %0{mr1}, %1, %2":"=&v"(vh_sign_exp):"r"(pose10), "v"(vh));
    vl_sign_exp = svor_uint32x16(one, vl_sign_exp);
    vh_sign_exp = svor_uint32x16(one, vh_sign_exp);
    vl_sign_exp = reinterpret_cast<uint32x16_t>(vfloat2fp16_lh_rz(reinterpret_cast<float32x16_t>(vl_sign_exp), reinterpret_cast<float32x16_t>(vh_sign_exp)));
    vl_sign_exp = svsll_uint32x16(pose7, vl_sign_exp);
    vl_sign_exp = svand_uint32x16(0xff80ff80, vl_sign_exp);

    // handle the mantissa
    vl = reinterpret_cast<float32x16_t>(svand_uint32x16(sin_exp_0, reinterpret_cast<uint32x16_t>(vl)));
    vh = reinterpret_cast<float32x16_t>(svand_uint32x16(sin_exp_0, reinterpret_cast<uint32x16_t>(vh)));
    vl = reinterpret_cast<float32x16_t>(svor_uint32x16(one, reinterpret_cast<uint32x16_t>(vl)));
    vh = reinterpret_cast<float32x16_t>(svor_uint32x16(one, reinterpret_cast<uint32x16_t>(vh)));
    vl = reinterpret_cast<float32x16_t>(vfloat2fp16_lh_rz(vl, vh));
    vl = reinterpret_cast<float32x16_t>(svsrl_uint32x16(pose3, reinterpret_cast<uint32x16_t>(vl)));


    // merge
    vl = reinterpret_cast<float32x16_t>(svand_uint32x16(mantissa, reinterpret_cast<uint32x16_t>(vl)));
    vl = reinterpret_cast<float32x16_t>(vvor_uint32x16(reinterpret_cast<uint32x16_t>(vl), vl_sign_exp));
    vstore_sm_float16x32(ptr, reinterpret_cast<float16x32_t>(vl));
}
static __device__ inline void vstore2_sm_unordered(_shared_ptr_ bfloat16* ptr, float32x16_t veven, float32x16_t vodd) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    vstore_sm_int16x32_mh(ptr, reinterpret_cast<int16x32_t>(veven), mask);
    constexpr int pose = 16;
    __asm__ __volatile__("vsrl.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
    vstore_sm_int16x32_mh(ptr, reinterpret_cast<int16x32_t>(vodd), (~mask));
}
static __device__ inline void vstore2_gsm_unordered(_group_shared_ptr_ bfloat16* ptr, float32x16_t veven, float32x16_t vodd) {
    constexpr int mask = 0xaaaaaaaa; // 0b10101010101010101010101010101010
    vstore_lm_int16x32_mh((bfloat16*)ptr, reinterpret_cast<int16x32_t>(veven), mask);
    constexpr int pose = 16;
    __asm__ __volatile__("vsrl.p %0, %1, %2":"=&v"(vodd):"r"(pose), "v"(vodd));
    vstore_lm_int16x32_mh((bfloat16*)ptr, reinterpret_cast<int16x32_t>(vodd), (~mask));
}
// #endif // __arch_xpu2__
#endif //

