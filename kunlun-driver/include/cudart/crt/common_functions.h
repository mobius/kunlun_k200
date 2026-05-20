#if !defined(__CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__)
#if defined(_MSC_VER)
#pragma message("crt/common_functions.h is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead.")
#else
#warning "crt/common_functions.h is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead."
#endif
#define __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#define __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_COMMON_FUNCTIONS_H__
#endif

#if !defined(__COMMON_FUNCTIONS_H__)
#define __COMMON_FUNCTIONS_H__

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#if defined(__cplusplus) && defined(__CUDACC__)

#include "builtin_types.h"
#include "host_defines.h"

#define __CUDACC_VER__ "__CUDACC_VER__ is no longer supported.  Use __CUDACC_VER_MAJOR__, __CUDACC_VER_MINOR__, and __CUDACC_VER_BUILD__ instead."

#if !defined(__CUDACC_RTC__)
#include <string.h>
#include <time.h>

extern "C"
{
#endif /* !__CUDACC_RTC__ */
extern _CRTIMP __host__ __device__ __device_builtin__ __cudart_builtin__ clock_t __cdecl clock(void)
#if defined(__QNX__)
asm("clock32")
#endif
__THROW;
extern         __host__ __device__ __device_builtin__ __cudart_builtin__ void*   __cdecl memset(void*, int, size_t) __THROW;
extern         __host__ __device__ __device_builtin__ __cudart_builtin__ void*   __cdecl memcpy(void*, const void*, size_t) __THROW;
#if !defined(__CUDACC_RTC__)
}
#endif /* !__CUDACC_RTC__ */

#if defined(__CUDA_ARCH__)

#if defined(__CUDACC_RTC__)
inline __host__ __device__ void* operator new(size_t, void *p) { return p; }
inline __host__ __device__ void* operator new[](size_t, void *p) { return p; }
inline __host__ __device__ void operator delete(void*, void*) { }
inline __host__ __device__ void operator delete[](void*, void*) { }
#else /* !__CUDACC_RTC__ */
#ifndef __CUDA_INTERNAL_SKIP_CPP_HEADERS__
#include <new>
#endif

#if defined (__GNUC__)

#define STD \
        std::

#else /* __GNUC__ */

#define STD

#endif /* __GNUC__ */

extern         __host__ __device__ __cudart_builtin__ void*   __cdecl operator new(STD size_t, void*) throw();
extern         __host__ __device__ __cudart_builtin__ void*   __cdecl operator new[](STD size_t, void*) throw();
extern         __host__ __device__ __cudart_builtin__ void    __cdecl operator delete(void*, void*) throw();
extern         __host__ __device__ __cudart_builtin__ void    __cdecl operator delete[](void*, void*) throw();
# if __cplusplus >= 201402L || (defined(_MSC_VER) && _MSC_VER >= 1900) || defined(__CUDA_XLC_CPP14__) || defined(__CUDA_ICC_CPP14__)
extern         __host__ __device__ __cudart_builtin__ void    __cdecl operator delete(void*, STD size_t) throw();
extern         __host__ __device__ __cudart_builtin__ void    __cdecl operator delete[](void*, STD size_t) throw();
#endif /* __cplusplus >= 201402L || (defined(_MSC_VER) && _MSC_VER >= 1900) || defined(__CUDA_XLC_CPP14__)  || defined(__CUDA_ICC_CPP14__) */
#endif /* __CUDACC_RTC__ */

#if !defined(__CUDACC_RTC__)
#include <stdio.h>
#include <stdlib.h>
#endif /* !__CUDACC_RTC__ */

#if defined(__QNX__) && !defined(_LIBCPP_VERSION)
namespace std {
#endif
extern "C"
{
extern
#if !defined(_MSC_VER) || _MSC_VER < 1900
_CRTIMP
#endif

#if defined(__GLIBC__) && defined(__GLIBC_MINOR__) && ( (__GLIBC__ < 2) || ( (__GLIBC__ == 2) && (__GLIBC_MINOR__ < 3) ) )
__host__ __device__ __device_builtin__ __cudart_builtin__ int     __cdecl printf(const char*, ...) __THROW;
#else /* newer glibc */
__host__ __device__ __device_builtin__ __cudart_builtin__ int     __cdecl printf(const char*, ...);
#endif /* defined(__GLIBC__) && defined(__GLIBC_MINOR__) && ( (__GLIBC__ < 2) || ( (__GLIBC__ == 2) && (__GLIBC_MINOR__ < 3) ) ) */


extern _CRTIMP __host__ __device__ __cudart_builtin__ void*   __cdecl malloc(size_t) __THROW;
extern _CRTIMP __host__ __device__ __cudart_builtin__ void    __cdecl free(void*) __THROW;

}
#if defined(__QNX__) && !defined(_LIBCPP_VERSION)
} /* std */
#endif

#if !defined(__CUDACC_RTC__)
#include <assert.h>
#endif /* !__CUDACC_RTC__ */

