#if !defined(__CUDA_SURFACE_TYPES_H__)
#define __CUDA_SURFACE_TYPES_H__

#if defined(__cplusplus) && defined(__CUDACC__)

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#if !defined(__CUDACC_RTC__)
#define EXCLUDE_FROM_RTC
#include "channel_descriptor.h"
#undef EXCLUDE_FROM_RTC
#endif /* !__CUDACC_RTC__ */
#include "cuda_runtime_api.h"

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

template<class T, int dim = 1>
struct __device_builtin_surface_type__ surface : public surfaceReference
{
#if !defined(__CUDACC_RTC__)
  __host__ surface(void)
  {
    channelDesc = cudaCreateChannelDesc<T>();
  }

  __host__ surface(struct cudaChannelFormatDesc desc)
  {
    channelDesc = desc;
  }
#endif /* !__CUDACC_RTC__ */
};

template<int dim>
struct  __device_builtin_surface_type__  surface<void, dim> : public surfaceReference
{
#if !defined(__CUDACC_RTC__)
  __host__ surface(void)
  {
    channelDesc = cudaCreateChannelDesc<void>();
  }
#endif /* !__CUDACC_RTC__ */
};

#endif /* __cplusplus && __CUDACC__ */

#endif /* !__CUDA_SURFACE_TYPES_H__ */
