#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_BINARY_OP_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_BINARY_OP_H
#include "xpu/xdnn.h"
#include "xpu/refactor/impl/xdnn_impl.h"
#include "xpu/refactor/gtest/xdnn_gtest.h"
#include "xpu/refactor/gtest/tensor.h"
#include "xpu/refactor/util/bitmask.h"
#include <functional>

static int
compare_in_bits(api::Context* ctx,
                unsigned int* cpu,
                unsigned int* xpu,
                int64_t len,
                float threashold,
                int64_t tail_align,
                unsigned int tail_mask,
                bool print_detail = true,
                int n = -1,
                int c = -1,
                int h = -1,
                int w = -1,
                const api::Tensor& t0 = api::Tensor(),
                const api::Tensor& t1 = api::Tensor(),
                std::function<void(uint64_t, unsigned int, api::Tensor, api::Tensor)> print_diff_val = nullptr) {
    api::Tensor x;
    api::Tensor y;
    bool enable_val_print = (n != -1);
    if (enable_val_print) {
        x = t0.to(api::kCPU).astype(api::kFLOAT32);
        y = t1.to(api::kCPU).astype(api::kFLOAT32);
    }
    int64_t hw = h * w;
    int64_t chw = hw * c;
    int64_t threashold_cnt = len * threashold * 32;
    threashold_cnt = threashold_cnt < 5 ? 5 : threashold_cnt;
    std::vector<unsigned int> xpu_result_v(len, 0);
    unsigned int* xpu_result = xpu_result_v.data();
    int ret = api::do_device2host(ctx, xpu, xpu_result, len * sizeof(unsigned int));
    if (ret) {
        printf("xpu_memcpy err, cpu addr: 0x%016llx, xpu: 0x%016llx, xpu_memcpy ret: %d\n",
                (unsigned long long)xpu_result, (unsigned long long)xpu, ret);
        return 0;
    }
    int64_t cnt = 0;
    int err_left = 10;
    for (int64_t i = 0; i < len; ++i) {
        unsigned int compare_result = 0;
        unsigned int masked_cpu = cpu[i];
        unsigned int masked_xpu = xpu_result[i];
        if ((i + 1) % tail_align == 0) {
            masked_cpu &= tail_mask;
            masked_xpu &= tail_mask;
        }
        compare_result = masked_cpu ^ masked_xpu;

        bool perfect = true;
        int bit_iter = 0;

        while (compare_result != 0) {
            unsigned int last_bit = compare_result & 0x1;
            if (last_bit != 0 && enable_val_print && err_left > 0) {
                print_diff_val(i, bit_iter, x, y);
            }
            cnt += last_bit;
            compare_result >>= 1;
            perfect = false;
            bit_iter++;
        }
        if (!perfect && print_detail) {
            if (err_left-- > 0) {
               printf("bit mismatch in data[%ld], cpu: 0x%x, xpu: 0x%x, ismask: %d, mask: 0x%x\n", i, cpu[i],
                       xpu_result[i], (i + 1) % tail_align == 0, tail_mask);
            }
        }
    }
    if (cnt != 0) {
        printf("err bits num: %ld, threashold_cnt: %ld, len: %ld, threashold: %f\n", cnt, threashold_cnt, len,
                threashold);
    } else {
        printf("bitwise identical.\n");
    }
    if (cnt > threashold_cnt) {
        return 1;
    } else {
        return 0;
    }
}

static int check_bn_mask_diff(api::Context* ctx, unsigned int* cpu, unsigned int* xpu, float threashold, int64_t n, int64_t c,
        int64_t h, int64_t w, bool is_nchw, bool print_detail = true) {
    unsigned int tail_mask = -1;
    int64_t align_width;
    int64_t align_space = 0, tail_align = 0;
    if (!is_nchw) {
        align_width = c;
    } else {
        align_width = n * h * w;
    }
    tail_align = (align_width + 31) / 32;
    align_space = align_width % 32;
    if (align_space != 0) {
        tail_mask <<= align_space;
        tail_mask = ~tail_mask;
    }
    int64_t len = get_bitmask_len(n, c, h, w, is_nchw);
    return compare_in_bits(ctx, cpu, xpu, len, threashold, tail_align, tail_mask, print_detail);
}

static int check_bn_mask_diff_val(api::Context* ctx, unsigned int* cpu, unsigned int* xpu, float threashold, int64_t n,
        int64_t c, int64_t h, int64_t w, bool is_nchw, bool print_detail, const api::Tensor& t0,
        const api::Tensor& t1) {
    unsigned int tail_mask = -1;
    int64_t align_width;
    int64_t align_space = 0, tail_align = 0;
    if (!is_nchw) {
        align_width = c;
    } else {
        align_width = n * h * w;
    }
    tail_align = (align_width + 31) / 32;
    align_space = align_width % 32;
    if (align_space != 0) {
        tail_mask <<= align_space;
        tail_mask = ~tail_mask;
    }
    int64_t len = get_bitmask_len(n, c, h, w, is_nchw);
    std::function<void(uint64_t, unsigned int, api::Tensor, api::Tensor)> print_diff_val = [&](unsigned long idx,
                                                                                               unsigned int biter,
                                                                                               api::Tensor x,
                                                                                               api::Tensor y) {
        int64_t global_idx;
        if (is_nchw) {
            int64_t hw = h * w;
            int64_t chw = c * h * w;
            int64_t bm_hw = (n * hw + 31) / 32;
            int64_t c_iter = idx / bm_hw;
            int64_t nhw_iter = (idx % bm_hw) * 32 + biter;
            int64_t n_iter = nhw_iter / hw;
            int64_t hw_iter = nhw_iter % hw;
            global_idx = n_iter * chw + c_iter * hw + hw_iter;
        } else {
            int64_t bm_hw = (c + 31) / 32;
            int64_t nhw_idx = idx / bm_hw;
            int64_t idx_in_c = (idx % bm_hw) * 32 + biter;
            global_idx = nhw_idx * c + idx_in_c;
        }
        printf("[mismatch value] [%ld => %d] cpu[%ld]:%f, xpu:%f\n",
               idx,
               biter,
               global_idx,
               x.data<float>()[global_idx],
               y.data<float>()[global_idx]);
    };
    return compare_in_bits(
            ctx, cpu, xpu, len, threashold, tail_align, tail_mask, print_detail, n, c, h, w, t0, t1, print_diff_val);
}

#endif