#ifndef BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_FINDMAX_H
#define BAIDU_XPU_API_SRC_KERNEL_INCLUDE_XPU_KERNEL_FINDMAX_H

#ifdef __xpu__
#include "xpu/kernel/gemm_libs_impl.h"
#include "xpu/kernel/cluster1_type.h"

static __device__ float get_cluster_max(float core_max, int ncores) {
    int cid = core_id();
    int max_core_num = 8;
    const int lm_align = 8;
    __local__ float local_array[max_core_num * lm_align];
    local_array[cid * lm_align] = core_max;
    float* sm_ptr = (float*) 0;
    LM2SM(&local_array[cid * lm_align], &sm_ptr[cid * lm_align], lm_align * sizeof(float));
    sync();
    SM2LM(sm_ptr, local_array, ncores * lm_align * sizeof(float));
    for (int i = 1; i < ncores; i++) {
        local_array[0] = fmax(local_array[0], local_array[i * lm_align]);
    }
    return local_array[0];
}

/**
 * findmax with one core
 */
template <typename SRC_TYPE>
static __device__ float findmax_one_core(_global_ptr_ const SRC_TYPE* ptr, int size, int dma_id = 0) {
    float core_max = 0.0f;
    SDNN_DEVICE dev = (dma_id == 0) ? DMAIN_0 : DMAIN_1;
    DMA_DATA_POSITION pos = (dma_id == 0) ? DMA_L2_0 : DMA_L2_1;
    xfence_lock(dev);
    dma_cfg((pos | DMA_FP32), (DMA_GM | dma_cfg<SRC_TYPE>::value | highptr(ptr)), 0.0f);
    dma_findmax(lowptr(ptr), size * sizeof(SRC_TYPE));
    xfence();
    if (dma_id == 0) {
        dma0_ldmax(core_max);
    } else {
        dma1_ldmax(core_max);
    }
    xfence_unlock(dev);
    return core_max;
}

template <typename SRC_TYPE>
static __device__ float findmax_multi_core(_global_ptr_ const SRC_TYPE* ptr, int size, int ncores) {
    float core_max = 0.0f;
    int cid = core_id();
    SDNN_DEVICE dev = ((cid & 1) == 0) ? DMAIN_0 : DMAIN_1;
    DMA_DATA_POSITION pos = ((cid & 1) == 0) ? DMA_L2_0 : DMA_L2_1;
    int size_percore = roundup128(size) / 2;
    size_percore = min(size_percore, 2 * 1024 * 1024); // DMAIN has bugs when readdata >= 16MB
    for (int start = cid * size_percore; start < size; start += ncores * size_percore) {
        int readsize = min(size_percore, size - start);
        float tmp = 0.0f;
        xfence_lock(dev);
        dma_cfg((pos | DMA_FP32), (DMA_GM | dma_cfg<SRC_TYPE>::value | highptr(ptr)), 0.0f);
        dma_findmax(lowptr(ptr + start), readsize * sizeof(SRC_TYPE));
        xfence();
        if ((cid & 1) == 0) {
            dma0_ldmax(tmp);
        } else {
            dma1_ldmax(tmp);
        }
        xfence_unlock(dev);
        core_max = fmax(core_max, tmp);
    }
    return get_cluster_max(core_max, ncores);
}

/**
 * findmax 2d with one core
 */
template <typename SRC_TYPE>
static __device__ float findmax_2d_one_core(_global_ptr_ const SRC_TYPE* ptr, int m, int n, int ldn, int dma_id = 0) {
    if (n == ldn) {
        return findmax_one_core(ptr, m * n, dma_id);
    }
    float core_max = 0.0f;
    SDNN_DEVICE dev = (dma_id == 0) ? DMAIN_0 : DMAIN_1;
    DMA_DATA_POSITION pos = (dma_id == 0) ? DMA_L2_0 : DMA_L2_1;
    xfence_lock(dev);
    dma_cfg((pos | DMA_FP32), (DMA_GM | dma_cfg<SRC_TYPE>::value | highptr(ptr)), 0.0f);
    dma_cfg_2d(m, 0, ldn * sizeof(SRC_TYPE));
    dma_findmax_2d(lowptr(ptr), n * sizeof(SRC_TYPE));
    xfence();
    if (dma_id == 0) {
        dma0_ldmax(core_max);
    } else {
        dma1_ldmax(core_max);
    }
    xfence_unlock(dev);
    return core_max;
}

template <typename SRC_TYPE>
static __device__ float findmax_2d_multi_core(_global_ptr_ const SRC_TYPE* ptr, int m, int n, int ldn, int ncores) {
    if (n == ldn) {
        return findmax_multi_core(ptr, m * n, ncores);
    }
    float core_max = 0.0f;
    int size_percore = roundup_div(m, min(ncores, 2));
    int start = core_id() * size_percore;
    int end = min(start + size_percore, m);
    if (start < end) {
        core_max = findmax_2d_one_core(ptr + start * ldn, end - start, n, ldn, core_id() % 2);
    }
    return get_cluster_max(core_max, ncores);
}

