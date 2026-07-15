/*!
 * S8 application-oriented pipeline microbench for K200.
 *
 * Compares end-to-end paths that real apps can control:
 *   - pageable vs xpu_host_alloc (S4/S6 DMA path)
 *   - FP32 vs FP16 GEMM (ncluster=4)
 *
 * Workload: H2D input → N× FC layers → D2H output (optional).
 * Not a full ResNet; measures how app choices map to driver wins.
 */
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <xpu/runtime.h>
#include <xpu/xdnn.h>

namespace xdnn = baidu::xpu::api;

static double now_us() {
    using clock = std::chrono::high_resolution_clock;
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(
               clock::now().time_since_epoch())
        .count();
}

enum class HostKind { Pageable, HostAlloc };

struct Cfg {
    const char *name;
    HostKind    host;
    bool        fp16;
    bool        include_h2d;
    bool        include_d2h;
};

static void *host_alloc_buf(HostKind k, size_t n, bool *used_host_alloc) {
    *used_host_alloc = false;
    if (k == HostKind::HostAlloc) {
        void *p = nullptr;
        if (xpu_host_alloc(&p, n, 0) == 0 && p) {
            *used_host_alloc = true;
            return p;
        }
        fprintf(stderr, "  WARN: xpu_host_alloc(%zu) failed, fallback pageable\n", n);
    }
    void *p = aligned_alloc(4096, n);
    return p;
}

static void host_free_buf(void *p, bool used_host_alloc) {
    if (!p)
        return;
    if (used_host_alloc)
        xpu_host_free(p);
    else
        free(p);
}

template <typename T>
static int run_fc(xdnn::Context *ctx, const T *A, const T *B, T *C, int m, int n, int k);

template <>
int run_fc<float>(xdnn::Context *ctx, const float *A, const float *B, float *C, int m, int n,
                  int k) {
    return xdnn::fc<float, float, float, float>(ctx, A, B, C, m, n, k, false, false, nullptr,
                                                nullptr, nullptr);
}

template <>
int run_fc<float16>(xdnn::Context *ctx, const float16 *A, const float16 *B, float16 *C, int m,
                    int n, int k) {
    return xdnn::fc<float16, float16, float16, short>(ctx, A, B, C, m, n, k, false, false, nullptr,
                                                      nullptr, nullptr);
}

template <typename T>
static int bench_one(int devid, const Cfg &cfg, int batch, int feat, int layers, int warmup,
                     int runs, double *out_img_s, double *out_ms) {
    xpu_set_device(devid);
    xdnn::Context *ctx = xdnn::create_context();
    if (!ctx)
        return -1;
    ctx->set_ncluster(4);

    // Input: [batch, feat]; each layer is feat×feat GEMM (stable K for FP16).
    const int m = batch, n = feat, k = feat;
    const size_t mat = (size_t)m * (size_t)k * sizeof(T);
    const size_t wgt = (size_t)k * (size_t)n * sizeof(T);
    const size_t out = (size_t)m * (size_t)n * sizeof(T);

    bool ha_in = false, ha_w = false, ha_out = false;
    void *h_in = host_alloc_buf(cfg.host, mat, &ha_in);
    void *h_w = host_alloc_buf(cfg.host, wgt, &ha_w);
    void *h_out = host_alloc_buf(cfg.host, out, &ha_out);
    if (!h_in || !h_w || !h_out) {
        host_free_buf(h_in, ha_in);
        host_free_buf(h_w, ha_w);
        host_free_buf(h_out, ha_out);
        xdnn::destroy_context(ctx);
        return -1;
    }
    memset(h_in, 1, mat);
    memset(h_w, 1, wgt);

    T *d_a = nullptr, *d_b = nullptr, *d_c = nullptr, *d_tmp = nullptr;
    if (xpu_malloc((void **)&d_a, mat) || xpu_malloc((void **)&d_b, wgt) ||
        xpu_malloc((void **)&d_c, out) || xpu_malloc((void **)&d_tmp, out)) {
        fprintf(stderr, "  xpu_malloc failed\n");
        host_free_buf(h_in, ha_in);
        host_free_buf(h_w, ha_w);
        host_free_buf(h_out, ha_out);
        xdnn::destroy_context(ctx);
        return -1;
    }

    // Weights once on device (typical inference).
    xpu_memcpy(d_b, h_w, wgt, XPU_HOST_TO_DEVICE);
    xpu_wait();

    auto one_iter = [&]() -> int {
        if (cfg.include_h2d) {
            if (xpu_memcpy(d_a, h_in, mat, XPU_HOST_TO_DEVICE))
                return -1;
        }
        T *in = d_a;
        T *o1 = d_c;
        T *o2 = d_tmp;
        for (int L = 0; L < layers; L++) {
            T *dst = (L & 1) ? o2 : o1;
            int ret = run_fc<T>(ctx, in, d_b, dst, m, n, k);
            if (ret)
                return ret;
            in = dst;
        }
        if (cfg.include_d2h) {
            T *final_out = ((layers - 1) & 1) ? o2 : o1;
            if (layers == 0)
                final_out = d_a;
            if (xpu_memcpy(h_out, final_out, out, XPU_DEVICE_TO_HOST))
                return -1;
        }
        xpu_wait();
        return 0;
    };

    for (int i = 0; i < warmup; i++) {
        if (one_iter()) {
            fprintf(stderr, "  warmup failed cfg=%s\n", cfg.name);
            xpu_free(d_a);
            xpu_free(d_b);
            xpu_free(d_c);
            xpu_free(d_tmp);
            host_free_buf(h_in, ha_in);
            host_free_buf(h_w, ha_w);
            host_free_buf(h_out, ha_out);
            xdnn::destroy_context(ctx);
            return -1;
        }
    }

    double t0 = now_us();
    for (int i = 0; i < runs; i++) {
        if (one_iter()) {
            fprintf(stderr, "  run failed cfg=%s\n", cfg.name);
            xpu_free(d_a);
            xpu_free(d_b);
            xpu_free(d_c);
            xpu_free(d_tmp);
            host_free_buf(h_in, ha_in);
            host_free_buf(h_w, ha_w);
            host_free_buf(h_out, ha_out);
            xdnn::destroy_context(ctx);
            return -1;
        }
    }
    double t1 = now_us();
    double sec = (t1 - t0) * 1e-6;
    *out_ms = sec / runs * 1000.0;
    *out_img_s = (double)batch * runs / sec;

    // FLOPs: layers * 2*m*n*k
    double gflops =
        (double)layers * 2.0 * m * n * k * runs / sec / 1e9;

    printf("  %-28s  %7.1f img/s  %7.2f ms  %7.1f GFLOPS  host=%s dtype=%s IO=%s%s\n", cfg.name,
           *out_img_s, *out_ms, gflops, ha_in ? "host_alloc" : "pageable",
           cfg.fp16 ? "FP16" : "FP32", cfg.include_h2d ? "H2D" : "-",
           cfg.include_d2h ? "+D2H" : "");

    xpu_free(d_a);
    xpu_free(d_b);
    xpu_free(d_c);
    xpu_free(d_tmp);
    host_free_buf(h_in, ha_in);
    host_free_buf(h_w, ha_w);
    host_free_buf(h_out, ha_out);
    xdnn::destroy_context(ctx);
    return 0;
}

