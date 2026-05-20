#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_GEMM_LIBS_IMPL_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_GEMM_LIBS_IMPL_H

#ifdef __xpu__

#include "xpu/kernel/cluster_header.h"
#include "xpu/kernel/math.h"
#include "xpu/kernel/sdnn_header.h"
#include "xpu/kernel/debug.h"
/*
 * ALL FUNCTIONS ARE STATIC TO REDUCE CODE SIZE.
 * THEY WILL BECOME INLINE WHEN USING -O2
 * */

static __device__ float merge_max_val(float* x_lm, int len) {
    // clear last few elements
    for (int i = len; i < roundup8(len); ++i) {
        x_lm[i] = x_lm[0];
    }
    float max_val0 = x_lm[0];
    float max_val1 = x_lm[1];
    float max_val2 = x_lm[2];
    float max_val3 = x_lm[3];
    for (int i = 4; i < len; i += 4) {
        max_val0 = fmax(max_val0, x_lm[i]);
        max_val1 = fmax(max_val1, x_lm[i + 1]);
        max_val2 = fmax(max_val2, x_lm[i + 2]);
        max_val3 = fmax(max_val3, x_lm[i + 3]);
    }
    max_val0 = fmax(max_val0, max_val1);
    max_val2 = fmax(max_val2, max_val3);
    max_val0 = fmax(max_val0, max_val2);
    return max_val0;
}

static __device__ float get_max_val(const float* max_lm_ptr) {
    float max_val0 = fmax(max_lm_ptr[0], max_lm_ptr[1]);
    float max_val1 = fmax(max_lm_ptr[2], max_lm_ptr[3]);
    return fmax(max_val0, max_val1);
}

static __device__ void store_res_max(_global_ptr_ float* res_max_gm_ptr, float res_max) {
    if (res_max_gm_ptr != nullptr) {
        __local__ float res_max_lm_ptr[1];
        res_max_lm_ptr[0] = res_max;
        LM2GM(res_max_lm_ptr, res_max_gm_ptr + cluster_id(), sizeof(float));
    }
}

// 1d dma_in
template <typename SRC_TYPE, typename DST_TYPE>
static __device__ void dma_hbm_to_l2_1d(_global_ptr_ const SRC_TYPE* src, float max_val,
        int m, int n, DST_TYPE* l2dw, DMA_DATA_POSITION pos) {
    dma_cfg((pos | dma_cfg<DST_TYPE>::value),
            (DMA_GM | dma_cfg<SRC_TYPE>::value | highptr(src)), max_val);
    dma_i((void*)l2dw, lowptr(src), m * n * sizeof(SRC_TYPE));
}

// the real work part of dma_matrix_hbm_to_l2
template <typename SRC_TYPE, typename DST_TYPE>
static __device__ void dma_hbm_to_l2(_global_ptr_ const SRC_TYPE* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn, DST_TYPE* l2dw,
        DMA_DATA_POSITION pos) {
    dma_cfg((pos | dma_cfg<DST_TYPE>::value),
            (DMA_GM | dma_cfg<SRC_TYPE>::value | highptr(matrix_mn)), max_val);
    dma_cfg_2d(m_end - m_start, (n_end - n_start) * sizeof(DST_TYPE), ldn * sizeof(SRC_TYPE));
    dma_i_2d(l2dw, lowptr(matrix_mn) + m_start * ldn + n_start,
            (n_end - n_start) * sizeof(SRC_TYPE));
}

template <typename SRC_TYPE, typename DST_TYPE>
static __device__ void dma_hbm_to_l2_3d(_global_ptr_ const SRC_TYPE* matrix_mnk, float max_val,
        int m_start, int m_end, int n_start, int n_end, int k_start, int k_end, int ldn, int ldk, DST_TYPE* l2dw,
        DMA_DATA_POSITION pos) {
    dma_cfg((pos | dma_cfg<DST_TYPE>::value), (DMA_GM | dma_cfg<SRC_TYPE>::value | highptr(matrix_mnk)), max_val);
    dma_cfg_2d(m_end - m_start, (n_end - n_start) * (k_end - k_start) * sizeof(DST_TYPE), ldn * ldk * sizeof(SRC_TYPE));
    for (int i = n_start; i < n_end; i++) {
        dma_i_2d(l2dw, lowptr(matrix_mnk) + m_start * ldn * ldk + i * ldk + k_start,
                (k_end - k_start) * sizeof(SRC_TYPE));
        l2dw = l2dw + (k_end - k_start);
    }
}

template <typename SRC_TYPE, typename DST_TYPE>
static __device__ void dma_matrix_hbm_to_l2(_global_ptr_ const SRC_TYPE* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn, DST_TYPE* l2dw) {
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DMAIN_0 : DMAIN_1;
    DMA_DATA_POSITION pos = ((cid & 1) == 0) ? DMA_L2_0 : DMA_L2_1;
    xfence_lock(dev);
    dma_cfg((pos | dma_cfg<DST_TYPE>::value), (DMA_GM | dma_cfg<SRC_TYPE>::value | highptr(matrix_mn)), max_val);
    dma_cfg_2d(m_end - m_start, (n_end - n_start) * sizeof(DST_TYPE), ldn * sizeof(SRC_TYPE));
    dma_i_2d(l2dw, lowptr(matrix_mn) + m_start * ldn + n_start, (n_end - n_start) * sizeof(SRC_TYPE));
    xfence_unlock(dev);
}

template <typename SRC_TYPE, typename DST_TYPE>
static __device__ void dma_matrix_hbm_to_l2_3d(_global_ptr_ const SRC_TYPE* matrix_mnk, float max_val,
        int m_start, int m_end, int n_start, int n_end, int k_start, int k_end, int ldn, int ldk, DST_TYPE* l2dw) {
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DMAIN_0 : DMAIN_1;
    DMA_DATA_POSITION pos = ((cid & 1) == 0) ? DMA_L2_0 : DMA_L2_1;
    xfence_lock(dev);
    dma_cfg((pos | dma_cfg<DST_TYPE>::value), (DMA_GM | dma_cfg<SRC_TYPE>::value | highptr(matrix_mnk)), max_val);
    dma_cfg_2d(m_end - m_start, (n_end - n_start) * (k_end - k_start) * sizeof(DST_TYPE), ldn * ldk * sizeof(SRC_TYPE));
    for (int i = n_start; i < n_end; i++) {
        dma_i_2d(l2dw, lowptr(matrix_mnk) + m_start * ldn * ldk + i * ldk + k_start,
                (k_end - k_start) * sizeof(SRC_TYPE));
        l2dw = l2dw + (k_end - k_start);
    }
    xfence_unlock(dev);
}

static __device__ void dma_hbm_to_l2_int31_1d(_global_ptr_ const float* src, float max_val,
        int m, int n, int16_t* l2dw, int dest_addr_offset, DMA_DATA_POSITION pos) {
    dma_cfg((pos | DMA_INT31 | (dest_addr_offset << 12)), (DMA_GM | DMA_FP32 | highptr(src)), max_val);
    dma_i(l2dw, lowptr(src), m * n * sizeof(float));
}

static __device__ void dma_hbm_to_l2_int31_2d(_global_ptr_ const float* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn, int16_t* l2dw,
        int dest_addr_offset, DMA_DATA_POSITION pos) {
    dma_cfg((pos | DMA_INT31 | (dest_addr_offset << 12)), (DMA_GM | DMA_FP32 | highptr(matrix_mn)), max_val);
    dma_cfg_2d(m_end - m_start, (n_end - n_start) * sizeof(int16_t), ldn * sizeof(float));
    dma_i_2d(l2dw, lowptr(matrix_mn) + m_start * ldn + n_start, (n_end - n_start) * sizeof(float));
}