template<typename data_type>
static __device__ float findmax_when_necessary(_global_ptr_ const data_type* data,
        float max_data, int len, int ncores) {
    if (max_data != 0.0f) {
        return max_data;
    }
    return findmax_multi_core(data, len, ncores);
}

template<>
__device__ float findmax_when_necessary<int16_t>(_global_ptr_ const int16_t* data,
        float max_data, int len, int ncores) {
    return max_data;
}

template<>
__device__ float findmax_when_necessary<int8_t>(_global_ptr_ const int8_t* data,
        float max_data, int len, int ncores) {
    return max_data;
}

template<typename data_type>
static __device__ float findmax_when_necessary(_global_ptr_ const data_type* data,
        _global_ptr_ const float* max_ptr, int len, int ncores) {
    if (max_ptr != nullptr) {
        __local__ float lm_max[8];
        GM2LM(max_ptr, lm_max, 4 * sizeof(float)); // at most 4 clusters
        float tmp0 = fmax(lm_max[0], lm_max[1]);
        float tmp1 = fmax(lm_max[2], lm_max[3]);
        return fmax(tmp0, tmp1);
    }
    return findmax_when_necessary(data, 0.0f, len, ncores);
}


template<typename data_type>
static __device__ float findmax_2d_when_necessary(_global_ptr_ const data_type* data, float max_data,
        int m, int n, int ldn, int ncores) {
    if (max_data != 0.0f) {
        return max_data;
    }
    return findmax_2d_multi_core(data, m, n, ldn, ncores);
}

template<>
__device__ float findmax_2d_when_necessary<int16_t>(_global_ptr_ const int16_t* data, float max_data,
        int m, int n, int ldn, int ncores) {
    return max_data;
}

template<>
__device__ float findmax_2d_when_necessary<int8_t>(_global_ptr_ const int8_t* data, float max_data,
        int m, int n, int ldn, int ncores) {
    return max_data;
}

template<typename data_type>
static __device__ float findmax_2d_when_necessary(_global_ptr_ const data_type* data, _global_ptr_ const float* max_ptr,
        int m, int n, int ldn, int ncores) {
    if (max_ptr != nullptr) {
        __local__ float lm_max[8];
        GM2LM(max_ptr, lm_max, 4 * sizeof(float)); // at most 4 clusters
        float tmp0 = fmax(lm_max[0], lm_max[1]);
        float tmp1 = fmax(lm_max[2], lm_max[3]);
        return fmax(tmp0, tmp1);
    }
    return findmax_2d_when_necessary(data, 0.0f, m, n, ldn, ncores);
}

template <int is_output, typename T>
static __device__ float readmax_one_core(_global_ptr_ const float* max_ptr) {
    __local__ float lm_max[8];
    GM2LM(max_ptr, lm_max, 4 * sizeof(float)); // at most 4 clusters
    float tmp0 = fmax(lm_max[0], lm_max[1]);
    float tmp1 = fmax(lm_max[2], lm_max[3]);
    return fmax(tmp0, tmp1);
}

template <> __device__ float readmax_one_core<1, float>(_global_ptr_ const float* max_ptr) {
    return 0.0f;
}

template <> __device__ float readmax_one_core<1, float16>(_global_ptr_ const float* max_ptr) {
    return 0.0f;
}

template <typename T>
static __device__ void writemax_when_necessary_one_core(_global_ptr_ float* max_ptr, float max_val) {
    if (max_ptr != nullptr) {
        __local__ float max_lm[8];
        int write_len = 1;
        if (cluster_id() == cluster_num() - 1) {
            write_len = 4 - cluster_id();
        }
        max_ptr = max_ptr + cluster_id();
        if ((((int)((long long)max_ptr)) & 1) != 0) {  // this is not first writemax
            max_ptr = (_global_ptr_ float*)((_global_ptr_ char*) max_ptr - 1);
            GM2LM(max_ptr, max_lm, write_len * sizeof(float));
            for (int i = 0; i < write_len; i++) {
                max_val = fmax(max_val, max_lm[i]);
            }
        }
        for (int i = 0; i < write_len; i++) {
            max_lm[i] = max_val;
        }
        LM2GM(max_lm, max_ptr, write_len * sizeof(float));
    }
}

template <> __device__ void writemax_when_necessary_one_core<int16_t>(_global_ptr_ float* max_ptr, float max_val) {
}

template <> __device__ void writemax_when_necessary_one_core<int8_t>(_global_ptr_ float* max_ptr, float max_val) {
}

static __device__ void writemax_when_necessary(_global_ptr_ float* max_ptr, float max_val, int ncores) {
    if (max_ptr != nullptr) {
        max_val = get_cluster_max(max_val, ncores);
        if (core_id() == 0) {
            writemax_when_necessary_one_core<float>(max_ptr, max_val);
        }
    }
}

template<typename result_type>
static __device__ float merge_output_max(float global_max, float local_max) {
    return fmax(global_max, local_max);
}

template<> __device__ float merge_output_max<int16_t>(float global_max, float local_max) {
    return global_max;
}

template<> __device__ float merge_output_max<int8_t>(float global_max, float local_max) {
    return global_max;
}
#endif
#endif
