#if !defined(__TEXTURE_TYPES_H__)
#define __TEXTURE_TYPES_H__

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#include "driver_types.h"

/**
 * \addtogroup CUDART_TYPES
 *
 * @{
 */

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#define cudaTextureType1D              0x01
#define cudaTextureType2D              0x02
#define cudaTextureType3D              0x03
#define cudaTextureTypeCubemap         0x0C
#define cudaTextureType1DLayered       0xF1
#define cudaTextureType2DLayered       0xF2
#define cudaTextureTypeCubemapLayered  0xFC

/**
 * CUDA texture address modes
 */
enum __device_builtin__ cudaTextureAddressMode
{
    cudaAddressModeWrap   = 0,    /**< Wrapping address mode */
    cudaAddressModeClamp  = 1,    /**< Clamp to edge address mode */
    cudaAddressModeMirror = 2,    /**< Mirror address mode */
    cudaAddressModeBorder = 3     /**< Border address mode */
};

/**
 * CUDA texture filter modes
 */
enum __device_builtin__ cudaTextureFilterMode
{
    cudaFilterModePoint  = 0,     /**< Point filter mode */
    cudaFilterModeLinear = 1      /**< Linear filter mode */
};

/**
 * CUDA texture read modes
 */
enum __device_builtin__ cudaTextureReadMode
{
    cudaReadModeElementType     = 0,  /**< Read texture as specified element type */
    cudaReadModeNormalizedFloat = 1   /**< Read texture as normalized float */
};

/**
 * CUDA texture reference
 */
struct __device_builtin__ textureReference
{
    /**
     * Indicates whether texture reads are normalized or not
     */
    int                          normalized;
    /**
     * Texture filter mode
     */
    enum cudaTextureFilterMode   filterMode;
    /**
     * Texture address mode for up to 3 dimensions
     */
    enum cudaTextureAddressMode  addressMode[3];
    /**
     * Channel descriptor for the texture reference
     */
    struct cudaChannelFormatDesc channelDesc;
    /**
     * Perform sRGB->linear conversion during texture read
     */
    int                          sRGB;
    /**
     * Limit to the anisotropy ratio
     */
    unsigned int                 maxAnisotropy;
    /**
     * Mipmap filter mode
     */
    enum cudaTextureFilterMode   mipmapFilterMode;
    /**
     * Offset applied to the supplied mipmap level
     */
    float                        mipmapLevelBias;
    /**
     * Lower end of the mipmap level range to clamp access to
     */
    float                        minMipmapLevelClamp;
    /**
     * Upper end of the mipmap level range to clamp access to
     */
    float                        maxMipmapLevelClamp;
    /**
     * Disable any trilinear filtering optimizations.
     */
    int                          disableTrilinearOptimization;
    int                          __cudaReserved[14];
};

/**
 * CUDA texture descriptor
 */
struct __device_builtin__ cudaTextureDesc
{
    /**
     * Texture address mode for up to 3 dimensions
     */
    enum cudaTextureAddressMode addressMode[3];
    /**
     * Texture filter mode
     */
    enum cudaTextureFilterMode  filterMode;
    /**
     * Texture read mode
     */
    enum cudaTextureReadMode    readMode;
    /**
     * Perform sRGB->linear conversion during texture read
     */
    int                         sRGB;
    /**
     * Texture Border Color
     */
    float                       borderColor[4];
    /**
     * Indicates whether texture reads are normalized or not
     */
    int                         normalizedCoords;
    /**
     * Limit to the anisotropy ratio
     */
    unsigned int                maxAnisotropy;
    /**
     * Mipmap filter mode
     */
    enum cudaTextureFilterMode  mipmapFilterMode;
    /**
     * Offset applied to the supplied mipmap level
     */
    float                       mipmapLevelBias;
    /**
     * Lower end of the mipmap level range to clamp access to
     */
    float                       minMipmapLevelClamp;
    /**
     * Upper end of the mipmap level range to clamp access to
     */
    float                       maxMipmapLevelClamp;
    /**
     * Disable any trilinear filtering optimizations.
     */
    int                         disableTrilinearOptimization;
    /**
     * Enable seamless cube map filtering.
     */
    int                         seamlessCubemap;
};

/**
 * An opaque value that represents a CUDA texture object
 */
typedef __device_builtin__ unsigned long long cudaTextureObject_t;

/** @} */
/** @} */ /* END CUDART_TYPES */

#endif /* !__TEXTURE_TYPES_H__ */
