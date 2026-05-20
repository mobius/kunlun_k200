#ifndef BAIDU_XPU_API_INCLUDE_XPU_UTIL_BITMASK_H
#define BAIDU_XPU_API_INCLUDE_XPU_UTIL_BITMASK_H
#include "xpu/xdnn.h"

static int64_t get_bitmask_len(int64_t n, int64_t c, int64_t h, int64_t w, bool is_nchw) {
    if (is_nchw) {
        return roundup32(n * h * w) / 32 * c;
    }

    int64_t total_len = 0;
    total_len = n * w * h * ((c + 31) / 32);
    return total_len;
}

template <typename T>
void gen_bitmask(int64_t n, int64_t c, int64_t h, int64_t w, T* y0ptr, uint32_t* bitmask, bool is_nchw) {
    if (is_nchw) {
        int64_t len = get_bitmask_len(n, c, h, w, is_nchw);
        for (int64_t i = 0; i < len; ++i) {
            bitmask[i] = 0;
        }

        for (int64_t c_iter = 0; c_iter < c; c_iter++) {
            for (int64_t p = 0; p < n * h * w; p++) {
                int64_t n_iter = p / (h * w);
                int64_t id_in_img = n_iter * (c * h * w) + c_iter * (h * w) + p % (h * w);
                int64_t bitmask_nhw = roundup32(n * h * w) / 32;
                int64_t id_in_bitmask = c_iter * bitmask_nhw + p / 32;
                int64_t id_in_mask = p % 32;

                if (y0ptr[id_in_img] > 0) {
                    uint32_t temp = 1;
                    temp <<= id_in_mask;
                    bitmask[id_in_bitmask] |= temp;
                }
            }
        }
        return;
    } else {

        // new version
        int64_t mask_len = 0;
        int64_t loop_i_len = 0;
        int64_t loop_j_len = 0;
        int64_t line_len = 0;
        mask_len = (c + 31) / 32;
        loop_i_len = n * h * w;
        loop_j_len = c;
        line_len = c;

        for (int64_t i = 0; i < loop_i_len * mask_len; ++i) {
            bitmask[i] = 0;
        }

        for (int64_t i = 0; i < loop_i_len; ++i) {
            for (int64_t j = 0; j < loop_j_len; ++j) {
                if (y0ptr[i * line_len + j] > 0) {
                    int64_t mask_idx = i * mask_len + j / 32;
                    int64_t bit_idx = j % 32;
                    uint32_t temp = 1;
                    temp <<= bit_idx;
                    bitmask[mask_idx] |= temp;
                }
            }
        }
    }
}

#endif