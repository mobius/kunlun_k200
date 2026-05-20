#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_PARTITION_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_CLUSTER2_PARTITION_H
#include "xpu/kernel/cluster.h"

template<typename TID>
__device__ TID roundup_div_p(TID a, TID b) {
    return (a + b - 1) / b;
}

template<typename T> 
__device__ T min_p(T a, T b){
    return a < b ? a : b;
}

// 一维分块策略
template<typename TID>
static __device__ inline void partition(int tid, int nthreads, TID len, int align, TID* start, TID* end) {
    TID block_cnt = roundup_div_p<TID>(len, align);
    TID remain_block = block_cnt % nthreads;
    TID start_block = block_cnt / nthreads * static_cast<TID>(tid) + min_p<TID>(tid, remain_block);
    TID end_block = start_block + block_cnt / nthreads + (tid < remain_block);
    *start = min_p<TID>(start_block * align, len);
    *end = min_p<TID>(end_block * align, len);
}

// 一维分块策略 2_level_align
// |---------------|-------|----------------| <-origin data
// |ps           pe|ps   pe|ps            pe| <-partition
// |  align_block  |#ab %aB|   align_block  | <-length
// #aB->align_boundary, #ab->align_block
template<typename TID>
static __device__ inline void partition(int tid, int nthreads, TID len, int align_block, TID* start, TID* end,
        int align_boundary) {
    TID large_block_cnt = roundup_div_p<TID>(len, align_boundary);
    TID small_block_cnt = roundup_div_p<TID>(align_boundary, align_block);
    TID block_cnt = small_block_cnt * large_block_cnt;
    TID remain_block = block_cnt % nthreads;
    TID start_block = block_cnt / nthreads * static_cast<TID>(tid) + min_p<TID>(tid, remain_block);
    TID end_block = start_block + block_cnt / nthreads + (tid < remain_block);
    *start = min_p<TID>(start_block / small_block_cnt * align_boundary + start_block % small_block_cnt * align_block, len);
    *end = min_p<TID>(end_block / small_block_cnt * align_boundary + end_block % small_block_cnt * align_block, len);
}

// 软件分组策略
// http://agroup.baidu.com/kunlun_software_doc/md/article/4860471
static __device__ inline void soft_grouping(int cid, int ncores, int ncores_per_group, int* gid_ptr, int* ngroups_ptr,
        int* id_inside_group_ptr) {
    *ngroups_ptr = ncores / ncores_per_group;
    *gid_ptr = cid % *ngroups_ptr;
    *id_inside_group_ptr = cid / *ngroups_ptr;
}
static __device__ inline void soft_group_sync(int gid, int ngroups) {
    if (ngroups == 64) {
        return;
    }
    sync_local();
    if (ngroups >= 16) {
        return;
    }
    int bitmap_array[9];
    bitmap_array[8] = 0x0101;
    bitmap_array[4] = 0x1111;
    bitmap_array[2] = 0x5555;
    bitmap_array[1] = 0xFFFF;
    sync_group((bitmap_array[ngroups] << gid));
}

// refer to http://agroup.baidu.com/kunlun_software_doc/md/article/4168552 for more detail
template<typename TID>
static __device__ inline float _partition2d_nfactor(TID nn, float dmaburst) {
    return dmaburst / fmin(static_cast<float>(nn), dmaburst);
}

template<typename TID>
static __device__ inline float _partition2d_mfactor(TID mm, float reuse_factor) {
    return (static_cast<float>(mm) + reuse_factor) / (static_cast<float>(mm) * (1.0f + reuse_factor));
}

template<typename TID>
static __device__ inline float _partition2d_score(int nthreads, TID m, TID n, TID mm, float reuse_factor,
        float dmaburst) {
    TID row = roundup_div_p<TID>(m, mm);
    TID col = static_cast<TID>(nthreads) / row;
    TID nn = roundup_div_p<TID>(n, col);
    float fair_factor = mm * nn * static_cast<float>(nthreads) / static_cast<float>(m * n);
    float n_factor = _partition2d_nfactor<TID>(nn, dmaburst);
    float m_factor = _partition2d_mfactor<TID>(mm, reuse_factor);
    return fair_factor * n_factor * m_factor;
}

template<typename TID>
static __device__ inline void partition2d(int tid, int nthreads, TID m, TID n, TID* mstart, TID* mend,
        TID* nstart, TID* nend, float reuse_factor = 0.0f, float dmaburst = 512.0f) {
    if (m == 1) {
        *mstart = 0;
        *mend = 1;
        partition(tid, nthreads, n, 16, nstart, nend);
        return;
    }
    partition(tid, nthreads, m, 1, mstart, mend);
    *nstart = 0;
    *nend = n;
    TID mm_first = roundup_div_p<TID>(m, nthreads);
    TID mm_last = min_p<TID>(m, 8);
    if (mm_first >= mm_last) {
        return;
    }
    TID best_mm = mm_first;
    float best_score = _partition2d_score(nthreads, m, n, best_mm, reuse_factor, dmaburst);
    for (TID mm = mm_first + 1; mm <= mm_last; mm++) {
        float curr_score = _partition2d_score(nthreads, m, n, mm, reuse_factor, dmaburst);
        if (curr_score < best_score) {
            best_score = curr_score;
            best_mm = mm;
        }
    }
    TID row = roundup_div_p<TID>(m, best_mm);
    TID col = nthreads / row;
    TID best_nn = roundup_div_p<TID>(n, col);
    if (tid >= row * col) { // idle cores
        *mstart = m;
        *mend = m;
        *nstart = n;
        *nend = n;
    } else {
        TID rowid = static_cast<TID>(tid) / col;
        TID colid = static_cast<TID>(tid)  % col;
        *mstart = min_p<TID>(rowid * best_mm, m);
        *mend = min_p<TID>((rowid + 1) * best_mm, m);
        *nstart = min_p<TID>(colid * best_nn, n);
        *nend = min_p<TID>((colid + 1) * best_nn, n);
    }
}

#endif