static __device__ void dma_matrix_hbm_to_l2_int31(
        _global_ptr_ const float* matrix_mn, float max_val, int m_start, int m_end,
        int n_start, int n_end, int ldn, int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    int cid = core_id();
    int16_t* l2_base = (int16_t*)((cid >> 1) * l2dw_size_per_core);
    int dest_addr_offset = (l2dw_size_per_core / 2) << 12;
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DMAIN_0 : DMAIN_1;
    DMA_DATA_POSITION pos = ((cid & 1) == 0) ? DMA_L2_0 : DMA_L2_1;
    xfence_lock(dev);
    dma_cfg((pos | DMA_INT31 | dest_addr_offset), (DMA_GM | DMA_FP32 | highptr(matrix_mn)), max_val);
    dma_cfg_2d(m_end - m_start, (n_end - n_start) * sizeof(int16_t), ldn * sizeof(float));
    dma_i_2d(l2_base, lowptr(matrix_mn) + m_start * ldn + n_start, (n_end - n_start) * sizeof(float));
    xfence_unlock(dev);
}

// the real work part of shuffle_l2_to_l1_int16
// L2.shape == [ncols, nrows]
template <int DST_SRAM>
static __device__ void l2_to_l1_int16(int nrows, int ncols, int row_stride,
        int16_t* l2dw, v16i16* dst_ptr) {
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(nrows);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    int16_t* l2_ptr = l2dw;
    int num_full_blocks = div16(ncols);
    int ncols_mod16 = mod16(ncols);
    for (int i = 0; i < num_full_blocks; i++) {
        ds_shuffle_batch(&dst_ptr[l1_iter][0], l2_ptr, nrows, DST_SRAM);
        l2_ptr += NBANKS * nrows;
        l1_iter += row_stride;
    }
    if (ncols_mod16 > 0) {
        ds_cfg_output_bank(ncols_mod16);
        ds_shuffle_batch(&dst_ptr[l1_iter][0], l2_ptr, nrows, DST_SRAM);
    }
}

// L2.shape == [ncols, nrows]
template <int DST_SRAM>
static __device__ void shuffle_l2_to_l1_int16(int nrows, int ncols, int row_stride,
        int16_t* l2dw, v16i16* dst_ptr) {
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(nrows);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    int16_t* l2_ptr = l2dw;
    for (int i = 0; i < ncols; i += NBANKS) {
        ds_shuffle_batch(&dst_ptr[l1_iter][0], l2_ptr, nrows, DST_SRAM);
        l2_ptr += NBANKS * nrows;
        l1_iter += row_stride;
    }
    // end of real work

    xfence_unlock(dev);
}

// the real work part of shuffle_coa_l2_to_l1_int16
// L2.shape == [nrows, ncols]
template <int DST_SRAM>
static __device__ void coa_l2_to_l1_int16(int nrows, int ncols, int row_stride,
        int16_t* l2dw, v16i16* dst_ptr) {
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(ncols);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    int16_t* l2_ptr = l2dw;
    int num_full_blocks = div16(ncols);
    int ncols_mod16 = mod16(ncols);
    for (int i = 0; i < num_full_blocks; i++) {
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][0], l2_ptr, DST_SRAM);
        l2_ptr += NBANKS;
        l1_iter += row_stride;
    }
    if (ncols_mod16 > 0) {
        ds_cfg_output_bank(ncols_mod16);
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][0], l2_ptr, DST_SRAM);
    }
}

// L2.shape == [nrows, ncols]
template <int DST_SRAM>
static __device__ void shuffle_coa_l2_to_l1_int16(int nrows, int ncols, int row_stride,
        int16_t* l2dw, v16i16* dst_ptr) {
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(ncols);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    int16_t* l2_ptr = l2dw;
    for (int i = 0; i < ncols; i += NBANKS) {
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][0], l2_ptr, DST_SRAM);
        l2_ptr += NBANKS;
        l1_iter += row_stride;
    }
    // end of real work

    xfence_unlock(dev);
}

// L2.shape == [ncols, nrows]
template <int DST_SRAM>
static __device__ void shuffle_l2_to_l1_int8(int nrows, int ncols, int row_stride,
        int8_t* l2dw, v32i8* dst_ptr) {
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_INT8);
    ds_shuffle_cfg_blkx(nrows);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    int8_t* l2_ptr = l2dw;
    for (int i = 0; i < ncols; i += 2 * NBANKS) {
        ds_shuffle_batch(&dst_ptr[l1_iter][0], l2_ptr, nrows, DST_SRAM);
        l2_ptr += NBANKS * nrows;
        ds_shuffle_batch(&dst_ptr[l1_iter][1], l2_ptr, nrows, DST_SRAM);
        l2_ptr += NBANKS * nrows;
        l1_iter += row_stride;
    }
    // end of real work

    xfence_unlock(dev);
}

// L2.shape == [nrows, ncols]
template <int DST_SRAM>
static __device__ void shuffle_coa_l2_to_l1_int8(int nrows, int ncols, int row_stride,
        int8_t* l2dw, v32i8* dst_ptr) {
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_INT8);
    ds_shuffle_cfg_blkx(ncols);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    int8_t* l2_ptr = l2dw;
    for (int i = 0; i < ncols; i += 2 * NBANKS) {
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][0], l2_ptr, DST_SRAM);
        l2_ptr += NBANKS;
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][1], l2_ptr, DST_SRAM);
        l2_ptr += NBANKS;
        l1_iter += row_stride;
    }
    // end of real work

    xfence_unlock(dev);
}

// L2.shape == [ncols, nrows]
template <int DST_SRAM>
static __device__ void l2_to_l1_int31(int nrows, int ncols, int row_stride,
        int16_t* l2dw, int dest_addr_offset, v16i16* dst_ptr) {
    int16_t* l2_low = l2dw;
    int16_t* l2_high = l2dw + dest_addr_offset / sizeof(int16_t);

    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(nrows);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    for (int i = 0; i < ncols; i += NBANKS) {
        if ((ncols - i) < NBANKS) {
            ds_cfg_output_bank(ncols - i);
        }
        ds_shuffle_batch(&dst_ptr[l1_iter][0], l2_low, nrows, DST_SRAM);
        l2_low += NBANKS * nrows;
        l1_iter += row_stride;
        ds_shuffle_batch(&dst_ptr[l1_iter][0], l2_high, nrows, DST_SRAM);
        l2_high += NBANKS * nrows;
        l1_iter += row_stride;
    }
}

// L2.shape == [nrows, ncols]
template <int DST_SRAM>
static __device__ void coa_l2_to_l1_int31(int nrows, int ncols, int row_stride,
        int16_t* l2dw, int dest_addr_offset, v16i16* dst_ptr) {
    int16_t* l2_low = l2dw;
    int16_t* l2_high = l2dw + dest_addr_offset / sizeof(int16_t);

    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(ncols);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    for (int i = 0; i < ncols; i += NBANKS) {
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][0], l2_low, DST_SRAM);
        l2_low += NBANKS;
        l1_iter += row_stride;
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][0], l2_high, DST_SRAM);
        l2_high += NBANKS;
        l1_iter += row_stride;
    }
}

