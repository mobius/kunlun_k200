// Copyright 2018 Baidu Inc. All Rights Reserved.
// authors: Han Jinchen hanjinche@baidu.com
//
// bincommon.h - common utils for xpurt binary
//
#pragma once

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/time.h>
#include "xpurt_priv/xarray.h"
#include "xpurt_priv/defs_private.h"

// Error defination
enum {
    XRTT_SUCCESS = 0,
    XRTT_EINVAL  = 1,     // invalid parameter
    XRTT_EINPUT  = 2,     // invalid input value
    XRTT_ERT     = 3,     // runtime error
    XRTT_ERESULT = 4,     // result error
};

class input_error : virtual public std::logic_error {
public:
    input_error(const std::string& msg) : logic_error(msg) {}
};

class xrt_error : virtual public std::logic_error {
public:
    xrt_error(const std::string& msg) : logic_error(msg) {}
};

class result_error : virtual public std::logic_error {
public:
    result_error(const std::string& msg) : logic_error(msg) {}
};

// Utilities

#define timeval_to_us(tv) (((tv).tv_sec * 1000000ULL) + (tv).tv_usec)

#if defined(__i386__)
static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)
static __inline__ unsigned long long rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#else
static __inline__ unsigned long long rdtsc(void){
    return 0;
}
#endif

// Log helper

#define LOGE(fmt, arg...) do {                  \
        fprintf(stderr, "[ERROR][%s:%d] " fmt,  \
                __func__, __LINE__, ## arg);    \
    } while (0)

#define LOGW(fmt, arg...) do {                  \
        fprintf(stderr, "[ WARN][%s:%d] " fmt,  \
                __func__, __LINE__, ## arg);    \
    } while (0)

#define LOGI(fmt, arg...) do {                  \
        fprintf(stderr, "[ INFO][%s:%d] " fmt,  \
                __func__, __LINE__, ## arg);    \
    } while (0)

#ifdef DEBUG_XPU_TEST
#define LOGD(fmt, arg...) do {                  \
        fprintf(stderr, "[DEBUG][%s:%d] " fmt,  \
                __func__, __LINE__, ## arg);    \
    }while(0)
#else
#define LOGD(fmt, arg...)
#endif

#define TRUN(fmt, arg...) do {                  \
        fprintf(stderr, "[ RUN] " fmt, ## arg); \
    } while (0)

#define TPASS(fmt, arg...) do {                 \
        fprintf(stderr, "[PASS] " fmt, ## arg); \
    } while (0)

#define TFAIL(fmt, arg...) do {                 \
        fprintf(stderr, "[FAIL] " fmt, ## arg); \
    } while (0)

static inline
int fp_eq(float a, float b) {
    return std::fabs(a - b) < 0.00001f;
}

static inline
int __check_output(std::shared_ptr<XArray> x, float value) {
    int error_count = 0;
    int print_count = 0;
    for (size_t i = 0; i < x->length(); ++i) {
        if (!fp_eq(x->cpu()[i], value)) {
            ++error_count;
            if (print_count < 1) {
                ++print_count;
                LOGE("  [%zd] exp= %f, real= %f\n", i, value, x->cpu()[i]);
            }
        }
    }
    return error_count;
}

// check that z[i] == x[i] + y[i]
static inline
int __check_output(std::shared_ptr<XArray> x, std::shared_ptr<XArray> y,
                          std::shared_ptr<XArray> z) {
    assert(x->length() == y->length());
    assert(x->length() == z->length());

    int error_count = 0;
    int print_count = 0;
    for (size_t i = 0; i < x->length(); ++i) {
        if (!fp_eq(x->cpu()[i] + y->cpu()[i], z->cpu()[i])) {
            ++error_count;
            if (print_count < 1) {
                ++print_count;
                LOGE("  [%zd] in0= %f, in1= %f, out= %f\n",
                     i, x->cpu()[i], y->cpu()[i], z->cpu()[i]);
            }
        }
    }
    return error_count;
}
