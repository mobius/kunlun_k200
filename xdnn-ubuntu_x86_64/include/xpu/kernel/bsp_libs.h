#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_BSP_LIBS_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_BSP_LIBS_H

#ifdef __xpu__
#include "xpu/kernel/gemm_libs_impl.h"
#include "xpu/kernel/conv_libs.h"

template <typename SRC_TYPE, int DST_SRAM, bool TRANS, int cid>
static __device__ void load_matrix_int16_bsp(_global_ptr_ const SRC_TYPE* matrix, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ld, int16_t* l2dw, v16i16* dst_ptr) {
    if (cid >= 4) {
        return;
    }
    if (((DST_SRAM == DS_L1D) && (!TRANS)) || ((DST_SRAM == DS_L1W) && (TRANS))) {
        int m_mid = m_start + roundup32(m_end - m_start) / 2;
        int l1_row_stride = roundup16(n_end - n_start);
        if ((cid & 1) == 0) {
            m_end = min(m_mid, m_end);
        } else {
            dst_ptr = &dst_ptr[(m_mid - m_start) / 16 * l1_row_stride];
            m_start = m_mid;
        }
        if (m_end <= m_start) { // this core doesn't have work
            return;
        }
        if (cid < 2) {
            dma_matrix_hbm_to_l2<SRC_TYPE, int16_t>(matrix, max_val, m_start, m_end, n_start, n_end, ld, l2dw);
        } else {
            int l1_ncols = m_end - m_start;
            int l1_nrows = n_end - n_start;
            shuffle_l2_to_l1_int16<DST_SRAM>(l1_nrows, l1_ncols, l1_row_stride, l2dw, dst_ptr);
        }
    }
    if (((DST_SRAM == DS_L1D) && (TRANS)) || ((DST_SRAM == DS_L1W) && (!TRANS))) {
        int m_mid = m_start + (m_end - m_start) / 2;
        int l1_row_stride = roundup16(m_end - m_start);
        if ((cid & 1) == 0) {
            m_end = min(m_mid, m_end);
        } else {
            dst_ptr = &dst_ptr[m_mid - m_start];
            m_start = m_mid;
        }
        if (m_end <= m_start) { // this core doesn't have work
            return;
        }
        if (cid < 2) {
            dma_matrix_hbm_to_l2<SRC_TYPE, int16_t>(matrix, max_val, m_start, m_end, n_start, n_end, ld, l2dw);
        } else {
            int l1_ncols = n_end - n_start;
            int l1_nrows = m_end - m_start;
            shuffle_coa_l2_to_l1_int16<DST_SRAM>(l1_nrows, l1_ncols, l1_row_stride, l2dw, dst_ptr);
        }
    }
}

