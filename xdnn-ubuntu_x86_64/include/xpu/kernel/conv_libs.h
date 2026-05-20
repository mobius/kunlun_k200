#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CONV_LIBS_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CONV_LIBS_H

#ifdef __xpu__

#include "xpu/kernel/cluster_header.h"
#include "xpu/kernel/math.h"
#include "xpu/kernel/simd.h"
#include "xpu/kernel/sdnn_header.h"
#include "xpu/kernel/gemm_libs.h"
#include "xpu/kernel/cluster1_type.h"

/*
 * ALL FUNCTIONS ARE STATIC TO REDUCE CODE SIZE.
 * THEY WILL BECOME INLINE WHEN USING -O2
 * */

template <typename SRC_TYPE, typename DST_TYPE>
static __device__ void dma_image_chw_hbm_to_l2(_global_ptr_ const SRC_TYPE* image_chw, float max_val, int h, int w,
        int c_start, int c_end, int h_start, int h_end) {
    int cid = core_id();
    DST_TYPE* l2dw = (DST_TYPE*)((cid >> 1) * L2DW_SIZE_PER_CORE);
    dma_matrix_hbm_to_l2<SRC_TYPE, DST_TYPE>(
            image_chw, max_val, c_start, c_end,
            h_start * w, h_end * w, h * w, l2dw);
}

template <int DST_SRAM>
static __device__ inline void win2vec_l2_to_l1_int16(
        int16_t* l2_base, int c_stride, int h, int w,
        int c_start, int c_end, int out_h_start, int out_h_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right, v16i16* dst_ptr) {

    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;

    // real work
    int out_hh = out_h_end - out_h_start;
    int in_h_start = out_h_start * stride_h - pad_up;
    int in_h_end = in_h_start + win_h + (out_hh - 1) * stride_h;
    int in_h_dma_start = max(in_h_start, 0);
    int in_h_dma_end = min(in_h_end, h);
    int read_per_channel = (in_h_dma_end - in_h_dma_start) * w;

    int cc = c_end - c_start;
    // here is c_stride * win_h * win_w, not cc * win_h * win_w
    // win2vec_l2_to_l1_int16 will be called multiple times
    int roundup_dim = roundup16(c_stride * win_h * win_w);
    int out_w = (w + pad_left + pad_right - win_w) / stride_w + 1;
    int roundup_div16_out_w = roundup_div16(out_w);
    xfence_lock(dev);
    ds_conv_cfg_channel(cc);
    ds_conv_cfg_stride(stride_w);
    ds_conv_cfg_dilation(1);
    ds_conv_cfg_datatype(DS_INT16);
    ds_conv_cfg_block_distance(read_per_channel * sizeof(int16_t));
    ds_conv_cfg_win_size(win_w, win_h);
    ds_conv_cfg_block_size(w, (in_h_dma_end - in_h_dma_start));

    // exchange order of loops, move complex codes to the outer loop
    for (int j = 0; j < out_w; j += NBANKS) {
        int nwin = min(out_w - j, NBANKS);
        int loop_w_start = j * stride_w - pad_left;
        int loop_w_end = loop_w_start + win_w + (nwin - 1) * stride_w;
        int loop_pad_left = max(-loop_w_start, 0);
        int loop_pad_right = max(loop_w_end - w, 0);
        int w2v_len = (nwin - 1) * stride_w + win_w - loop_pad_left - loop_pad_right;
        ds_conv_cfg_pad_horizontal(loop_pad_left, loop_pad_right);
        for (int i = 0; i < out_hh; i++) {
            int loop_h_start = in_h_start + i * stride_h;
            int loop_h_end = loop_h_start + win_h;
            int loop_pad_up = max(-loop_h_start, 0);
            int loop_pad_down = max(loop_h_end - h, 0);
            int16_t* ptr_l2 = l2_base + (loop_h_start + loop_pad_up - in_h_dma_start) * w
                    + (loop_w_start + loop_pad_left);
            v16i16* ptr_l1 = &dst_ptr[(roundup_div16_out_w * i + div16(j)) * roundup_dim];
            ds_conv_cfg_pad_vertical(loop_pad_up, loop_pad_down);
            ds_win2vec(ptr_l1, ptr_l2, w2v_len, DST_SRAM);
        }
    }
    // end of real work

    xfence_unlock(dev);
}

