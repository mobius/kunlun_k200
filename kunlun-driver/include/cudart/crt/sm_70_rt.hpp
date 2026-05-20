#if !defined(__CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__)
#if defined(_MSC_VER)
#pragma message("crt/sm_70_rt.hpp is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead.")
#else
#warning "crt/sm_70_rt.hpp is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead."
#endif
#define __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#define __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_70_RT_HPP__
#endif

#if !defined(__SM_70_RT_HPP__)
#define __SM_70_RT_HPP__

#if defined(__CUDACC_RTC__)
#define __SM_70_RT_DECL__ __host__ __device__
#else /* !__CUDACC_RTC__ */
#define __SM_70_RT_DECL__ static __device__ __inline__
#endif /* __CUDACC_RTC__ */

#if defined(__cplusplus) && defined(__CUDACC__)

#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 700

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#include "builtin_types.h"
#include "device_types.h"
#include "host_defines.h"

/*******************************************************************************
*                                                                              *
*  Below are implementations of SM-7.0 builtin functions which are included as *
*  source (instead of being built in to the compiler)                          *
*                                                                              *
*******************************************************************************/

//
// __match_any_sync
//
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, unsigned value) {
  return __match32_any_sync(mask, value);
}

__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, int value) {
  return __match32_any_sync(mask, value);
}

__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, unsigned long value) {
  return (sizeof(long) == sizeof(long long)) ?
    __match64_any_sync(mask, (unsigned long long)value):
    __match32_any_sync(mask, (unsigned)value);
}

__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, long value) {
  return (sizeof(long) == sizeof(long long)) ?
    __match64_any_sync(mask, (unsigned long long)value):
    __match32_any_sync(mask, (unsigned)value);
}

__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, unsigned long long value) {
  return __match64_any_sync(mask, value);
}

__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, long long value) {
  return __match64_any_sync(mask, value);
}

__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, float value) {
  return __match32_any_sync(mask, float_as_uint(value));
}

__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, double value) {
  return __match64_any_sync(mask, __double_as_longlong(value));
}

//
// __match_all_sync
//
__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, unsigned value, int *pred) {
  return __match32_all_sync(mask, value, pred);
}

__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, int value, int *pred) {
  return __match32_all_sync(mask, value, pred);
}

__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, unsigned long value, int *pred) {
  return (sizeof(long) == sizeof(long long)) ?
    __match64_all_sync(mask, (unsigned long long)value, pred):
    __match32_all_sync(mask, (unsigned)value, pred);
}

__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, long value, int *pred) {
  return (sizeof(long) == sizeof(long long)) ?
    __match64_all_sync(mask, (unsigned long long)value, pred):
    __match32_all_sync(mask, (unsigned)value, pred);
}

__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, unsigned long long value, int *pred) {
  return __match64_all_sync(mask, value, pred);
}

__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, long long value, int *pred) {
  return __match64_all_sync(mask, value, pred);
}

__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, float value, int *pred) {
  return __match32_all_sync(mask, float_as_uint(value), pred);
}

__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, double value, int *pred) {
  return __match64_all_sync(mask, __double_as_longlong(value), pred);
}

__SM_70_RT_DECL__ void __nanosleep(unsigned int ns) {
    asm volatile("nanosleep.u32 %0;" :: "r"(ns));
}


extern "C" __device__ __device_builtin__
unsigned short __usAtomicCAS(unsigned short *, unsigned short, unsigned short);

__SM_70_RT_DECL__ unsigned short int atomicCAS(unsigned short int *address, unsigned short int compare, unsigned short int val) {
  return __usAtomicCAS(address, compare, val);
}


#endif /* !__CUDA_ARCH__ || __CUDA_ARCH__ >= 700 */

#endif /* __cplusplus && __CUDACC__ */

#undef __SM_70_RT_DECL__

#endif /* !__SM_70_RT_HPP__ */

#if defined(__UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_70_RT_HPP__)
#undef __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#undef __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_70_RT_HPP__
#endif