template <typename SRC_TYPE, int DST_SRAM, bool TRANS, int cid>
static __device__ void load_matrix_int8_bsp(_global_ptr_ const SRC_TYPE* matrix, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ld, int8_t* l2dw, v32i8* dst_ptr) {
    if (cid >= 4) {
        return;
    }
    if (((DST_SRAM == DS_L1D) && (!TRANS)) || ((DST_SRAM == DS_L1W) && (TRANS))) {
        int m_mid = m_start + roundup64(m_end - m_start) / 2;
        int l1_row_stride = roundup16(n_end - n_start);
        if ((cid & 1) == 0) {
            m_end = min(m_mid, m_end);
        } else {
            dst_ptr = &dst_ptr[(m_mid - m_start) / 32 * l1_row_stride];
            m_start = m_mid;
        }
        if (m_end <= m_start) { // this core doesn't have work
            return;
        }
        if (cid < 2) {
            dma_matrix_hbm_to_l2<SRC_TYPE, int8_t>(matrix, max_val, m_start, m_end, n_start, n_end, ld, l2dw);
        } else {
            int l1_ncols = m_end - m_start;
            int l1_nrows = n_end - n_start;
            shuffle_l2_to_l1_int8<DST_SRAM>(l1_nrows, l1_ncols, l1_row_stride, l2dw, dst_ptr);
        }
    }
    if (((DST_SRAM == DS_L1D) && (TRANS)) || ((DST_SRAM == DS_L1W) && (!TRANS))) {
        int m_mid = m_start + (m_end - m_start) / 2;
        int l1_row_stride = roundup16(m_end - m_start);
        if ((cid & 1) == 0) {
            m_end = min(m_mid, m_end);
        } else {
            dst_ptr = &dst_ptr[m_mid - m_start];
            m_start = m_mid;
        }
        if (m_end <= m_start) { // this core doesn't have work
            return;
        }
        if (cid < 2) {
            dma_matrix_hbm_to_l2<SRC_TYPE, int8_t>(matrix, max_val, m_start, m_end, n_start, n_end, ld, l2dw);
        } else {
            int l1_ncols = n_end - n_start;
            int l1_nrows = m_end - m_start;
            shuffle_coa_l2_to_l1_int8<DST_SRAM>(l1_nrows, l1_ncols, l1_row_stride, l2dw, dst_ptr);
        }
    }
}
template <typename SRC_TYPE, int DST_SRAM, bool TRANS, int cid>
static __device__ void load_matrix_fp32_bsp(_global_ptr_ const SRC_TYPE* matrix,
        int m_start, int m_end, int n_start, int n_end, int ld, float* l2dw, v16f32* dst_ptr) {
    if (cid >= 4) {
        return;
    }
    if ((DST_SRAM == DS_L1E) && TRANS) {
        int m_mid = m_start + roundup32(m_end - m_start) / 2;
        int l1_row_stride = n_end - n_start;
        if ((cid & 1) == 0) {
            m_end = min(m_mid, m_end);
        } else {
            dst_ptr = &dst_ptr[(m_mid - m_start) / 16 * l1_row_stride];
            m_start = m_mid;
        }
        if (m_end <= m_start) { // this core doesn't have work
            return;
        }
        if (cid < 2) {
            dma_matrix_hbm_to_l2<SRC_TYPE, float>(matrix, 0.0f, m_start, m_end, n_start, n_end, ld, l2dw);
        } else {
            int l1_ncols = m_end - m_start;
            int l1_nrows = n_end - n_start;
            shuffle_l2_to_l1_fp32(l1_ncols, l1_nrows, l2dw, dst_ptr);
        }
    }
    if ((DST_SRAM == DS_L1E) && (!TRANS)) {
        int m_mid = m_start + roundup2(m_end - m_start) / 2;
        int l1_row_stride = m_end - m_start;
        if ((cid & 1) == 0) {
            m_end = min(m_mid, m_end);
        } else {
            dst_ptr = &dst_ptr[m_mid - m_start];
            m_start = m_mid;
        }
        if (m_end <= m_start) { // this core doesn't have work
            return;
        }
        if (cid < 2) {
            dma_matrix_hbm_to_l2<SRC_TYPE, float>(matrix, 0.0f, m_start, m_end, n_start, n_end, ld, l2dw);
        } else {
            int l1_ncols = n_end - n_start;
            int l1_nrows = m_end - m_start;
            shuffle_coa_l2_to_l1_fp32(l1_nrows, l1_ncols, l1_row_stride, l2dw, dst_ptr);
        }
    }
}
template <typename SRC_TYPE, int DST_SRAM, int cid>
static __device__ void load_image_int16_chw_bsp(
        _global_ptr_ const SRC_TYPE* image_chw, float max_val, int c, int h, int w,
        int c_start, int c_end, int out_h_start, int out_h_end,
        int win_h, int win_w, int stride_h, int stride_w,
        int pad_up, int pad_down, int pad_left, int pad_right,
        int16_t* l2dw, v16i16* dst_ptr, int c_stride) {
    if (cid >= 4) {
        return;
    }
    int c_mid = c_start + (c_end - c_start) / 2;
    if ((cid & 1) == 0) {
        c_end = min(c_mid, c_end);
    } else {
        dst_ptr = &dst_ptr[(c_mid - c_start) * win_h * win_w];
        c_start = c_mid;
    }
    if (c_end <= c_start) { // this core doesn't have work
        return;
    }
    int out_hh = out_h_end - out_h_start;
    int in_h_start = out_h_start * stride_h - pad_up;
    int in_h_end = in_h_start + win_h + (out_hh - 1) * stride_h;
    int in_h_dma_start = max(in_h_start, 0);
    int in_h_dma_end = min(in_h_end, h);
    if (cid < 2) {
        dma_matrix_hbm_to_l2<SRC_TYPE, int16_t>(image_chw, max_val, c_start, c_end,
                in_h_dma_start * w, in_h_dma_end * w, h * w, l2dw);
    } else {
        win2vec_l2_to_l1_int16<DST_SRAM>(l2dw, c_stride, h, w, c_start, c_end, out_h_start, out_h_end,
                win_h, win_w, stride_h, stride_w, pad_up, pad_down, pad_left, pad_right, dst_ptr);
    }
}

