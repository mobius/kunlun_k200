/*!
 * C3: same-card dual-PD pipeline micro-app for K200.
 *
 * Stage A on src_dev (FC stack) → xpu_memcpy_peer → Stage B on dst_dev → D2H.
 * Compare against single-PD serial (both stages on src_dev).
 *
 * Usage:
 *   ./xpu_pipeline_p2p [src_dev] [dst_dev]
 *   defaults: src=0 dst=1 (same card)
 */
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <xpu/runtime.h>
#include <xpu/xdnn.h>

namespace xdnn = baidu::xpu::api;

static double now_us() {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

static void *halloc(size_t n, bool *pin) {
    *pin = false;
    void *p = nullptr;
    if (xpu_host_alloc(&p, n, 0) == 0 && p) {
        *pin = true;
        return p;
    }
    return aligned_alloc(4096, n);
}
static void hfree(void *p, bool pin) {
    if (!p)
        return;
    if (pin)
        xpu_host_free(p);
    else
        free(p);
}

static int fc16(xdnn::Context *ctx, const float16 *A, const float16 *W, float16 *C, int m, int n,
                int k) {
    return xdnn::fc<float16, float16, float16, short>(ctx, A, W, C, m, n, k, false, false, nullptr,
                                                      nullptr, nullptr);
}

struct Stage {
    int devid;
    xdnn::Context *ctx = nullptr;
    float16 *W = nullptr;
    float16 *in = nullptr;
    float16 *mid = nullptr;
    float16 *out = nullptr;
    int m = 0, n = 0, k = 0, layers = 0;

    bool init(int dev, int batch, int feat, int nlayers) {
        devid = dev;
        m = batch;
        n = feat;
        k = feat;
        layers = nlayers;
        xpu_set_device(devid);
        ctx = xdnn::create_context();
        if (!ctx)
            return false;
        ctx->set_ncluster(4);
        size_t mat = (size_t)m * k * sizeof(float16);
        size_t wgt = (size_t)k * n * sizeof(float16);
        if (xpu_malloc((void **)&W, wgt) || xpu_malloc((void **)&in, mat) ||
            xpu_malloc((void **)&mid, mat) || xpu_malloc((void **)&out, mat))
            return false;
        // unit-ish weights
        bool pin = false;
        float16 *hw = (float16 *)halloc(wgt, &pin);
        if (!hw)
            return false;
        for (size_t i = 0; i < (size_t)k * n; i++)
            hw[i] = float16(0.01f);
        xpu_memcpy(W, hw, wgt, XPU_HOST_TO_DEVICE);
        xpu_wait();
        hfree(hw, pin);
        return true;
    }

    void destroy() {
        xpu_set_device(devid);
        if (W)
            xpu_free(W);
        if (in)
            xpu_free(in);
        if (mid)
            xpu_free(mid);
        if (out)
            xpu_free(out);
        if (ctx)
            xdnn::destroy_context(ctx);
        W = in = mid = out = nullptr;
        ctx = nullptr;
    }

    int run_layers(float16 *src, float16 *dst) {
        float16 *cur = src;
        float16 *a = mid, *b = out;
        for (int L = 0; L < layers; L++) {
            float16 *dst_l = (L & 1) ? b : a;
            int ret = fc16(ctx, cur, W, dst_l, m, n, k);
            if (ret)
                return ret;
            cur = dst_l;
        }
        if (cur != dst) {
            size_t mat = (size_t)m * n * sizeof(float16);
            xpu_memcpy(dst, cur, mat, XPU_DEVICE_TO_DEVICE);
        }
        xpu_wait();
        return 0;
    }
};

int main(int argc, char **argv) {
    int src_dev = 0, dst_dev = 1;
    int batch = 32, feat = 2048, layers_a = 4, layers_b = 4;
    int warmup = 5, runs = 30;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s") && i + 1 < argc)
            src_dev = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-t") && i + 1 < argc)
            dst_dev = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-b") && i + 1 < argc)
            batch = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-f") && i + 1 < argc)
            feat = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-r") && i + 1 < argc)
            runs = atoi(argv[++i]);
        else if (argv[i][0] >= '0' && argv[i][0] <= '9' && i == 1)
            src_dev = atoi(argv[i]);
        else if (argv[i][0] >= '0' && argv[i][0] <= '9' && i == 2)
            dst_dev = atoi(argv[i]);
    }

    int dc = 0;
    xpu_device_count(&dc);
    printf("========================================\n");
    printf(" C3 Dual-PD P2P Pipeline (K200)\n");
    printf("========================================\n");
    printf("devices=%d | src=%d dst=%d batch=%d feat=%d layers=%d+%d runs=%d\n", dc, src_dev,
           dst_dev, batch, feat, layers_a, layers_b, runs);

    if (src_dev / 2 != dst_dev / 2) {
        fprintf(stderr, "WARN: src/dst look cross-card; KL1 P2P only supports same card\n");
    }

    size_t mat = (size_t)batch * feat * sizeof(float16);
    bool pin = false;
    float16 *h_in = (float16 *)halloc(mat, &pin);
    float16 *h_out = (float16 *)halloc(mat, &pin);
    if (!h_in || !h_out)
        return 1;
    for (size_t i = 0; i < (size_t)batch * feat; i++)
        h_in[i] = float16(1.0f);

    Stage A, B, SoloA, SoloB;
    if (!A.init(src_dev, batch, feat, layers_a) || !B.init(dst_dev, batch, feat, layers_b)) {
        fprintf(stderr, "stage init failed\n");
        return 1;
    }
    // Single-PD baseline: both stages on src_dev
    if (!SoloA.init(src_dev, batch, feat, layers_a) || !SoloB.init(src_dev, batch, feat, layers_b)) {
        fprintf(stderr, "solo init failed\n");
        return 1;
    }

    auto dual_once = [&]() -> int {
        xpu_set_device(src_dev);
        if (xpu_memcpy(A.in, h_in, mat, XPU_HOST_TO_DEVICE))
            return -1;
        xpu_wait();
        if (A.run_layers(A.in, A.out))
            return -2;
        // peer: src A.out → dst B.in
        if (xpu_memcpy_peer(dst_dev, B.in, src_dev, A.out, mat))
            return -3;
        xpu_wait();
        xpu_set_device(dst_dev);
        if (B.run_layers(B.in, B.out))
            return -4;
        if (xpu_memcpy(h_out, B.out, mat, XPU_DEVICE_TO_HOST))
            return -5;
        xpu_wait();
        return 0;
    };

    auto solo_once = [&]() -> int {
        xpu_set_device(src_dev);
        if (xpu_memcpy(SoloA.in, h_in, mat, XPU_HOST_TO_DEVICE))
            return -1;
        xpu_wait();
        if (SoloA.run_layers(SoloA.in, SoloA.out))
            return -2;
        // local D2D instead of peer
        if (xpu_memcpy(SoloB.in, SoloA.out, mat, XPU_DEVICE_TO_DEVICE))
            return -3;
        xpu_wait();
        if (SoloB.run_layers(SoloB.in, SoloB.out))
            return -4;
        if (xpu_memcpy(h_out, SoloB.out, mat, XPU_DEVICE_TO_HOST))
            return -5;
        xpu_wait();
        return 0;
    };

    printf("\n--- Dual-PD + P2P ---\n");
    for (int i = 0; i < warmup; i++) {
        int r = dual_once();
        if (r) {
            fprintf(stderr, "dual warmup failed ret=%d\n", r);
            return 1;
        }
    }
    double t0 = now_us();
    for (int i = 0; i < runs; i++) {
        int r = dual_once();
        if (r) {
            fprintf(stderr, "dual run failed ret=%d\n", r);
            return 1;
        }
    }
    double t1 = now_us();
    double dual_ms = (t1 - t0) / 1000.0 / runs;
    double dual_sps = batch * 1000.0 / dual_ms;
    printf("dual+P2P: %.3f ms/batch | %.1f samples/s\n", dual_ms, dual_sps);

    printf("\n--- Single-PD serial (D2D mid) ---\n");
    for (int i = 0; i < warmup; i++) {
        if (solo_once()) {
            fprintf(stderr, "solo warmup failed\n");
            return 1;
        }
    }
    t0 = now_us();
    for (int i = 0; i < runs; i++) {
        if (solo_once()) {
            fprintf(stderr, "solo run failed\n");
            return 1;
        }
    }
    t1 = now_us();
    double solo_ms = (t1 - t0) / 1000.0 / runs;
    double solo_sps = batch * 1000.0 / solo_ms;
    printf("single-PD: %.3f ms/batch | %.1f samples/s\n", solo_ms, solo_sps);

    printf("\nSUMMARY dual_ms=%.3f solo_ms=%.3f ratio_solo/dual=%.2f\n", dual_ms, solo_ms,
           solo_ms / dual_ms);
    printf("Note: dual may be slower if stages are small (P2P overhead); value is functional split.\n");

    A.destroy();
    B.destroy();
    SoloA.destroy();
    SoloB.destroy();
    hfree(h_in, pin);
    hfree(h_out, pin);
    return 0;
}
