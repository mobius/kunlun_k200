#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_GEMM_LIBS_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_GEMM_LIBS_H

#ifdef __xpu__

#include "xpu/kernel/gemm_libs_impl.h"
#include "xpu/kernel/findmax.h"
/*
 * ALL FUNCTIONS ARE STATIC TO REDUCE CODE SIZE.
 * THEY WILL BECOME INLINE WHEN USING -O2
 * */

// load matrix[m_start:m_end, n_start:n_end] in hbm, then do trans
template <typename SRC_TYPE, int DST_SRAM, bool TRANS, bool IN_PARALLEL = false>
static __device__ void load_matrix_int16(_global_ptr_ const SRC_TYPE* matrix, float max_val, int m_start, int m_end,
        int n_start, int n_end, int ld, v16i16* dst_ptr, int ncores = 1) {
    if (((DST_SRAM == DS_L1D) && (!TRANS)) || ((DST_SRAM == DS_L1W) && (TRANS))) {
        dma_shuffle_hbm_to_l1_int16<SRC_TYPE, DST_SRAM, IN_PARALLEL>(
                matrix, max_val, m_start, m_end, n_start, n_end, ld, dst_ptr, ncores);
        return;
    }
    if (((DST_SRAM == DS_L1D) && (TRANS)) || ((DST_SRAM == DS_L1W) && (!TRANS))) {
        dma_shuffle_coa_hbm_to_l1_int16<SRC_TYPE, DST_SRAM, IN_PARALLEL>(
                matrix, max_val, m_start, m_end, n_start, n_end, ld, dst_ptr, ncores);
        return;
    }
}

// load matrix[m_start:m_end, n_start:n_end] in hbm, then do trans
template <typename SRC_TYPE, int DST_SRAM, bool TRANS, bool IN_PARALLEL = false>
static __device__ void load_matrix_int8(_global_ptr_ const SRC_TYPE* matrix, float max_val, int m_start, int m_end,
        int n_start, int n_end, int ld, v32i8* dst_ptr, int ncores = 1) {
    if (((DST_SRAM == DS_L1D) && (!TRANS)) || ((DST_SRAM == DS_L1W) && (TRANS))) {
        dma_shuffle_hbm_to_l1_int8<SRC_TYPE, DST_SRAM, IN_PARALLEL>(
                matrix, max_val, m_start, m_end, n_start, n_end, ld, dst_ptr, ncores);
        return;
    }
    if (((DST_SRAM == DS_L1D) && (TRANS)) || ((DST_SRAM == DS_L1W) && (!TRANS))) {
        dma_shuffle_coa_hbm_to_l1_int8<SRC_TYPE, DST_SRAM, IN_PARALLEL>(
                matrix, max_val, m_start, m_end, n_start, n_end, ld, dst_ptr, ncores);
        return;
    }
}

// load matrix[m_start:m_end, n_start:n_end] in hbm, then do trans
template <typename SRC_TYPE, int DST_SRAM, bool TRANS, bool IN_PARALLEL = false>
static __device__ void load_matrix_int31(_global_ptr_ const SRC_TYPE* matrix, float max_val, int m_start, int m_end,
        int n_start, int n_end, int ld, v16i16* dst_ptr, int ncores = 1, int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    static_assert(xpu_std::is_same<SRC_TYPE, float>::value,
            "load_matrix_int31 only support SRC_TYPE == float");

    if ((l2dw_size_per_core < 0) || (l2dw_size_per_core * ncores > 2 * L2DW_SIZE_PER_CLUSTER)) {
        return;
    }
    if (((DST_SRAM == DS_L1D) && (!TRANS)) || ((DST_SRAM == DS_L1W) && (TRANS))) {
        dma_shuffle_hbm_to_l1_int31<DST_SRAM, IN_PARALLEL>(
                matrix, max_val, m_start, m_end, n_start, n_end, ld, dst_ptr, ncores, l2dw_size_per_core);
        return;
    }
    if (((DST_SRAM == DS_L1D) && (TRANS)) || ((DST_SRAM == DS_L1W) && (!TRANS))) {
        dma_shuffle_coa_hbm_to_l1_int31<DST_SRAM, IN_PARALLEL>(
                matrix, max_val, m_start, m_end, n_start, n_end, ld, dst_ptr, ncores, l2dw_size_per_core);
        return;
    }
}

template <typename SRC_TYPE>
static __device__ void load_matrix_l1e_fp32_notrans(_global_ptr_ const SRC_TYPE* matrix_mn,
        int m_start, int m_end, int n_start, int n_end, int ldn, v16f32* dst_ptr,
        int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    if ((l2dw_size_per_core < 0) || (l2dw_size_per_core > L2DW_SIZE_PER_CLUSTER)) {
        return;
    }
    dma_shuffle_coa_hbm_to_l1_fp32(matrix_mn, 0.0f,
            m_start, m_end, n_start, n_end, ldn, dst_ptr, l2dw_size_per_core);
}

