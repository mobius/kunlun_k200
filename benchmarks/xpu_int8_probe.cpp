// S3: INT8 GEMM / quant API probe for Kunlun K200 (KL1)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <chrono>
#include <xpu/runtime.h>
#include <xpu/xdnn.h>

namespace xdnn = baidu::xpu::api;

static inline double us_now() {
    auto n = std::chrono::high_resolution_clock::now();
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(n.time_since_epoch()).count();
}

static const char* xdnn_err(int e) {
    switch (e) {
    case 0: return "OK";
    case 1: return "INVALID_PARAM";
    case 2: return "RUNTIME_ERROR";
    case 3: return "NOT_IMPLEMENTED";
    default: return "OTHER";
    }
}

template <typename TX, typename TW, typename TY, typename TGEMM>
static int run_fc(xdnn::Context* ctx, int m, int n, int k, bool with_maxptr) {
    size_t sa = (size_t)m * k * sizeof(TX);
    size_t sb = (size_t)k * n * sizeof(TW);
    size_t sc = (size_t)m * n * sizeof(TY);

    TX *A = nullptr;
    TW *B = nullptr;
    TY *C = nullptr;
    void *ha = nullptr, *hb = nullptr;

    if (xpu_malloc((void**)&A, sa) || xpu_malloc((void**)&B, sb) || xpu_malloc((void**)&C, sc))
        return -999;

    ha = aligned_alloc(64, sa);
    hb = aligned_alloc(64, sb);
    memset(ha, 1, sa);
    memset(hb, 1, sb);
    xpu_memcpy(A, ha, sa, XPU_HOST_TO_DEVICE);
    xpu_memcpy(B, hb, sb, XPU_HOST_TO_DEVICE);
    xpu_wait();

    float xmax = 1.0f, wmax = 1.0f, ymax = 1.0f;
    const float* px = with_maxptr ? &xmax : nullptr;
    const float* pw = with_maxptr ? &wmax : nullptr;
    float* py = with_maxptr ? &ymax : nullptr;

    int ret = xdnn::fc<TX, TW, TY, TGEMM>(ctx, A, B, C, m, n, k, false, false, px, pw, py);
    xpu_wait();

    free(ha);
    free(hb);
    xpu_free(A);
    xpu_free(B);
    xpu_free(C);
    return ret;
}

template <typename TX, typename TW, typename TY, typename TGEMM>
static void bench_fc(xdnn::Context* ctx, const char* label, int m, int n, int k, bool with_maxptr) {
    size_t sa = (size_t)m * k * sizeof(TX);
    size_t sb = (size_t)k * n * sizeof(TW);
    size_t sc = (size_t)m * n * sizeof(TY);

    TX *A;
    TW *B;
    TY *C;
    void *ha = nullptr, *hb = nullptr;

    if (xpu_malloc((void**)&A, sa) || xpu_malloc((void**)&B, sb) || xpu_malloc((void**)&C, sc)) {
        printf("  %-36s  OOM\n", label);
        return;
    }
    ha = aligned_alloc(64, sa);
    hb = aligned_alloc(64, sb);
    memset(ha, 1, sa);
    memset(hb, 1, sb);
    xpu_memcpy(A, ha, sa, XPU_HOST_TO_DEVICE);
    xpu_memcpy(B, hb, sb, XPU_HOST_TO_DEVICE);
    xpu_wait();

    float xmax = 1.0f, wmax = 1.0f, ymax = 1.0f;
    const float* px = with_maxptr ? &xmax : nullptr;
    const float* pw = with_maxptr ? &wmax : nullptr;
    float* py = with_maxptr ? &ymax : nullptr;

    for (int i = 0; i < 3; i++) {
        if (xdnn::fc<TX, TW, TY, TGEMM>(ctx, A, B, C, m, n, k, false, false, px, pw, py)) {
            printf("  %-36s  warmup fail\n", label);
            free(ha); free(hb);
            xpu_free(A); xpu_free(B); xpu_free(C);
            return;
        }
        xpu_wait();
    }
    double t0 = us_now();
    const int runs = 50;
    for (int i = 0; i < runs; i++) {
        if (xdnn::fc<TX, TW, TY, TGEMM>(ctx, A, B, C, m, n, k, false, false, px, pw, py)) {
            printf("  %-36s  bench fail at iter %d\n", label, i);
            free(ha); free(hb);
            xpu_free(A); xpu_free(B); xpu_free(C);
            return;
        }
        xpu_wait();
    }
    double t1 = us_now();
    double ms = (t1 - t0) / runs / 1000.0;
    double tops = (2.0 * m * n * k) / (ms * 1e-3) / 1e12;
    printf("  %-36s  %8.2f ms  %8.1f TOPS  (eff: %.2f%%)\n", label, ms, tops, tops / 230.4 * 100.0);
    free(ha);
    free(hb);
    xpu_free(A);
    xpu_free(B);
    xpu_free(C);
}

