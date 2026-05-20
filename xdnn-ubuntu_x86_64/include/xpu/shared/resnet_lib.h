#ifndef __XPU_RESNET_LIB_H
#define __XPU_RESNET_LIB_H
#if defined(__XPU1__) || defined(__XPU2__) || defined(__XPU3__) || defined(__XPU4__)
#define __XPU__
#endif

#ifdef __XPU__
#define __share__ __device__
#else
#define __share__ 
#endif

///////// SHARED FUNCTIONS ////////
// WARNNING: shared functions should always use '__share__' prefix
#define __CONV_WITH_SUM_SCALE__ 1.8f
static __share__ inline float conv_with_sum_scale(int n, int f, int h, int w) {
    if (n * f * h * w < 65535 || n <= 10) {
        return 1;
    }
    return (0.0f + n) / __CONV_WITH_SUM_SCALE__;
}
#endif