template <typename SRC_TYPE>
static __device__ void load_matrix_l1e_fp32_trans(_global_ptr_ const SRC_TYPE* matrix_mn,
        int m_start, int m_end, int n_start, int n_end, int ldn, v16f32* dst_ptr,
        int l2dw_size_per_core = L2DW_SIZE_PER_CORE) {
    if ((l2dw_size_per_core < 0) || (l2dw_size_per_core > L2DW_SIZE_PER_CLUSTER)) {
        return;
    }
    dma_shuffle_hbm_to_l1_fp32(matrix_mn, 0.0f, m_start, m_end,
            n_start, n_end, ldn, dst_ptr, l2dw_size_per_core);
}

static __device__ float get_int16_dequant_scale(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x30800200;  // 1/(2^15-1)^2
    return max_a * max_b * magic_number;
}

static __device__ float get_int8_dequant_scale(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x38820610;  // 1/(2^7-1)^2
    return max_a * max_b * magic_number;
}

static __device__ float get_int31_ll_dequant_scale(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x21800000;  // 1/(2^30-1)^2
    return max_a * max_b * magic_number;
}

static __device__ float get_int31_hl_dequant_scale(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x29000000;  // 1/(2^30-1)^2 * 2^15
    return max_a * max_b * magic_number;
}

static __device__ float get_int31_hh_dequant_scale(float max_a, float max_b) {
    float magic_number = 0;
    *(int*)(&magic_number) = 0x30800000;  // 1/(2^30-1)^2 * 2^30
    return max_a * max_b * magic_number;
}

template <int I8_MODE>
float __device__ get_int4_dequant_scale(float max_a, float max_b) {
    constexpr const float l1d_scale = ((I8_MODE == MAC_INT8_D4W8) || (I8_MODE == MAC_INT8_D4W4)) ? 127.0f / 7.0f : 1.0f;
    constexpr const float l1w_scale = ((I8_MODE == MAC_INT8_D8W4) || (I8_MODE == MAC_INT8_D4W4)) ? 127.0f / 7.0f : 1.0f;
    float magic_number = 0.0f;
    *(int*)(&magic_number) = 0x38820610;  // 1/(2^7-1)^2
    return (max_a * l1d_scale) * (max_b * l1w_scale) * magic_number;
}

