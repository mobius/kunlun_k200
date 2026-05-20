#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_TYPE_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER3_TYPE_H

#if (defined(__XPU3__) || defined(__XPU4__))
struct bfloat16 {
public:
    signed short val_;

public:
    __device__ bfloat16() {}
    __device__ bfloat16(float a) {
        __asm__("float2bfloat.rn %0, %1" : "=r"(val_) : "r"(a));
    }
    __device__ bfloat16(short a) {
        val_ = a;
    }
    bfloat16(float a) { val_ = (unsigned short)a; }
    inline __device__ bfloat16 operator=(bfloat16 a) {
        val_ = a.val_;
        return *this;
    }
    inline __device__ bfloat16& operator=(const float& a) {
        __asm__("float2bfloat.rn %0, %1" : "=r"(val_) : "r"(a));
        return *this;
    }
    friend inline __device__ bool operator == (const bfloat16& a, const bfloat16& b) {
      return a.val_ == b.val_;
    }
    friend inline __device__ bool operator != (const bfloat16& a, const bfloat16& b) {
      return a.val_ != b.val_;
    }
    friend inline __device__ bfloat16 operator+(const bfloat16& a, const bfloat16& b) {
        bfloat16 ret((short)0);
        __asm__("add.bf.rn %0, %1, %2" : "=r"(ret.val_) : "r"(a.val_), "r"(b.val_));
        return ret;
    }
    friend inline __device__ bfloat16 operator-(const bfloat16& a, const bfloat16& b) {
        bfloat16 ret((short)0);
        __asm__("sub.bf.rn %0, %1, %2" : "=r"(ret.val_) : "r"(a.val_), "r"(b.val_));
        return ret;
    }
    friend inline __device__ bfloat16 operator*(const bfloat16& a, const bfloat16& b) {
        bfloat16 ret((short)0);
        __asm__("mul.bf.rn %0, %1, %2" : "=r"(ret.val_) : "r"(a.val_), "r"(b.val_));
        return ret;
    }
    friend inline __device__ bfloat16 operator/(const bfloat16& a, const bfloat16& b) {
        bfloat16 ret((short)0);
        float a_f = 0.0;
        float b_f = 0.0;
        float ret_f = 0.0;
        __asm__("bfloat2float.rn %0, %1" : "=r"(a_f) : "r"(a.val_));
        __asm__("bfloat2float.rn %0, %1" : "=r"(b_f) : "r"(b.val_));
        ret_f = __builtin_xpu_fdiv_rn(a_f, b_f);
        __asm__("float2bfloat.rn %0, %1" : "=r"(ret.val_) : "r"(ret_f));
        return ret;
    }
    explicit inline __device__ operator float() const;
};

typedef bfloat16 bfloat16_t;

static __device__ inline bfloat16_t float2bfloat(const float a) {
    bfloat16 ret((short)0);
    __asm__("float2bfloat.rn %0, %1" : "=r"(ret.val_) : "r"(a));
    return ret;
}

static __device__ inline float bfloat2float(const bfloat16_t a) {
    float ret = 0.0;
    __asm__("bfloat2float.rn %0, %1" : "=r"(ret) : "r"(a.val_));
    return ret;
}

__device__ inline bfloat16::operator float() const {
    return bfloat2float(*this);
}
#endif //__arch_xpu3__

#ifdef __xpu__
typedef struct {
    unsigned short val;
} bit16_t;
#endif // __xpu__

#if defined(__XPU3__) || defined(__XPU4__)
#include <xpu/kernel/xtdk_fp16.h>
#include <xpu/kernel/xtdk_simd.h>
struct float16 {
public:
    union {
        _Float16 data;
        signed short val_;
    };
public:
#ifdef __XPU_KERNEL_XTDK_SIMD_XPU3_H
    __device__ operator xpufloat16_t() { return xpufloat16_t(val_); }
    __device__ float16(int a) = delete;
    __device__ float16(xpufloat16_t arg) { val_ = arg.val_; }
    inline __device__ float16& operator=(xpufloat16_t a) {
        val_ = a.val_;
        return *this;
    }

