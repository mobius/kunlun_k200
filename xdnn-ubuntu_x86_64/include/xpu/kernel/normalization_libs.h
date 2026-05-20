#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_NORMALIZATION_LIBS_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_NORMALIZATION_LIBS_H

#ifdef __xpu__

#include "xpu/kernel/cluster_header.h"
#include "xpu/kernel/cluster_memory.h"

/**
 * rtv = base + ptr[0] + .. ptr[7]
 */
static __device__ float sum8(float base, const float* ptr) {
    float a = ptr[0];
    float b = ptr[1];
    float c = ptr[2];
    float d = ptr[3];
    a += ptr[4];
    b += ptr[5];
    c += ptr[6];
    d += ptr[7];
    a += base;
    b += c;
    a += d;
    a += b;
    return a;
}

/**
 * rvt = sum(lmptr[i]), i in[0, roundup(size, 8))
 */
static __device__ float calculate_sum(const float* lmptr, int size) {
    int rounddown_size = rounddown32(size);
    __local__ float acc_buf[32];
    memset_lm_float(acc_buf, 32);
    float* result0 = acc_buf;
    float* result1 = acc_buf + 8;
    float* result2 = acc_buf + 16;
    float* result3 = acc_buf + 24;

    int i = 0;
    for (; i < rounddown_size; i += 32) {
        const float* ptr0 = lmptr;
        const float* ptr1 = lmptr + 8;
        const float* ptr2 = lmptr + 16;
        const float* ptr3 = lmptr + 24;
        _x256_vvadd_ls(result0, ptr0, result0);
        _x256_vvadd_ls(result1, ptr1, result1);
        _x256_vvadd_ls(result2, ptr2, result2);
        _x256_vvadd_ls(result3, ptr3, result3);
        lmptr += 32;
    }
    while (i < size) {
        _x256_vvadd_ls(result0, lmptr, result0);
        i += 8;
        lmptr += 8;
    }

    _x256_vvadd_ls(result0, result1, result0);
    _x256_vvadd_ls(result2, result3, result2);
    _x256_vvadd_ls(result0, result2, result0);

    return sum8(0.0f, result0);
}

static __device__ float calculate_sum_with_inadequate_lm(_global_ptr_ const float* gmptr,
        int gmsize, float* lmptr, int lmsize) {
    int lmsize_rounddown8 = rounddown8(lmsize);
    int rounds = gmsize / lmsize_rounddown8;
    int remains = gmsize % lmsize_rounddown8;
    float sum = 0.0f;

    for (int i = 0; i < rounds; ++i) {
        GM2LM(gmptr + i * lmsize_rounddown8, lmptr, lmsize_rounddown8 * sizeof(float));
        sum += calculate_sum(lmptr, lmsize_rounddown8);
    }
    if (remains) {
        GM2LM(gmptr + gmsize - remains, lmptr, remains * sizeof(float));
        int remains_8align = roundup8(remains);
        for (int i = remains; i < remains_8align; ++i) {
            lmptr[i] = 0.0f;
        }
        sum += calculate_sum(lmptr, remains_8align);
    }
    return sum;
}

