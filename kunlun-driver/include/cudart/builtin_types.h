/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#include "device_types.h"
#if !defined(__CUDACC_RTC__)
#define EXCLUDE_FROM_RTC
#include "driver_types.h"
#undef EXCLUDE_FROM_RTC
#endif /* !__CUDACC_RTC__ */
#include "surface_types.h"
#include "texture_types.h"
#include "vector_types.h"
