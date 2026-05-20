#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_PROFILING_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_PROFILING_H
#include <cstdint>
#include <cstddef>
#include "xpu/dll_export.h"
#include "xpu/refactor/core/dtype.h"
#include "xpu/refactor/context/newcontext.h"
#include "xpu/refactor/impl_public/xdnn_debug.h"
namespace baidu {
namespace xpu {
namespace api {

enum class KernelType { CLUSTER, SDNN };

std::string kernel_type_str(KernelType type);

struct profiling_result {
    int64_t total_bytes {0L};
    double equivalent_efficiency {0.0f};
    double dma_gbps {0.0f};
    double mac_tops {0.0f};
    double ew_op_per_cycle {0.0f};
    double mac_efficiency {0.0f};
    double ew_efficiency {0.0f};
    profiling_result() = default;
};

// ignore read/write when its ratio is below this threshold
static double gm_read_write_ratio_threshold = 0.1;

struct profiling_data {
    size_t total_mac_op_cnt {0UL};
    size_t total_ew_op_cnt {0UL};
    Dtype mac_data_type {kFLOAT32};
    Dtype ew_data_type {kFLOAT32};
    size_t l3_read_bytes {0UL};
    size_t l3_write_bytes {0UL};
    size_t gm_read_bytes {0UL};
    size_t gm_write_bytes {0UL};
    profiling_data() = default;
    void set_mac_stat(size_t op_cnt, Dtype data_type) {
        total_mac_op_cnt = op_cnt;
        mac_data_type = data_type;
    }
    void set_ew_stat(size_t op_cnt, Dtype data_type) {
        total_ew_op_cnt = op_cnt;
        ew_data_type = data_type;
    }
    template<typename T>
    void accumulate_bytes(const T* ptr, size_t len = 1, size_t count = 1) {
        size_t bytes = count * len * sizeof(T);
        if (is_gm(ptr)) {
            gm_read_bytes += bytes;
        } else {
            l3_read_bytes += bytes;
        }
    }
    template<typename T>
    void accumulate_bytes(T* ptr, size_t len = 1, size_t count = 1) {
        size_t bytes = count * len * sizeof(T);
        if (is_gm(ptr)) {
            gm_write_bytes += bytes;
        } else {
            l3_write_bytes += bytes;
        }
    }
    size_t total_bytes() {
        size_t sum = l3_read_bytes + l3_write_bytes + gm_read_bytes + gm_write_bytes;
        return sum;
    }
    profiling_result calculate_profiling_result(Context* ctx, KernelType type, double real_time_ns) const;
};

class XpuPerfTimer;
class DLL_EXPORT ProfilingKernelGuard {
    private:
        XpuPerfTimer* _timer_ptr;
        KernelType _kernel_type = KernelType::CLUSTER;
        Context* _ctx;
        profiling_data _data;
    public:
        ProfilingKernelGuard(const ProfilingKernelGuard&) = delete;
        ProfilingKernelGuard& operator=(const ProfilingKernelGuard&) = delete;
        ProfilingKernelGuard(Context* ctx);
        void start_profiling(const std::string& name);
        void set_sdnn_kernel();
        void set_cluster_kernel();
        void end_profiling();
        ~ProfilingKernelGuard();
        template<typename F, typename... Args>
        void calculate_profiling_data(const F& calc_func, Args... args) {
            _data = calc_func(args...);
        }
};

#define PROFILING_KERNEL_START(ctx, kernel_type, kernel_name) \
baidu::xpu::api::ProfilingKernelGuard _internal_profiling_kernel_guard(ctx); \
if (debug_any_enable(ctx)) { \
    if (baidu::xpu::api::KernelType::kernel_type == \
        baidu::xpu::api::KernelType::CLUSTER) { \
        _internal_profiling_kernel_guard.set_cluster_kernel(); \
    } else { \
        _internal_profiling_kernel_guard.set_sdnn_kernel(); \
    } \
    _internal_profiling_kernel_guard.start_profiling(kernel_name); \
}

#define PROFILING_CALCULATE_PROFILING_DATA(ctx, func, ...) \
if (debug_any_enable(ctx)) { \
    _internal_profiling_kernel_guard.calculate_profiling_data(func, __VA_ARGS__); \
}

#define PROFILING_KERNEL_END(ctx) \
if (debug_any_enable(ctx)) { \
    _internal_profiling_kernel_guard.end_profiling(); \
}

}
}
}

#endif