    __device__ operator half() { return half(data); }
    __device__ float16(half arg) { data = static_cast<__half_raw>(arg).data; }
    inline __device__ float16& operator=(half a) {
        data = static_cast<__half_raw>(a).data;
        return *this;
    }
#endif
    __device__ float16() {}
    __device__ float16(float a) {
        data = static_cast<_Float16>(a);
    }
    __device__ float16(short a) {
        val_ = a;
    }
    inline __device__ float16 operator = (float16 a) {
        val_ = a.val_;
        return *this;
    }
    inline __device__ float16 operator - () const {
        float16 ret;
        ret.val_ = val_ ^ (1 << 15);
        return ret;
    }
    inline __device__ float16& operator = (const float& a) {
        data = static_cast<_Float16>(a);
        return *this;
    }
    inline __device__ float16 operator += (const float16& a) {
        data = data + a.data;
        return *this;
    }
    friend inline __device__ float16 operator + (const float16& a, const float16& b) {
        float16 ret;
        ret.data = a.data + b.data;
        return ret;
    }
    friend inline __device__ float16 operator - (const float16& a, const float16& b) {
        float16 ret;
        ret.data = a.data - b.data;
        return ret;
    }
    friend inline __device__ float16 operator * (const float16& a, const float16& b) {
        float16 ret;
        ret.data = a.data * b.data;
        return ret;
    }
    friend inline __device__ float16 max_zero(const float16& input) {
        float16 ret;
        float16 zero(0.0f);
        ret.data = fmax_hf(zero.data, input.data);
        return ret;
    }
    friend inline __device__ float16 sqrt(const float16& input) {
        float16 ret;
        float a_f = static_cast<float>(input.data);
        float ret_f = __builtin_xpu_sqrtf(a_f);
        ret.data = static_cast<_Float16>(ret_f);
        return ret;
    }
    friend inline __device__ float16 operator / (const float16& a, const float16& b) {
        float16 ret;
        float a_f = static_cast<float>(a.data);
        float b_f = static_cast<float>(b.data);
        float ret_f = a_f / b_f;
        ret.data = static_cast<_Float16>(ret_f);
        return ret;
    }
    friend inline __device__ float16 operator / (const float16& a, const long long& b) {
        float16 ret;
        float a_f = static_cast<float>(a.data);
        float b_f = (float)b;
        float ret_f = a_f / b_f;
        ret.data = static_cast<_Float16>(ret_f);
        return ret;
    }
    friend inline __device__ float16 operator / (const float& a, const float16& b) {
        float16 ret;
        float a_f = a;
        float b_f = static_cast<float>(b.data);
        float ret_f = a_f / b_f;
        ret.data = static_cast<_Float16>(ret_f);
        return ret;
    }
    friend inline __device__ float16 operator / (const float16& a, const float& b) {
        float16 ret;
        float a_f = static_cast<float>(a.data);
        float b_f = b;
        float ret_f = a_f / b_f;
        ret.data = static_cast<_Float16>(ret_f);
        return ret;
    }
    friend inline __device__ bool operator == (const float16& a, const float16& b) {
        return a.val_ == b.val_;
    }
    explicit inline __device__ operator float() const;
};

typedef float16 float16_t;

static __device__ inline float16_t float2float16(const float a) {
    float16 ret((short)0);
    ret.data = static_cast<_Float16>(a);
    return ret;
}

static __device__ inline float float162float(const float16 a) {
    float ret = static_cast<float>(a.data);
    return ret;
}
inline __device__ float16::operator float() const {
    return float162float(*this);
}
#endif  //__arch_xpu3__

#ifdef __xpu__
typedef struct {
    int8_t val;
} int4_t;
#endif // __xpu__

#ifdef __xpu__
typedef struct {
    unsigned int val;
} tfloat32;
#endif // __xpu__

#endif