// L2.shape == [ncols, nrows]
template <int DST_SRAM>
static __device__ void shuffle_l2_to_l1_int31(int nrows, int ncols, int row_stride,
        v16i16* dst_ptr, int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    int cid = core_id();
    int16_t* l2_low = (int16_t*)((cid >> 1) * l2dw_size_per_core);
    int16_t* l2_high = (int16_t*)((cid >> 1) * l2dw_size_per_core + l2dw_size_per_core / 2);
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(nrows);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    for (int i = 0; i < ncols; i += NBANKS) {
        if ((ncols - i) < NBANKS) {
            ds_cfg_output_bank(ncols - i);
        }
        ds_shuffle_batch(&dst_ptr[l1_iter][0], l2_low, nrows, DST_SRAM);
        l2_low += NBANKS * nrows;
        l1_iter += row_stride;
        ds_shuffle_batch(&dst_ptr[l1_iter][0], l2_high, nrows, DST_SRAM);
        l2_high += NBANKS * nrows;
        l1_iter += row_stride;
    }
    // end of real work

    xfence_unlock(dev);
}

// L2.shape == [nrows, ncols]
template <int DST_SRAM>
static __device__ void shuffle_coa_l2_to_l1_int31(int nrows, int ncols, int row_stride,
        v16i16* dst_ptr, int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    int cid = core_id();
    int16_t* l2_low = (int16_t*)((cid >> 1) * l2dw_size_per_core);
    int16_t* l2_high = (int16_t*)((cid >> 1) * l2dw_size_per_core + l2dw_size_per_core / 2);
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_INT16);
    ds_shuffle_cfg_blkx(ncols);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    for (int i = 0; i < ncols; i += NBANKS) {
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][0], l2_low, DST_SRAM);
        l2_low += NBANKS;
        l1_iter += row_stride;
        ds_shuffle_coa(nrows, &dst_ptr[l1_iter][0], l2_high, DST_SRAM);
        l2_high += NBANKS;
        l1_iter += row_stride;
    }
    // end of real work

    xfence_unlock(dev);
}

static __device__ void shuffle_l2_to_l1_fp32(int mm, int nn, float* l2dw, v16f32* dst_ptr) {
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_FP32);
    ds_shuffle_cfg_blkx(nn);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    float* l2_ptr = l2dw;
    for (int i = 0; i < mm; i += NBANKS) {
        ds_shuffle_batch(&dst_ptr[l1_iter], l2_ptr, nn, DS_L1E);
        l2_ptr += NBANKS * nn;
        l1_iter += nn;
    }
    // end of real work

    xfence_unlock(dev);
}

static __device__ void shuffle_coa_l2_to_l1_fp32(int mm, int nn, int mm_stride, float* l2dw, v16f32* dst_ptr) {
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DS_0 : DS_1;
    xfence_lock(dev);

    // real work
    ds_shuffle_cfg_datatype(DS_FP32);
    ds_shuffle_cfg_blkx(nn);
    ds_cfg_output_bank(NBANKS);
    int l1_iter = 0;
    float* l2_ptr = l2dw;
    for (int i = 0; i < nn; i += NBANKS) {
        ds_shuffle_coa(mm, &dst_ptr[l1_iter][0], l2_ptr, DS_L1E);
        l2_ptr += NBANKS;
        l1_iter += mm_stride;
    }
    // end of real work

    xfence_unlock(dev);
}


template<typename TYPE> constexpr int get_coeff_mode_alpha() {
    return EW_COEFF0_VECTOR;
}
template<> constexpr int get_coeff_mode_alpha<float>() {
    return EW_COEFF0_SCALAR;
}

template<typename TYPE> constexpr int get_coeff_mode_beta() {
    return EW_COEFF1_VECTOR;
}
template<> constexpr int get_coeff_mode_beta<float>() {
    return EW_COEFF1_SCALAR;
}

template<typename TYPE> constexpr int get_coeff_mode_gamma() {
    return EW_COEFF2_VECTOR;
}
template<> constexpr int get_coeff_mode_gamma<float>() {
    return EW_COEFF2_SCALAR;
}

template<typename ALPHA_T, typename BETA_T, typename GAMMA_T>
constexpr int get_coeff_mode() {
    return get_coeff_mode_alpha<ALPHA_T>() | get_coeff_mode_beta<BETA_T>() | get_coeff_mode_gamma<GAMMA_T>();
}

template <typename T> // T should match v16f32*
static __device__ T get_coeff(T coeff, int offset) {
    return &coeff[offset];
}

template <>
__device__ float get_coeff(float coeff, int offset) {
    return coeff;
}

template <bool IS_ADD, int EW_SRAM>
static __device__ void ew_fp32_helper(v16f32* result, v16f32* A, v16f32* B) {
    if (IS_ADD) {
        /* ew_cfg_activation_type() ew_cfg_coeff_scalar()
        ew_cfg_stream_size() ew_cfg_col_size() is configured in function - ew_matrix_helper() */
        ew_dsmadd(result, A, B, EW_SRAM);
    } else {
        /* ew_cfg_activation_type() ew_cfg_coeff_scalar()
        ew_cfg_stream_size() ew_cfg_col_size() is configured in function - ew_matrix_helper() */
        ew_dsmul(result, A, B, EW_SRAM);
    }
}

// ALPHA_T, BETA_T, GAMMA_T can be float or v16f32*
template <bool IS_ADD, typename ALPHA_T, typename BETA_T, typename GAMMA_T>
static __device__ void ew_matrix_helper(int mm, int nn,
        ALPHA_T alpha, v16f32* A, BETA_T beta, v16f32* B, GAMMA_T gamma,
        EW_ACTIVATION_TYPE act_type, v16f32* result) {
    int num_blocks = roundup_div16(nn);
    int nn_mod16 = mod16(nn);
    int stream_size = mm * num_blocks;
    const int mode = get_coeff_mode<ALPHA_T, BETA_T, GAMMA_T>();
    if (mode == (EW_COEFF0_SCALAR | EW_COEFF1_SCALAR | EW_COEFF2_SCALAR)) {
        xfence_lock(EW);
        ew_cfg_activation_type(act_type);
        ew_cfg_coeff(alpha, beta, gamma, mode);
        ew_cfg_stream_size(stream_size);
        ew_cfg_col_size(NBANKS);
        ew_fp32_helper<IS_ADD, EW_L1E>(result, A, B);
        xfence_unlock(EW);
    } else {
        xfence_lock(EW);
        ew_cfg_activation_type(act_type);
        ew_cfg_stream_size(mm);
        ew_cfg_col_size(NBANKS);
        for (int i = 0; i < num_blocks; i++) {
            ew_cfg_coeff(get_coeff(alpha, i), get_coeff(beta, i),
                    get_coeff(gamma, i), mode);
            ew_fp32_helper<IS_ADD, EW_L1E>(&result[i * mm], &A[i * mm], &B[i * mm]);
        }
        xfence_unlock(EW);
    }
    return;
}

static __device__ float ew_findmax_l1e_to_l2e(int mm, int nn, v16f32* l1e, v16f32* l2e, EW_ACTIVATION_TYPE act_type,
        float alpha = 1.0f, float beta = 0.0f) {
    float ew_max = 0.0f;
    int num_full_blocks = div16(nn);
    int nn_mod16 = mod16(nn);
    int full_stream_size = mm * num_full_blocks;

    xfence_lock(EW);
    ew_cfg_activation_type(act_type);
    ew_cfg_coeff_scalar(alpha, 0.0f, beta);
    ew_cfg_findmax(1, ew_max);
    if (num_full_blocks > 0) {
        ew_cfg_stream_size(full_stream_size);
        ew_cfg_col_size(NBANKS);
        ew_dsmadd(l2e, l1e, l1e, EW_L2E);
    }
    if (nn_mod16 > 0) {
        ew_cfg_stream_size(mm);
        ew_cfg_col_size(nn_mod16);
        ew_dsmadd(&l2e[full_stream_size], &l1e[full_stream_size], &l1e[full_stream_size],
                EW_L2E);
    }
    ew_ldmax(ew_max);
    xfence_unlock(EW);
    return ew_max;
}

