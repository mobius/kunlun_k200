#if !defined(__CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__)
#if defined(_MSC_VER)
#pragma message("crt/device_functions.hpp is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead.")
#else
#warning "crt/device_functions.hpp is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead."
#endif
#define __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#define __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_DEVICE_FUNCTIONS_HPP__
#endif

#if !defined(__DEVICE_FUNCTIONS_HPP__)
#define __DEVICE_FUNCTIONS_HPP__

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#if defined(__cplusplus) && defined(__CUDACC__)

#if defined(__CUDACC_RTC__)
#define __DEVICE_FUNCTIONS_DECL__ __device__
#define __DEVICE_FUNCTIONS_STATIC_DECL__ __device__
#else
#define __DEVICE_FUNCTIONS_DECL__ __device__
#define __DEVICE_FUNCTIONS_STATIC_DECL__ static __inline__ __device__
#endif /* __CUDACC_RTC__ */

#include "builtin_types.h"
#include "device_types.h"
#include "host_defines.h"


/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

__DEVICE_FUNCTIONS_STATIC_DECL__ int mulhi(int a, int b)
{
  return __mulhi(a, b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned int mulhi(unsigned int a, unsigned int b)
{
  return __umulhi(a, b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned int mulhi(int a, unsigned int b)
{
  return __umulhi((unsigned int)a, b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned int mulhi(unsigned int a, int b)
{
  return __umulhi(a, (unsigned int)b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ long long int mul64hi(long long int a, long long int b)
{
  return __mul64hi(a, b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned long long int mul64hi(unsigned long long int a, unsigned long long int b)
{
  return __umul64hi(a, b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned long long int mul64hi(long long int a, unsigned long long int b)
{
  return __umul64hi((unsigned long long int)a, b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned long long int mul64hi(unsigned long long int a, long long int b)
{
  return __umul64hi(a, (unsigned long long int)b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ int float_as_int(float a)
{
  return __float_as_int(a);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ float int_as_float(int a)
{
  return __int_as_float(a);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned int float_as_uint(float a)
{
  return __float_as_uint(a);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ float uint_as_float(unsigned int a)
{
  return __uint_as_float(a);
}
__DEVICE_FUNCTIONS_STATIC_DECL__ float saturate(float a)
{
  return __saturatef(a);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ int mul24(int a, int b)
{
  return __mul24(a, b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned int umul24(unsigned int a, unsigned int b)
{
  return __umul24(a, b);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ int float2int(float a, enum cudaRoundMode mode)
{
  return mode == cudaRoundNearest ? __float2int_rn(a) :
         mode == cudaRoundPosInf  ? __float2int_ru(a) :
         mode == cudaRoundMinInf  ? __float2int_rd(a) :
                                    __float2int_rz(a);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ unsigned int float2uint(float a, enum cudaRoundMode mode)
{
  return mode == cudaRoundNearest ? __float2uint_rn(a) :
         mode == cudaRoundPosInf  ? __float2uint_ru(a) :
         mode == cudaRoundMinInf  ? __float2uint_rd(a) :
                                    __float2uint_rz(a);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ float int2float(int a, enum cudaRoundMode mode)
{
  return mode == cudaRoundZero   ? __int2float_rz(a) :
         mode == cudaRoundPosInf ? __int2float_ru(a) :
         mode == cudaRoundMinInf ? __int2float_rd(a) :
                                   __int2float_rn(a);
}

__DEVICE_FUNCTIONS_STATIC_DECL__ float uint2float(unsigned int a, enum cudaRoundMode mode)
{
  return mode == cudaRoundZero   ? __uint2float_rz(a) :
         mode == cudaRoundPosInf ? __uint2float_ru(a) :
         mode == cudaRoundMinInf ? __uint2float_rd(a) :
                                   __uint2float_rn(a);
}

#undef __DEVICE_FUNCTIONS_DECL__
#undef __DEVICE_FUNCTIONS_STATIC_DECL__

#endif /* __cplusplus && __CUDACC__ */

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#endif /* !__DEVICE_FUNCTIONS_HPP__ */

#if defined(__UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_DEVICE_FUNCTIONS_HPP__)
#undef __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#undef __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_DEVICE_FUNCTIONS_HPP__
#endif
