#if !defined(__CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__)
#if defined(_MSC_VER)
#pragma message("crt/sm_70_rt.h is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead.")
#else
#warning "crt/sm_70_rt.h is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead."
#endif
#define __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#define __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_70_RT_H__
#endif

#if !defined(__SM_70_RT_H__)
#define __SM_70_RT_H__

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

#ifndef __CUDA_ARCH__
#define __DEF_IF_HOST { }
#else  /* !__CUDA_ARCH__ */
#define __DEF_IF_HOST ;
#endif /* __CUDA_ARCH__ */


/******************************************************************************
 *                                   match                                   *
 ******************************************************************************/
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, unsigned value) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, int value) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, unsigned long value) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, long value) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, unsigned long long value) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, long long value) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, float value) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_any_sync(unsigned mask, double value) __DEF_IF_HOST

__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, unsigned value, int *pred) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, int value, int *pred) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, unsigned long value, int *pred) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, long value, int *pred) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, unsigned long long value, int *pred) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, long long value, int *pred) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, float value, int *pred) __DEF_IF_HOST
__SM_70_RT_DECL__ unsigned int __match_all_sync(unsigned mask, double value, int *pred) __DEF_IF_HOST

__SM_70_RT_DECL__ void __nanosleep(unsigned int ns) __DEF_IF_HOST

__SM_70_RT_DECL__ unsigned short int atomicCAS(unsigned short int *address, unsigned short int compare, unsigned short int val) __DEF_IF_HOST

#endif /* !__CUDA_ARCH__ || __CUDA_ARCH__ >= 700 */

#endif /* __cplusplus && __CUDACC__ */

#undef __DEF_IF_HOST
#undef __SM_70_RT_DECL__

#if !defined(__CUDACC_RTC__) && defined(__CUDA_ARCH__)
#include "sm_70_rt.hpp"
#endif /* !__CUDACC_RTC__ && defined(__CUDA_ARCH__) */

#endif /* !__SM_70_RT_H__ */

#if defined(__UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_70_RT_H__)
#undef __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#undef __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_70_RT_H__
#endif