template <typename SRC_TYPE, int DST_SRAM, int cid>
static __device__ void load_image_int16_chw_bsp(
        _global_ptr_ const SRC_TYPE* image_chw, float max_val, int c, int h, int w,
        int c_start, int c_end, int out_h_idx, int out_w_start, int out_w_end,
        int win_h, int win_w, int stride_h, int stride_w, int pad_up, int pad_down, int pad_left, int pad_right,
        int16_t* l2dw, v16i16* dst_ptr) {
    if (cid >= 4) {
        return;
    }
    int c_mid = c_start + (c_end - c_start) / 2;
    int c_stride = c_end - c_start;
    if ((cid & 1) == 0) {
        c_end = min(c_mid, c_end);
    } else {
        dst_ptr = &dst_ptr[(c_mid - c_start) * win_h * win_w];
        c_start = c_mid;
    }
    if (c_end <= c_start) { // this core doesn't have work
        return;
    }
    int in_h_start = out_h_idx * stride_h - pad_up;
    int in_h_end = in_h_start + win_h;
    int in_h_dma_start = max(in_h_start, 0);
    int in_h_dma_end = min(in_h_end, h);

    int out_ww = out_w_end - out_w_start;
    int in_w_start = out_w_start * stride_w - pad_left;
    int in_w_end = in_w_start + win_w + (out_ww - 1) * stride_w;
    int in_w_dma_start = max(in_w_start, 0);
    int in_w_dma_end = min(in_w_end, w);
    if (cid < 2) {
        if (in_w_dma_end - in_w_dma_start == w) {
            dma_matrix_hbm_to_l2<SRC_TYPE, int16_t>(image_chw, max_val, c_start, c_end,
                    in_h_dma_start * w, in_h_dma_end * w, h * w, l2dw);
        } else {
            dma_matrix_hbm_to_l2_3d<SRC_TYPE, int16_t>(image_chw, max_val, c_start, c_end, in_h_dma_start, in_h_dma_end,
                    in_w_dma_start, in_w_dma_end, h, w, l2dw);
        }
    } else {
        win2vec_l2_to_l1_int16<DST_SRAM>(l2dw, c_stride, h, w, c_start, c_end, out_h_idx, out_w_start,
                out_w_end, win_h, win_w, stride_h, stride_w, pad_up, pad_down, pad_left, pad_right, dst_ptr);
    }
}