static __device__ float ew_findmax_l1e_to_l2e_with_bias(int mm, int nn, v16f32* l1e, v16f32* l1e_bias,
        v16f32* l2e, EW_ACTIVATION_TYPE act_type) {
    float ew_max = 0.0f;
    int num_full_blocks = div16(nn);
    int nn_mod16 = mod16(nn);
    int full_stream_size = mm * num_full_blocks;

    xfence_lock(EW);
    ew_cfg_activation_type(act_type);
    ew_cfg_findmax(1, ew_max);
    ew_cfg_stream_size(mm);
    ew_cfg_col_size(NBANKS);
    for (int i = 0; i < num_full_blocks; i++) {
        ew_cfg_coeff(1.0f, 0.0f, &l1e_bias[i], (EW_COEFF0_SCALAR | EW_COEFF1_SCALAR | EW_COEFF2_VECTOR));
        ew_dsmadd(l2e, l1e, l1e, EW_L2E);
        l2e = &l2e[mm];
        l1e = &l1e[mm];
    }
    if (nn_mod16 > 0) {
        ew_cfg_col_size(nn_mod16);
        ew_cfg_coeff(1.0f, 0.0f, &l1e_bias[num_full_blocks], (EW_COEFF0_SCALAR | EW_COEFF1_SCALAR | EW_COEFF2_VECTOR));
        ew_dsmadd(l2e, l1e, l1e, EW_L2E);
    }
    ew_ldmax(ew_max);
    xfence_unlock(EW);
    return ew_max;
}

static __device__ void rsrow_l2e_to_l2r(int mm, int nn, v16f32* l2e, v16f32* l2r) {
    int num_blocks = roundup_div16(nn);
    int src_stride = 1;
    int dst_stride = num_blocks;
    xfence_lock(RS);
    rs_row_cfg_stride(src_stride, dst_stride);
    rs_row_cfg_loop(mm);
    for (int i = 0; i < num_blocks; i++) {
        rs_row_batch(&l2r[i][0], &l2e[i * mm][0], NBANKS);
    }
    xfence_unlock(RS);
}


static __device__ void rscol_l2e_to_l2r(int mm, int nn, v16f32* l2e, v16f32* l2r) {
    int num_blocks = roundup_div16(nn);
    int src_row_stride = 1;
    int dst_row_stride = roundup_div16(mm);
    int dst_col_stride = 1;
    xfence_lock(RS);
    rs_col_cfg_loop(NBANKS);
    rs_col_cfg_stride(src_row_stride, dst_row_stride, dst_col_stride);
    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < dst_row_stride; j++) {
            rs_col_batch(&l2r[j], &l2e[j * NBANKS], NBANKS);
        }
        l2r = &l2r[NBANKS * dst_row_stride];
        l2e = &l2e[mm];
    }
    xfence_unlock(RS);
}

template <typename DST_TYPE>
static __device__ void dmaout_l2r_to_hbm(v16f32* l2r, _global_ptr_ DST_TYPE* matrix_mn, int m_start, int m_end,
        int n_start, int n_end, int ldn, float matrix_max = 0.0f) { // matrix_max works when DST_TYPE == int8/int16
    int l2r_stride = roundup16(n_end - n_start);
    xfence_lock(DMAOUT);
    dma_cfg((DMA_GM | dma_cfg<DST_TYPE>::value | highptr(matrix_mn)), (DMA_L2R | DMA_FP32), matrix_max);
    dma_cfg_2d(m_end - m_start, ldn * sizeof(DST_TYPE), l2r_stride * sizeof(float));
    dma_o_2d(lowptr(matrix_mn) + m_start * ldn + n_start, l2r, (n_end - n_start) * sizeof(float));
    xfence_unlock(DMAOUT);
}

template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void dma_shuffle_hbm_to_l1_int16(_global_ptr_ const SRC_TYPE* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn, v16i16* dst_ptr, int ncores = 1) {
    int cid = core_id();
    int16_t* l2dw = (int16_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    // need to make sure:
    //      max_mm * max_nn * sizeof(int16_t) <= L2DW_SIZE_PER_CORE
    //      max_mm % 16 == 0
    //      "large max_nn" is better than "large max_mm" for better DMA performance
    int max_nn = min(n_end - n_start, L2DW_SIZE_PER_CORE / (2 * NBANKS));
    // let max_mm >= NBANKS, otherwise max_mm will become 0, and cause infinite loop
    int max_mm = rounddown16(L2DW_SIZE_PER_CORE / (2 * max_nn));
    int mm_div16 = div16(max_mm);
    int l1_row_stride = roundup16(n_end - n_start);
    int blockid = -1;
    for (int loop_m_start = m_start; loop_m_start < m_end; loop_m_start += max_mm) {
        int loop_m_end = min(loop_m_start + max_mm, m_end);
        int l1_ncols = loop_m_end - loop_m_start;
        for (int loop_n_start = n_start; loop_n_start < n_end; loop_n_start += max_nn) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_n_end = min(loop_n_start + max_nn, n_end);
            int l1_nrows = loop_n_end - loop_n_start;
            dma_matrix_hbm_to_l2<SRC_TYPE, int16_t>(
                    matrix_mn, max_val, loop_m_start, loop_m_end,
                    loop_n_start, loop_n_end, ldn, l2dw);
            int row_idx = loop_n_start - n_start;
            shuffle_l2_to_l1_int16<DST_SRAM>(l1_nrows, l1_ncols, l1_row_stride, l2dw, &dst_ptr[row_idx]);
        }
        dst_ptr = &dst_ptr[mm_div16 * l1_row_stride];
    }
}

template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void dma_shuffle_coa_hbm_to_l1_int16(_global_ptr_ const SRC_TYPE* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn, v16i16* dst_ptr, int ncores = 1) {
    int cid = core_id();
    int16_t* l2dw = (int16_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    // need to make sure:
    //      max_mm * max_nn * sizeof(int16_t) <= L2DW_SIZE_PER_CORE
    //      max_nn % 16 == 0
    //      "large max_nn" is better than "large max_mm" for better DMA performance
    int max_nn = min(n_end - n_start, L2DW_SIZE_PER_CORE / (2 * NBANKS));
    // let max_mm >= NBANKS, otherwise shuffle_coa will be very slow
    max_nn = roundup16(max_nn);
    int max_mm = min(m_end - m_start, L2DW_SIZE_PER_CORE / (2 * max_nn));
    int nn_div16 = div16(max_nn);
    int row_stride = roundup16(m_end - m_start);
    int blockid = -1;
    for (int loop_n_start = n_start; loop_n_start < n_end; loop_n_start += max_nn) {
        int loop_n_end = min(loop_n_start + max_nn, n_end);
        int ncols = loop_n_end - loop_n_start;
        for (int loop_m_start = m_start; loop_m_start < m_end; loop_m_start += max_mm) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_m_end = min(loop_m_start + max_mm, m_end);
            int nrows = loop_m_end - loop_m_start;
            dma_matrix_hbm_to_l2<SRC_TYPE, int16_t>(
                    matrix_mn, max_val, loop_m_start, loop_m_end,
                    loop_n_start, loop_n_end, ldn, l2dw);
            int row_idx = loop_m_start - m_start;
            shuffle_coa_l2_to_l1_int16<DST_SRAM>(nrows, ncols, row_stride, l2dw, &dst_ptr[row_idx]);
        }
        dst_ptr = &dst_ptr[nn_div16 * row_stride];
    }
}

