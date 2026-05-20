#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_BIT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_BIT_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <xpu/dll_export.h>

struct DLL_EXPORT ALIGN_ATTR bit16_t {
    unsigned short val;

    inline void reset() {
        val = 0;
    }
    inline void set_bit(int bit_idx, bool b) {
        val = (val & (~(0x1 << bit_idx))) | ((b ? 0x1 : 0x0) << bit_idx);
    }
    inline bool get_bit(int bit_idx) {
        return (val & (1 << bit_idx)) != 0;
    }
    inline bit16_t(unsigned short value);
    inline operator uint16_t() const;
};

/// Constructors
inline bit16_t::bit16_t(unsigned short value) {
    val = value;
}

/// Implicit conversions
inline bit16_t::operator uint16_t() const {
    return val;
}

#endif