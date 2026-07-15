/*!
 * C2 XPU denoise — FP16 residual CNN for K200.
 *
 * Usage:
 *   ./xpu_denoise <dev> <weights.bin|rand> <in.ppm> <out.ppm> [options]
 * Options:
 *   --pinned | --pageable   host staging (default: pinned = xpu_host_alloc)
 *   --bench N               timed runs after warmup (default 0 = single shot)
 *   --warmup N              warmup iters for --bench (default 3)
 */
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <xpu/runtime.h>
#include <xpu/xdnn.h>

namespace xdnn = baidu::xpu::api;

constexpr int IN_CH = 3, OUT_CH = 3, MID_CH = 32, N_BLOCKS = 8, KSIZE = 3;

static double now_us() {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

static bool read_ppm(const char *path, std::vector<uint8_t> &d, int &w, int &h) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    char m[3];
    int mv;
    if (fscanf(f, "%2s %d %d %d", m, &w, &h, &mv) != 4 || strcmp(m, "P6") || mv != 255) {
        fclose(f);
        return false;
    }
    fgetc(f);
    d.resize((size_t)w * h * 3);
    fread(d.data(), 1, d.size(), f);
    fclose(f);
    return true;
}

static bool write_ppm(const char *path, const uint8_t *d, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    fwrite(d, 1, (size_t)w * h * 3, f);
    fclose(f);
    return true;
}

struct DenoiseWeights {
    static int n_fp16() {
        return 3 * 3 * 3 * 32 + N_BLOCKS * 2 * (3 * 3 * 32 * 32) + 3 * 3 * 32 * 32 + 3 * 3 * 32 * 3;
    }
    static int n_bias() { return 32 + N_BLOCKS * 2 * 32 + 32 + 3; }
    static int n_total_fp16() { return n_fp16() + n_bias(); }
};

static void *host_buf_alloc(size_t n, bool pinned, bool *used_pin) {
    *used_pin = false;
    if (pinned) {
        void *p = nullptr;
        if (xpu_host_alloc(&p, n, 0) == 0 && p) {
            *used_pin = true;
            return p;
        }
        fprintf(stderr, "WARN: xpu_host_alloc failed, fallback pageable\n");
    }
    return aligned_alloc(4096, n);
}

static void host_buf_free(void *p, bool used_pin) {
    if (!p)
        return;
    if (used_pin)
        xpu_host_free(p);
    else
        free(p);
}

class DenoiseEngine {
    xdnn::Context *ctx_;
    float16 *w16_;
    float *b32_;
    float16 *feat_, *feat_skip_, *rb_in_, *rb_mid_, *rb_out_;

public:
    explicit DenoiseEngine(int devid) {
        xpu_set_device(devid);
        ctx_ = xdnn::create_context();
        if (ctx_)
            ctx_->set_ncluster(4);
        w16_ = nullptr;
        b32_ = nullptr;
        feat_ = feat_skip_ = rb_in_ = rb_mid_ = rb_out_ = nullptr;
    }
    ~DenoiseEngine() { cleanup(); }

    bool init() {
        int nw = DenoiseWeights::n_total_fp16();
        int nb = DenoiseWeights::n_bias();
        printf("Model: %d fp16 vals (%.1f KB) + %d biases, ncluster=4\n", nw, nw * 2.0 / 1024, nb);
        if (xpu_malloc((void **)&w16_, (size_t)nw * sizeof(float16)))
            return false;
        if (xpu_malloc((void **)&b32_, (size_t)nb * sizeof(float)))
            return false;
        return true;
    }

