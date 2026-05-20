/// \file   dll_export.h
/// \brief  设置符号是否在DSO动态符号表中可见的宏，凡使用DLL_EXPORT的文件均需包含本头文件
/// \author miaotianxiang@baidu.com
/// \copyright (C) 2019 Baidu, Inc

#ifndef BAIDU_XPU_API_INCLUDE_XPU_DLL_EXPORT_H
#define BAIDU_XPU_API_INCLUDE_XPU_DLL_EXPORT_H

#if defined(_WIN32) || defined(WIN32)
#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)
#elif defined(XPUAPI_HIDE_ALL)
#define DLL_EXPORT
#define DLL_IMPORT
#else
#define DLL_EXPORT __attribute__((visibility("default")))
#define DLL_IMPORT __attribute__((visibility("default")))
#endif

#endif // BAIDU_XPU_API_INCLUDE_XPU_DLL_EXPORT_H