extern "C"
{
#if defined(__CUDACC_RTC__)
extern __host__ __device__ void __assertfail(const char * __assertion,
                                             const char *__file,
                                             unsigned int __line,
                                             const char *__function,
                                             size_t charsize);
#elif defined(__APPLE__)
#define __builtin_expect(exp,c) (exp)
extern __host__ __device__ __cudart_builtin__ void __assert_rtn(
  const char *, const char *, int, const char *);
#elif defined(__ANDROID__)
extern __host__ __device__ __cudart_builtin__ void __assert2(
  const char *, int, const char *, const char *);
#elif defined(__QNX__)
#if !defined(_LIBCPP_VERSION)
namespace std {
#endif
extern __host__ __device__ __cudart_builtin__ void __assert(
  const char *, const char *, unsigned int, const char *);
#if !defined(_LIBCPP_VERSION)
}
#endif
#elif defined(__HORIZON__)
extern __host__ __device__ __cudart_builtin__ void __assert_fail(
  const char *, const char *, int, const char *);
#elif defined(__GNUC__)
extern __host__ __device__ __cudart_builtin__ void __assert_fail(
  const char *, const char *, unsigned int, const char *)
  __THROW;
#elif defined(_WIN32)
extern __host__ __device__ __cudart_builtin__ _CRTIMP void __cdecl _wassert(
  const wchar_t *, const wchar_t *, unsigned);
#endif
}

#if defined(__CUDACC_RTC__)
#ifdef NDEBUG
#define assert(e) (static_cast<void>(0))
#else /* !NDEBUG */
#define __ASSERT_STR_HELPER(x) #x
#define assert(e) ((e) ? static_cast<void>(0)\
                       : __assertfail(__ASSERT_STR_HELPER(e), __FILE__,\
                                      __LINE__, __PRETTY_FUNCTION__,\
                                      sizeof(char)))
#endif /* NDEBUG */
inline  __host__ __device__  void* operator new(size_t in) {  return malloc(in); }
inline  __host__ __device__  void* operator new[](size_t in) { return malloc(in); }
inline __host__ __device__  void operator delete(void* in) { return free(in); }
inline __host__ __device__  void operator delete[](void* in) {  return free(in); }
# if __cplusplus >= 201402L || defined(__CUDA_XLC_CPP14__)
inline __host__ __device__  void operator delete(void* in, size_t) { return free(in); }
inline __host__ __device__  void operator delete[](void* in, size_t) {  return free(in); }
#endif /* __cplusplus >= 201402L || defined(__CUDA_XLC_CPP14__) */
#else /* !__CUDACC_RTC__ */
#if defined (__GNUC__)

#define __NV_GLIBCXX_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

# if (__cplusplus >= 201103L)  && !(defined(__QNX__) && defined(_LIBCPP_VERSION))
#define THROWBADALLOC
#else
#if defined(__ANDROID__) && !defined(_LIBCPP_VERSION) && (defined(__BIONIC__) || __NV_GLIBCXX_VERSION < 40900)
#define THROWBADALLOC
#else
#define THROWBADALLOC  throw(STD bad_alloc)
#endif
#endif
#define __DELETE_THROW throw()

#undef __NV_GLIBCXX_VERSION

#else /* __GNUC__ */

#define THROWBADALLOC  throw(...)

#endif /* __GNUC__ */

extern         __host__ __device__ __cudart_builtin__ void*   __cdecl operator new(STD size_t) THROWBADALLOC;
extern         __host__ __device__ __cudart_builtin__ void*   __cdecl operator new[](STD size_t) THROWBADALLOC;
extern         __host__ __device__ __cudart_builtin__ void    __cdecl operator delete(void*) throw();
extern         __host__ __device__ __cudart_builtin__ void    __cdecl operator delete[](void*) throw();
# if __cplusplus >= 201402L || (defined(_MSC_VER) && _MSC_VER >= 1900) || defined(__CUDA_XLC_CPP14__) || defined(__CUDA_ICC_CPP14__)
extern         __host__ __device__ __cudart_builtin__ void    __cdecl operator delete(void*, STD size_t) throw();
extern         __host__ __device__ __cudart_builtin__ void    __cdecl operator delete[](void*, STD size_t) throw();
#endif /* __cplusplus >= 201402L || (defined(_MSC_VER) && _MSC_VER >= 1900) || defined(__CUDA_XLC_CPP14__) || defined(__CUDA_ICC_CPP14__)  */

#undef THROWBADALLOC
#undef STD
#endif /* __CUDACC_RTC__ */

#endif /* __CUDA_ARCH__ */

#endif /* __cplusplus && __CUDACC__ */

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#if defined(__CUDACC_RTC__) && (__CUDA_ARCH__ >= 350)
#include "cuda_device_runtime_api.h"
#endif

#include "math_functions.h"

#endif /* !__COMMON_FUNCTIONS_H__ */

#if defined(__UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_COMMON_FUNCTIONS_H__)
#undef __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#undef __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_COMMON_FUNCTIONS_H__
#endif