    bool load_weights(const char *path, bool pinned_upload) {
        int nw = DenoiseWeights::n_total_fp16();
        int nb = DenoiseWeights::n_bias();
        bool pin = false;
        float16 *host = (float16 *)host_buf_alloc((size_t)nw * sizeof(float16), pinned_upload, &pin);
        if (!host)
            return false;
        FILE *f = fopen(path, "rb");
        if (!f) {
            host_buf_free(host, pin);
            return false;
        }
        size_t nr = fread(host, sizeof(float16), (size_t)nw, f);
        fclose(f);
        if (nr != (size_t)nw) {
            fprintf(stderr, "weight file too small\n");
            host_buf_free(host, pin);
            return false;
        }
        xpu_memcpy(w16_, host, (size_t)nw * sizeof(float16), XPU_HOST_TO_DEVICE);
        xpu_wait();

        int nfp16 = DenoiseWeights::n_fp16();
        std::vector<float> b32_host((size_t)nb);
        for (int i = 0; i < nb; i++)
            b32_host[(size_t)i] = (float)host[nfp16 + i];
        xpu_memcpy(b32_, b32_host.data(), (size_t)nb * sizeof(float), XPU_HOST_TO_DEVICE);
        xpu_wait();
        host_buf_free(host, pin);
        printf("Loaded weights from %s (upload=%s)\n", path, pin ? "host_alloc" : "pageable");
        return true;
    }

    void cleanup() {
        if (w16_) {
            xpu_free(w16_);
            w16_ = nullptr;
        }
        if (b32_) {
            xpu_free(b32_);
            b32_ = nullptr;
        }
        if (feat_) {
            xpu_free(feat_);
            feat_ = nullptr;
        }
        if (feat_skip_) {
            xpu_free(feat_skip_);
            feat_skip_ = nullptr;
        }
        if (rb_in_) {
            xpu_free(rb_in_);
            rb_in_ = nullptr;
        }
        if (rb_mid_) {
            xpu_free(rb_mid_);
            rb_mid_ = nullptr;
        }
        if (rb_out_) {
            xpu_free(rb_out_);
            rb_out_ = nullptr;
        }
        if (ctx_) {
            xdnn::destroy_context(ctx_);
            ctx_ = nullptr;
        }
    }