template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void dma_shuffle_hbm_to_l1_int8(_global_ptr_ const SRC_TYPE* matrix_mn,
        float max_val, int m_start, int m_end, int n_start, int n_end, int ldn, v32i8* dst_ptr, int ncores = 1) {
    int cid = core_id();
    int8_t* l2dw = (int8_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    // need to make sure:
    //      max_mm * max_nn <= L2DW_SIZE_PER_CORE
    //      max_mm % 32 == 0
    //      "large max_nn" is better than "large max_mm"
    int max_nn = min(n_end - n_start, L2DW_SIZE_PER_CORE / (2 * NBANKS));
    // let max_mm >= 2 * NBANKS, otherwise max_mm will become 0, and cause infinite loop
    int max_mm = rounddown32(L2DW_SIZE_PER_CORE / max_nn);
    int mm_div32 = div32(max_mm);
    int l1_row_stride = roundup16(n_end - n_start);
    int blockid = -1;
    for (int loop_m_start = m_start; loop_m_start < m_end; loop_m_start += max_mm) {
        int loop_m_end = min(loop_m_start + max_mm, m_end);
        int l1_ncols = loop_m_end - loop_m_start;
        for (int loop_n_start = n_start; loop_n_start < n_end; loop_n_start += max_nn) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_n_end = min(loop_n_start + max_nn, n_end);
            int l1_nrows = loop_n_end - loop_n_start;
            dma_matrix_hbm_to_l2<SRC_TYPE, int8_t>(
                    matrix_mn, max_val, loop_m_start, loop_m_end,
                    loop_n_start, loop_n_end, ldn, l2dw);
            int row_idx = loop_n_start - n_start;
            shuffle_l2_to_l1_int8<DST_SRAM>(l1_nrows, l1_ncols, l1_row_stride, l2dw, &dst_ptr[row_idx]);
        }
        dst_ptr = &dst_ptr[mm_div32 * l1_row_stride];
    }
}

template <typename SRC_TYPE, int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void dma_shuffle_coa_hbm_to_l1_int8(_global_ptr_ const SRC_TYPE* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn, v32i8* dst_ptr, int ncores = 1) {
    int cid = core_id();
    int8_t* l2dw = (int8_t*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    // need to make sure:
    //      max_mm * max_nn <= L2DW_SIZE_PER_CORE
    //      max_nn % 32 == 0
    //      "large max_nn" is better than "large max_mm"
    int max_nn = min(n_end - n_start, L2DW_SIZE_PER_CORE / (2 * NBANKS));
    // let max_mm >= 2 * NBANKS, otherwise shuffle_coa will be very slow
    max_nn = roundup32(max_nn);
    int max_mm = min(m_end - m_start, L2DW_SIZE_PER_CORE / max_nn);
    int nn_div32 = div32(max_nn);
    int row_stride = roundup16(m_end - m_start);
    int blockid = -1;
    for (int loop_n_start = n_start; loop_n_start < n_end; loop_n_start += max_nn) {
        int loop_n_end = min(loop_n_start + max_nn, n_end);
        int ncols = loop_n_end - loop_n_start;
        for (int loop_m_start = m_start; loop_m_start < m_end; loop_m_start += max_mm) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_m_end = min(loop_m_start + max_mm, m_end);
            int nrows = loop_m_end - loop_m_start;
            dma_matrix_hbm_to_l2<SRC_TYPE, int8_t>(
                    matrix_mn, max_val, loop_m_start, loop_m_end,
                    loop_n_start, loop_n_end, ldn, l2dw);
            int row_idx = loop_m_start - m_start;
            shuffle_coa_l2_to_l1_int8<DST_SRAM>(nrows, ncols, row_stride, l2dw, &dst_ptr[row_idx]);
        }
        dst_ptr = &dst_ptr[nn_div32 * row_stride];
    }
}

template <int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void dma_shuffle_hbm_to_l1_int31(_global_ptr_ const float* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn, v16i16* dst_ptr,
        int ncores = 1, int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    int cid = core_id();

    // need to make sure:
    //      max_mm * max_nn * sizeof(int16_t) <= l2dw_size_per_core / 2
    //      max_mm % 16 == 0
    //      "large max_nn" is better than "large max_mm" for better DMA performance
    int max_nn = min(n_end - n_start, l2dw_size_per_core / (4 * NBANKS));
    // let max_mm >= NBANKS, otherwise max_mm will become 0, and cause infinite loop
    int max_mm = rounddown16(l2dw_size_per_core / (4 * max_nn));
    int mm_div16 = div16(max_mm);
    int l1_row_stride = roundup16(n_end - n_start);
    int blockid = -1;
    for (int loop_m_start = m_start; loop_m_start < m_end; loop_m_start += max_mm) {
        int loop_m_end = min(loop_m_start + max_mm, m_end);
        int l1_ncols = loop_m_end - loop_m_start;
        for (int loop_n_start = n_start; loop_n_start < n_end; loop_n_start += max_nn) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_n_end = min(loop_n_start + max_nn, n_end);
            int l1_nrows = loop_n_end - loop_n_start;
            dma_matrix_hbm_to_l2_int31(
                    matrix_mn, max_val, loop_m_start, loop_m_end,
                    loop_n_start, loop_n_end, ldn, l2dw_size_per_core);
            int row_idx = loop_n_start - n_start;
            shuffle_l2_to_l1_int31<DST_SRAM>(l1_nrows, l1_ncols, l1_row_stride, &dst_ptr[row_idx], l2dw_size_per_core);
        }
        dst_ptr = &dst_ptr[mm_div16 * l1_row_stride * 2];
    }
}

template <int DST_SRAM, bool IN_PARALLEL = false>
static __device__ void dma_shuffle_coa_hbm_to_l1_int31(_global_ptr_ const float* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn, v16i16* dst_ptr,
        int ncores = 1, int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    int cid = core_id();

    // need to make sure:
    //      max_mm * max_nn * sizeof(int16_t) <= l2dw_size_per_core / 2
    //      max_nn % 16 == 0
    //      "large max_nn" is better than "large max_mm" for better DMA performance
    int max_nn = min(n_end - n_start, l2dw_size_per_core / (4 * NBANKS));
    // let max_mm >= NBANKS, otherwise shuffle_coa will be very slow
    max_nn = roundup16(max_nn);
    int max_mm = min(m_end - m_start, l2dw_size_per_core / (4 * max_nn));
    int nn_div16 = div16(max_nn);
    int row_stride = roundup16(m_end - m_start);
    int blockid = -1;
    for (int loop_n_start = n_start; loop_n_start < n_end; loop_n_start += max_nn) {
        int loop_n_end = min(loop_n_start + max_nn, n_end);
        int ncols = loop_n_end - loop_n_start;
        for (int loop_m_start = m_start; loop_m_start < m_end; loop_m_start += max_mm) {
            if (IN_PARALLEL) {
                blockid++;
                if (blockid % ncores != cid) {
                    continue;
                }
            }
            int loop_m_end = min(loop_m_start + max_mm, m_end);
            int nrows = loop_m_end - loop_m_start;
            dma_matrix_hbm_to_l2_int31(
                    matrix_mn, max_val, loop_m_start, loop_m_end,
                    loop_n_start, loop_n_end, ldn, l2dw_size_per_core);
            int row_idx = loop_m_start - m_start;
            shuffle_coa_l2_to_l1_int31<DST_SRAM>(nrows, ncols, row_stride, &dst_ptr[row_idx], l2dw_size_per_core);
        }
        dst_ptr = &dst_ptr[nn_div16 * row_stride * 2];
    }
}

