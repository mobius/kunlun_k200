/*!
 *  Copyright (c) 2015 by Contributors
 * \file logging.h
 * \brief defines logging macros of dmlc
 *  allows use of GLOG, fall back to internal
 *  implementation when disabled
 */
#pragma once
// The content of this file has been migrated from include/internal/logging.h.
// The purpose of creating a new header file is to distinguish the roles of the two header files.
// This current file is dedicated to addressing shared lightweight logging among future API product libraries.
// The old file is only used for lightweight logging in the XDNN library.

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

#define LOGE(fmt, arg...) do {                      \
        fprintf(stderr, "[ERR][XPUAPI][%s:%d] " fmt, \
                __FILE__, __LINE__, ## arg);        \
    } while (0)
#define LOGW(fmt, arg...) do {                          \
        fprintf(stderr, "[WARN][XPUAPI][%s:%d] " fmt,    \
                __FILE__, __LINE__, ## arg);            \
    } while (0)
#define LOGI(fmt, arg...) do {                          \
        fprintf(stderr, "[INFO][XPUAPI][%s:%d] " fmt,    \
                __FILE__, __LINE__, ## arg);            \
    } while (0)

#include <assert.h>
#define CHECK(a)
#define CHECK_EQ(a, b)
#define CHECK_NE(a, b)
#define CHECK_GT(a, b)
#define CHECK_LT(a, b)
#define CHECK_GE(a, b)
#define CHECK_LE(a, b)