static __device__ float calculate_square_sum(float* lmptr, int size,
        float sample_mean) {
    float* tmp_ptr = lmptr;
    __local__ float acc_buf[32];
    __local__ float mul_buf[32];
    memset_lm_float(acc_buf, 32);
    float* acc0 = acc_buf;
    float* acc1 = acc_buf + 8;
    float* acc2 = acc_buf + 16;
    float* acc3 = acc_buf + 24;
    float* mul0 = mul_buf;
    float* mul1 = mul_buf + 8;
    float* mul2 = mul_buf + 16;
    float* mul3 = mul_buf + 24;

    int rounddown_size = rounddown32(size);
    int i = 0;
    for (; i < rounddown_size; i += 32) {
        float* ptr0 = tmp_ptr;
        float* ptr1 = tmp_ptr + 8;
        float* ptr2 = tmp_ptr + 16;
        float* ptr3 = tmp_ptr + 24;
        _x256_svsub_ls(sample_mean, ptr0, mul0);
        _x256_svsub_ls(sample_mean, ptr1, mul1);
        _x256_svsub_ls(sample_mean, ptr2, mul2);
        _x256_svsub_ls(sample_mean, ptr3, mul3);
        _x256_vvmul_ls(mul0, mul0, mul0);
        _x256_vvmul_ls(mul1, mul1, mul1);
        _x256_vvmul_ls(mul2, mul2, mul2);
        _x256_vvmul_ls(mul3, mul3, mul3);
        _x256_vvadd_ls(acc0, mul0, acc0);
        _x256_vvadd_ls(acc1, mul1, acc1);
        _x256_vvadd_ls(acc2, mul2, acc2);
        _x256_vvadd_ls(acc3, mul3, acc3);
        tmp_ptr += 32;
    }

    while (i < rounddown8(size)) {
        _x256_svsub_ls(sample_mean, tmp_ptr, mul0);
        _x256_vvmul_ls(mul0, mul0, mul0);
        _x256_vvadd_ls(acc0, mul0, acc0);
        i += 8;
        tmp_ptr += 8;
    }

    _x256_svsub_ls(sample_mean, tmp_ptr, mul0);
    _x256_vvmul_ls(mul0, mul0, mul0);
    for (int j = size - rounddown8(size); j < 8; j++) {
        mul0[j] = 0.0f;
    }
    _x256_vvadd_ls(acc0, mul0, acc0);

    _x256_vvadd_ls(acc0, acc1, acc0);
    _x256_vvadd_ls(acc2, acc3, acc2);
    _x256_vvadd_ls(acc0, acc2, acc0);

    return sum8(0.0f, acc0);
}

static __device__ float calculate_square_sum_with_inadequate_lm(_global_ptr_ const float* gmptr,
        int gmsize, float* lmptr, int lmsize, float sample_mean) {
    int lmsize_rounddown8 = rounddown8(lmsize);
    int rounds = gmsize / lmsize_rounddown8;
    int remains = gmsize % lmsize_rounddown8;
    float sum = 0.0f;

    for (int i = 0; i < rounds; ++i) {
        GM2LM(gmptr + i * lmsize_rounddown8, lmptr, lmsize_rounddown8 * sizeof(float));
        sum += calculate_square_sum(lmptr, lmsize_rounddown8, sample_mean);
    }
    if (remains) {
        GM2LM(gmptr + gmsize - remains, lmptr, remains * sizeof(float));
        //int remains_8align = roundup8(remains);
        //for (int i = remains; i < remains_8align; ++i) {
        //lmptr[i] = 0.0f;
        //}
        //sum += calculate_square_sum(lmptr, remains_8align, sample_mean);
        sum += calculate_square_sum(lmptr, remains, sample_mean);
    }
    return sum;
}