static __device__ void mac_int16(float dequant, int k, int mm, int nn,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {
    mac_int16_helper<false>(dequant, k, mm, nn, l1d, l1w, l1e);
}
static __device__ void mac_acc_int16(float dequant, int k, int mm, int nn,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {
    mac_int16_helper<true>(dequant, k, mm, nn, l1d, l1w, l1e);
}

static __device__ void mac_int8(float dequant, int k, int mm, int nn,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {
    mac_int8_helper<false>(dequant, k, mm, nn, l1d, l1w, l1e);
}
static __device__ void mac_acc_int8(float dequant, int k, int mm, int nn,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {
    mac_int8_helper<true>(dequant, k, mm, nn, l1d, l1w, l1e);
}

static __device__ void mac_int31(float dequant_ll, float dequant_hl, float dequant_hh, int k, int mm, int nn,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {
    mac_int31_helper<false>(dequant_ll, dequant_hl, dequant_hh, k, mm, nn, l1d, l1w, l1e);
}
static __device__ void mac_acc_int31(float dequant_ll, float dequant_hl, float dequant_hh, int k, int mm, int nn,
        v16i16* l1d, v16i16* l1w, v16f32* l1e) {
    mac_int31_helper<true>(dequant_ll, dequant_hl, dequant_hh, k, mm, nn, l1d, l1w, l1e);
}

template <int I8_MODE>
static __device__ void mac_int4(float dequant, int k, int mm, int nn,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {
    mac_int4_helper<false, I8_MODE>(dequant, k, mm, nn, l1d, l1w, l1e);
}

template <int I8_MODE>
static __device__ void mac_acc_int4(float dequant, int k, int mm, int nn,
        v32i8* l1d, v32i8* l1w, v16f32* l1e) {
    mac_int4_helper<true, I8_MODE>(dequant, k, mm, nn, l1d, l1w, l1e);
}
// activation(alpha * A + beta * B + gamma)
// alpha, beta, gamma can be scalar or vector
// ALPHA_T, BETA_T, GAMMA_T can be float or v16f32*
template <typename ALPHA_T, typename BETA_T, typename GAMMA_T>
static __device__ void ew_matrix_add(int mm, int nn,
        ALPHA_T alpha, v16f32* A, BETA_T beta, v16f32* B, GAMMA_T gamma,
        EW_ACTIVATION_TYPE act_type, v16f32* result) {
    ew_matrix_helper<true, ALPHA_T, BETA_T, GAMMA_T>(
            mm, nn, alpha, A, beta, B, gamma, act_type, result);
}

// activation((alpha + A) * (beta + B) * gamma)
// alpha, beta, gamma can be scalar or vector
// ALPHA_T, BETA_T, GAMMA_T can be float or v16f32*
template <typename ALPHA_T, typename BETA_T, typename GAMMA_T>
static __device__ void ew_matrix_mul(int mm, int nn,
        ALPHA_T alpha, v16f32* A, BETA_T beta, v16f32* B, GAMMA_T gamma,
        EW_ACTIVATION_TYPE act_type, v16f32* result) {
    ew_matrix_helper<false, ALPHA_T, BETA_T, GAMMA_T>(
            mm, nn, alpha, A, beta, B, gamma, act_type, result);
}

// NOTrans
template <typename DST_TYPE>
static __device__ float store_matrix_l1e_notrans(v16f32* l1e, _global_ptr_ DST_TYPE* matrix_mn,
        int m_start, int m_end, int n_start, int n_end, int ldn,
        EW_ACTIVATION_TYPE act_type = EW_NOACT, int l2e_size_per_core = L2E_SIZE_PER_CORE,
        int l2r_size_per_core = L2R_SIZE_PER_CORE) {
    return rsrow_l1e_or_l2e_to_hbm<EW_L1E, false, DST_TYPE>(
                    l1e, 0, matrix_mn, m_start, m_end, n_start, n_end, ldn, act_type,
                    l2e_size_per_core, l2r_size_per_core);
}

template <typename DST_TYPE>
static __device__ float store_matrix_l1e_notrans_with_bias(v16f32* l1e, v16f32* l1e_bias,
        _global_ptr_ DST_TYPE* matrix_mn, int m_start, int m_end, int n_start, int n_end, int ldn,
        EW_ACTIVATION_TYPE act_type = EW_NOACT, int l2e_size_per_core = L2E_SIZE_PER_CORE,
        int l2r_size_per_core = L2R_SIZE_PER_CORE) {
    return rsrow_l1e_or_l2e_to_hbm<EW_L1E, true, DST_TYPE>(
                    l1e, l1e_bias, matrix_mn, m_start, m_end, n_start, n_end, ldn, act_type,
                    l2e_size_per_core, l2r_size_per_core);
}

template <typename DST_TYPE>
static __device__ void store_matrix_l2e_notrans(v16f32* l2e, _global_ptr_ DST_TYPE* matrix_mn,
        int m_start, int m_end, int n_start, int n_end, int ldn, int l2e_size_per_core = L2E_SIZE_PER_CORE,
        int l2r_size_per_core = L2R_SIZE_PER_CORE) {
    rsrow_l1e_or_l2e_to_hbm<EW_L2E, false, DST_TYPE>(
            l2e, 0, matrix_mn, m_start, m_end, n_start, n_end, ldn, EW_NOACT,
            l2e_size_per_core, l2r_size_per_core);
}

// Trans
template <typename DST_TYPE>
static __device__ float store_matrix_l1e_trans(v16f32* l1e, _global_ptr_ DST_TYPE* matrix_nm,
        int n_start, int n_end, int m_start, int m_end, int ldm,
        EW_ACTIVATION_TYPE act_type = EW_NOACT, int l2e_size_per_core = L2E_SIZE_PER_CORE,
        int l2r_size_per_core = L2R_SIZE_PER_CORE) {
    return rscol_l1e_or_l2e_to_hbm<EW_L1E, false, DST_TYPE>(
                    l1e, 0, matrix_nm, n_start, n_end, m_start, m_end, ldm, act_type,
                    l2e_size_per_core, l2r_size_per_core);
}

template <typename DST_TYPE>
static __device__ float store_matrix_l1e_trans_with_bias(v16f32* l1e, v16f32* l1e_bias,
        _global_ptr_ DST_TYPE* matrix_nm, int n_start, int n_end, int m_start, int m_end, int ldm,
        EW_ACTIVATION_TYPE act_type = EW_NOACT, int l2e_size_per_core = L2E_SIZE_PER_CORE,
        int l2r_size_per_core = L2R_SIZE_PER_CORE) {
    return rscol_l1e_or_l2e_to_hbm<EW_L1E, true, DST_TYPE>(
                    l1e, l1e_bias, matrix_nm, n_start, n_end, m_start, m_end, ldm, act_type,
                    l2e_size_per_core, l2r_size_per_core);
}

template <typename DST_TYPE>
static __device__ void store_matrix_l2e_trans(v16f32* l2e, _global_ptr_ DST_TYPE* matrix_nm,
        int n_start, int n_end, int m_start, int m_end, int ldm, int l2e_size_per_core = L2E_SIZE_PER_CORE,
        int l2r_size_per_core = L2R_SIZE_PER_CORE) {
    rscol_l1e_or_l2e_to_hbm<EW_L2E, false, DST_TYPE>(
            l2e, 0, matrix_nm, n_start, n_end, m_start, m_end, ldm, EW_NOACT,
            l2e_size_per_core, l2r_size_per_core);
}

// packed_id {
//      param       :       19bit   :   (packed_id >> 13)
//      table_type  :       4bit    :   (packed_id >> 9) & 0xf
//      table_mode  :       3bit    :   (packed_id >> 6) & 0x7
//      table_acc   :       2bit    :   (packed_id >> 4) & 0x3
//      act_type    :       4bit    :   packed_id & 0xf
// }

// param is not used by kernel
// table_type is not used by kernel
static __device__ EW_TABLE_MODE unpack_ew_table_mode(int packed_id) {
    return (EW_TABLE_MODE)((packed_id >> 6) & 0x7) ;
}
static __device__ EW_TABLE_ACCURACY unpack_ew_table_accuracy(int packed_id) {
    return (EW_TABLE_ACCURACY)((packed_id >> 4) & 0x3) ;
}
static __device__ EW_ACTIVATION_TYPE unpack_ew_act_type(int packed_id) {
    return (EW_ACTIVATION_TYPE)(packed_id & 0xf);
}
static __device__ int unpack_ew_table_id(int packed_id) {
    // (param, table_type, table_mode, table_acc) -> unique table id
    return (packed_id >> 4);
}

static __device__ void load_ewtable2(_global_ptr_ const float* ewtable, int packed_id) {
    if ((packed_id == EW_NOACT) || (packed_id == EW_RELU)) {
        return;
    }
    const int k_lens = 512;
    const int b_lens = 512;
    int cid = core_id();
    float* l2 = (float*) 0;
    v16f32* l1e = (v16f32*) 0;
    // BUGFIX: disable "talbe_id related logic" to avoid bugs about different version of ewtable
    // TODO: use talbe_id again
    int table_id = unpack_ew_table_id(packed_id);
    // if (cid == 0) {
    //     xfence_lock(EW);
    //     float old_id_float = 0.0f;
    //     // ew_ldtableid will call __builtin_xpu_ld_sd, the return type is float, but the actual data-type is int
    //     // so we need some trick to get real value
    //     ew_ldtableid(old_id_float);
    //     xfence_unlock(EW);
    //     int old_id = *(reinterpret_cast<int*>(&old_id_float));
    //     // Do not need to load table
    //     if (old_id == table_id) {
    //         sync();
    //         return;
    //     }
    // }
    // end of BUGFIX
    if (cid == 0) {
        EW_TABLE_MODE table_mode = unpack_ew_table_mode(packed_id);
        EW_TABLE_ACCURACY accuracy = unpack_ew_table_accuracy(packed_id);
        float* l2dw = (float*)((cid >> 1) * L2DW_SIZE_PER_CORE);
        dma_matrix_hbm_to_l2<float, float>(ewtable, 0.0f, 0, 1, 0, k_lens + b_lens, k_lens + b_lens, l2dw);
        xfence_lock(DS_0);
        ds_shuffle_cfg_datatype(DS_FP32);
        // EW require all data in NBANKS banks are the same
        // use ds_shuffle to duplicate (k_lens + b_lens) float to (k_lens + b_lens) rows
        for (int j = 0; j < NBANKS; j++) {
            ds_shuffle(&l1e[0][j], l2, k_lens + b_lens, DS_L1E);
        }
        xfence_unlock(DS_0);
        xfence_lock(EW);
        // set to EW_NOACT before change ewtable_mode
        // otherwise may activation_type may conflict with ewtable_mode
        ew_cfg_activation_type(EW_NOACT);
        // BUGFIX
        // ew_cfg_ewtable(table_mode, accuracy, table_id);
        ew_cfg_ewtable(table_mode, accuracy, 0);
        // end of BUGFIX
        ew_cfg_k_table(l1e);            // first k_lens row belongs to table-k
        ew_cfg_b_table(&l1e[k_lens]);   // next b_lens row belongs to table-b
        xfence_unlock(EW);
    }
    sync();
}

#endif
#endif
