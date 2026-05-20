#if !defined(__SURFACE_TYPES_H__)
#define __SURFACE_TYPES_H__

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

#define cudaSurfaceType1D              0x01
#define cudaSurfaceType2D              0x02
#define cudaSurfaceType3D              0x03
#define cudaSurfaceTypeCubemap         0x0C
#define cudaSurfaceType1DLayered       0xF1
#define cudaSurfaceType2DLayered       0xF2
#define cudaSurfaceTypeCubemapLayered  0xFC

/**
 * CUDA Surface boundary modes
 */
enum __device_builtin__ cudaSurfaceBoundaryMode
{
    cudaBoundaryModeZero  = 0,    /**< Zero boundary mode */
    cudaBoundaryModeClamp = 1,    /**< Clamp boundary mode */
    cudaBoundaryModeTrap  = 2     /**< Trap boundary mode */
};

/**
 * CUDA Surface format modes
 */
enum __device_builtin__  cudaSurfaceFormatMode
{
    cudaFormatModeForced = 0,     /**< Forced format mode */
    cudaFormatModeAuto = 1        /**< Auto format mode */
};

/**
 * CUDA Surface reference
 */
struct __device_builtin__ surfaceReference
{
    /**
     * Channel descriptor for surface reference
     */
    struct cudaChannelFormatDesc channelDesc;
};

/**
 * An opaque value that represents a CUDA Surface object
 */
typedef __device_builtin__ unsigned long long cudaSurfaceObject_t;

/** @} */
/** @} */ /* END CUDART_TYPES */

#endif /* !__SURFACE_TYPES_H__ */
