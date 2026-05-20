#if !defined(__CUDA_TEXTURE_TYPES_H__)
#define __CUDA_TEXTURE_TYPES_H__

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

template<class T, int texType = cudaTextureType1D, enum cudaTextureReadMode mode = cudaReadModeElementType>
struct __device_builtin_texture_type__ texture : public textureReference
{
#if !defined(__CUDACC_RTC__)
  __host__ texture(int                         norm  = 0,
                   enum cudaTextureFilterMode  fMode = cudaFilterModePoint,
                   enum cudaTextureAddressMode aMode = cudaAddressModeClamp)
  {
    normalized     = norm;
    filterMode     = fMode;
    addressMode[0] = aMode;
    addressMode[1] = aMode;
    addressMode[2] = aMode;
    channelDesc    = cudaCreateChannelDesc<T>();
    sRGB           = 0;
  }

  __host__ texture(int                          norm,
                   enum cudaTextureFilterMode   fMode,
                   enum cudaTextureAddressMode  aMode,
                   struct cudaChannelFormatDesc desc)
  {
    normalized     = norm;
    filterMode     = fMode;
    addressMode[0] = aMode;
    addressMode[1] = aMode;
    addressMode[2] = aMode;
    channelDesc    = desc;
    sRGB           = 0;
  }
#endif /* !__CUDACC_RTC__ */
};

#endif /* __cplusplus && __CUDACC__ */

#endif /* !__CUDA_TEXTURE_TYPES_H__ */