static int run_cfg(int devid, const Cfg &cfg, int batch, int feat, int layers, int warmup,
                   int runs) {
    double img_s = 0, ms = 0;
    int ret;
    if (cfg.fp16)
        ret = bench_one<float16>(devid, cfg, batch, feat, layers, warmup, runs, &img_s, &ms);
    else
        ret = bench_one<float>(devid, cfg, batch, feat, layers, warmup, runs, &img_s, &ms);
    return ret;
}

int main(int argc, char **argv) {
    int devid = 0;
    int batch = 32;
    int feat = 2048; // K200 FP16 sweet-spot K
    int layers = 8;
    int warmup = 5;
    int runs = 30;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d") && i + 1 < argc)
            devid = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-b") && i + 1 < argc)
            batch = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-f") && i + 1 < argc)
            feat = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-l") && i + 1 < argc)
            layers = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-r") && i + 1 < argc)
            runs = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("Usage: %s [dev] [-d dev] [-b batch] [-f feat] [-l layers] [-r runs]\n",
                   argv[0]);
            return 0;
        } else if (argv[i][0] >= '0' && argv[i][0] <= '9') {
            devid = atoi(argv[i]);
        }
    }

    int dc = 0;
    xpu_device_count(&dc);
    printf("========================================\n");
    printf(" S8 App Pipeline Microbench (K200)\n");
    printf("========================================\n");
    printf("Devices: %d | dev=%d batch=%d feat=%d layers=%d runs=%d\n", dc, devid, batch, feat,
           layers, runs);
    printf("Guidance: prefer xpu_host_alloc I/O + FP16 compute (K<=4096)\n\n");

    Cfg cfgs[] = {
        {"pageable+FP32 e2e", HostKind::Pageable, false, true, true},
        {"pageable+FP16 e2e", HostKind::Pageable, true, true, true},
        {"host_alloc+FP32 e2e", HostKind::HostAlloc, false, true, true},
        {"host_alloc+FP16 e2e", HostKind::HostAlloc, true, true, true},
        {"host_alloc+FP16 compute", HostKind::HostAlloc, true, false, false},
        {"pageable+FP16 compute", HostKind::Pageable, true, false, false},
    };

    printf("%-30s  %8s  %8s  %8s  notes\n", "config", "img/s", "ms", "GFLOPS");
    int fails = 0;
    for (auto &c : cfgs) {
        if (run_cfg(devid, c, batch, feat, layers, warmup, runs))
            fails++;
    }

    printf("\nRecommended app defaults:\n");
    printf("  1) Stage inputs/weights via xpu_host_alloc (S4 ~12 GB/s)\n");
    printf("  2) Run compute in FP16 with ncluster=4; keep K in 1024..4096\n");
    printf("  3) Keep weights on device; only stream activations H2D/D2H\n");
    printf("  4) pageable still OK after S6 (~10 GB/s) but pinned is faster\n");

    return fails ? 1 : 0;
}
