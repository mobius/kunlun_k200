/*!\file  copy from xtdk_bfloat16.h
 * \brief XPU bfloat16类型
*/

#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_TYPE_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_TYPE_H

#ifdef __xpu__

#if defined(__arch_xpu2__) || defined(__arch_xpu3__) || defined(__xpu_on_host__)

class bfloat16 {
 public:
   signed short val_;

 public:
  __device__ bfloat16() {}
  __device__ bfloat16(float a) {
    val_ = __builtin_xpu_float2bf16_rn(a);
  }
  __device__ bfloat16(short a) {
    val_ = a;
  }
  bfloat16(float a) {
    val_ = (unsigned short)a;
  }
  inline __device__ bfloat16 operator = (bfloat16 a) {
    val_ = a.val_;
    return *this;
  }
  friend inline __device__ bool operator == (const bfloat16& a, const bfloat16& b) {
    return a.val_ == b.val_;
  }
  friend inline __device__ bool operator != (const bfloat16& a, const bfloat16& b) {
    return a.val_ != b.val_;
  }
  inline __device__ bfloat16& operator = (const float& a) {
    val_ = __builtin_xpu_float2bf16_rn(a);
    return *this;
  }
  friend inline __device__ bfloat16 operator + (const bfloat16& a, const bfloat16& b) {
    bfloat16 ret((short)0);
    ret.val_ =  __builtin_xpu_add_bf16_rn(a.val_, b.val_);
    return ret;
  }
  friend inline __device__ bfloat16 operator - (const bfloat16& a, const bfloat16& b) {
    bfloat16 ret((short)0);
    ret.val_ = __builtin_xpu_sub_bf16_rn(a.val_, b.val_);
    return ret;
  }
  friend inline __device__ bfloat16 operator * (const bfloat16& a, const bfloat16& b) {
    bfloat16 ret((short)0);
    ret.val_ = __builtin_xpu_mul_bf16_rn(a.val_, b.val_);
    return ret;
  }
  friend inline __device__ bfloat16 operator / (const bfloat16& a, const bfloat16& b) {
    bfloat16 ret((short)0);
    float a_f = 0.0;
    float b_f = 0.0;
    float ret_f = 0.0;
    a_f = __builtin_xpu_bf162float_rn(a.val_);
    b_f = __builtin_xpu_bf162float_rn(b.val_);
    ret_f = __builtin_xpu_fdiv_rn(a_f,b_f);
    ret.val_ = __builtin_xpu_float2bf16_rn(ret_f);
    return ret;
  }
};

typedef bfloat16 bfloat16_t;

static __device__ inline bfloat16_t float2bfloat(const float a) {
  bfloat16 ret((short)0);
  ret.val_ = __builtin_xpu_float2bf16_rn(a);
  return ret;
}

static __device__ inline float bfloat2float(const bfloat16_t a) {
  float ret = 0.0;
  ret = __builtin_xpu_bf162float_rn(a.val_);
  return ret;
}

#endif // __arch_xpu2__ __arch_xpu3__ __xpu_on_host__
#endif // __xpu__

#ifdef __xpu__
typedef struct {
    unsigned short val;
} float16;
#endif // __xpu__

#ifdef __xpu__
typedef struct {
    int8_t val;
} int4_t;
#endif // __xpu__

#endif // BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_TYPE_H
