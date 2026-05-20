/**
 * @file cluster3_primitive_template.h
 * @author wangrun(wangrun06@baidu.com)
 * @brief
 * @version 0.1
 * @date 2023-06-27
 *
 * @copyright Copyright (c) 2023
 *
 * @brief primitivle template for cluster3
 *  1. LmPtr, SmPtr, GsmPtr to generalize different memory domain ops
 *  So write one op, is writing all 3 domain ops
 *
 *  2. template T generalize all different type ops: float, fp16, uint32
 *  So one op is suitable for all data types
 */

#ifndef XPU_KERNEL_CLUSTER_PRIMITIVE_TEMPLATE_H_
#define XPU_KERNEL_CLUSTER_PRIMITIVE_TEMPLATE_H_
#include "xpu/kernel/cluster_simd.h"

template <typename Type> struct LmPtr {
    using T = Type;
    __device__ LmPtr(T* p) : ptr(p) {}
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    T* ptr;
};

template <> struct LmPtr<float> {
    using VT = float32x16_t;
    using T = float;
    __device__ LmPtr(float* p) : ptr(p) {}
    __device__ float32x16_t vload(int offset = 0) { return vload_lm_float32x16(ptr + offset); }
    __device__ void vstore(int offset, VT a) { vstore_lm_float32x16(ptr + offset, a); }
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    float* ptr;
};

template <> struct LmPtr<bfloat16> {
    using VT = bfloat16x32_t;
    using T = bfloat16;
    __device__ LmPtr(bfloat16* p) : ptr(p) {}
    __device__ bfloat16x32_t vload(int offset = 0) { return vload_lm_bfloat16x32(ptr + offset); }
    __device__ void vstore(int offset, VT a) { vstore_lm_bfloat16x32(ptr + offset, a); }
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    bfloat16* ptr;
};

template <typename T> struct SmPtr;