static __device__ void calculate_sum_and_squaresum(const float* lmptr, int size, float* sum, float* squaresum) {
    int rounddown_size = rounddown32(size);
    __local__ float acc_buf[64];
    __local__ float mul_buf[32];
    memset_lm_float(acc_buf, 64);
    float* sum0 = acc_buf;
    float* sum1 = acc_buf + 8;
    float* sum2 = acc_buf + 16;
    float* sum3 = acc_buf + 24;
    float* squaresum0 = acc_buf + 32;
    float* squaresum1 = acc_buf + 40;
    float* squaresum2 = acc_buf + 48;
    float* squaresum3 = acc_buf + 56;
    float* mul0 = mul_buf;
    float* mul1 = mul_buf + 8;
    float* mul2 = mul_buf + 16;
    float* mul3 = mul_buf + 24;

    int i = 0;
    for (; i < rounddown_size; i += 32) {
        const float* ptr0 = lmptr;
        const float* ptr1 = lmptr + 8;
        const float* ptr2 = lmptr + 16;
        const float* ptr3 = lmptr + 24;
        _x256_vvmul_ls(ptr0, ptr0, mul0);
        _x256_vvmul_ls(ptr1, ptr1, mul1);
        _x256_vvmul_ls(ptr2, ptr2, mul2);
        _x256_vvmul_ls(ptr3, ptr3, mul3);
        _x256_vvadd_ls(sum0, ptr0, sum0);
        _x256_vvadd_ls(sum1, ptr1, sum1);
        _x256_vvadd_ls(sum2, ptr2, sum2);
        _x256_vvadd_ls(sum3, ptr3, sum3);
        _x256_vvadd_ls(squaresum0, mul0, squaresum0);
        _x256_vvadd_ls(squaresum1, mul1, squaresum1);
        _x256_vvadd_ls(squaresum2, mul2, squaresum2);
        _x256_vvadd_ls(squaresum3, mul3, squaresum3);
        lmptr += 32;
    }
    while (i < size) {
        _x256_vvmul_ls(lmptr, lmptr, mul0);
        _x256_vvadd_ls(sum0, lmptr, sum0);
        _x256_vvadd_ls(squaresum0, mul0, squaresum0);
        i += 8;
        lmptr += 8;
    }

    _x256_vvadd_ls(sum0, sum1, sum0);
    _x256_vvadd_ls(sum2, sum3, sum2);
    _x256_vvadd_ls(sum0, sum2, sum0);
    _x256_vvadd_ls(squaresum0, squaresum1, squaresum0);
    _x256_vvadd_ls(squaresum2, squaresum3, squaresum2);
    _x256_vvadd_ls(squaresum0, squaresum2, squaresum0);

    *sum = sum8(0.0f, sum0);
    *squaresum = sum8(0.0f, squaresum0);
}
/**
 * calc (x - mean) * var in SIMD
 * do_norm(lmptr[i]), i in [0, roundup(size, 32))
 */
static __device__ void do_norm(float* lmptr, int size, float mean, float var) {
    int i = 0;
    for (; i < size; i += 32) {
        float* ptr0 = lmptr;
        float* ptr1 = lmptr + 8;
        float* ptr2 = lmptr + 16;
        float* ptr3 = lmptr + 24;
        // x - mean
        _x256_svadd_ls(-mean, ptr0, ptr0);
        _x256_svadd_ls(-mean, ptr1, ptr1);
        _x256_svadd_ls(-mean, ptr2, ptr2);
        _x256_svadd_ls(-mean, ptr3, ptr3);
        _x256_svmul_ls(var, ptr0, ptr0);
        _x256_svmul_ls(var, ptr1, ptr1);
        _x256_svmul_ls(var, ptr2, ptr2);
        _x256_svmul_ls(var, ptr3, ptr3);
        lmptr += 32;
    }
}

/**
 * calc x * scale in SIMD
 * do_scale(lmptr[i]), i in [0, roundup(size, 32))
 */
static __device__ void do_scale(float* lmptr, int size, const float* scale_ptr) {
    int i = 0;
    for (; i < size; i += 32) {
        float* ptr0 = lmptr;
        float* ptr1 = lmptr + 8;
        float* ptr2 = lmptr + 16;
        float* ptr3 = lmptr + 24;
        const float* scale_ptr0 = scale_ptr;
        const float* scale_ptr1 = scale_ptr + 8;
        const float* scale_ptr2 = scale_ptr + 16;
        const float* scale_ptr3 = scale_ptr + 24;
        _x256_vvmul_ls(ptr0, scale_ptr0, ptr0);
        _x256_vvmul_ls(ptr1, scale_ptr1, ptr1);
        _x256_vvmul_ls(ptr2, scale_ptr2, ptr2);
        _x256_vvmul_ls(ptr3, scale_ptr3, ptr3);
        lmptr += 32;
        scale_ptr += 32;
    }
}

/**
 * calc x + bias in SIMD
 * do_bias(lmptr[i]), i in [0, roundup(size, 32))
 */
static __device__ void do_bias(float* lmptr, int size, const float* bias_ptr) {
    int i = 0;
    for (; i < size; i += 32) {
        float* ptr0 = lmptr;
        float* ptr1 = lmptr + 8;
        float* ptr2 = lmptr + 16;
        float* ptr3 = lmptr + 24;
        const float* bias_ptr0 = bias_ptr;
        const float* bias_ptr1 = bias_ptr + 8;
        const float* bias_ptr2 = bias_ptr + 16;
        const float* bias_ptr3 = bias_ptr + 24;
        _x256_vvadd_ls(ptr0, bias_ptr0, ptr0);
        _x256_vvadd_ls(ptr1, bias_ptr1, ptr1);
        _x256_vvadd_ls(ptr2, bias_ptr2, ptr2);
        _x256_vvadd_ls(ptr3, bias_ptr3, ptr3);
        lmptr += 32;
        bias_ptr += 32;
    }
}