    bool forward(int H, int W, const float16 *input, float16 *output) {
        int fsz = H * W * MID_CH;
        auto ensure = [&](float16 *&p, int n) {
            if (!p && xpu_malloc((void **)&p, (size_t)n * sizeof(float16)))
                return false;
            return true;
        };
        if (!ensure(feat_, fsz) || !ensure(feat_skip_, fsz) || !ensure(rb_in_, fsz) ||
            !ensure(rb_mid_, fsz) || !ensure(rb_out_, fsz))
            return false;

        int bn = 1, gp = 1;
        std::vector<int64_t> ks{KSIZE, KSIZE}, st{1, 1}, pd{KSIZE / 2, KSIZE / 2}, dl{1, 1};
        int wp = 0, bp = 0;
        auto wt = [&](int n) {
            int r = wp;
            wp += n;
            return w16_ + r;
        };
        auto bi = [&]() { return b32_ + (bp++); };

        {
            float16 *wgt = wt(3 * 3 * 3 * 32);
            float *b = bi();
            int ret = xdnn::conv2d_fusion<float16, float16, float16, signed char>(
                ctx_, input, wgt, feat_, bn, IN_CH, H, W, MID_CH, ks, st, pd, dl, gp, nullptr,
                nullptr, nullptr, true, b, nullptr, xdnn::Activation_t::LEAKY_RELU);
            if (ret) {
                fprintf(stderr, "conv_first err=%d\n", ret);
                return false;
            }
            xpu_wait();
        }
        xpu_memcpy(feat_skip_, feat_, (size_t)fsz * sizeof(float16), XPU_DEVICE_TO_DEVICE);
        xpu_wait();

        for (int b = 0; b < N_BLOCKS; b++) {
            xpu_memcpy(rb_in_, feat_, (size_t)fsz * sizeof(float16), XPU_DEVICE_TO_DEVICE);
            xpu_wait();
            {
                float16 *wgt = wt(3 * 3 * 32 * 32);
                float *bs = bi();
                int ret = xdnn::conv2d_fusion<float16, float16, float16, signed char>(
                    ctx_, feat_, wgt, rb_mid_, bn, MID_CH, H, W, MID_CH, ks, st, pd, dl, gp, nullptr,
                    nullptr, nullptr, true, bs, nullptr, xdnn::Activation_t::LEAKY_RELU);
                if (ret) {
                    fprintf(stderr, "rb%d_c1 err=%d\n", b, ret);
                    return false;
                }
                xpu_wait();
            }
            {
                float16 *wgt = wt(3 * 3 * 32 * 32);
                float *bs = bi();
                int ret = xdnn::conv2d_fusion<float16, float16, float16, signed char>(
                    ctx_, rb_mid_, wgt, rb_out_, bn, MID_CH, H, W, MID_CH, ks, st, pd, dl, gp,
                    nullptr, nullptr, nullptr, true, bs, nullptr, xdnn::Activation_t::LINEAR);
                if (ret) {
                    fprintf(stderr, "rb%d_c2 err=%d\n", b, ret);
                    return false;
                }
                xpu_wait();
            }
            {
                int ret = xdnn::add<float16>(ctx_, rb_in_, rb_out_, feat_, fsz);
                if (ret) {
                    fprintf(stderr, "rb%d_add err=%d\n", b, ret);
                    return false;
                }
                xpu_wait();
            }
        }

        {
            float16 *wgt = wt(3 * 3 * 32 * 32);
            float *bs = bi();
            int ret = xdnn::conv2d_fusion<float16, float16, float16, signed char>(
                ctx_, feat_, wgt, rb_mid_, bn, MID_CH, H, W, MID_CH, ks, st, pd, dl, gp, nullptr,
                nullptr, nullptr, true, bs, nullptr, xdnn::Activation_t::LINEAR);
            if (ret) {
                fprintf(stderr, "conv_mid err=%d\n", ret);
                return false;
            }
            xpu_wait();
        }
        {
            int ret = xdnn::add<float16>(ctx_, feat_skip_, rb_mid_, feat_, fsz);
            if (ret) {
                fprintf(stderr, "global_add err=%d\n", ret);
                return false;
            }
            xpu_wait();
        }
        {
            float16 *wgt = wt(3 * 3 * 32 * 3);
            float *bs = bi();
            int ret = xdnn::conv2d_fusion<float16, float16, float16, signed char>(
                ctx_, feat_, wgt, output, bn, MID_CH, H, W, OUT_CH, ks, st, pd, dl, gp, nullptr,
                nullptr, nullptr, true, bs, nullptr, xdnn::Activation_t::LINEAR);
            if (ret) {
                fprintf(stderr, "conv_last err=%d\n", ret);
                return false;
            }
            xpu_wait();
        }
        return true;
    }
};

