#ifndef _KERNEL_RANDOM_PHLIOX_H_
#define _KERNEL_RANDOM_PHLIOX_H_

#include "xpu/kernel/cluster.h"
#include "xpu/kernel/cluster_simd.h"

#define PHILO2X32X16_ROUND(counter_0, counter_1, m_, key_, hi)                                                         \
    hi = svmulh_uint32x16(m_, counter_0);                                                                              \
    hi = vvxor_uint32x16(hi, counter_1);                                                                               \
    counter_1 = svmul_uint32x16(m_, counter_0);                                                                        \
    counter_0 = svxor_uint32x16(key_, hi);

/**
 * @brief
 * based on philo2x32, [tid0, tid1] -> [random0, random1]
 * philo2x32x16 is:
 *                 [[tid0, tid1],
 *                  [tid2, tid3],
 *                  ...
 *                  [tid30, tid31]].T
 * ->
 *                 [[rand0, rand1],
 *                  [rand2, rand3],
 *                  ...
 *                  [rand30, rand31]].T
 *
 * every uint32x16 is interleaved tid
 *
 * @tparam ROUNDS
 * @param counter_0[in/out]
 * @param counter_1[in/out]
 * @param key[in]
 */

struct Philo2x32x16 {
    static constexpr unsigned int PHILOX_M2x32_0 = 0xd256d193;
    static constexpr unsigned int PHILOX_W32_0 = 0x9e3779b9;

    __device__ Philo2x32x16(uint32_t key) : key_(key) {}

    __device__ void hash(uint32x16_t& counter_0, uint32x16_t& counter_1) {
        uint32x16_t hi;
        uint32_t l_key = key_;
#if 0
        for (int i = 0; i < 9; ++i) {
            PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
            l_key += PHILOX_W32_0;
        }
#else
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
        l_key += PHILOX_W32_0;
#endif
        PHILO2X32X16_ROUND(counter_0, counter_1, PHILOX_M2x32_0, l_key, hi);
    }
    uint32_t key_;
};

__device__ static uint32x16_t range16() {
    uint32x16_t a;
    a = vvxor_uint32x16(a, a);
    a = svadd_uint32x16_mz(8, a, 0xff00);
    a = svadd_uint32x16_mh(4, a, a, 0xf0f0);
    a = svadd_uint32x16_mh(2, a, a, 0xcccc);
    a = svadd_uint32x16_mh(1, a, a, 0xaaaa);
    return a;
}

#endif // _KERNEL_RANDOM_PHLIOX_H_