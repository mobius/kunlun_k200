/// \file   xarray.h
/// \brief  An XPU array utility
/// \author hanjinchen@baidu.com
/// \copyright (C) 2018 Baidu, Inc
#pragma once
#include "xpu/runtime.h"
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
class XArray {
 public:
    XArray(size_t len) : _len(len), _sz(len * sizeof(float)) {
#ifdef KL2_SOC
        xpu_soc_malloc(&_xpu, &_cpu, _sz);
#else
        xpu_malloc(&_xpu, _sz);
        _cpu = std::malloc(_sz);
#endif
        assert(_xpu != NULL);
        assert(_cpu != NULL);
        memset(_cpu, 0, _sz);
    }

    ~XArray() {
#ifdef KL2_SOC
        xpu_soc_free(_xpu, _cpu);
#else
        xpu_free(_xpu);
        std::free(_cpu);
#endif
    }

    float *xpu() { return (float *)_xpu; }
    float *cpu() { return (float *)_cpu; }
    size_t length() { return _len; }
    size_t size() { return _sz; }
 private:
    void *_xpu;
    void *_cpu;
    size_t _len;
    size_t _sz;
};


class XIArray {
 public:
    XIArray(size_t len) : _len(len), _sz(len * sizeof(int)) {
#ifdef KL2_SOC
        xpu_soc_malloc(&_xpu, &_cpu, _sz);
#else
        xpu_malloc(&_xpu, _sz);
        _cpu = std::malloc(_sz);
#endif
        assert(_xpu != NULL);
        assert(_cpu != NULL);
        memset(_cpu, 0, _sz);
    }

    ~XIArray() {
#ifdef KL2_SOC
        xpu_soc_free(_xpu, _cpu);
#else
        xpu_free(_xpu);
        std::free(_cpu);
#endif
    }

    int *xpu() { return (int *)_xpu; }
    int *cpu() { return (int *)_cpu; }
    size_t length() { return _len; }
    size_t size() { return _sz; }
 private:
    void *_xpu;
    void *_cpu;
    size_t _len;
    size_t _sz;
};

class XUIArray {
 public:
    XUIArray(size_t len) : _len(len), _sz(len * sizeof(unsigned int)) {
#ifdef KL2_SOC
        xpu_soc_malloc(&_xpu, &_cpu, _sz);
#else
        xpu_malloc(&_xpu, _sz);
        _cpu = std::malloc(_sz);
#endif
        assert(_xpu != NULL);
        assert(_cpu != NULL);
        memset(_cpu, 0, _sz);
    }

    ~XUIArray() {
#ifdef KL2_SOC
        xpu_soc_free(_xpu, _cpu);
#else
        xpu_free(_xpu);
        std::free(_cpu);
#endif
    }

    unsigned int *xpu() { return (unsigned int *)_xpu; }
    unsigned int *cpu() { return (unsigned int *)_cpu; }
    size_t length() { return _len; }
    size_t size() { return _sz; }
 private:
    void *_xpu;
    void *_cpu;
    size_t _len;
    size_t _sz;
};