template <int DST_SRAM>
static __device__ inline void win2vec_l2_to_l1_int16(
        int16_t* l2_base, int c_stride, int h, int w,
        int c_start, int c_end, int out_h_idx,
        int out_w_start, int out_w_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right, v16i16* dst_ptr) {

    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;

    int in_h_start = out_h_idx * stride_h - pad_up;
    int in_h_end = in_h_start + win_h;
    int in_h_dma_start = max(in_h_start, 0);
    int in_h_dma_end = min(in_h_end, h);

    int out_ww = out_w_end - out_w_start;
    int in_w_start = out_w_start * stride_w - pad_left;
    int in_w_end = in_w_start + win_w + (out_ww - 1) * stride_w;
    int in_w_dma_start = max(in_w_start, 0);
    int in_w_dma_end = min(in_w_end, w);

    int cc = c_end - c_start;
    int roundup_dim = roundup16(c_stride * win_h * win_w);
    // here is c_stride * win_h * win_w, not cc * win_h * win_w
    // win2vec_l2_to_l1_int16 will be called multiple times

    xfence_lock(dev);
    ds_conv_cfg_channel(cc);
    ds_conv_cfg_stride(stride_w);
    ds_conv_cfg_dilation(1);
    ds_conv_cfg_datatype(DS_INT16);
    ds_conv_cfg_block_distance((in_h_dma_end - in_h_dma_start) * (in_w_dma_end - in_w_dma_start) * sizeof(int16_t));
    ds_conv_cfg_win_size(win_w, win_h);
    ds_conv_cfg_block_size((in_w_dma_end - in_w_dma_start), (in_h_dma_end - in_h_dma_start));

    int loop_pad_up = max(-in_h_start, 0);
    int loop_pad_down = max(in_h_end - h, 0);
    for (int j = out_w_start; j < out_w_end; j += NBANKS) {
        int nwin = min(out_w_end - j, NBANKS);
        int loop_w_start = j * stride_w - pad_left;
        int loop_w_end = loop_w_start + win_w + (nwin - 1) * stride_w;
        int loop_pad_left = max(-loop_w_start, 0);
        int loop_pad_right = max(loop_w_end - w, 0);
        int w2v_len = (nwin - 1) * stride_w + win_w - loop_pad_left - loop_pad_right;
        int16_t* ptr_l2 = l2_base + (loop_w_start + loop_pad_left - in_w_dma_start);
        v16i16* ptr_l1 = &dst_ptr[(div16(j - out_w_start)) * roundup_dim];
        ds_conv_cfg_pad_horizontal(loop_pad_left, loop_pad_right);
        ds_conv_cfg_pad_vertical(loop_pad_up, loop_pad_down);
        ds_win2vec(ptr_l1, ptr_l2, w2v_len, DST_SRAM);
    }
    // end of real work

    xfence_unlock(dev);
}
template <int DST_SRAM>
static __device__ inline void win2vec_l2_to_l1_int8(
        int8_t* l2_base, int c_stride, int h, int w,
        int c_start, int c_end, int out_h_start, int out_h_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right, v32i8* dst_ptr) {

    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;

    // real work
    int out_hh = out_h_end - out_h_start;
    int in_h_start = out_h_start * stride_h - pad_up;
    int in_h_end = in_h_start + win_h + (out_hh - 1) * stride_h;
    int in_h_dma_start = max(in_h_start, 0);
    int in_h_dma_end = min(in_h_end, h);
    int read_per_channel = (in_h_dma_end - in_h_dma_start) * w;

    int cc = c_end - c_start;
    // here is c_stride * win_h * win_w, not cc * win_h * win_w
    // win2vec_l2_to_l1_int8 will be called multiple times
    int roundup_dim = roundup16(c_stride * win_h * win_w);
    int out_w = (w + pad_left + pad_right - win_w) / stride_w + 1;
    int roundup_div32_out_w = roundup_div32(out_w);
    xfence_lock(dev);
    ds_conv_cfg_channel(cc);
    ds_conv_cfg_stride(stride_w);
    ds_conv_cfg_dilation(1);
    ds_conv_cfg_datatype(DS_INT8);
    ds_conv_cfg_block_distance(read_per_channel * sizeof(int8_t));
    ds_conv_cfg_win_size(win_w, win_h);
    ds_conv_cfg_block_size(w, (in_h_dma_end - in_h_dma_start));

    // exchange order of loops, move complex codes to the outer loop
    for (int j = 0; j < out_w; j += NBANKS) {
        int is_odd = mod2(div16(j));
        int nwin = min(out_w - j, NBANKS);
        int loop_w_start = j * stride_w - pad_left;
        int loop_w_end = loop_w_start + win_w + (nwin - 1) * stride_w;
        int loop_pad_left = max(-loop_w_start, 0);
        int loop_pad_right = max(loop_w_end - w, 0);
        ds_conv_cfg_pad_horizontal(loop_pad_left, loop_pad_right);
        int w2v_len = (nwin - 1) * stride_w + win_w - loop_pad_left - loop_pad_right;
        for (int i = 0; i < out_hh; i++) {
            int loop_h_start = in_h_start + i * stride_h;
            int loop_h_end = loop_h_start + win_h;
            int loop_pad_up = max(-loop_h_start, 0);
            int loop_pad_down = max(loop_h_end - h, 0);
            ds_conv_cfg_pad_vertical(loop_pad_up, loop_pad_down);
            int8_t* ptr_l1 = &dst_ptr[(roundup_div32_out_w * i + div32(j)) * roundup_dim][is_odd];
            int8_t* ptr_l2 = l2_base + (loop_h_start + loop_pad_up - in_h_dma_start) * w
                    + (loop_w_start + loop_pad_left);
            ds_win2vec(ptr_l1, ptr_l2, w2v_len, DST_SRAM);
        }
    }

    xfence_unlock(dev);
}

