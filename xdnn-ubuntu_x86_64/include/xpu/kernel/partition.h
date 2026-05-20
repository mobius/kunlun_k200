#ifndef __XPU_KERNEL_PARTITION_H
#define __XPU_KERNEL_PARTITION_H

#include "xpu/kernel/math.h"
static __device__ inline void partition(int tid, int nthreads, int len, int align, int* start, int* end) {
    int block_cnt = roundup_div(len, align);
    int remain_block = block_cnt % nthreads;
    int start_block = block_cnt / nthreads * tid + min(tid, remain_block);
    int end_block = start_block + block_cnt / nthreads + (tid < remain_block);
    *start = min(start_block * align, len);
    *end = min(end_block * align, len);
}

// refer to http://agroup.baidu.com/kunlun_software_doc/md/article/4168552 for more detail
static __device__ inline float _partition2d_nfactor(int nn, float dmaburst) {
    return dmaburst / fmin(static_cast<float>(nn), dmaburst);
}
static __device__ inline float _partition2d_mfactor(int mm, float reuse_factor) {
    return (static_cast<float>(mm) + reuse_factor) / (static_cast<float>(mm) * (1.0f + reuse_factor));
}
static __device__ inline float _partition2d_score(int nthreads, int m, int n, int mm, float reuse_factor,
        float dmaburst) {
    int row = roundup_div(m, mm);
    int col = nthreads / row;
    int nn = roundup_div(n, col);
    float fair_factor = mm * nn * static_cast<float>(nthreads) / static_cast<float>(m * n);
    float n_factor = _partition2d_nfactor(nn, dmaburst);
    float m_factor = _partition2d_mfactor(mm, reuse_factor);
    return fair_factor * n_factor * m_factor;
}

static __device__ inline void partition2d(int tid, int nthreads, int m, int n, int* mstart, int* mend,
        int* nstart, int* nend, float reuse_factor = 0.0f, float dmaburst = 512.0f) {
    if (m == 1) {
        *mstart = 0;
        *mend = 1;
        partition(tid, nthreads, n, 16, nstart, nend);
        return;
    }
    partition(tid, nthreads, m, 1, mstart, mend);
    *nstart = 0;
    *nend = n;
    int mm_first = roundup_div(m, nthreads);
    int mm_last = min(m, 4);
    if (mm_first >= mm_last) {
        return;
    }
    int best_mm = mm_first;
    float best_score = _partition2d_score(nthreads, m, n, best_mm, reuse_factor, dmaburst);
    for (int mm = mm_first + 1; mm <= mm_last; mm++) {
        float curr_score = _partition2d_score(nthreads, m, n, mm, reuse_factor, dmaburst);
        if (curr_score < best_score) {
            best_score = curr_score;
            best_mm = mm;
        }
    }
    int row = roundup_div(m, best_mm);
    int col = nthreads / row;
    int best_nn = roundup_div(n, col);
    if (tid >= row * col) { // idle cores
        *mstart = m;
        *mend = m;
        *nstart = n;
        *nend = n;
    } else {
        int rowid = tid / col;
        int colid = tid % col;
        *mstart = min(rowid * best_mm, m);
        *mend = min((rowid + 1) * best_mm, m);
        *nstart = min(colid * best_nn, n);
        *nend = min((colid + 1) * best_nn, n);
    }
}

#endif
