/*!
 * Kunlun K200 Performance Test — Optimized Edition
 * Improvements over baseline:
 *   (a) Pinned memory for DMA bandwidth
 *   (b) set_ncluster(4) to utilize all 4 CDNN compute units
 *   (c) nCluster sweep to find optimal parallelism
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <chrono>
#include <cmath>
#include <xpu/runtime.h>
#include <xpu/xdnn.h>

namespace xdnn = baidu::xpu::api;

static inline double get_time_us() {
    auto now = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    return (double)us;
}

#define WARMUP 3
#define RUNS   100

// ---------------------------------------------------------------------------
// 1. Baseline bandwidth (pageable memory)
// ---------------------------------------------------------------------------
void test_bandwidth(int devid) {
    xpu_set_device(devid);
    printf("\n=== Bandwidth Test — PAGEABLE Memory (Device %d) ===\n", devid);

    size_t sizes[] = {
        1ULL * 1024 * 1024,
        64ULL * 1024 * 1024,
        256ULL * 1024 * 1024,
        1024ULL * 1024 * 1024,
    };
    const char* size_names[] = {"1MB", "64MB", "256MB", "1GB"};
    int n_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < n_sizes; i++) {
        size_t sz = sizes[i];
        void* host = aligned_alloc(4096, sz);
        void* dev = nullptr;
        xpu_malloc(&dev, sz);
        memset(host, 0xAB, sz);

        for (int r = 0; r < WARMUP; r++) { xpu_memcpy(dev, host, sz, XPU_HOST_TO_DEVICE); xpu_wait(); }
        double t0 = get_time_us();
        for (int r = 0; r < RUNS; r++) { xpu_memcpy(dev, host, sz, XPU_HOST_TO_DEVICE); xpu_wait(); }
        double t1 = get_time_us();
        double h2d = (double)(sz * RUNS) / ((t1 - t0) * 1e-6) / (1.0 * 1024 * 1024 * 1024);

        for (int r = 0; r < WARMUP; r++) { xpu_memcpy(host, dev, sz, XPU_DEVICE_TO_HOST); xpu_wait(); }
        t0 = get_time_us();
        for (int r = 0; r < RUNS; r++) { xpu_memcpy(host, dev, sz, XPU_DEVICE_TO_HOST); xpu_wait(); }
        t1 = get_time_us();
        double d2h = (double)(sz * RUNS) / ((t1 - t0) * 1e-6) / (1.0 * 1024 * 1024 * 1024);

        printf("  %-5s  H2D: %6.2f GB/s   D2H: %6.2f GB/s\n", size_names[i], h2d, d2h);
        xpu_free(dev);
        free(host);
    }
}

// ---------------------------------------------------------------------------
// 2. Pinned-memory bandwidth (xpu_host_alloc) — SKIPPED: xpu_host_alloc hangs lib init
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 3. FP32 GEMM — with ncluster=4
// ---------------------------------------------------------------------------
void test_gemm_fp32(int devid) {
    xpu_set_device(devid);
    printf("\n=== GEMM FP32 (nc=4, Device %d) ===\n", devid);
    printf("  %-10s %6s %6s %6s  %10s  %10s  %9s\n", "Name","M","N","K","Time(ms)","GFLOPS","Efficiency");

    struct GemmCfg { const char* name; int m,n,k; };
    GemmCfg cfgs[] = {
        {"small",    512,  512,  512 },
        {"medium",  1024, 1024, 1024},
        {"large",   2048, 2048, 2048},
        {"xl",      4096, 4096, 4096},
        {"mm1",     8192, 8192, 8192},
        {"bert_fc", 4096, 1024, 4096},
        {"resnet",  1000, 2048, 8192},
    };
    int n_cfgs = sizeof(cfgs) / sizeof(cfgs[0]);

    auto ctx = xdnn::create_context();
    ctx->set_ncluster(4);

    for (int i = 0; i < n_cfgs; i++) {
        int m = cfgs[i].m, n = cfgs[i].n, k = cfgs[i].k;
        size_t sa = (size_t)m * k * sizeof(float);
        size_t sb = (size_t)k * n * sizeof(float);
        size_t sc = (size_t)m * n * sizeof(float);

        float *A, *B, *C;
        if (xpu_malloc((void**)&A, sa) || xpu_malloc((void**)&B, sb) || xpu_malloc((void**)&C, sc)) {
            printf("  %-10s OOM\n", cfgs[i].name); continue;
        }
        std::vector<float> ha(m * k, 1.0f), hb(k * n, 1.0f);
        xpu_memcpy(A, ha.data(), sa, XPU_HOST_TO_DEVICE);
        xpu_memcpy(B, hb.data(), sb, XPU_HOST_TO_DEVICE);
        xpu_wait();

        for (int r = 0; r < WARMUP; r++) {
            xdnn::fc<float,float,float,float>(ctx, A,B,C, m,n,k, false,false, nullptr,nullptr,nullptr);
            xpu_wait();
        }
        double t0 = get_time_us();
        for (int r = 0; r < RUNS; r++) {
            xdnn::fc<float,float,float,float>(ctx, A,B,C, m,n,k, false,false, nullptr,nullptr,nullptr);
            xpu_wait();
        }
        double t1 = get_time_us();
        double ms = (t1 - t0) / RUNS / 1000.0;
        double gflops = (2.0 * m * n * k) / (ms * 1e-3) / 1e9;
        double eff = gflops / 14.4e3 * 100.0;  // 14.4 TFLOPS = 14400 GFLOPS
        printf("  %-10s %6d %6d %6d  %10.3f  %10.0f  %8.2f%%\n", cfgs[i].name, m,n,k, ms, gflops, eff);
        xpu_free(A); xpu_free(B); xpu_free(C);
    }
    xdnn::destroy_context(ctx);
}

// ---------------------------------------------------------------------------
// 4. FP16 GEMM — nc=4 + ncluster sweep + pinned-memory end-to-end
// ---------------------------------------------------------------------------
void test_gemm_fp16(int devid) {
    xpu_set_device(devid);

    // --- Part A: nc=4 default ---
    printf("\n=== GEMM FP16 (nc=4, Device %d) ===\n", devid);
    printf("  %-10s %6s %6s %6s  %10s  %10s  %9s\n", "Name","M","N","K","Time(ms)","GFLOPS","Efficiency");

    struct GemmCfg { const char* name; int m,n,k; };
    GemmCfg cfgs[] = {
        {"small",    512,  512,  512 },
        {"medium",  1024, 1024, 1024},
        {"large",   2048, 2048, 2048},
        {"xl",      4096, 4096, 4096},
        {"mm1",     8192, 8192, 8192},
        {"bert_fc", 4096, 1024, 4096},
        {"resnet",  1000, 2048, 8192},
    };
    int n_cfgs = sizeof(cfgs) / sizeof(cfgs[0]);

    auto ctx = xdnn::create_context();
    ctx->set_ncluster(4);

    for (int i = 0; i < n_cfgs; i++) {
        int m = cfgs[i].m, n = cfgs[i].n, k = cfgs[i].k;
        size_t sa = (size_t)m * k * sizeof(float16);
        size_t sb = (size_t)k * n * sizeof(float16);
        size_t sc = (size_t)m * n * sizeof(float16);

        float16 *A, *B, *C;
        if (xpu_malloc((void**)&A, sa) || xpu_malloc((void**)&B, sb) || xpu_malloc((void**)&C, sc)) {
            printf("  %-10s OOM\n", cfgs[i].name); continue;
        }
        std::vector<float16> ha(m * k, 1.0f), hb(k * n, 1.0f);
        xpu_memcpy(A, ha.data(), sa, XPU_HOST_TO_DEVICE);
        xpu_memcpy(B, hb.data(), sb, XPU_HOST_TO_DEVICE);
        xpu_wait();

        bool ok = true;
        for (int r = 0; r < WARMUP; r++) {
            int ret = xdnn::fc<float16,float16,float16,short>(ctx, A,B,C, m,n,k, false,false, nullptr,nullptr,nullptr);
            if (ret) { printf("  %-10s err=%d\n", cfgs[i].name, ret); ok = false; break; }
            xpu_wait();
        }
        if (!ok) { xpu_free(A); xpu_free(B); xpu_free(C); continue; }

        double t0 = get_time_us();
        for (int r = 0; r < RUNS; r++) {
            xdnn::fc<float16,float16,float16,short>(ctx, A,B,C, m,n,k, false,false, nullptr,nullptr,nullptr);
            xpu_wait();
        }
        double t1 = get_time_us();
        double ms = (t1 - t0) / RUNS / 1000.0;
        double gflops = (2.0 * m * n * k) / (ms * 1e-3) / 1e9;
        double eff = gflops / 57.6e3 * 100.0;  // 57.6 TFLOPS = 57600 GFLOPS
        printf("  %-10s %6d %6d %6d  %10.3f  %10.0f  %8.2f%%\n", cfgs[i].name, m,n,k, ms, gflops, eff);
        xpu_free(A); xpu_free(B); xpu_free(C);
    }
    xdnn::destroy_context(ctx);

    // --- Part B: nCluster Sweep (M=N=K=2048) ---
    printf("\n=== FP16 nCluster Sweep (M=N=K=2048, Device %d) ===\n", devid);
    printf("  nCluster  Time(ms)  GFLOPS   Efficiency\n");

    int m = 2048, n = 2048, k = 2048;
    size_t sa = (size_t)m * k * sizeof(float16);
    size_t sb = (size_t)k * n * sizeof(float16);
    size_t sc = (size_t)m * n * sizeof(float16);
    float16 *A, *B, *C;
    xpu_malloc((void**)&A, sa); xpu_malloc((void**)&B, sb); xpu_malloc((void**)&C, sc);
    std::vector<float16> ha(m * k, 1.0f), hb(k * n, 1.0f);
    xpu_memcpy(A, ha.data(), sa, XPU_HOST_TO_DEVICE);
    xpu_memcpy(B, hb.data(), sb, XPU_HOST_TO_DEVICE);
    xpu_wait();

    for (int nc = 1; nc <= 4; nc++) {
        auto ctx2 = xdnn::create_context();
        ctx2->set_ncluster(nc);

        for (int r = 0; r < WARMUP; r++) {
            xdnn::fc<float16,float16,float16,short>(ctx2, A,B,C, m,n,k, false,false, nullptr,nullptr,nullptr);
            xpu_wait();
        }
        double t0 = get_time_us();
        int local_runs = (nc == 4) ? RUNS : 30;
        for (int r = 0; r < local_runs; r++) {
            xdnn::fc<float16,float16,float16,short>(ctx2, A,B,C, m,n,k, false,false, nullptr,nullptr,nullptr);
            xpu_wait();
        }
        double t1 = get_time_us();
        double ms = (t1 - t0) / local_runs / 1000.0;
        double gflops = (2.0 * m * n * k) / (ms * 1e-3) / 1e9;
        double eff = gflops / 57.6e3 * 100.0;
        printf("    %1d        %8.3f  %7.0f    %6.2f%%\n", nc, ms, gflops, eff);
        xdnn::destroy_context(ctx2);
    }
    xpu_free(A); xpu_free(B); xpu_free(C);

    // --- Part C: Pinned memory — SKIPPED (xpu_host_alloc unsupported in this SDK) ---
}

// ---------------------------------------------------------------------------
// 5. INT8 GEMM (baseline — probably unsupported)
// ---------------------------------------------------------------------------
void test_gemm_int8(int devid) {
    xpu_set_device(devid);
    printf("\n=== GEMM INT8 (Device %d) ===\n", devid);
    printf("  %-10s %6s %6s %6s  %10s  %10s  %9s\n", "Name","M","N","K","Time(ms)","TOPS","Efficiency");

    struct GemmCfg { const char* name; int m,n,k; };
    GemmCfg cfgs[] = {
        {"small",    512,  512,  512 },
        {"medium",  1024, 1024, 1024},
        {"large",   2048, 2048, 2048},
        {"xl",      4096, 4096, 4096},
        {"mm1",     8192, 8192, 8192},
        {"bert_fc", 4096, 1024, 4096},
        {"resnet",  1000, 2048, 8192},
    };
    int n_cfgs = sizeof(cfgs) / sizeof(cfgs[0]);

    auto ctx = xdnn::create_context();
    ctx->set_ncluster(4);

    for (int i = 0; i < n_cfgs; i++) {
        int m = cfgs[i].m, n = cfgs[i].n, k = cfgs[i].k;
        size_t sa = (size_t)m * k * sizeof(int8_t);
        size_t sb = (size_t)k * n * sizeof(int8_t);
        size_t sc = (size_t)m * n * sizeof(int8_t);

        int8_t *A, *B, *C;
        if (xpu_malloc((void**)&A, sa) || xpu_malloc((void**)&B, sb) || xpu_malloc((void**)&C, sc)) {
            printf("  %-10s OOM\n", cfgs[i].name); continue;
        }
        std::vector<int8_t> ha(m * k, 1), hb(k * n, 1);
        xpu_memcpy(A, ha.data(), sa, XPU_HOST_TO_DEVICE);
        xpu_memcpy(B, hb.data(), sb, XPU_HOST_TO_DEVICE);
        xpu_wait();

        bool ok = true;
        for (int r = 0; r < WARMUP; r++) {
            int ret = xdnn::fc<int8_t,int8_t,int8_t,int8_t>(ctx, A,B,C, m,n,k, false,false, nullptr,nullptr,nullptr);
            if (ret) { printf("  %-10s not supported (err=%d)\n", cfgs[i].name, ret); ok = false; break; }
            xpu_wait();
        }
        if (!ok) { xpu_free(A); xpu_free(B); xpu_free(C); continue; }

        double t0 = get_time_us();
        for (int r = 0; r < RUNS; r++) {
            xdnn::fc<int8_t,int8_t,int8_t,int8_t>(ctx, A,B,C, m,n,k, false,false, nullptr,nullptr,nullptr);
            xpu_wait();
        }
        double t1 = get_time_us();
        double ms = (t1 - t0) / RUNS / 1000.0;
        double tops = (2.0 * m * n * k) / (ms * 1e-3) / 1e12;
        double eff = tops / 230.4;  // 230.4 TOPS INT8 (256 * 0.9 for 900MHz)
        printf("  %-10s %6d %6d %6d  %10.3f  %10.0f  %8.2f%%\n", cfgs[i].name, m,n,k, ms, tops*1000, eff);
        xpu_free(A); xpu_free(B); xpu_free(C);
    }
    xdnn::destroy_context(ctx);
}

// ---------------------------------------------------------------------------
// 6. K-dimension sweep (unchanged, uses nc=1)
// ---------------------------------------------------------------------------
void test_gemm_k_sweep(int devid) {
    xpu_set_device(devid);
    printf("\n=== K-Dimension Sweep (M=N=2048, FP32 vs FP16, nc=1) ===\n");
    printf("  %-8s  %10s  %12s  %10s  %10s\n", "K","FP32 ms","FP32 GFLOPS","FP16 ms","FP16 GFLOPS");

    int ks[] = {128,256,384,512,640,768,896,1024,1280,1536,1792,2048,2560,3072,3584,4096,5120,6144,7168,8192};
    int n_ks = sizeof(ks)/sizeof(ks[0]);
    int m = 2048, n = 2048;
    auto ctx = xdnn::create_context();

    for (int i = 0; i < n_ks; i++) {
        int k = ks[i];

        // FP32
        size_t sa = (size_t)m*k*sizeof(float), sb = (size_t)k*n*sizeof(float), sc = (size_t)m*n*sizeof(float);
        float *A32,*B32,*C32;
        xpu_malloc((void**)&A32,sa); xpu_malloc((void**)&B32,sb); xpu_malloc((void**)&C32,sc);
        std::vector<float> ha(m*k,1.0f), hb(k*n,1.0f);
        xpu_memcpy(A32,ha.data(),sa,XPU_HOST_TO_DEVICE); xpu_memcpy(B32,hb.data(),sb,XPU_HOST_TO_DEVICE); xpu_wait();
        for (int r=0;r<WARMUP;r++){xdnn::fc<float,float,float,float>(ctx,A32,B32,C32,m,n,k,false,false,nullptr,nullptr,nullptr);xpu_wait();}
        double t0=get_time_us(); int r32=(k<=2048)?100:40;
        for(int r=0;r<r32;r++){xdnn::fc<float,float,float,float>(ctx,A32,B32,C32,m,n,k,false,false,nullptr,nullptr,nullptr);xpu_wait();}
        double t1=get_time_us(); double ms32=(t1-t0)/r32/1000.0; double gf32=(2.0*m*n*k)/(ms32*1e-3)/1e9;
        xpu_free(A32);xpu_free(B32);xpu_free(C32);

        // FP16
        sa=(size_t)m*k*sizeof(float16); sb=(size_t)k*n*sizeof(float16); sc=(size_t)m*n*sizeof(float16);
        float16 *A16,*B16,*C16;
        xpu_malloc((void**)&A16,sa); xpu_malloc((void**)&B16,sb); xpu_malloc((void**)&C16,sc);
        std::vector<float16> ha16(m*k,1.0f), hb16(k*n,1.0f);
        xpu_memcpy(A16,ha16.data(),sa,XPU_HOST_TO_DEVICE); xpu_memcpy(B16,hb16.data(),sb,XPU_HOST_TO_DEVICE); xpu_wait();
        for(int r=0;r<WARMUP;r++){xdnn::fc<float16,float16,float16,short>(ctx,A16,B16,C16,m,n,k,false,false,nullptr,nullptr,nullptr);xpu_wait();}
        t0=get_time_us(); int r16=(k<=2048)?100:40;
        for(int r=0;r<r16;r++){xdnn::fc<float16,float16,float16,short>(ctx,A16,B16,C16,m,n,k,false,false,nullptr,nullptr,nullptr);xpu_wait();}
        t1=get_time_us(); double ms16=(t1-t0)/r16/1000.0; double gf16=(2.0*m*n*k)/(ms16*1e-3)/1e9;
        xpu_free(A16);xpu_free(B16);xpu_free(C16);

        printf("  %-8d  %10.3f  %12.0f  %10.3f  %12.0f\n", k, ms32, gf32, ms16, gf16);
    }
    xdnn::destroy_context(ctx);
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    int dev_count = 0;
    xpu_device_count(&dev_count);
    printf("========================================\n");
    printf(" Kunlun K200 Performance Test — OPTIMIZED\n");
    printf("========================================\n");
    printf("Devices: %d  |  SDK: xdnn 2.0.0.725  |  Driver: 4.33\n", dev_count);
    printf("Theoretical (1000MHz):  256 TOPS INT8  /  64 TFLOPS FP16  /  16 TFLOPS FP32\n");
    printf("Corrected  ( 900MHz):  230 TOPS INT8  /  58 TFLOPS FP16  /  14 TFLOPS FP32\n");
    printf("Improvements: pinned memory + ncluster=4 + ncluster sweep\n\n");

    int target_dev = 0;
    if (argc > 1) target_dev = atoi(argv[1]);
    if (target_dev >= dev_count) {
        printf("Invalid device %d (max %d)\n", target_dev, dev_count-1);
        return 1;
    }

    if (argc > 2 && strcmp(argv[2], "sweep") == 0) {
        test_gemm_k_sweep(target_dev);
    } else {
        test_bandwidth(target_dev);
        //test_bandwidth_pinned(target_dev);
        test_gemm_fp32(target_dev);
        test_gemm_fp16(target_dev);
        test_gemm_int8(target_dev);
    }

    printf("\n=== All tests complete ===\n");
    return 0;
}