template <typename SRC_TYPE>
static __device__ void dma_shuffle_hbm_to_l1_fp32(_global_ptr_ const SRC_TYPE* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn,
        v16f32* dst_ptr, int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    int cid = core_id();
    float* l2dw = (float*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    // need to make sure:
    //      roundup16(max_mm) * max_nn * sizeof(float) <= l2dw_size_per_core
    int max_nn = min(n_end - n_start, l2dw_size_per_core / (4 * NBANKS));
    int max_mm = rounddown16(l2dw_size_per_core / (4 * max_nn));
    int mm_div16 = div16(max_mm);
    for (int m_iter = m_start; m_iter < m_end; m_iter += max_mm) {
        int m_iter_end = min(m_iter + max_mm, m_end);
        int mm = m_iter_end - m_iter;
        for (int n_iter = n_start; n_iter < n_end; n_iter += max_nn) {
            int n_iter_end = min(n_iter + max_nn, n_end);
            int nn = n_iter_end - n_iter;
            dma_matrix_hbm_to_l2<SRC_TYPE, float>(
                    matrix_mn, max_val, m_iter, m_iter_end,
                    n_iter, n_iter_end, ldn, l2dw);
            shuffle_l2_to_l1_fp32(mm, nn, l2dw, dst_ptr);
            dst_ptr = &dst_ptr[mm_div16 * nn];
        }
    }
}

template <typename SRC_TYPE>
static __device__ void dma_shuffle_coa_hbm_to_l1_fp32(_global_ptr_ const SRC_TYPE* matrix_mn, float max_val,
        int m_start, int m_end, int n_start, int n_end, int ldn,
        v16f32* dst_ptr, int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    int cid = core_id();
    float* l2dw = (float*)((cid >> 1) * L2DW_SIZE_PER_CORE);

    // need to make sure:
    //      max_mm * roundup16(max_nn) * sizeof(float) <= l2dw_size_per_core
    int max_mm = min(m_end - m_start, l2dw_size_per_core / (sizeof_fp32 * NBANKS));
    int max_nn = rounddown16(l2dw_size_per_core / sizeof_fp32 / max_mm);
    int nn_div16 = div16(max_nn);
    for (int n_iter = n_start; n_iter < n_end; n_iter += max_nn) {
        int n_iter_end = min(n_iter + max_nn, n_end);
        int nn = n_iter_end - n_iter;
        for (int m_iter = m_start; m_iter < m_end; m_iter += max_mm) {
            int m_iter_end = min(m_iter + max_mm, m_end);
            int mm = m_iter_end - m_iter;
            dma_matrix_hbm_to_l2<SRC_TYPE, float>(
                    matrix_mn, max_val, m_iter, m_iter_end,
                    n_iter, n_iter_end, ldn, l2dw);
            shuffle_coa_l2_to_l1_fp32(mm, nn, mm, l2dw, dst_ptr);
            dst_ptr = &dst_ptr[nn_div16 * mm];
        }
    }
}

template <bool ACC>
static __device__ void mm_int16_helper(v16f32* l1e, v16i16* l1d, v16i16* l1w) {
    /*
       mm_cfg_stride(), mm_cfg_basic_int16(), mm_cfg_dequant_scale()
       are configured before
    */
    if (ACC) {
        mm_acc_int16(l1e, l1d, l1w);
    } else {
        mm_int16(l1e, l1d, l1w);
    }
}

template <> __device__ void mm_int16_helper<true>(v16f32* l1e, v16i16* l1d, v16i16* l1w) {
    /*
       mm_cfg_stride(), mm_cfg_basic_int16(), mm_cfg_dequant_scale()
       are configured before
    */
    mm_acc_int16(l1e, l1d, l1w);
}

template <> __device__ void mm_int16_helper<false>(v16f32* l1e, v16i16* l1d, v16i16* l1w) {
    /*
       mm_cfg_stride(), mm_cfg_basic_int16(), mm_cfg_dequant_scale()
       are configured before
    */
    mm_int16(l1e, l1d, l1w);
}

// sram memroy usage
//  [l1d: roundup16(mm) * roundup16(k) * sizeof(int16_t)]
//  [l1w: roundup16(nn) * roundup16(k) * sizeof(int16_t)]
//  [l1e: mm * roundup16(nn) * sizeof(float)]
template <bool ACC>
static __device__ void mac_int16_helper(float dequant, int k, int mm, int nn,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {

    int roundup_k = roundup16(k);
    int roundup_div16_nn = roundup_div16(nn);
    xfence_lock(MAC);
    mm_cfg_stride(1);
    mm_cfg_dequant_scale(dequant);
    mm_cfg_basic_int16(k, mm);
    for (int n_iter = 0; n_iter < roundup_div16_nn; n_iter += 1) {
        mm_int16_helper<ACC>(&l1e[n_iter * mm], l1d, &l1w[n_iter * roundup_k]);
    }
    xfence_unlock(MAC);
}

template <bool ACC>
static __device__ void mm_int8_helper(v16f32* l1e, v32i8* l1d, v32i8* l1w) {
    /*
       mm_cfg_stride(), mm_cfg_basic_int8(), mm_cfg_dequant_scale()
       are configured in function - mac_int8_helper()
    */
    if (ACC) {
        mm_acc_int8(l1e, l1d, l1w);
    } else {
        mm_int8(l1e, l1d, l1w);
    }
}

// sram memroy usage
//  [l1d: roundup32(mm) * roundup16(k) * sizeof(int16_t)]
//  [l1w: roundup32(nn) * roundup16(k) * sizeof(int16_t)]
//  [l1e: mm * roundup32(nn) * sizeof(float)]
template <bool ACC>
static __device__ void mac_int8_helper(float dequant, int k, int mm, int nn,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {

    int roundup_k = roundup16(k);
    int roundup_div32_nn = roundup_div32(nn);
    int div32_mm = div32(mm);
    int mod32_mm = mod32(mm);
    xfence_lock(MAC);
    mm_cfg_stride(1);
    mm_cfg_dequant_scale(dequant);
    mm_cfg_basic_int8(k, mm, mm);
    for (int n_iter = 0; n_iter < roundup_div32_nn; n_iter += 1) {
        mm_int8_helper<ACC>(&l1e[n_iter * mm * 2], l1d, &l1w[n_iter * roundup_k]);
    }
    xfence_unlock(MAC);
}

template <bool ACC>
static __device__ void mac_int31_step(float dequant, int k, int mm, int nn,
        int roundup_k, int roundup_div16_nn, int div16_mm, int mod16_mm,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {
    mm_cfg_dequant_scale(dequant);
    if (mm > nn) { // make inner loop larger for better performance
        for (int n_iter = 0; n_iter < roundup_div16_nn; n_iter += 1) {
            int m_iter = 0;
            mm_cfg_basic_int16(k, NBANKS);
            for (; m_iter < div16_mm; m_iter += 1) {
                mm_int16_helper<ACC>(&l1e[n_iter * mm + m_iter * NBANKS],
                        &l1d[2 * m_iter * roundup_k], &l1w[2 * n_iter * roundup_k]);
            }
            if (mod16_mm > 0) {
                mm_cfg_basic_int16(k, mod16_mm);
                mm_int16_helper<ACC>(&l1e[n_iter * mm + m_iter * NBANKS],
                        &l1d[2 * m_iter * roundup_k], &l1w[2 * n_iter * roundup_k]);
            }
        }
    } else {
        int m_iter = 0;
        mm_cfg_basic_int16(k, NBANKS);
        for (; m_iter < div16_mm; m_iter += 1) {
            for (int n_iter = 0; n_iter < roundup_div16_nn; n_iter += 1) {
                mm_int16_helper<ACC>(&l1e[n_iter * mm + m_iter * NBANKS],
                        &l1d[2 * m_iter * roundup_k], &l1w[2 * n_iter * roundup_k]);
            }
        }
        if (mod16_mm > 0) {
            mm_cfg_basic_int16(k, mod16_mm);
            for (int n_iter = 0; n_iter < roundup_div16_nn; n_iter += 1) {
                mm_int16_helper<ACC>(&l1e[n_iter * mm + m_iter * NBANKS],
                        &l1d[2 * m_iter * roundup_k], &l1w[2 * n_iter * roundup_k]);
            }
        }
    }
}

// sram memroy usage
//  [l1d: roundup16(mm) * roundup16(k) * sizeof(int16_t) * 2]
//  [l1w: roundup16(nn) * roundup16(k) * sizeof(int16_t) * 2]
//  [l1e: mm * roundup16(nn) * sizeof(float)]
template <bool ACC>
static __device__ void mac_int31_helper(float dequant_ll, float dequant_hl, float dequant_hh, int k, int mm, int nn,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {

    int roundup_k = roundup16(k);
    int roundup_div16_nn = roundup_div16(nn);
    int div16_mm = div16(mm);
    int mod16_mm = mod16(mm);

    xfence_lock(MAC);
    mm_cfg_stride(1);
    //mac_int31_step<ACC>(dequant_ll, k, mm, nn,
    //                    roundup_k, roundup_div16_nn, div16_mm, mod16_mm,
    //                    l1d, l1w, l1e);
    mac_int31_step<ACC>(dequant_hl, k, mm, nn,
            roundup_k, roundup_div16_nn, div16_mm, mod16_mm,
            &l1d[roundup_k], l1w, l1e);
    mac_int31_step<true>(dequant_hl, k, mm, nn,
            roundup_k, roundup_div16_nn, div16_mm, mod16_mm,
            l1d, &l1w[roundup_k], l1e);
    mac_int31_step<true>(dequant_hh, k, mm, nn,
            roundup_k, roundup_div16_nn, div16_mm, mod16_mm,
            &l1d[roundup_k], &l1w[roundup_k], l1e);
    xfence_unlock(MAC);
}

template <bool ACC, int I8_MODE>
static __device__ void mm_int4_helper(v16f32* l1e, v32i8* l1d, v32i8* l1w) {
    /*
       mm_cfg_stride(), mm_cfg_basic_int8(), mm_cfg_dequant_scale()
       are configured in function - mac_int4_helper()
    */
    if (ACC) {
        mm_acc_int4(l1e, l1d, l1w, I8_MODE);
    } else {
        mm_int4(l1e, l1d, l1w, I8_MODE);
    }
}

template <bool ACC, int I8_MODE>
static __device__ void mac_int4_helper(float dequant, int k, int mm, int nn,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {
    constexpr const int is_l1d_int4 = ((I8_MODE == MAC_INT8_D4W8) || (I8_MODE == MAC_INT8_D4W4)) ? 2 : 1;
    constexpr const int is_l1w_int4 = ((I8_MODE == MAC_INT8_D8W4) || (I8_MODE == MAC_INT8_D4W4)) ? 2 : 1;
    int l1d_dim = roundup16(k / is_l1d_int4);
    int l1w_dim = roundup16(k / is_l1w_int4);
    int roundup_div32_nn = roundup_div32(nn);
    int div32_mm = div32(mm);
    int mod32_mm = mod32(mm);

    xfence_lock(MAC);
    mm_cfg_stride(1);
    mm_cfg_dequant_scale(dequant);
    mm_cfg_basic_int8(k, mm, mm);
    for (int n_iter = 0; n_iter < roundup_div32_nn; n_iter += 1) {
        mm_int4_helper<ACC, I8_MODE>(&l1e[n_iter * mm * 2], l1d, &l1w[n_iter * l1w_dim]);
    }
    xfence_unlock(MAC);
}

// bias, act_type, findmax only supported when SRC_SRAM == EW_L1E
template <int SRC_SRAM, bool add_bias>
static __device__ float rscol_helper(v16f32* src, v16f32* l1e_bias,
        int mm, int nn, EW_ACTIVATION_TYPE act_type, float old_max,
        int l2e_size_per_core, int l2r_size_per_core) {
    // not supported
    return 0.0f;
}

template <>
__device__ float rscol_helper<EW_L2E, false>(v16f32* src, v16f32* l1e_bias,
        int mm, int nn, EW_ACTIVATION_TYPE act_type, float old_max,
        int l2e_size_per_core, int l2r_size_per_core) {
    int cid = core_id();
    v16f32* l2r_base = (v16f32*)(cid * l2r_size_per_core);
    rscol_l2e_to_l2r(mm, nn, src, l2r_base);
    return 0.0f;
}

template <>
__device__ float rscol_helper<EW_L1E, true>(v16f32* src, v16f32* l1e_bias,
        int mm, int nn, EW_ACTIVATION_TYPE act_type, float old_max,
        int l2e_size_per_core, int l2r_size_per_core) {
    int cid = core_id();
    v16f32* l2e_base = (v16f32*)(cid * l2e_size_per_core);
    v16f32* l2r_base = (v16f32*)(cid * l2r_size_per_core);
    float ew_max = ew_findmax_l1e_to_l2e_with_bias(mm, nn, src, l1e_bias, l2e_base, act_type);
    rscol_l2e_to_l2r(mm, nn, l2e_base, l2r_base);
    return fmax(ew_max, old_max);
}

template <>
__device__ float rscol_helper<EW_L1E, false>(v16f32* src, v16f32* l1e_bias,
        int mm, int nn, EW_ACTIVATION_TYPE act_type, float old_max,
        int l2e_size_per_core, int l2r_size_per_core) {
    int cid = core_id();
    v16f32* l2e_base = (v16f32*)(cid * l2e_size_per_core);
    v16f32* l2r_base = (v16f32*)(cid * l2r_size_per_core);
    float ew_max = ew_findmax_l1e_to_l2e(mm, nn, src, l2e_base, act_type);
    rscol_l2e_to_l2r(mm, nn, l2e_base, l2r_base);
    return fmax(ew_max, old_max);
}

template <int SRC_SRAM, bool add_bias, typename DST_TYPE>
static __device__ float rscol_l1e_or_l2e_to_hbm(v16f32* src, v16f32* l1e_bias, _global_ptr_ DST_TYPE* matrix_nm,
        int n_start, int n_end, int m_start, int m_end, int ldm, EW_ACTIVATION_TYPE act_type,
        int l2e_size_per_core, int l2r_size_per_core) {
    int cid = core_id();
    v16f32* l2r_base = (v16f32*)(cid * l2r_size_per_core);
    // need to make sure:
    //      roundup16(max_mm) * roundup16(max_nn) * sizeof(float) <= l2r_size_per_core
    //      case-1: max_nn = NBANKS, max_mm <= m_end - m_start
    //      case-2: max_nn > NBANKS, max_mm = m_end - m_start;
    int max_nn = NBANKS;
    int max_mm = min(m_end - m_start, rounddown16(L2R_SIZE_PER_CORE / 4 / max_nn));
    max_nn = rounddown16(l2r_size_per_core / 4 / roundup16(max_mm));
    float matrix_max = 0.0f;
    for (int n_iter = n_start; n_iter < n_end; n_iter += max_nn) {
        int n_iter_end = min(n_iter + max_nn, n_end);
        int nn = n_iter_end - n_iter;
        for (int m_iter = m_start; m_iter < m_end; m_iter += max_mm) {
            int m_iter_end = min(m_iter + max_mm, m_end);
            int mm = m_iter_end - m_iter;
            matrix_max = rscol_helper<SRC_SRAM, add_bias>(
                            src, &l1e_bias[n_iter / NBANKS], mm, nn, act_type, matrix_max,
                            l2e_size_per_core, l2r_size_per_core);
            dmaout_l2r_to_hbm(l2r_base, matrix_nm, n_iter, n_iter_end, m_iter, m_iter_end, ldm);
            src = &src[mm * div16(max_nn)];
        }
    }
    return matrix_max;
}

// bias, act_type, findmax only supported when SRC_SRAM == EW_L1E
template <int SRC_SRAM, bool add_bias>
static __device__ float rsrow_helper(v16f32* src, v16f32* l1e_bias,
        int mm, int nn, EW_ACTIVATION_TYPE act_type, float old_max,
        int l2e_size_per_core, int l2r_size_per_core) {
    // not supported
    return 0.0f;
}

template <>
__device__ float rsrow_helper<EW_L1E, false>(v16f32* src, v16f32* l1e_bias,
        int mm, int nn, EW_ACTIVATION_TYPE act_type, float old_max,
        int l2e_size_per_core, int l2r_size_per_core) {
    int cid = core_id();
    v16f32* l2e_base = (v16f32*)(cid * l2e_size_per_core);
    v16f32* l2r_base = (v16f32*)(cid * l2r_size_per_core);
    float ew_max = ew_findmax_l1e_to_l2e(mm, nn, src, l2e_base, act_type);
    rsrow_l2e_to_l2r(mm, nn, l2e_base, l2r_base);
    return fmax(ew_max, old_max);
}

template <>
__device__ float rsrow_helper<EW_L1E, true>(v16f32* src, v16f32* l1e_bias,
        int mm, int nn, EW_ACTIVATION_TYPE act_type, float old_max,
        int l2e_size_per_core, int l2r_size_per_core) {
    int cid = core_id();
    v16f32* l2e_base = (v16f32*)(cid * l2e_size_per_core);
    v16f32* l2r_base = (v16f32*)(cid * l2r_size_per_core);
    float ew_max = ew_findmax_l1e_to_l2e_with_bias(mm, nn, src, l1e_bias, l2e_base, act_type);
    rsrow_l2e_to_l2r(mm, nn, l2e_base, l2r_base);
    return fmax(ew_max, old_max);
}

template <>
__device__ float rsrow_helper<EW_L2E, false>(v16f32* src, v16f32* l1e_bias,
        int mm, int nn, EW_ACTIVATION_TYPE act_type, float old_max,
        int l2e_size_per_core, int l2r_size_per_core) {
    int cid = core_id();
    v16f32* l2r_base = (v16f32*)(cid * l2r_size_per_core);
    rsrow_l2e_to_l2r(mm, nn, src, l2r_base);
    return 0.0f;
}

template <int SRC_SRAM, bool add_bias, typename DST_TYPE>
static __device__ float rsrow_l1e_or_l2e_to_hbm(v16f32* src, v16f32* l1e_bias, _global_ptr_ DST_TYPE* matrix_mn,
        int m_start, int m_end, int n_start, int n_end, int ldn, EW_ACTIVATION_TYPE act_type,
        int l2e_size_per_core = L2E_SIZE_PER_CORE, int l2r_size_per_core = L2R_SIZE_PER_CORE) {
    int cid = core_id();
    v16f32* l2r_base = (v16f32*)(cid * l2r_size_per_core);
    // need to make sure:
    //      max_mm * roundup16(max_nn) * sizeof(float) <= l2r_size_per_core
    //      case-1: max_nn = NBANKS, max_mm <= m_end - m_start
    //      case-2: max_nn > NBANKS, max_mm = m_end - m_start;
    int max_mm = l2r_size_per_core / 4 / NBANKS;
    max_mm = min(max_mm, m_end - m_start);
    int max_nn = l2r_size_per_core / 4 / max_mm;
    max_nn = rounddown16(max_nn);
    int max_nn_div16 = div16(max_nn);
    float matrix_max = 0.0f;
    for (int n_iter = n_start; n_iter < n_end; n_iter += max_nn) {
        int nn = min(n_end - n_iter, max_nn);
        int n_iter_end = n_iter + nn;
        for (int m_iter = m_start; m_iter < m_end; m_iter += max_mm) {
            int mm = min(m_end - m_iter, max_mm);
            int m_iter_end = m_iter + mm;
            matrix_max = rsrow_helper<SRC_SRAM, add_bias>(
                            src, &l1e_bias[n_iter / NBANKS], mm, nn, act_type, matrix_max,
                            l2e_size_per_core, l2r_size_per_core);
            dmaout_l2r_to_hbm(l2r_base, matrix_mn, m_iter, m_iter_end, n_iter, n_iter_end, ldn);
            src = &src[max_nn_div16 * mm];
        }
    }
    return matrix_max;
}

/**
 * get element offset in Tensor[axis3, axis2, axis1, axis0] with variable stride
 * case: Tensor[3, 4, 5, 6], after trans0213:[3, 5, 4, 6], index < 3*5:
 *      index = 0 -> offset = 0
 *      index = 1 -> offset = 6 * 1
 *      index = 5 -> offset = 6 * 5 + 6 * 1
 *      index = x -> offset = 6 * 5 * 4 * (x / 5) + 6 * (x % 5)
 *
 * origin Tensor shape is [axis3, axis2, axis1, axis0],
 * after transpose0213, the new Tensor shape is [axis3, axis1, axis2, axis0]
 */
static __device__ int get_offset(int axis3, int axis2, int axis1, int axis0, int index) {
    //assert 0 < index < axis3 * axis1
    //axis3 is bs0, axis2 is seq, axis1 is bs1 and axis0 is k
    return axis0 * axis1 * axis2 * (index / axis1) + axis0 * (index % axis1);
}

/**
 * get element offset in Variable Length Tensor[axis3, axis2, axis1, axis0] with VSL offset array
 *
 */
static __device__ int get_offset_vsl(int axis3, int axis2, int axis1, int axis0, int index,
        int start_axis2_pos) {
    //axis3 == bs0, axis2 == seq, axis1 == bs1 and axis0 == k
    //assert 0 < index < axis3 * axis1
    //assert for eatch i, 0 <= lm_vsl_offset_axis2[i] <= axis2
    return axis0 * axis1 * start_axis2_pos + axis0 * (index % axis1);
}

// get element offset of bias, for gemm_strided_batched_int16_tiny_softmax_*.xpu
static __device__ int get_bias_offset(int offset, int bias_format, int bs1, int m, int stride_c, int* bias_m) {
    int bias_offset = stride_c * offset; // default shape of bias is [bs0, bs1, m, n]
    if (bias_format == 1) { // for bias in tensorflow, shape is [bs0, 1, m, n]
        bias_offset = stride_c * (offset / bs1);
    } else if (bias_format == 2) { // for bias in onnx, shape is [bs0, 1, 1, n]
        bias_offset = stride_c * (offset / bs1) / m;
        *bias_m = 1;
    } else if (bias_format == 3) { // for bias in zhiyuan/xtcl, shape is [1, 1, m, n]
        bias_offset = 0;
    }
    return bias_offset;
}

// get global pointer of data from some parameters, used by batch_gemm_softmax/transpose0213
template <typename T>
static __device__ _global_ptr_ const T* get_g_cur_ptr(_global_ptr_ const T* data_ptr,
        int trans_0213, int bs0, int bs1, int m, int n, int offset) {
    int _off = 0;
    if (trans_0213) {
        _off = get_offset(bs0, m, bs1, n, offset);
    } else {
        _off = m * n * offset;
    }
    _global_ptr_ const T* g_cur_ptr = data_ptr + _off;
    return g_cur_ptr;
}

#endif
#endif