template <int DST_SRAM>
static __device__ inline void win2vec_l2_to_l1_int8(
        int8_t* l2_base, int c_stride, int h, int w,
        int c_start, int c_end, int out_h_idx,
        int out_w_start, int out_w_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right, v32i8* dst_ptr) {

    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;

    int in_h_start = out_h_idx * stride_h - pad_up;
    int in_h_end = in_h_start + win_h;
    int in_h_dma_start = max(in_h_start, 0);
    int in_h_dma_end = min(in_h_end, h);

    int out_ww = out_w_end - out_w_start;
    int in_w_start = out_w_start * stride_w - pad_left;
    int in_w_end = in_w_start + win_w + (out_ww - 1) * stride_w;
    int in_w_dma_start = max(in_w_start, 0);
    int in_w_dma_end = min(in_w_end, w);

    int cc = c_end - c_start;
    int roundup_dim = roundup16(c_stride * win_h * win_w);

    xfence_lock(dev);
    ds_conv_cfg_channel(cc);
    ds_conv_cfg_stride(stride_w);
    ds_conv_cfg_dilation(1);
    ds_conv_cfg_datatype(DS_INT8);
    ds_conv_cfg_block_distance((in_h_dma_end - in_h_dma_start) * (in_w_dma_end - in_w_dma_start));
    ds_conv_cfg_win_size(win_w, win_h);
    ds_conv_cfg_block_size((in_w_dma_end - in_w_dma_start), (in_h_dma_end - in_h_dma_start));

    int loop_pad_up = max(-in_h_start, 0);
    int loop_pad_down = max(in_h_end - h, 0);
    for (int j = out_w_start; j < out_w_end; j += NBANKS) {
        int is_odd = mod2(div16(j));
        int nwin = min(out_w_end - j, NBANKS);
        int loop_w_start = j * stride_w - pad_left;
        int loop_w_end = loop_w_start + win_w + (nwin - 1) * stride_w;
        int loop_pad_left = max(-loop_w_start, 0);
        int loop_pad_right = max(loop_w_end - w, 0);
        int w2v_len = (nwin - 1) * stride_w + win_w - loop_pad_left - loop_pad_right;
        int8_t* ptr_l2 = l2_base + (loop_w_start + loop_pad_left - in_w_dma_start);
        int8_t* ptr_l1 = &dst_ptr[(div32(j - out_w_start)) * roundup_dim][is_odd];
        ds_conv_cfg_pad_horizontal(loop_pad_left, loop_pad_right);
        ds_conv_cfg_pad_vertical(loop_pad_up, loop_pad_down);
        ds_win2vec(ptr_l1, ptr_l2, w2v_len, DST_SRAM);
    }
    // end of real work

    xfence_unlock(dev);
}

// size of l2dw is much smaller than l1d / l1w, so we need a loop here
template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void dma_win2vec_hbm_to_l1_int16(
        _global_ptr_ const SRC_TYPE* image_chw, float max_val,
        int c, int h, int w, int c_start, int c_end,
        int out_h_start, int out_h_end, int win_h, int win_w,
        int stride_h, int stride_w, int pad_up, int pad_down,
        int pad_left, int pad_right, v16i16* dst_ptr, int ncores = 1) {
    int cid = core_id();
    int16_t* l2_base = (int16_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    int out_w = (w + pad_left + pad_right - win_w) / stride_w + 1;
    // max_in_hh = win_h + (max_out_hh - 1) * stride_h
    // need to make sure:
    //      max_in_hh * w * max_cc * sizeof(int16_t) <= L2DW_SIZE_PER_CORE
    //      max_cc % 16 == 0 or max_cc == c

    int max_cc = min(c_end - c_start, 16);
    int max_in_hh = L2DW_SIZE_PER_CORE / (2 * max_cc * w);
    int max_out_hh = (max_in_hh - win_h) / stride_h + 1;
    if ((max_out_hh > out_h_end - out_h_start) && (max_cc == 16)) {
        max_out_hh = out_h_end - out_h_start;
        if (win_h < stride_h) { // not necessary to load too much rows
            max_out_hh = 1;
        }
        max_in_hh = (max_out_hh - 1) * stride_h + win_h;
        max_cc = rounddown16(L2DW_SIZE_PER_CORE / (max_in_hh * w * 2));
    }
    int blockid = -1;
    for (int loop_out_h_start = out_h_start; loop_out_h_start < out_h_end;
            loop_out_h_start += max_out_hh) {
        int loop_out_h_end = min(loop_out_h_start + max_out_hh, out_h_end);
        int loop_out_hh = loop_out_h_end - loop_out_h_start;
        int loop_in_h_start = loop_out_h_start * stride_h - pad_up;
        int loop_in_h_end = loop_in_h_start + win_h + (loop_out_hh - 1) * stride_h;
        int in_h_dma_start = max(loop_in_h_start, 0);
        int in_h_dma_end = min(loop_in_h_end, h);
        for (int loop_c_start = c_start; loop_c_start < c_end; loop_c_start += max_cc) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_c_end = min(loop_c_start + max_cc, c_end);
            dma_image_chw_hbm_to_l2<SRC_TYPE, int16_t>(
                    image_chw, max_val, h, w, loop_c_start, loop_c_end,
                    in_h_dma_start, in_h_dma_end);
            int c_offset = (loop_c_start - c_start) * win_h * win_w;
            win2vec_l2_to_l1_int16<DST_SRAM>(l2_base, c_end - c_start, h, w, loop_c_start, loop_c_end,
                    loop_out_h_start, loop_out_h_end, win_h, win_w,
                    stride_h, stride_w, pad_up, pad_down,
                    pad_left, pad_right, &dst_ptr[c_offset]);
        }
        int total_wins = max_out_hh * roundup_div16(out_w);
        dst_ptr = &dst_ptr[total_wins * roundup16((c_end - c_start) * win_h * win_w)];
    }
}

