/*
 * SPDX-FileCopyrightText: Copyright (c) 2021 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef _NV_STDARG_H_
#define _NV_STDARG_H_

// XXX(miaotianxiang): 高版本linux中包含<linux/stdarg.h>，与gcc自带stdarg.h冲突造成宏重定义
//                     此为临时修复方案，可能还需要适当调整头文件顺序
#ifndef va_start

#if defined(NV_KERNEL_INTERFACE_LAYER) && defined(NV_LINUX)
  #include "conftest.h"
  #if defined(NV_LINUX_STDARG_H_PRESENT)
    #include <linux/stdarg.h>
  #else
    #include <stdarg.h>
  #endif
#else
  #include <stdarg.h>
#endif

#endif // va_start

#endif // _NV_STDARG_H_