static void init_rand_weights(float16 *h, int n) {
    srand(42);
    for (int i = 0; i < n; i++)
        h[i] = float16(((float)rand() / RAND_MAX - 0.5f) * 0.1f);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr,
                "Usage: %s <dev> <weights.bin|rand> <in.ppm> <out.ppm> "
                "[--pinned|--pageable] [--bench N] [--warmup N]\n",
                argv[0]);
        return 1;
    }
    int dev = atoi(argv[1]);
    const char *wp = argv[2], *ip = argv[3], *op = argv[4];
    bool pinned = true;
    int bench = 0, warmup = 3;
    for (int i = 5; i < argc; i++) {
        if (!strcmp(argv[i], "--pinned"))
            pinned = true;
        else if (!strcmp(argv[i], "--pageable"))
            pinned = false;
        else if (!strcmp(argv[i], "--bench") && i + 1 < argc)
            bench = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--warmup") && i + 1 < argc)
            warmup = atoi(argv[++i]);
    }

    int w, h;
    std::vector<uint8_t> img;
    if (!read_ppm(ip, img, w, h)) {
        fprintf(stderr, "read_ppm failed: %s\n", ip);
        return 1;
    }
    printf("C2 denoise | Input: %dx%d | staging=%s\n", w, h, pinned ? "host_alloc" : "pageable");

    size_t npx = (size_t)w * h * 3;
    bool pin_in = false, pin_out = false;
    float16 *h_in = (float16 *)host_buf_alloc(npx * sizeof(float16), pinned, &pin_in);
    float16 *h_out = (float16 *)host_buf_alloc(npx * sizeof(float16), pinned, &pin_out);
    if (!h_in || !h_out) {
        fprintf(stderr, "host staging alloc failed\n");
        return 1;
    }
    for (size_t i = 0; i < npx; i++)
        h_in[i] = float16(img[i] / 255.0f);

    float16 *gi = nullptr, *go = nullptr;
    xpu_set_device(dev);
    if (xpu_malloc((void **)&gi, npx * sizeof(float16)) ||
        xpu_malloc((void **)&go, npx * sizeof(float16))) {
        fprintf(stderr, "device malloc failed\n");
        return 1;
    }

    DenoiseEngine eng(dev);
    if (!eng.init())
        return 1;

    if (!strcmp(wp, "rand")) {
        int nw = DenoiseWeights::n_total_fp16();
        std::vector<float16> rw((size_t)nw);
        init_rand_weights(rw.data(), nw);
        FILE *f = fopen("/tmp/xpu_denoise_rand.bin", "wb");
        fwrite(rw.data(), sizeof(float16), (size_t)nw, f);
        fclose(f);
        if (!eng.load_weights("/tmp/xpu_denoise_rand.bin", pinned))
            return 1;
    } else if (!eng.load_weights(wp, pinned)) {
        fprintf(stderr, "load_weights failed: %s\n", wp);
        return 1;
    }

    auto one = [&]() -> bool {
        if (xpu_memcpy(gi, h_in, npx * sizeof(float16), XPU_HOST_TO_DEVICE))
            return false;
        xpu_wait();
        if (!eng.forward(h, w, gi, go))
            return false;
        if (xpu_memcpy(h_out, go, npx * sizeof(float16), XPU_DEVICE_TO_HOST))
            return false;
        xpu_wait();
        return true;
    };

    if (bench > 0) {
        for (int i = 0; i < warmup; i++) {
            if (!one()) {
                fprintf(stderr, "warmup failed\n");
                return 1;
            }
        }
        double t0 = now_us();
        for (int i = 0; i < bench; i++) {
            if (!one()) {
                fprintf(stderr, "bench iter failed\n");
                return 1;
            }
        }
        double t1 = now_us();
        double ms = (t1 - t0) / 1000.0 / bench;
        double fps = 1000.0 / ms;
        printf("BENCH runs=%d warmup=%d | %.3f ms/img | %.2f img/s | staging=%s\n", bench, warmup,
               ms, fps, (pin_in && pin_out) ? "host_alloc" : "pageable");
    } else {
        printf("Running single inference...\n");
        double t0 = now_us();
        if (!one()) {
            fprintf(stderr, "Inference failed\n");
            return 1;
        }
        double t1 = now_us();
        printf("Inference e2e: %.2f ms\n", (t1 - t0) / 1000.0);
    }

    std::vector<uint8_t> oi(npx);
    for (size_t i = 0; i < npx; i++) {
        float v = (float)h_out[i];
        if (v < 0)
            v = 0;
        if (v > 1)
            v = 1;
        oi[i] = (uint8_t)(v * 255.0f);
    }
    write_ppm(op, oi.data(), w, h);
    printf("Output: %s\n", op);

    xpu_free(gi);
    xpu_free(go);
    host_buf_free(h_in, pin_in);
    host_buf_free(h_out, pin_out);
    return 0;
}