static void probe_fc_sizes(xdnn::Context* ctx) {
    struct Cfg { const char* name; int m, n, k; } cfgs[] = {
        {"small", 512, 512, 512},
        {"medium", 1024, 1024, 1024},
        {"large", 2048, 2048, 2048},
    };
    printf("\n=== fc<int8,int8,int8,int8> size sweep ===\n");
    for (auto& c : cfgs) {
        int ret = run_fc<int8_t, int8_t, int8_t, int8_t>(ctx, c.m, c.n, c.k, false);
        printf("  %-8s %4d^3  ret=%d (%s)\n", c.name, c.m, ret, xdnn_err(ret));
    }
}

static void probe_quant(xdnn::Context* ctx) {
    printf("\n=== Quant API probe ===\n");
    const int len = 4096;
    float* xf = (float*)aligned_alloc(64, len * sizeof(float));
    int8_t* yi = (int8_t*)aligned_alloc(64, len);
    float* yf = (float*)aligned_alloc(64, len * sizeof(float));
    float maxv = 1.0f;
    for (int i = 0; i < len; i++) xf[i] = 0.01f * (float)(i % 127);

    int r1 = xdnn::quantization<float, int8_t>(ctx, xf, yi, len, &maxv);
    printf("  quantization<float,int8>     ret=%d (%s)\n", r1, xdnn_err(r1));

    float16* xh = (float16*)aligned_alloc(64, len * sizeof(float16));
    for (int i = 0; i < len; i++) xh[i] = (float16)xf[i];
    int r2 = xdnn::gpt_fp16_quant_2int8(ctx, xh, yi, len, &maxv);
    printf("  gpt_fp16_quant_2int8         ret=%d (%s)\n", r2, xdnn_err(r2));

    int r3 = xdnn::dequantization<int8_t, float>(ctx, yi, yf, len, &maxv);
    printf("  dequantization<int8,float>   ret=%d (%s)\n", r3, xdnn_err(r3));

    free(xf); free(xh); free(yi); free(yf);
}

int main(int argc, char** argv) {
    int dev = 0;
    if (argc > 1) dev = atoi(argv[1]);

    int dc = 0;
    xpu_device_count(&dc);
    xpu_set_device(dev);

    printf("========================================\n");
    printf(" Kunlun K200 INT8 Probe (S3)\n");
    printf("========================================\n");
    printf("Devices: %d  |  target: %d  |  SDK: xdnn 2.0.0.725\n", dc, dev);
    printf("Public fc<> templates with INT8: int8/int8/int8/int8 only (links)\n");
    printf("libxpuapi internal: 45x fc_int8_v1/v2/v3 (mixed precision, no public wrapper)\n\n");

    auto ctx = xdnn::create_context();
    ctx->set_ncluster(4);

    printf("=== fc<int8,int8,int8,int8> probe ===\n");
    int r0 = run_fc<int8_t, int8_t, int8_t, int8_t>(ctx, 512, 512, 512, false);
    printf("  512^3  maxptr=null   ret=%d (%s)\n", r0, xdnn_err(r0));
    int r1 = run_fc<int8_t, int8_t, int8_t, int8_t>(ctx, 512, 512, 512, true);
    printf("  512^3  maxptr=set    ret=%d (%s)\n", r1, xdnn_err(r1));

    probe_fc_sizes(ctx);
    probe_quant(ctx);

    if (r0 == 0 || r1 == 0) {
        printf("\n=== INT8 GEMM benchmark (2048^3) ===\n");
        bench_fc<int8_t, int8_t, int8_t, int8_t>(ctx, "int8^3", 2048, 2048, 2048, false);
        bench_fc<int8_t, int8_t, int8_t, int8_t>(ctx, "int8^3+maxptr", 2048, 2048, 2048, true);
    } else {
        printf("\n=== INT8 GEMM benchmark skipped (API returns INVALID_PARAM) ===\n");
        printf("Compare FP16 baseline at 2048^3 for reference:\n");
        bench_fc<float16, float16, float16, short>(ctx, "fp16 baseline", 2048, 2048, 2048, false);
    }

    xdnn::destroy_context(ctx);
    printf("\n=== Probe complete ===\n");
    return 0;
}