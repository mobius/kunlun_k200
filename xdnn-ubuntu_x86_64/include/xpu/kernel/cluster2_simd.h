#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_SIMD_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_SIMD_H
#include "xpu/kernel/cluster2.h"
#include "xpu/kernel/xtdk_simd.h"

static __device__ inline float32x16_t vset_zero() {
    float32x16_t ret;
    ret = __builtin_xpu2_vvxor_s_mr1(ret, ret);
    return ret;
}
static __device__ inline int32x16_t vset_zero_int() {
    int32x16_t ret(0);
    return ret;
}
static __device__ inline bfloat16x32_t vset_zero_bf() {
    bfloat16x32_t ret;
    ret = __builtin_xpu2_vvxor_bf_mr1_rn(ret, ret);
    return ret;
}

static __device__ inline float32x16_t vset_one() {
    float32x16_t ret;
    ret = svadd_float32x16(1.0f, vset_zero());
    return ret;
}
static __device__ inline int32x16_t vset_one_int() {
    int32x16_t ret;
    ret = svadd_int32x16(1.0f, vset_zero_int());
    return ret;
}
static __device__ inline bfloat16x32_t vset_one_bf() {
    bfloat16x32_t ret;
    ret = svadd_bfloat16x32(1.0f, vset_zero_bf());
    return ret;
}

static __device__ inline void vload2_lm(const float* ptr, float32x16_t& vl, float32x16_t& vh) {
    vl = vload_lm_float32x16(ptr);
    vh = vload_lm_float32x16(ptr + 16);
}
static __device__ inline void vload2_lm(const float16* ptr, float32x16_t& vl, float32x16_t& vh) {
    bfloat16x32_t vfp16 = __builtin_xpu2_vload_mr1(ptr, 0);
    vl = __builtin_xpu2_vfp162float_l_rn(vfp16);
    vh = __builtin_xpu2_vfp162float_h_rn(vfp16);
}
static __device__ inline void vload2_lm(const bfloat16* ptr, float32x16_t& vl, float32x16_t& vh) {
    bfloat16x32_t vbf16 = __builtin_xpu2_vload_mr1(ptr, 0);
    vl = __builtin_xpu2_vbf162float_l(vbf16);
    vh = __builtin_xpu2_vbf162float_h(vbf16);
}
static __device__ inline void vload2_lm(const int* ptr, int32x16_t& vl, int32x16_t& vh) {
    vl = vload_lm_int32x16(ptr);
    vh = vload_lm_int32x16(ptr + 16);
}
static __device__ inline void vload2_lm(const bfloat16* ptr, bfloat16x32_t& vl, bfloat16x32_t& vh) {
    vl = vload_lm_bfloat16x32(ptr);
    vh = vload_lm_bfloat16x32(ptr + 32);
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
    bfloat16x32_t vfp16 = __builtin_xpu2_vloads_mr1(ptr, 0);
    vl = __builtin_xpu2_vfp162float_l_rn(vfp16);
    vh = __builtin_xpu2_vfp162float_h_rn(vfp16);
}
static __device__ inline void vload2_sm(_shared_ptr_ const bfloat16* ptr, bfloat16x32_t& vl, bfloat16x32_t& vh) {
    vl = __builtin_xpu2_vloads_mr1(ptr, 0);
    vh = __builtin_xpu2_vloads_mr1(ptr + 32, 0);
}
static __device__ inline void vload2_sm(_shared_ptr_ const bfloat16* ptr, float32x16_t& vl, float32x16_t& vh) {
    bfloat16x32_t vbf16 = __builtin_xpu2_vloads_mr1(ptr, 0);
    vl = __builtin_xpu2_vbf162float_l(vbf16);
    vh = __builtin_xpu2_vbf162float_h(vbf16);
}

