/// \file   types.h
/// \brief  XPU API types
/// \author shijiaxin01@baidu.com
/// \copyright (C) 2019 Baidu, Inc
#ifndef BAIDU_XPU_API_INCLUDE_XPU_ERROR_H
#define BAIDU_XPU_API_INCLUDE_XPU_ERROR_H

#include <cstdint>
#include <stdlib.h>
#include <vector>

namespace baidu {
namespace xpu {
namespace api {
/// \brief  XPUAPI使用的错误类型
typedef enum {
    SUCCESS = 0,
    INVALID_PARAM = 1,
    RUNTIME_ERROR = 2,
    NO_ENOUGH_WORKSPACE = 3,
    NOT_IMPLEMENT = 4,
} Error_t;

}  // namespace api
}  // namespace xpu
}  // namespace baidu

#endif // BAIDU_XPU_API_INCLUDE_XPU_ERROR_H