/**
 * calc x * scale + bias in SIMD
 * do_scale_bias(lmptr[i]), i in [0, roundup(size, 8))
 */
static __device__ void do_scale_bias(float* lmptr, int size, const float* scale_ptr,
        const float* bias_ptr) {
    int i = 0;
    int rounddown_size = rounddown32(size);
    for (; i < rounddown_size; i += 32) {
        float* ptr0 = lmptr;
        float* ptr1 = lmptr + 8;
        float* ptr2 = lmptr + 16;
        float* ptr3 = lmptr + 24;
        const float* scale_ptr0 = scale_ptr;
        const float* scale_ptr1 = scale_ptr + 8;
        const float* scale_ptr2 = scale_ptr + 16;
        const float* scale_ptr3 = scale_ptr + 24;
        const float* bias_ptr0 = bias_ptr;
        const float* bias_ptr1 = bias_ptr + 8;
        const float* bias_ptr2 = bias_ptr + 16;
        const float* bias_ptr3 = bias_ptr + 24;
        _x256_vvmul_ls(ptr0, scale_ptr0, ptr0);
        _x256_vvmul_ls(ptr1, scale_ptr1, ptr1);
        _x256_vvmul_ls(ptr2, scale_ptr2, ptr2);
        _x256_vvmul_ls(ptr3, scale_ptr3, ptr3);
        _x256_vvadd_ls(ptr0, bias_ptr0, ptr0);
        _x256_vvadd_ls(ptr1, bias_ptr1, ptr1);
        _x256_vvadd_ls(ptr2, bias_ptr2, ptr2);
        _x256_vvadd_ls(ptr3, bias_ptr3, ptr3);
        lmptr += 32;
        scale_ptr += 32;
        bias_ptr += 32;
    }
    while (i < size) {
        _x256_vvmul_ls(lmptr, scale_ptr, lmptr);
        _x256_vvadd_ls(lmptr, bias_ptr, lmptr);
        i += 8;
        lmptr += 8;
        scale_ptr += 8;
        bias_ptr += 8;
    }
}

template <bool IS_TEST>
static __device__ void store_mean_var(const float* mean_lm,
        const float* var_lm, _global_ptr_ float* mean_gm,
        _global_ptr_ float* var_gm, int offset) {
    return;
}

// for train phase
template <>
__device__ void store_mean_var<false>(const float* mean_lm,
        const float* var_lm, _global_ptr_ float* mean_gm,
        _global_ptr_ float* var_gm, int offset) {
    LM2GM(mean_lm, mean_gm + offset, sizeof(float));
    LM2GM(var_lm, var_gm + offset, sizeof(float));
}

// TODO: support more elementwise type
template <int CALC_TYPE>
static __device__ void elementwise_calc(const float* x, const float* y, float* z, int len) {
    return;
}

// add: z[i] = x[i] + y[i]
template <>
__device__ void elementwise_calc<1>(const float* x, const float* y, float* z, int len) {
    int rounddown32_len = rounddown32(len);
    for (int i = 0; i < rounddown32_len; i += 32) {
        _x256_vvadd_ls(x + i, y + i, z + i);
        _x256_vvadd_ls(x + i + 8, y + i + 8, z + i + 8);
        _x256_vvadd_ls(x + i + 16, y + i + 16, z + i + 16);
        _x256_vvadd_ls(x + i + 24, y + i + 24, z + i + 24);
    }
    int rounddown8_len = rounddown8(len);
    for (int i = rounddown32_len; i < rounddown8_len; i += 8) {
        _x256_vvadd_ls(x + i, y + i, z + i);
    }
    for (int i = rounddown8_len; i < len; ++i) {
        z[i] = x[i] + y[i];
    }
}
#endif
#endif