template <> struct SmPtr<float> {
    using VT = float32x16_t;
    using T = float;
    __device__ SmPtr(_shared_ptr_ float* p) : ptr(p) {}
    __device__ float32x16_t vload(int offset = 0) { return vload_sm_float32x16(ptr + offset); }
    __device__ void vstore(int offset, VT a) { vstore_sm_float32x16(ptr + offset, a); }
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2SM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2SM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { SM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { SM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_sm(); }
    _shared_ptr_ float* ptr;
};

template <typename T> struct GsmPtr;

template <> struct GsmPtr<float> {
    using VT = float32x16_t;
    using T = float;
    __device__ GsmPtr(_group_shared_ptr_ float* p) : ptr(p) {}
    __device__ float32x16_t vload(int offset = 0) { return vload_gsm_float32x16(ptr + offset); }
    __device__ void vstore(int offset, VT a) { vstore_gsm_float32x16(ptr + offset, a); }
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2GSM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2GSM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { GSM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { GSM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    _group_shared_ptr_ float* ptr;
};

template <> struct LmPtr<float16> {
#ifdef __XPU3__
    using VT = float16x32_t;
#endif
    using T = float16;
    __device__ LmPtr(float16* p) : ptr(p) {}
#ifdef __XPU3__
    __device__ float16x32_t vload(int offset = 0) { return vload_lm_float16x32(ptr + offset); }
    __device__ void vstore(int offset, VT a) { vstore_lm_float16x32(ptr + offset, a); }
#endif
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    float16* ptr;
};

template <> struct SmPtr<float16> {
#ifdef __XPU3__
    using VT = float16x32_t;
#endif
    using T = float16;
    __device__ SmPtr(_shared_ptr_ float16* p) : ptr(p) {}
#ifdef __XPU3__
    __device__ float16x32_t vload(int offset = 0) { return vload_sm_float16x32(ptr + offset); }
    __device__ void vstore(int offset, VT a) { vstore_sm_float16x32(ptr + offset, a); }
#endif
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2SM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2SM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { SM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { SM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_sm(); }
    _shared_ptr_ float16* ptr;
};

template <> struct GsmPtr<float16> {
#ifdef __XPU3__
    using VT = float16x32_t;
#endif
    using T = float16;
    __device__ GsmPtr(_group_shared_ptr_ float16* p) : ptr(p) {}
#ifdef __XPU3__
    __device__ float16x32_t vload(int offset = 0) { return vload_gsm_float16x32(ptr + offset); }
    __device__ void vstore(int offset, VT a) { vstore_gsm_float16x32(ptr + offset, a); }
#endif
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2GSM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2GSM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { GSM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { GSM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    _group_shared_ptr_ float16* ptr;
};

template <> struct GsmPtr<bfloat16> {
#ifdef __XPU2__
    using VT = bfloat16x32_t;
#elif defined(__XPU3__)
    using VT = float32x16_t;
#else
#endif
    using T = bfloat16;
    __device__ GsmPtr(_group_shared_ptr_ bfloat16* p) : ptr(p) {}
#ifdef __XPU2__
    __device__ bfloat16x32_t vload(int offset = 0) { return vload_gsm_bfloat16x32(ptr + offset); }
    __device__ void vstore(int offset, VT a) { vstore_gsm_bfloat16x32(ptr + offset, a); }
#endif
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2GSM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2GSM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { GSM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { GSM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    _group_shared_ptr_ bfloat16* ptr;
};


template <> struct LmPtr<bool> {
    using T = bool;
    __device__ LmPtr(bool* p) : ptr(p) {}
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    bool* ptr;
};

template <> struct LmPtr<uint32_t> {
    using T = uint32_t;
    __device__ LmPtr(uint32_t* p) : ptr(p) {}
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    uint32_t* ptr;
};

template <> struct LmPtr<int> {
    using T = int;
    __device__ LmPtr(int* p) : ptr(p) {}
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    int* ptr;
};

template <> struct LmPtr<char> {
    using T = char;
    __device__ LmPtr(char* p) : ptr(p) {}
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    char* ptr;
};

template <> struct LmPtr<long long> {
    using T = long long;
    __device__ LmPtr(long long* p) : ptr(p) {}
    __device__ void gm_load_async(_global_ptr_ const T* _gm, uint32_t len) { GM2LM_ASYNC(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_load(_global_ptr_ const T* _gm, uint32_t len) { GM2LM(_gm, ptr, len * sizeof(T)); }
    __device__ void gm_store_async(_global_ptr_ T* _gm, uint32_t len) { LM2GM_ASYNC(ptr, _gm, len * sizeof(T)); }
    __device__ void gm_store(_global_ptr_ T* _gm, uint32_t len) { LM2GM(ptr, _gm, len * sizeof(T)); }
    __device__ void mfence() { mfence_lm(); }
    long long* ptr;
};

/**
 * @brief continuous allocated double buffer
 * subclass of Ptr, so can be seen and used as LmPtr/SmPtr/GsmPtr
 * but with toggle and next function
 *
 *  the double buffer should be 64B aligned and 2 buffers must be continuous in mem
 *
 * @tparam BUFSIZE
 * @tparam Ptr
 */
template <int BUFSIZE, typename Ptr> struct DoublePtr : public Ptr {
    using T = typename Ptr::T;
    __device__ DoublePtr(Ptr p) : Ptr(p) {
        ptr_arr_[0] = p;
        ptr_arr_[1] = p + BUFSIZE;
    }
    __device__ DoublePtr(Ptr p1, Ptr p2) : Ptr(p1) {
        ptr_arr_[0] = p1;
        ptr_arr_[1] = p2;
    }
    __device__ void toggle() {
        bufidx ^= 1;
        Ptr::ptr = ptr_arr_[bufidx].ptr;
    }
    __device__ Ptr next() const { return  ptr_arr_[(bufidx ^ 1)]; }
    int bufidx{0};
    Ptr ptr_arr_[2] = {0, 0};
};

/**
 * @brief continuous allocated ring buffer
 * subclass of Ptr, so can be seen and used as LmPtr/SmPtr/GsmPtr
 * but with toggle and next function
 *
 *  the ring buffer should be 64B aligned and 3 buffers must be continuous in mem
 *
 * @tparam BUFSIZE
 * @tparam Ptr
 */
template <int BUFSIZE, typename Ptr> struct TriplePtr : public Ptr {
    using T = typename Ptr::T;
    __device__ TriplePtr(Ptr p) : Ptr(p) {
        ptr_arr_[0] = p;
        ptr_arr_[1] = p + BUFSIZE;
        ptr_arr_[2] = p + 2 * BUFSIZE;
    }
    __device__ TriplePtr(Ptr p1, Ptr p2, Ptr p3) : Ptr(p1) {
        ptr_arr_[0] = p1;
        ptr_arr_[1] = p2;
        ptr_arr_[2] = p3;
    }
    __device__ void toggle() {
        bufidx = (bufidx + 1) % 3;
        Ptr::ptr = ptr_arr_[bufidx].ptr;
    }
    __device__ Ptr next() const { return  ptr_arr_[(bufidx + 1) % 3]; }
    int bufidx{0};
    Ptr ptr_arr_[3] = {0, 0, 0};
};


template <typename T> __device__ LmPtr<T> operator+(LmPtr<T> p, int n) { return LmPtr<T>(p.ptr + n); }
template <typename T> __device__ SmPtr<T> operator+(SmPtr<T> p, int n) { return SmPtr<T>(p.ptr + n); }
template <typename T> __device__ GsmPtr<T> operator+(GsmPtr<T> p, int n) { return GsmPtr<T>(p.ptr + n); }

__device__ float32x16_t __vvxor(float32x16_t a, float32x16_t b) { return vvxor_float32x16(a, b); }
__device__ int32x16_t __vvxor(int32x16_t a, int32x16_t b) { return vvxor_int32x16(a, b); }
__device__ uint32x16_t __vvxor(uint32x16_t a, uint32x16_t b) { return vvxor_uint32x16(a, b); }
__device__ float32x16_t __vvadd(float32x16_t a, float32x16_t b) { return vvadd_float32x16(a, b); }
__device__ int32x16_t __vvadd(int32x16_t a, int32x16_t b) { return vvadd_int32x16(a, b); }
__device__ uint32x16_t __vvadd(uint32x16_t a, uint32x16_t b) { return vvadd_uint32x16(a, b); }
__device__ float32x16_t __svadd(float a, float32x16_t b) { return svadd_float32x16(a, b); }
__device__ int32x16_t __svadd(int a, int32x16_t b) { return svadd_int32x16(a, b); }
__device__ uint32x16_t __svadd(uint32_t a, uint32x16_t b) { return svadd_uint32x16(a, b); }
__device__ float32x16_t __vvmul(float32x16_t a, float32x16_t b) { return vvmul_float32x16(a, b); }
__device__ int32x16_t __vvmul(int32x16_t a, int32x16_t b) { return vvmul_int32x16(a, b); }
__device__ uint32x16_t __vvmul(uint32x16_t a, uint32x16_t b) { return vvmul_uint32x16(a, b); }
__device__ float32x16_t __svmul(float a, float32x16_t b) { return svmul_float32x16(a, b); }
__device__ int32x16_t __svmul(int a, int32x16_t b) { return svmul_int32x16(a, b); }
__device__ uint32x16_t __svmul(uint32_t a, uint32x16_t b) { return svmul_uint32x16(a, b); }

#ifdef __XPU3__
__device__ float16x32_t __vvxor(float16x32_t a, float16x32_t b) { return vvxor_float16x32(a, b); }
__device__ float16x32_t __vvadd(float16x32_t a, float16x32_t b) { return vvadd_float16x32(a, b); }
__device__ float16x32_t __svadd(float16 a, float16x32_t b) { return svadd_float16x32(a, b); }
__device__ float16x32_t __vvmul(float16x32_t a, float16x32_t b) { return vvmul_float16x32(a, b); }
__device__ float16x32_t __svmul(float16 a, float16x32_t b) { return svmul_float16x32(a, b); }
#endif  // __XPU3__

template <typename T> struct VTrait;

template <> struct VTrait<float> {
    template <typename Ptr1, typename Ptr2, typename Fn> static __device__ void add128(Ptr1 x, Ptr2 y, Fn __fn) {
        auto v0 = x.vload();
        auto v1 = y.vload();
        auto v2 = x.vload(16);
        auto v3 = y.vload(16);
        auto v4 = x.vload(32);
        auto v5 = y.vload(32);
        auto v6 = x.vload(48);
        auto v7 = y.vload(48);
        __fn(v0, v1);
        x.vstore(0, v0);
        v0 = x.vload(64);
        v1 = y.vload(64);
        __fn(v2, v3);
        x.vstore(16, v2);
        v2 = x.vload(80);
        v3 = y.vload(80);
        __fn(v4, v5);
        x.vstore(32, v4);
        v4 = x.vload(96);
        v5 = y.vload(96);
        __fn(v6, v7);
        x.vstore(48, v6);
        v6 = x.vload(112);
        v7 = y.vload(112);
        __fn(v0, v1);
        x.vstore(64, v0);
        __fn(v2, v3);
        x.vstore(80, v2);
        __fn(v4, v5);
        x.vstore(96, v4);
        __fn(v6, v7);
        x.vstore(112, v6);
    }

    template <typename Ptr1, typename Ptr2, typename Fn> static __device__ void add32(Ptr1 x, Ptr2 y, Fn __fn) {
        auto v0 = x.vload();
        auto v1 = y.vload();
        auto v2 = x.vload(16);
        auto v3 = y.vload(16);
        __fn(v0, v1);
        __fn(v2, v3);
        x.vstore(0, v0);
        x.vstore(16, v2);
    }
};

#ifdef __XPU3__
template <> struct VTrait<float16> {
    template <typename Ptr1, typename Ptr2, typename Fn> static __device__ void add128(Ptr1 x, Ptr2 y, Fn __fn) {
        auto v0 = x.vload();
        auto v1 = y.vload();
        auto v2 = x.vload(32);
        auto v3 = y.vload(32);
        auto v4 = x.vload(64);
        auto v5 = y.vload(64);
        auto v6 = x.vload(96);
        auto v7 = y.vload(96);
        __fn(v0, v1);
        x.vstore(0, v0);
        __fn(v2, v3);
        x.vstore(32, v2);
        __fn(v4, v5);
        x.vstore(64, v4);
        __fn(v6, v7);
        x.vstore(96, v6);
    }

    template <typename Ptr1, typename Ptr2, typename Fn> static __device__ void add32(Ptr1 x, Ptr2 y, Fn __fn) {
        auto v0 = x.vload();
        auto v1 = y.vload();
        __fn(v0, v1);
        x.vstore(0, v0);
    }
};
#endif  // __XPU3__

template <typename Ptr1, typename Ptr2, typename Fn> __device__ void for_each2(Ptr1 x, Ptr2 y, int len, Fn __fn) {
    using T = typename Ptr1::T;
    int off = 0;
    while (len >= 128) {
        VTrait<T>::template add128<Ptr1, Ptr2, Fn>(x + off, y + off, __fn);
        len -= 128;
        off += 128;
    }
    while (len > 0) {
        VTrait<T>::template add32<Ptr1, Ptr2, Fn>(x + off, y + off, __fn);
        len -= 32;
        off += 32;
    }
}

template <typename Ptr1, typename Ptr2, typename Fn> __device__ void for_each(Ptr1 dst, Ptr2 src, int len, Fn __fn) {
    using T = typename Ptr1::T;
    constexpr int kStep = xpu_std::is_same<T, float>::value ? 16 : 32;
    int off = 0;
    while (len >= kStep * 8) {
        auto v0 = src.vload(off);
        auto v1 = src.vload(off + kStep);
        auto v2 = src.vload(off + kStep * 2);
        auto v3 = src.vload(off + kStep * 3);
        auto v4 = src.vload(off + kStep * 4);
        auto v5 = src.vload(off + kStep * 5);
        auto v6 = src.vload(off + kStep * 6);
        auto v7 = src.vload(off + kStep * 7);
        __fn(v0);
        dst.vstore(off, v0);
        __fn(v1);
        dst.vstore(off + kStep, v1);
        __fn(v2);
        dst.vstore(off + kStep * 2, v2);
        __fn(v3);
        dst.vstore(off + kStep * 3, v3);
        __fn(v4);
        dst.vstore(off + kStep * 4, v4);
        __fn(v5);
        dst.vstore(off + kStep * 5, v5);
        __fn(v6);
        dst.vstore(off + kStep * 6, v6);
        __fn(v7);
        dst.vstore(off + kStep * 7, v7);
        off += kStep * 8;
        len -= kStep * 8;
    }

    while (len >= kStep * 4) {
        auto v0 = src.vload(off);
        auto v1 = src.vload(off + kStep);
        auto v2 = src.vload(off + kStep * 2);
        auto v3 = src.vload(off + kStep * 3);
        __fn(v0);
        dst.vstore(off, v0);
        __fn(v1);
        dst.vstore(off + kStep, v1);
        __fn(v2);
        dst.vstore(off + kStep * 2, v2);
        __fn(v3);
        dst.vstore(off + kStep * 3, v3);
        off += kStep * 4;
        len -= kStep * 4;
    }

    while (len > 0) {
        auto v0 = src.vload(off);
        __fn(v0);
        dst.vstore(off, v0);
        off += kStep;
        len -= kStep;
    }
}

template <typename Ptr1, typename Ptr2> __device__ void add_all(Ptr1 x, Ptr2 y, int len) {
    for_each2(x, y, len, [](typename Ptr1::VT& a, typename Ptr1::VT b) { a = __vvadd(a, b); });
}
template <typename Ptr1, typename Ptr2> __device__ void mul_all(Ptr1 x, Ptr2 y, int len) {
    for_each2(x, y, len, [](typename Ptr1::VT& a, typename Ptr1::VT b) { a = __vvmul(a, b); });
}

/**
 * @brief memcpy for different domains, sm2lm, gsm2sm, lm2lm ...
 *
 * @tparam Ptr1
 * @tparam Ptr2
 * @param dst
 * @param src
 * @param len
 * @return __device__
 */
template <typename Ptr1, typename Ptr2> __device__ void __memcpy(Ptr1 dst, Ptr2 src, int len) {
    for_each(dst, src, len, [](typename Ptr1::VT) {});
}
template <typename Ptr> __device__ void __zero(Ptr dst, int len) {
    using T = typename Ptr::T;
    constexpr int kStep = xpu_std::is_same<T, float>::value ? 16 : 32;
    typename Ptr::VT v0;
    v0 = __vvxor(v0, v0);
    int off = 0;
    while (len >= kStep * 8) {
        dst.vstore(off, v0);
        dst.vstore(off + kStep, v0);
        dst.vstore(off + kStep * 2, v0);
        dst.vstore(off + kStep * 3, v0);
        dst.vstore(off + kStep * 4, v0);
        dst.vstore(off + kStep * 5, v0);
        dst.vstore(off + kStep * 6, v0);
        dst.vstore(off + kStep * 7, v0);
        off += kStep * 8;
        len -= kStep * 8;
    }
    while (len >= kStep * 4) {
        dst.vstore(off, v0);
        dst.vstore(off + kStep, v0);
        dst.vstore(off + kStep * 2, v0);
        dst.vstore(off + kStep * 3, v0);
        off += kStep * 4;
        len -= kStep * 4;
    }
    while (len > 0) {
        dst.vstore(off, v0);
        off += kStep;
        len -= kStep;
    }
}

#define __REDUCE_SUM_PART(__part)                                                                                      \
    if (u == 0 && v < __part) {                                                                                        \
        add_all(sm + v * STRIDE, sm + (v + __part) * STRIDE, readlen);                                                 \
        sm.mfence();                                                                                                   \
    }

/**
 * @brief reducesum 64cores, source domain is lm/sm/gsm, dst domain is sm
 *
 * @tparam T
 * @tparam STRIDE
 * @param tmp_sum: local sum for each core, could be in sm/lm/gsm
 * @param sm_buf: shared buf, size at least 32 * STRIDE
 * @param readlen
 * @param cid
 * @return __device__
 */
template <int STRIDE, typename Ptr>
__device__ void cluster_reduce_sum(Ptr tmp_sum, _shared_ptr_ typename Ptr::T* sm_buf, int readlen, int cid) {
    using T = typename Ptr::T;
    SmPtr<T> sm(sm_buf);
    int u = cid >> 4;
    int v = cid & 0xf;
    // group reduce
    if (u >= 2) {
        __memcpy(sm + ((u - 2) * 16 + v) * STRIDE, tmp_sum, readlen);
        sm.mfence();
    }
    sync_local();
    if (u < 2) {
        add_all(tmp_sum, sm + (u * 16 + v) * STRIDE, readlen);
        tmp_sum.mfence();
    }
    sync_local();
    if (u == 1) {
        __memcpy(sm + v * STRIDE, tmp_sum, readlen);
        sm.mfence();
    }
    sync_local();
    if (u == 0) {
        add_all(sm + v * STRIDE, tmp_sum, readlen);
        sm.mfence();
    }
    sync_all();
    // 16 reduce in sm
    __REDUCE_SUM_PART(8)
    sync_all();
    __REDUCE_SUM_PART(4)
    sync_all();
    __REDUCE_SUM_PART(2)
    sync_all();
    __REDUCE_SUM_PART(1)
    sync_all();
}

/**
 * @brief reducesum for 4 buffers in a group
 * result is in u == 0
 *
 * @tparam Ptr
 * @tparam STRIDE
 * @param gsm_sum
 * @param readlen
 * @param u
 * @return __device__
 */
template <int STRIDE, typename Ptr> __device__ void group_reduce_sum_inplace(Ptr gsm_sum, int readlen, int u) {
    if (u < 2) {
        add_all(gsm_sum + u * STRIDE, gsm_sum + (u + 2) * STRIDE, readlen);
        gsm_sum.mfence();
    }
    sync_local();
    if (u == 0) {
        add_all(gsm_sum, gsm_sum + STRIDE, readlen);
        gsm_sum.mfence();
    }
    sync_local();
}


__device__ static uint32x16_t range16_u32() {
    uint32x16_t a;
    a = vvxor_uint32x16(a, a);
    a = svadd_uint32x16_mz(8, a, 0xff00);
    a = svadd_uint32x16_mh(4, a, a, 0xf0f0);
    a = svadd_uint32x16_mh(2, a, a, 0xcccc);
    a = svadd_uint32x16_mh(1, a, a, 0xaaaa);
    return a;
}

__device__ static int32x16_t range16_i32() {
    int32x16_t a;
    a = vvxor_int32x16(a, a);
    a = svadd_int32x16_mz(8, a, 0xff00);
    a = svadd_int32x16_mh(4, a, a, 0xf0f0);
    a = svadd_int32x16_mh(2, a, a, 0xcccc);
    a = svadd_int32x16_mh(1, a, a, 0xaaaa);
    return a;
}

namespace detail {
template <typename T, bool sel> struct StaticSelect;

template <typename T> struct StaticSelect<T, true> {
    __device__ static void run(_shared_ptr_ T* c, _shared_ptr_ T* a, T b) { *c = *a; }
};

template <typename T> struct StaticSelect<T, false> {
    __device__ static void run(_shared_ptr_ T* c, _shared_ptr_ T* a, T b) { *c = b; }
};
}  // namespace detail


template <int BUCKBITS> struct RadixSort;
template <> struct RadixSort<4> {
    static constexpr int BUCKBITS = 4;
    static constexpr int BUCKCNT = 2 << BUCKBITS;
    static constexpr int MASK = (1 << BUCKBITS) - 1;

    template <typename T_KEY, typename T_VAL, bool USE_IDX, typename Fn>
    static __device__ void radixsort_part(_shared_ptr_ T_KEY* sm_src,
                                          _shared_ptr_ T_KEY* sm_dst,
                                          _shared_ptr_ T_VAL* sm_val_src,
                                          _shared_ptr_ T_VAL* sm_val_dst,
                                          _shared_ptr_ uint32_t* g_hist,
                                          int cid,
                                          int start,
                                          int end,
                                          Fn fn_bucket_idx) {
        __shared_ptr__ uint32_t* hist = g_hist + cid * BUCKCNT;
        __shared_ptr__ uint32_t* hist_prefix_l = g_hist + ((cid + 63) % 64) * BUCKCNT;
        __shared_ptr__ uint32_t* sum_prefixsum = g_hist + 63 * BUCKCNT;

        // make histgram
        __simd__ uint32_t lm_hist[BUCKCNT];
        __simd__ uint32_t lm_prefixsum[BUCKCNT];
        uint32x16_t vtmp;
        vtmp = vvxor_uint32x16(vtmp, vtmp);
        vstore_lm_uint32x16(lm_hist, vtmp);
        mfence_lm();
        for (int i = start; i < end; ++i) {
            uint32_t s = fn_bucket_idx(sm_src[i]);
            lm_hist[s]++;
        }
        mfence_lm();
        vtmp = vload_lm_uint32x16(lm_hist);
        vstore_sm_uint32x16(hist, vtmp);
        mfence_sm();
        sync_all();

        // local prefixsum of each bucket, to sum local offset of element
        // exclusive prefix_sum histogram to hist_prefixsum
        // inclusive prefix_sum histogram to g_sum, which is the sum of eles in buckets
        if (cid == 0) {
            uint32x16_t v[8];
            v[0] = vload_sm_uint32x16(g_hist);

#define PREFIX_SUM_16(__i)                                                                                             \
    vtmp = vload_sm_uint32x16(g_hist + (__i)*BUCKCNT);                                                                 \
    v[(__i) % 8] = vvadd_uint32x16(v[((__i)-1) % 8], vtmp);                                                            \
    vstore_sm_uint32x16(g_hist + (__i)*BUCKCNT, v[(__i) % 8]);

#define PREFIX_SUM_10_16(__s)                                                                                          \
    PREFIX_SUM_16(__s)                                                                                                 \
    PREFIX_SUM_16(__s + 1)                                                                                             \
    PREFIX_SUM_16(__s + 2)                                                                                             \
    PREFIX_SUM_16(__s + 3)                                                                                             \
    PREFIX_SUM_16(__s + 4)                                                                                             \
    PREFIX_SUM_16(__s + 5)                                                                                             \
    PREFIX_SUM_16(__s + 6)                                                                                             \
    PREFIX_SUM_16(__s + 7)                                                                                             \
    PREFIX_SUM_16(__s + 8)                                                                                             \
    PREFIX_SUM_16(__s + 9)

            PREFIX_SUM_10_16(1)
            PREFIX_SUM_10_16(11)
            PREFIX_SUM_10_16(21)
            PREFIX_SUM_10_16(31)
            PREFIX_SUM_10_16(41)
            PREFIX_SUM_10_16(51)
            PREFIX_SUM_16(61)
            PREFIX_SUM_16(62)
            PREFIX_SUM_16(63)

            // global exclusive prefixsum of all buckets
            uint32_t* lm_sum = lm_hist;
            vstore_lm_uint32x16(lm_sum, v[63 % 8]);
            mfence_lm();
            lm_prefixsum[0] = 0;
            for (int i = 1; i < BUCKCNT; ++i) {
                lm_prefixsum[i] = lm_prefixsum[i - 1] + lm_sum[i - 1];
            }
            mfence_lm();
            vtmp = vload_lm_uint32x16(lm_prefixsum);
            vstore_sm_uint32x16(sum_prefixsum, vtmp);
        }
        mfence_sm();
        sync_all();
        uint32x16_t vtmp1;
        if (cid != 0) {
            vtmp = vload_sm_uint32x16(hist_prefix_l);
            vtmp1 = vload_sm_uint32x16(sum_prefixsum);
            vtmp = vvadd_uint32x16(vtmp, vtmp1);
            vstore_lm_uint32x16(lm_hist, vtmp);
        } else {
            vtmp = vload_sm_uint32x16(hist_prefix_l);
            vstore_lm_uint32x16(lm_hist, vtmp);
        }
        mfence_lm();
        // rearrange
        for (int i = start; i < end; ++i) {
            auto v = sm_src[i];
            uint32_t s = fn_bucket_idx(v);
            auto idx = lm_hist[s];
            sm_dst[idx] = v;
            detail::StaticSelect<T_VAL, !USE_IDX>::run(&sm_val_dst[idx], &sm_val_src[i], i);
            lm_hist[s]++;
        }
        mfence_lm();
        sync_all();
    }

    template <typename T_KEY, typename T_VAL>
    static __device__ void Run(_shared_ptr_ T_KEY* sm_src,
                               _shared_ptr_ T_KEY* sm_dst,
                               _shared_ptr_ T_VAL* sm_val_src,
                               _shared_ptr_ T_VAL* sm_val_dst,
                               _shared_ptr_ uint32_t* g_hist,
                               int cid,
                               int start,
                               int end) {
#define SORT_PART(_use_idx, _src, _dst, __k)                                                                           \
    radixsort_part<T_KEY, T_VAL, _use_idx>(                                                                            \
            sm_##_src, sm_##_dst, sm_val_##_src, sm_val_##_dst, g_hist, cid, start, end, [](T_KEY a) -> uint32_t {     \
                return ((a >> (__k)) & 0xf);                                                                           \
            })

        SORT_PART(true, src, dst, 0);
        SORT_PART(false, dst, src, 4);
        SORT_PART(false, src, dst, 8);
        SORT_PART(false, dst, src, 12);
        SORT_PART(false, src, dst, 16);
        SORT_PART(false, dst, src, 20);
        SORT_PART(false, src, dst, 24);
        SORT_PART(false, dst, src, 28);
    }
};

template <> struct RadixSort<8> {
    static constexpr int BUCKBITS = 8;
    static constexpr int BUCKCNT = 2 << BUCKBITS;
    static constexpr int MASK = (1 << BUCKBITS) - 1;

    template <typename T_KEY, typename T_VAL, bool USE_IDX, typename Fn>
    static __device__ void radixsort_part(_shared_ptr_ T_KEY* sm_src,
                                          _shared_ptr_ T_KEY* sm_dst,
                                          _shared_ptr_ T_VAL* sm_val_src,
                                          _shared_ptr_ T_VAL* sm_val_dst,
                                          _shared_ptr_ uint32_t* g_hist,
                                          int cid,
                                          int start,
                                          int end,
                                          Fn fn_bucket_idx) {
        constexpr int BUCKCNT = 256;
        constexpr int VLOOP = BUCKCNT / 16;
        __shared_ptr__ uint32_t* hist = g_hist + cid * BUCKCNT;
        __shared_ptr__ uint32_t* hist_prefix_l = g_hist + ((cid + 63) % 64) * BUCKCNT;
        __shared_ptr__ uint32_t* sum_prefixsum = g_hist + 63 * BUCKCNT;

        // make histgram
        __simd__ uint32_t lm_hist[BUCKCNT];
        uint32_t* lm_sum = lm_hist;
        uint32_t* lm_prefixsum = lm_hist + 16;
        uint32x16_t vtmp;
        uint32x16_t v[8];
        vtmp = vvxor_uint32x16(vtmp, vtmp);
        for (int i = 0; i < VLOOP / 4; i++) {
            vstore_lm_uint32x16(lm_hist + i * 64 + 0 * 16, vtmp);
            vstore_lm_uint32x16(lm_hist + i * 64 + 1 * 16, vtmp);
            vstore_lm_uint32x16(lm_hist + i * 64 + 2 * 16, vtmp);
            vstore_lm_uint32x16(lm_hist + i * 64 + 3 * 16, vtmp);
        }
        mfence_lm();

        for (int i = start; i < end; ++i) {
            uint32_t s = fn_bucket_idx(sm_src[i]);
            lm_hist[s]++;
        }
        mfence_lm();

        for (int i = 0; i < VLOOP / 4; ++i) {
            v[0] = vload_lm_uint32x16(lm_hist + i * 64);
            v[1] = vload_lm_uint32x16(lm_hist + i * 64 + 16);
            v[2] = vload_lm_uint32x16(lm_hist + i * 64 + 32);
            v[3] = vload_lm_uint32x16(lm_hist + i * 64 + 48);
            vstore_sm_uint32x16(hist + i * 64, v[0]);
            vstore_sm_uint32x16(hist + i * 64 + 16, v[1]);
            vstore_sm_uint32x16(hist + i * 64 + 32, v[2]);
            vstore_sm_uint32x16(hist + i * 64 + 48, v[3]);
        }
        mfence_sm();
        sync_all();

        // local prefixsum of each bucket, to sum local offset of element
        // exclusive prefix_sum histogram to hist_prefixsum
        // inclusive prefix_sum histogram to g_sum, which is the sum of eles in buckets
        if (cid < VLOOP) {
            v[0] = vload_sm_uint32x16(g_hist + cid * 16);

#define PREFIX_SUM_256(__i)                                                                                            \
    v[(__i) % 8] = vload_sm_uint32x16(g_hist + (__i)*BUCKCNT + cid * 16);                                              \
    v[(__i) % 8] = vvadd_uint32x16(v[((__i)-1) % 8], v[(__i) % 8]);                                                    \
    vstore_sm_uint32x16(g_hist + (__i)*BUCKCNT + cid * 16, v[(__i) % 8]);

#define PREFIX_SUM_10_256(__s)                                                                                         \
    PREFIX_SUM_256(__s)                                                                                                \
    PREFIX_SUM_256(__s + 1)                                                                                            \
    PREFIX_SUM_256(__s + 2)                                                                                            \
    PREFIX_SUM_256(__s + 3)                                                                                            \
    PREFIX_SUM_256(__s + 4)                                                                                            \
    PREFIX_SUM_256(__s + 5)                                                                                            \
    PREFIX_SUM_256(__s + 6)                                                                                            \
    PREFIX_SUM_256(__s + 7)                                                                                            \
    PREFIX_SUM_256(__s + 8)                                                                                            \
    PREFIX_SUM_256(__s + 9)

            PREFIX_SUM_10_256(1)
            PREFIX_SUM_10_256(11)
            PREFIX_SUM_10_256(21)
            PREFIX_SUM_10_256(31)
            PREFIX_SUM_10_256(41)
            PREFIX_SUM_10_256(51)
            PREFIX_SUM_256(61)
            PREFIX_SUM_256(62)
            PREFIX_SUM_256(63)

            // global exclusive prefixsum of all buckets
            vstore_lm_uint32x16(lm_sum, v[63 % 8]);
            mfence_lm();
            lm_prefixsum[0] = 0;
            for (int i = 1; i < 16; ++i) {
                lm_prefixsum[i] = lm_prefixsum[i - 1] + lm_sum[i - 1];
            }
            mfence_lm();
            vtmp = vload_lm_uint32x16(lm_prefixsum);
            vstore_sm_uint32x16(sum_prefixsum + cid * 16, vtmp);
            sm_dst[cid] = lm_prefixsum[15] + lm_sum[15];
            mfence_sm();
        }
        sync_all();
        if (cid == 0) {
            for (int i = 1; i < 16; ++i) {
                sum_prefixsum[i * 16] += sm_dst[i - 1] + sum_prefixsum[(i - 1) * 16];
            }
            mfence_sm();
        }
        sync_all();
        if (cid < VLOOP) {
            for (int i = 1; i < 16; ++i) {
                sum_prefixsum[cid * 16 + i] += sum_prefixsum[cid * 16];
            }
            mfence_sm();
        }
        mfence_lm();
        sync_all();

        if (cid != 0) {
            for (int i = 0; i < VLOOP / 4; ++i) {
                v[0] = vload_sm_uint32x16(hist_prefix_l + i * 64);
                v[1] = vload_sm_uint32x16(sum_prefixsum + i * 64);
                v[2] = vload_sm_uint32x16(hist_prefix_l + i * 64 + 16);
                v[3] = vload_sm_uint32x16(sum_prefixsum + i * 64 + 16);
                v[4] = vload_sm_uint32x16(hist_prefix_l + i * 64 + 32);
                v[5] = vload_sm_uint32x16(sum_prefixsum + i * 64 + 32);
                v[6] = vload_sm_uint32x16(hist_prefix_l + i * 64 + 48);
                v[7] = vload_sm_uint32x16(sum_prefixsum + i * 64 + 48);
                v[0] = vvadd_uint32x16(v[0], v[1]);
                v[2] = vvadd_uint32x16(v[2], v[3]);
                v[4] = vvadd_uint32x16(v[4], v[5]);
                v[6] = vvadd_uint32x16(v[6], v[7]);
                vstore_lm_uint32x16(lm_hist + i * 64, v[0]);
                vstore_lm_uint32x16(lm_hist + i * 64 + 16, v[2]);
                vstore_lm_uint32x16(lm_hist + i * 64 + 32, v[4]);
                vstore_lm_uint32x16(lm_hist + i * 64 + 48, v[6]);
            }
        } else {
            for (int i = 0; i < VLOOP / 4; ++i) {
                v[0] = vload_sm_uint32x16(hist_prefix_l + i * 64);
                v[2] = vload_sm_uint32x16(hist_prefix_l + i * 64 + 16);
                v[4] = vload_sm_uint32x16(hist_prefix_l + i * 64 + 32);
                v[6] = vload_sm_uint32x16(hist_prefix_l + i * 64 + 48);
                vstore_lm_uint32x16(lm_hist + i * 64, v[0]);
                vstore_lm_uint32x16(lm_hist + i * 64 + 16, v[2]);
                vstore_lm_uint32x16(lm_hist + i * 64 + 32, v[4]);
                vstore_lm_uint32x16(lm_hist + i * 64 + 48, v[6]);
            }
        }
        mfence_lm();
        // rearrange
        for (int i = start; i < end; ++i) {
            auto v = sm_src[i];
            uint32_t s = fn_bucket_idx(v);
            auto idx = lm_hist[s];
            sm_dst[idx] = v;
            detail::StaticSelect<T_VAL, !USE_IDX>::run(&sm_val_dst[idx], &sm_val_src[i], i);
            lm_hist[s]++;
        }
        mfence_lm_sm();
        sync_all();
    }

    template <typename T_KEY, typename T_VAL>
    static __device__ void Run(_shared_ptr_ T_KEY* sm_src,
                               _shared_ptr_ T_KEY* sm_dst,
                               _shared_ptr_ T_VAL* sm_val_src,
                               _shared_ptr_ T_VAL* sm_val_dst,
                               _shared_ptr_ uint32_t* g_hist,
                               int cid,
                               int start,
                               int end) {
#define SORT_PART_256(_use_idx, _src, _dst, __k)                                                                       \
    radixsort_part<T_KEY, T_VAL, _use_idx>(                                                                            \
            sm_##_src, sm_##_dst, sm_val_##_src, sm_val_##_dst, g_hist, cid, start, end, [](T_KEY a) -> uint32_t {     \
                return ((a >> (__k)) & 0xff);                                                                          \
            })

        SORT_PART_256(true, src, dst, 0);
        SORT_PART_256(false, dst, src, 8);
        SORT_PART_256(false, src, dst, 16);
        SORT_PART_256(false, dst, src, 24);
    }
};

template <typename T, typename T_IDX>
__device__ void cluster_unique(const _shared_ptr_ T* src,
                               int len,
                               int cid,
                               _shared_ptr_ uint32_t* sm_hist,  //! need uint32 128 hist
                               _shared_ptr_ T* dst,
                               _shared_ptr_ T_IDX* dst_val,
                               __shared_ptr__ uint32_t* sm_uniq_len) {
    __shared_ptr__ uint32_t* sm_hist_prefixsum = sm_hist + 64;

    int part = (len + 63) / 64;
    int start = cid * part;
    int end = min(len - 1, start + part);
    // split len into
    // 0, (...], (...] ...
    // and prefix sum if diffs, to calc offset of unique key
    uint32_t hist = 0;
    for (int i = start; i < end; ++i) {
        if (src[i] != src[i + 1]) {
            hist++;
        }
    }
    sm_hist[cid] = hist;
    mfence_sm();
    sync_all();
    // prefixsum global
    if (cid == 0) {
        sm_hist_prefixsum[0] = 1; // @NOTE. because the first ele must in 0 pos
        for (int i = 1; i < 64; ++i) {
            sm_hist_prefixsum[i] = sm_hist_prefixsum[i - 1] + sm_hist[i - 1];
        }
        *sm_uniq_len = sm_hist_prefixsum[63] + sm_hist[63];
        mfence_sm();
    }
    sync_all();
    if (cid == 0) {
        dst[0] = src[0];
        dst_val[0] = 0;
    }
    auto prefix = sm_hist_prefixsum[cid];
    for (int i = start; i < end; ++i) {
        if (src[i] != src[i + 1]) {
            dst[prefix] = src[i + 1];
            dst_val[prefix] = i + 1;
            prefix++;
        }
    }
    mfence_sm();
    sync_all();
}


#endif // XPU_KERNEL_CLUSTER_PRIMITIVE_TEMPLATE_H_