// size of l2dw is much smaller than l1d / l1w, so we need a loop here
template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void dma_win2vec_hbm_to_l1_int8(
        _global_ptr_ const SRC_TYPE* image_chw, float max_val,
        int c, int h, int w, int c_start, int c_end,
        int out_h_start, int out_h_end, int win_h, int win_w,
        int stride_h, int stride_w, int pad_up, int pad_down,
        int pad_left, int pad_right, v32i8* dst_ptr, int ncores = 1) {
    int cid = core_id();
    int8_t* l2_base = (int8_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    int out_w = (w + pad_left + pad_right - win_w) / stride_w + 1;
    // max_in_hh = win_h + (max_out_hh - 1) * stride_h
    // need to make sure:
    //      max_in_hh * w * max_cc * sizeof(int8_t) <= L2DW_SIZE_PER_CORE
    //      max_cc % 16 == 0 or max_cc == c

    int max_cc = min(c_end - c_start, 16);
    int max_in_hh = L2DW_SIZE_PER_CORE / (max_cc * w);
    int max_out_hh = (max_in_hh - win_h) / stride_h + 1;
    if ((max_out_hh > out_h_end - out_h_start) && (max_cc == 16)) {
        max_out_hh = out_h_end - out_h_start;
        if (win_h < stride_h) { // not necessary to load too much rows
            max_out_hh = 1;
        }
        max_in_hh = (max_out_hh - 1) * stride_h + win_h;
        max_cc = rounddown16(L2DW_SIZE_PER_CORE / (max_in_hh * w));
    }
    int blockid = -1;
    for (int loop_out_h_start = out_h_start; loop_out_h_start < out_h_end;
            loop_out_h_start += max_out_hh) {
        int loop_out_h_end = min(loop_out_h_start + max_out_hh, out_h_end);
        int loop_out_hh = loop_out_h_end - loop_out_h_start;
        int loop_in_h_start = loop_out_h_start * stride_h - pad_up;
        int loop_in_h_end = loop_in_h_start + win_h + (loop_out_hh - 1) * stride_h;
        int in_h_dma_start = max(loop_in_h_start, 0);
        int in_h_dma_end = min(loop_in_h_end, h);
        for (int loop_c_start = c_start; loop_c_start < c_end; loop_c_start += max_cc) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_c_end = min(loop_c_start + max_cc, c_end);
            dma_image_chw_hbm_to_l2<SRC_TYPE, int8_t>(
                    image_chw, max_val, h, w, loop_c_start, loop_c_end,
                    in_h_dma_start, in_h_dma_end);
            int c_offset = (loop_c_start - c_start) * win_h * win_w;
            win2vec_l2_to_l1_int8<DST_SRAM>(l2_base, c_end - c_start, h, w, loop_c_start, loop_c_end,
                    loop_out_h_start, loop_out_h_end, win_h, win_w,
                    stride_h, stride_w, pad_up, pad_down,
                    pad_left, pad_right, &dst_ptr[c_offset]);
        }
        int total_wins = max_out_hh * roundup_div32(out_w);
        dst_ptr = &dst_ptr[total_wins * roundup16((c_end - c_start) * win_h * win_w)];
    }
}

// sram memroy usage
//  [l1d: (out_h_end - out_h_start) * roundup16(out_w) *
//        roundup16((c_end - c_start) * win_h * win_w) * sizeof(int16_t) ]
template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void load_image_int16_chw(
        _global_ptr_ const SRC_TYPE* image_chw, float max_val, int c, int h, int w,
        int c_start, int c_end, int out_h_start, int out_h_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right,
        v16i16* dst_ptr, int ncores = 1) {
    dma_win2vec_hbm_to_l1_int16<SRC_TYPE, DST_SRAM, IN_PARALLEL>(
            image_chw, max_val, c, h, w, c_start, c_end,
            out_h_start, out_h_end, win_h, win_w, stride_h, stride_w,
            pad_up, pad_down, pad_left, pad_right, dst_ptr, ncores);
}

// sram memroy usage
//  [l1d: (out_h_end - out_h_start) * roundup32(out_w) *
//        roundup16((c_end - c_start) * win_h * win_w)]
template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void load_image_int8_chw(
        _global_ptr_ const SRC_TYPE* image_chw, float max_val, int c, int h, int w,
        int c_start, int c_end, int out_h_start, int out_h_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right,
        v32i8* dst_ptr, int ncores = 1) {
    dma_win2vec_hbm_to_l1_int8<SRC_TYPE, DST_SRAM, IN_PARALLEL>(
            image_chw, max_val, c, h, w, c_start, c_end,
            out_h_start, out_h_end, win_h, win_w, stride_h, stride_w,
            pad_up, pad_down, pad_left, pad_right, dst_ptr, ncores);
}

template <typename SRC_TYPE>
static __device__ void load_image_l1e_fp32_chw(_global_ptr_ const SRC_TYPE* image_chw, float max_val, int c, int h,
        int w,
        int c_start, int c_end, int h_start, int h_end,
        v16f32* dst_ptr) {
    dma_shuffle_hbm_to_l1_fp32(image_chw, 0.0f, c_start, c_end,
            h_start * w, h_end * w, h * w, dst_ptr);
}