static __device__ inline void vload2_lm_mz(const float* ptr, float32x16_t& vl, float32x16_t& vh, int mask) {
    vl = vload_lm_float32x16_mz(ptr, mask);
    vh = vload_lm_float32x16_mz(ptr + 16, (mask >> 16));
}
static __device__ inline void vload2_lm_mz(const float16* ptr, float32x16_t& vl, float32x16_t& vh, int mask) {
    bfloat16x32_t vfp16 = __builtin_xpu2_vload_mz(mask, ptr, 0);
    vl = __builtin_xpu2_vfp162float_l_rn(vfp16);
    vh = __builtin_xpu2_vfp162float_h_rn(vfp16);
}
static __device__ inline void vload2_lm_mz(const int* ptr, int32x16_t& vl, int32x16_t& vh, int mask) {
    vl = vload_lm_int32x16_mz(ptr, mask);
    vh = vload_lm_int32x16_mz(ptr + 16, (mask >> 16));
}
static __device__ inline void vload2_lm_mz(const bfloat16* ptr, float32x16_t& vl, float32x16_t& vh, int mask) {
    bfloat16x32_t vbf16 = __builtin_xpu2_vload_mz(mask, ptr, 0);
    vl = __builtin_xpu2_vbf162float_l(vbf16);
    vh = __builtin_xpu2_vbf162float_h(vbf16);
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
    bfloat16x32_t vfp16 = __builtin_xpu2_vloads_mz(mask, ptr, 0);
    vl = __builtin_xpu2_vfp162float_l_rn(vfp16);
    vh = __builtin_xpu2_vfp162float_h_rn(vfp16);
}
static __device__ inline void vload2_sm_mz(_shared_ptr_ const bfloat16* ptr, float32x16_t& vl, float32x16_t& vh, int mask) {
    bfloat16x32_t vbf16 = __builtin_xpu2_vloads_mz(mask, ptr, 0);
    vl = __builtin_xpu2_vbf162float_l(vbf16);
    vh = __builtin_xpu2_vbf162float_h(vbf16);
}

static __device__ inline void vstore2_lm(float* ptr, float32x16_t& vl, float32x16_t& vh) {
    vstore_lm_float32x16(ptr, vl);
    vstore_lm_float32x16(ptr + 16, vh);
}
static __device__ inline void vstore2_lm(float16* ptr, float32x16_t& vl, float32x16_t& vh) {
    bfloat16x32_t temp =  __builtin_xpu2_vfloat2fp16_lh_rn(vl, vh);
    vstore_lm_bfloat16x32(ptr, temp);
}
static __device__ inline void vstore2_lm(bfloat16* ptr, float32x16_t& vl, float32x16_t& vh) {
    bfloat16x32_t temp =  __builtin_xpu2_vfloat2bf16_lh_rn(vl, vh);
    vstore_lm_bfloat16x32(ptr, temp);
}
static __device__ inline void vstore2_lm(int* ptr, int32x16_t& vl, int32x16_t& vh) {
    vstore_lm_int32x16(ptr, vl);
    vstore_lm_int32x16(ptr + 16, vh);
}
static __device__ inline void vstore2_lm(bfloat16* ptr, bfloat16x32_t& vl, bfloat16x32_t& vh) {
    vstore_lm_bfloat16x32(ptr, vl);
    vstore_lm_bfloat16x32(ptr + 32, vh);
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
    bfloat16x32_t temp =  __builtin_xpu2_vfloat2fp16_lh_rn(vl, vh);
    vstore_lm_bfloat16x32_mz(ptr, temp, bi_mask);
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
    bfloat16x32_t temp =  __builtin_xpu2_vfloat2fp16_lh_rn(vl, vh);
    vstore_sm_bfloat16x32(ptr, temp);
}
static __device__ void vstore2_sm(_shared_ptr_ bfloat16* ptr, bfloat16x32_t& vl, bfloat16x32_t& vh) {
    vstore_sm_bfloat16x32(ptr, vl);
    vstore_sm_bfloat16x32(ptr + 32, vh);
}
static __device__ inline void vstore2_sm(_shared_ptr_ bfloat16* ptr, float32x16_t& vl, float32x16_t& vh) {
    bfloat16x32_t temp =  __builtin_xpu2_vfloat2bf16_lh_rn(vl, vh);
    vstore_sm_bfloat16x32(ptr, temp);
}
// #endif // __arch_xpu2__
#endif //