template <typename SRC_TYPE, int DST_SRAM, int cid>
static __device__ void load_image_int8_chw_bsp(
        _global_ptr_ const SRC_TYPE* image_chw, float max_val, int c, int h, int w,
        int c_start, int c_end, int out_h_idx, int out_w_start, int out_w_end,
        int win_h, int win_w, int stride_h, int stride_w, int pad_up, int pad_down, int pad_left, int pad_right,
        int8_t* l2dw, v32i8* dst_ptr) {
    if (cid >= 4) {
        return;
    }
    int c_mid = c_start + (c_end - c_start) / 2;
    int c_stride = c_end - c_start;
    if ((cid & 1) == 0) {
        c_end = min(c_mid, c_end);
    } else {
        dst_ptr = &dst_ptr[(c_mid - c_start) * win_h * win_w];
        c_start = c_mid;
    }
    if (c_end <= c_start) { // this core doesn't have work
        return;
    }
    int in_h_start = out_h_idx * stride_h - pad_up;
    int in_h_end = in_h_start + win_h;
    int in_h_dma_start = max(in_h_start, 0);
    int in_h_dma_end = min(in_h_end, h);

    int out_ww = out_w_end - out_w_start;
    int in_w_start = out_w_start * stride_w - pad_left;
    int in_w_end = in_w_start + win_w + (out_ww - 1) * stride_w;
    int in_w_dma_start = max(in_w_start, 0);
    int in_w_dma_end = min(in_w_end, w);
    if (cid < 2) {
        if (in_w_dma_end - in_w_dma_start == w) {
            dma_matrix_hbm_to_l2<SRC_TYPE, int8_t>(image_chw, max_val, c_start, c_end,
                    in_h_dma_start * w, in_h_dma_end * w, h * w, l2dw);
        } else {
            dma_matrix_hbm_to_l2_3d<SRC_TYPE, int8_t>(image_chw, max_val, c_start, c_end, in_h_dma_start, in_h_dma_end,
                    in_w_dma_start, in_w_dma_end, h, w, l2dw);
        }
    } else {
        win2vec_l2_to_l1_int8<DST_SRAM>(l2dw, c_stride, h, w, c_start, c_end, out_h_idx, out_w_start,
                out_w_end, win_h, win_w, stride_h, stride_w, pad_up, pad_down, pad_left, pad_right, dst_ptr);
    }
}

template <typename DST_TYPE, int cid>
static __device__ float store_matrix_l1e_trans_bsp(v16f32* l1e, v16f32* l1e_bias, v16f32* l2e, v16f32* l2r,
        _global_ptr_ DST_TYPE* matrix_nm, int n_start, int n_end, int m_start, int m_end, int ldm,
        EW_ACTIVATION_TYPE act_type = EW_NOACT, float matrix_max = 0.0f) {
    int mm = m_end - m_start;
    int nn = n_end - n_start;
    if (cid == 5) {
        if (l1e_bias == (v16f32*)(0)) {
            float ret = ew_findmax_l1e_to_l2e(mm, nn, l1e, l2e, act_type);
            return ret;
        } else {
            float ret = ew_findmax_l1e_to_l2e_with_bias(mm, nn, l1e, &l1e_bias[n_start / NBANKS], l2e, act_type);
            return ret;
        }
    }
    if (cid == 6) {
        rscol_l2e_to_l2r(mm, nn, l2e, l2r);
    }
    if (cid == 7) {
        dmaout_l2r_to_hbm(l2r, matrix_nm, n_start, n_end, m_start, m_end, ldm, matrix_max);
    }
    return 0.0f;
}

template <typename DST_TYPE, int cid>
static __device__ float store_matrix_l1e_notrans_bsp(v16f32* l1e, v16f32* l1e_bias, v16f32* l2e, v16f32* l2r,
        _global_ptr_ DST_TYPE* matrix_mn, int m_start, int m_end, int n_start, int n_end, int ldn,
        EW_ACTIVATION_TYPE act_type = EW_NOACT, float matrix_max = 0.0f) {
    int mm = m_end - m_start;
    int nn = n_end - n_start;
    if (cid == 5) {
        if (l1e_bias == (v16f32*)(0)) {
            float ret = ew_findmax_l1e_to_l2e(mm, nn, l1e, l2e, act_type);
            return ret;
        } else {
            float ret = ew_findmax_l1e_to_l2e_with_bias(mm, nn, l1e, &l1e_bias[n_start / NBANKS], l2e, act_type);
            return ret;
        }
    }
    if (cid == 6) {
        rsrow_l2e_to_l2r(mm, nn, l2e, l2r);
    }
    if (cid == 7) {
        dmaout_l2r_to_hbm(l2r, matrix_mn, m_start, m_end, n_start, n_end, ldn, matrix_max);
    }
    return 0.0f;
}

#endif
#endif