template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void load_filter_int16_fchw(_global_ptr_ const SRC_TYPE* filter_fchw, float max_val,
        int f, int c, int h, int w, int f_start, int f_end,
        int c_start, int c_end, v16i16* dst_ptr, int ncores = 1) {
    int shuffle_start = c_start * h * w;
    int shuffle_end = c_end * h * w;
    dma_shuffle_hbm_to_l1_int16<SRC_TYPE, DST_SRAM, IN_PARALLEL>(
            filter_fchw, max_val, f_start, f_end,
            shuffle_start, shuffle_end, c * h * w, dst_ptr, ncores);
}

template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void load_filter_int8_fchw(_global_ptr_ const SRC_TYPE* filter_fchw, float max_val,
        int f, int c, int h, int w, int f_start, int f_end,
        int c_start, int c_end, v32i8* dst_ptr, int ncores = 1) {
    int shuffle_start = c_start * h * w;
    int shuffle_end = c_end * h * w;
    dma_shuffle_hbm_to_l1_int8<SRC_TYPE, DST_SRAM, IN_PARALLEL>(
            filter_fchw, max_val, f_start, f_end,
            shuffle_start, shuffle_end, c * h * w, dst_ptr, ncores);
}

// sram memroy usage
//  [l1d: out_hh * roundup16(out_w) * roundup16(dim) * sizeof(int16_t)]
//  [l1w: roundup16(ff) * roundup16(dim) * sizeof(int16_t)]
//  [l1e: out_hh * out_w * roundup16(ff) * sizeof(float)]
template <bool ACC>
static __device__ void conv_mac_int16_helper(float dequant,
        int out_hh, int out_w, int ff, int dim,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {

    int roundup_dim = roundup16(dim);
    int roundup_div16_ff = roundup_div16(ff);
    int roundup_div16_out_w = roundup_div16(out_w);

    xfence_lock(MAC);
    mm_cfg_stride(1);
    mm_cfg_dequant_scale(dequant);
    mm_cfg_basic_int16(dim, out_w);
    int l1e_iter = 0;
    for (int f_iter = 0; f_iter < roundup_div16_ff; f_iter += 1) {
        int l1d_iter = 0;
        v16i16* l1wptr = &l1w[f_iter * roundup_dim];
        for (int i = 0; i < out_hh; i++) {
            mm_int16_helper<ACC>(&l1e[l1e_iter], &l1d[l1d_iter], l1wptr);
            l1e_iter += out_w;
            l1d_iter += roundup_dim * roundup_div16_out_w;
        }
    }
    xfence_unlock(MAC);
}

static __device__ void conv_mac_int16(float dequant,
        int out_hh, int out_w, int ff, int dim,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {
    conv_mac_int16_helper<false>(dequant, out_hh, out_w, ff, dim, l1d, l1w, l1e);
}

static __device__ void conv_mac_acc_int16(float dequant,
        int out_hh, int out_w, int ff, int dim,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {
    conv_mac_int16_helper<true>(dequant, out_hh, out_w, ff, dim, l1d, l1w, l1e);
}
// sram memroy usage
//  [l1d: out_hh * roundup32(out_w) * roundup16(dim)]
//  [l1w: roundup32(ff) * roundup16(dim)]
//  [l1e: out_hh * out_w * roundup32(ff) * sizeof(float)]
template <bool ACC>
static __device__ void conv_mac_int8_helper(float dequant,
        int out_hh, int out_w, int ff, int dim,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {

    int roundup_dim = roundup16(dim);
    int roundup_div32_ff = roundup_div32(ff);
    int roundup_div32_out_w = roundup_div32(out_w);

    xfence_lock(MAC);
    mm_cfg_stride(1);
    mm_cfg_dequant_scale(dequant);
    int l1e_iter = 0;
    mm_cfg_basic_int8(dim, out_w, out_hh * out_w);
    for (int f_iter = 0; f_iter < roundup_div32_ff; f_iter += 1) {
        int l1d_iter = 0;
        for (int i = 0; i < out_hh; i++) {
            mm_int8_helper<ACC>(&l1e[l1e_iter], &l1d[l1d_iter],
                    &l1w[f_iter * roundup_dim]);
            l1e_iter += out_w;
            l1d_iter += roundup_dim * roundup_div32_out_w;
        }
        // skip result of filter[:, 16: 32]
        l1e_iter = l1e_iter + out_hh * out_w;
    }
    xfence_unlock(MAC);
}

static __device__ void conv_mac_int8(float dequant,
        int out_hh, int out_w, int ff, int dim,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {
    conv_mac_int8_helper<false>(dequant, out_hh, out_w, ff, dim, l1d, l1w, l1e);
}

static __device__ void conv_mac_acc_int8(float dequant,
        int out_hh, int out_w, int ff, int dim,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {
    conv_mac_int8_helper<true>(dequant, out_hh, out_w, ff, dim, l1d, l1w, l1e);
}

template <int DST_SRAM>
static __device__ void deconv_y_grad_l2_to_l1_int16(
        int f_stride, int ff, int y_hh, int y_w, v16i16* dst_ptr) {
    int cid = core_id();
    int16_t* l2_base = (int16_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(y_hh * y_w);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    for (int i = 0; i < y_hh; i++) {
        int16_t* l2_ptr = l2_base + i * y_w;
        for (int j = 0; j < y_w; j += NBANKS) {
            ds_shuffle_coa(ff, &dst_ptr[l1_iter][0], l2_ptr, DST_SRAM);
            l2_ptr += NBANKS;
            l1_iter += f_stride;
        }
    }
    // end of real work
    xfence_unlock(dev);
}

template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void deconv_y_grad_hbm_to_l1_int16(
        _global_ptr_ float* y_fhw, float max_val,
        int f, int y_h, int y_w,
        int y_h_start, int y_h_end,
        v16i16* dst_ptr, int ncores = 1) {
    // need to make sure:
    //      max_ff * out_w * max_out_hh * sizeof(int16_t) <= L2DW_SIZE_PER_CORE
    int max_ff = NBANKS;
    int f_stride = roundup16(f);
    int max_y_hh = L2DW_SIZE_PER_CORE / (2 * max_ff * y_w);
    int y_w_roundup_div16 = roundup_div16(y_w);
    int cid = core_id();
    int blockid = -1;
    for (int loop_f_start = 0; loop_f_start < f; loop_f_start += max_ff) {
        int loop_f_end = min(loop_f_start + max_ff, f);
        for (int loop_y_h_start = y_h_start; loop_y_h_start < y_h_end; loop_y_h_start += max_y_hh) {
            if (IN_PARALLEL) {
                blockid++;
                if ((blockid % ncores) != cid) {
                    continue;
                }
            }
            int loop_y_h_end = min(loop_y_h_start + max_y_hh, y_h_end);
            int loop_y_hh = loop_y_h_end - loop_y_h_start;
            v16i16* loop_dst_ptr = &dst_ptr[loop_f_start + f_stride * y_w_roundup_div16 * (loop_y_h_start - y_h_start)];
            dma_image_chw_hbm_to_l2<SRC_TYPE, int16_t>(
                    y_fhw, max_val, y_h, y_w,
                    loop_f_start, loop_f_end, loop_y_h_start, loop_y_h_end);
            deconv_y_grad_l2_to_l1_int16<DS_L1D>(
                    f_stride, loop_f_end - loop_f_start,
                    loop_y_h_end - loop_y_h_start, y_w, loop_dst_ptr);
        }
    }
}

template <int DST_SRAM>
static __device__ void deconv_filter_trans_l2_to_l1_int16(
        int f_stride, int ff, int cc, int winh, int winw, v16i16* dst_ptr) {
    // data format in L2 is [ff, cc, winh, winw]
    int cid = core_id();
    int16_t* l2_base = (int16_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);
    int16_t* l2_trans = l2_base + L2DW_SIZE_PER_CORE / 4;   // half of the space
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // step1: ff, cc, winh, winw  ->  winh, winw, ff, cc, still at L2
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(winh * winw);
    ds_cfg_l2_wb_stride(ff * cc);
    int16_t* base_ptr = l2_base;
    int16_t* trans_ptr = l2_trans;
    for (int i = 0; i < winh * winw; i += NBANKS) {
        ds_cfg_output_bank(min(winh * winw - i, NBANKS));
        ds_shuffle_coa(ff * cc, trans_ptr, base_ptr, DS_L2DW);
        base_ptr += NBANKS;
        trans_ptr += NBANKS * ff * cc;
    }

    xfence(); // add xfence since we will change destination

    // step2: write to L1
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(cc);
    ds_cfg_output_bank(NBANKS);
    int roundup16_ff = roundup16(ff);
    int l1_iter = 0;
    for (int c_iter = 0; c_iter < cc; c_iter += NBANKS) {
        int16_t* l2_ptr = l2_trans + c_iter;
        for (int i = 0; i < winh * winw; i++) {
            ds_shuffle_coa(ff, &dst_ptr[l1_iter], l2_ptr, DST_SRAM);
            l2_ptr += ff * cc;
            l1_iter += f_stride;
        }
    }
    xfence_unlock(dev);
}

template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void deconv_filter_trans_hbm_to_l1_int16(
        _global_ptr_ float* filter_fchw, float max_val,
        int f, int c, int winh, int winw, int c_start, int c_end,
        v16i16* dst_ptr, int ncores = 1) {
    int cid = core_id();
    int16_t* l2dw = (int16_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    // need to make sure:
    //      max_ff * max_cc * winh * winw * 2 * sizeof(int16_t) <= L2DW_SIZE_PER_CORE
    int max_cc = NBANKS;
    int f_stride = roundup16(f);
    int max_ff = L2DW_SIZE_PER_CORE / (4 * max_cc * winh * winw);
    int blockid = -1;
    for (int loop_c_start = c_start; loop_c_start < c_end; loop_c_start += max_cc) {
        int loop_c_end = min(c_end, loop_c_start + max_cc);
        int cc = loop_c_end - loop_c_start;
        int n_start = loop_c_start * winh * winw;
        int n_end = loop_c_end * winh * winw;
        for (int loop_f_start = 0; loop_f_start < f; loop_f_start += max_ff) {
            if (IN_PARALLEL) {
                blockid++;
                if ((blockid % ncores) != cid) {
                    continue;
                }
            }
            int loop_f_end = min(f, loop_f_start + max_ff);
            dma_matrix_hbm_to_l2<SRC_TYPE, int16_t>(
                    filter_fchw, max_val, loop_f_start,
                    loop_f_end, n_start, n_end, c * winh * winw, l2dw);
            deconv_filter_trans_l2_to_l1_int16<DST_SRAM>(f_stride, loop_f_end - loop_f_start,
                    cc, winh, winw, &dst_ptr[loop_f_start]);
        }
        dst_ptr = &dst_ptr[f_stride * winh * winw];
    }
}

// sram memroy usage
//  [l1d: out_hh * roundup16(out_w) * roundup16(f) * sizeof(int16_t)]
//  [l1w: win_h * win_w * roundup16(cc) * roundup16(f) * sizeof(int16_t)]
//  [l1e: image_hh * (image_w + pad_left + pad_right) * roundup16(cc) * sizeof(float)]
static __device__ void deconv_mac_int16(float dequant,
        int out_h_start, int out_h_end, int out_w, int f, int cc,
        int image_h_start, int image_h_end, int image_w,
        int win_h, int win_w,
        int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {
    // assert (cc <= 16)
    int roundup16_f = roundup16(f);
    int out_hh = out_h_end - out_h_start;
    int in_h_start = out_h_start * stride_h - pad_up; // in_h_start <= image_h_start
    int in_h_end = in_h_start + win_h + (out_hh - 1) * stride_h; //in_h_end >= image_h_end

    // first clear old data in L1E
    xfence_lock(EW);
    ew_cfg_activation_type(EW_NOACT);
    ew_cfg_coeff_scalar(0.0f, 0.0f, 0.0f);
    ew_cfg_stream_size((image_h_end - image_h_start) * (image_w + pad_left + pad_right));
    ew_cfg_col_size(NBANKS);
    ew_dsmadd(l1e, l1e, l1e, EW_L1E);
    xfence_unlock(EW);

    xfence_lock(MAC);
    mm_cfg_stride(stride_w);
    mm_cfg_dequant_scale(dequant);
    for (int i = 0; i < out_hh; i++) {
        for (int j = 0; j < out_w; j += NBANKS) {
            mm_cfg_basic_int16(f, min(NBANKS, out_w - j));
            int index_in_win = 0;
            for (int win_h_iter = 0; win_h_iter < win_h; win_h_iter++) {
                for (int win_w_iter = 0; win_w_iter < win_w; win_w_iter++) {
                    int l1e_row = (in_h_start + i * stride_h + win_h_iter - image_h_start);
                    int total_col = (pad_left + pad_right + image_w);
                    int l1e_col = j * stride_w + win_w_iter;
                    if (l1e_row >= 0 && (l1e_row < (image_h_end - image_h_start))) {
                        mm_acc_int16(&l1e[l1e_row * total_col + l1e_col], l1d, &l1w[index_in_win * roundup16_f]);
                    }
                    index_in_win++;
                }
            }
            l1d = &l1d[roundup16_f];
        }
    }
    xfence_unlock(MAC);
}

static __device__ void deconv_store_image_l1e_chw(v16f32* l1e, _global_ptr_ float* image_grad, int image_h, int image_w,
        int c_start, int c_end, int image_h_start, int image_h_end,
        int pad_left, int pad_right) {
    // assert(c_end - c_start <= 16)
    if (pad_left + pad_right > 0) {
        xfence_lock(EW);
        ew_cfg_activation_type(EW_NOACT);
        ew_cfg_coeff_scalar(1.0f, 0.0f, 0.0f);
        ew_cfg_stream_size(image_w);
        ew_cfg_col_size(NBANKS);
        int pad_image_w = image_w + pad_left + pad_right;
        for (int i = 0; i < image_h_end - image_h_start; i++) {
            ew_dsmadd(&l1e[i * image_w][0], &l1e[i * pad_image_w + pad_left][0],
                    &l1e[i * image_w][0], EW_L1E);
        }
        xfence_unlock(EW);
    }
    store_matrix_l1e_trans(l1e, image_grad, c_start, c_end,
            image_h_start * image_w, image_h_end * image_w, image_h * image_w);
}

template <int DST_SRAM>
static __device__ void win2vec_trans_l2_to_l1_int16(
        int out_h_stride, int h, int w,
        int c_start, int c_end, int out_h_start, int out_h_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right, v16i16* dst_ptr) {

    int cid = core_id();
    int16_t* l2_base = (int16_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    int out_hh = out_h_end - out_h_start;
    int in_h_start = out_h_start * stride_h - pad_up;
    int in_h_end = in_h_start + win_h + (out_hh - 1) * stride_h;
    int in_h_dma_start = max(in_h_start, 0);
    int in_h_dma_end = min(in_h_end, h);
    int read_per_channel = (in_h_dma_end - in_h_dma_start) * w;

    int cc = c_end - c_start;
    int out_w = (w + pad_left + pad_right - win_w) / stride_w + 1;
    ds_conv_cfg_channel(cc);
    ds_conv_cfg_stride(stride_w);
    ds_conv_cfg_dilation(1);
    ds_conv_cfg_datatype(DS_INT16);
    ds_conv_cfg_block_distance(read_per_channel * sizeof(int16_t));
    ds_conv_cfg_win_size(win_w, win_h);
    ds_conv_cfg_block_size(w, (in_h_dma_end - in_h_dma_start));

    ds_cfg_l2_wb_stride(cc * win_w * win_h);
    int16_t* l2_middle = l2_base + read_per_channel * cc;
    int16_t* ptr_l2_wb = l2_middle;
    for (int i = 0; i < out_hh; i++) {
        int loop_h_start = in_h_start + i * stride_h;
        int loop_h_end = loop_h_start + win_h;
        int loop_pad_up = max(-loop_h_start, 0);
        int loop_pad_down = max(loop_h_end - h, 0);
        ds_conv_cfg_pad_vertical(loop_pad_up, loop_pad_down);
        int16_t* ptr_l2 = l2_base + (loop_h_start + loop_pad_up - in_h_dma_start) * w;
        for (int j = 0; j < out_w; j += NBANKS) {
            int nwin = min(out_w - j, NBANKS);
            int loop_w_start = j * stride_w - pad_left;
            int loop_w_end = loop_w_start + win_w + (nwin - 1) * stride_w;
            int loop_pad_left = max(-loop_w_start, 0);
            int loop_pad_right = max(loop_w_end - w, 0);
            ds_cfg_output_bank(nwin);
            ds_conv_cfg_pad_horizontal(loop_pad_left, loop_pad_right);
            int w2v_len = (nwin - 1) * stride_w + win_w - loop_pad_left - loop_pad_right;
            ds_win2vec(ptr_l2_wb, ptr_l2 + (loop_w_start + loop_pad_left), w2v_len, DS_L2DW);
            ptr_l2_wb += nwin * cc * win_w * win_h;
        }
    }
    // make sure write-back finish
    xfence();

    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(cc * win_w * win_h);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    int16_t* l2_ptr = l2_middle;
    for (int i = 0; i < cc * win_w * win_h; i += NBANKS) {
        ds_shuffle_coa((out_h_end - out_h_start) * out_w,
                &dst_ptr[l1_iter][0], l2_ptr, DST_SRAM);
        l2_ptr += NBANKS;
        l1_iter += roundup16(out_h_stride * out_w);
    }

    xfence_unlock(dev);
}

// size of l2dw is much smaller than l1d / l1w, so we need a loop here
template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void win2vec_trans_hbm_to_l1_int16(
        _global_ptr_ float* image_chw, float max_val, int c, int h, int w,
        int c_start, int c_end, int partition_cc, int out_h_start, int out_h_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right, v16i16* dst_ptr, int ncores = 1) {
    int out_w = (w + pad_left + pad_right - win_w) / stride_w + 1;
    // max_in_hh = win_h + (max_out_hh - 1) * stride_h
    // need to make sure:
    //      max_in_hh * w * partition_cc * sizeof(int16_t) + \
    //      max_out_hh * out_w * partition_cc * win_h * win_w * sizeof(int16_t)   <= L2DW_SIZE_PER_CORE
    //      ==>     partition_cc * w * (win_h + (max_out_hh - 1) * stride_h) +
    //              partition_cc * out_w * win_h * win_w * max_out_hh <= L2DW_SIZE_PER_CORE / 2
    //      ==>     partition_cc * (w * (win_h - stride_h) + (w * stride_h + out_w * win_h * win_w) * max_out_hh) <= L2DW_SIZE_PER_CORE / 2

    //int max_cc = min(c_end - c_start, 16);
    int max_out_hh = L2DW_SIZE_PER_CORE / 2 / partition_cc - (win_h - stride_h) * w;
    max_out_hh = max_out_hh / (w * stride_h + out_w * win_h * win_w);
    int max_in_hh = win_h + (max_out_hh - 1) * stride_h;
    int cid = core_id();
    int blockid = -1;
    for (int loop_c_start = c_start; loop_c_start < c_end; loop_c_start += partition_cc) {
        int loop_c_end = min(loop_c_start + partition_cc, c_end);
        for (int loop_out_h_start = out_h_start; loop_out_h_start < out_h_end;
                loop_out_h_start += max_out_hh) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_out_h_end = min(loop_out_h_start + max_out_hh, out_h_end);
            int loop_out_hh = loop_out_h_end - loop_out_h_start;
            int loop_in_h_start = loop_out_h_start * stride_h - pad_up;
            int loop_in_h_end = loop_in_h_start + win_h + (loop_out_hh - 1) * stride_h;
            int in_h_dma_start = max(loop_in_h_start, 0);
            int in_h_dma_end = min(loop_in_h_end, h);
            dma_image_chw_hbm_to_l2<SRC_TYPE, int16_t>(
                    image_chw, max_val, h, w, loop_c_start, loop_c_end,
                    in_h_dma_start, in_h_dma_end);
            int l1_offset = (loop_out_h_start - out_h_start) * out_w;
            win2vec_trans_l2_to_l1_int16<DST_SRAM>(
                    out_h_end - out_h_start, h, w, loop_c_start, loop_c_end,
                    loop_out_h_start, loop_out_h_end, win_h, win_w,
                    stride_h, stride_w, pad_up, pad_down,
                    pad_left, pad_right, &dst_ptr[l1_offset]);
        }
        int skip_rows = roundup_div16(partition_cc * win_h * win_w) * roundup16((out_h_end - out_h_start) * out_w);
        dst_ptr = &dst_ptr[skip_rows];
    }
}

#endif
#endif
