/*!
 * XPU-Denoise: Lightweight CNN Denoising Engine for K200
 * Model: 8× Residual Convolutional Blocks, FP16
 * 
 * Usage: ./xpu_denoise <device_id> <weights_fp16.bin> <input.ppm> <output.ppm>
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <chrono>
#include <xpu/runtime.h>
#include <xpu/xdnn.h>

namespace xdnn = baidu::xpu::api;

constexpr int IN_CH=3, OUT_CH=3, MID_CH=32, N_BLOCKS=8, KSIZE=3;

static inline double now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// --- PPM I/O ---
static bool read_ppm(const char* path, std::vector<uint8_t>& d, int& w, int& h) {
    FILE* f=fopen(path,"rb"); if(!f)return false;
    char m[3]; int mv;
    if(fscanf(f,"%2s %d %d %d",m,&w,&h,&mv)!=4||strcmp(m,"P6")||mv!=255){fclose(f);return false;}
    fgetc(f); d.resize(w*h*3); fread(d.data(),1,d.size(),f); fclose(f); return true;
}
static bool write_ppm(const char* path, const uint8_t* d, int w, int h) {
    FILE* f=fopen(path,"wb"); if(!f)return false;
    fprintf(f,"P6\n%d %d\n255\n",w,h); fwrite(d,1,w*h*3,f); fclose(f); return true;
}

// --- Weight layout ---
struct DenoiseWeights {
    static int n_fp16() {
        return 3*3*3*32 + N_BLOCKS*2*(3*3*32*32) + 3*3*32*32 + 3*3*32*3;
    }
    static int n_bias() {
        return 32 + N_BLOCKS*2*32 + 32 + 3; // first + 8×2 + mid + last
    }
    static int n_total_fp16() { return n_fp16() + n_bias(); }
};

class DenoiseEngine {
    xdnn::Context* ctx_;
    float16 *w16_, *b16_; // weights & biases on GPU (all stored as float16)
    float *b32_;          // biases as float32 (what conv2d_fusion expects)
    float16 *feat_, *feat_skip_, *rb_in_, *rb_mid_, *rb_out_;
    
    static float16* bias_offset(int idx) {
        // biases are stored after all weights: offset = n_fp16() + idx
        return nullptr; // placehoder, use b32_ instead
    }
    
public:
    DenoiseEngine(int devid) {
        xpu_set_device(devid);
        ctx_=xdnn::create_context();
        w16_=b16_=feat_=feat_skip_=rb_in_=rb_mid_=rb_out_=nullptr; b32_=nullptr;
    }
    ~DenoiseEngine() { cleanup(); }
    
    bool init() {
        int nw = DenoiseWeights::n_total_fp16();
        int nb = DenoiseWeights::n_bias();
        printf("Model: %d weights (%.1f KB) + %d biases\n", nw, nw*2.0/1024, nb);
        if(xpu_malloc((void**)&w16_,nw*sizeof(float16))) return false;
        if(xpu_malloc((void**)&b32_,nb*sizeof(float)))  return false;
        return true;
    }
    
    bool load_weights(const char* path) {
        int nw = DenoiseWeights::n_total_fp16();
        int nb = DenoiseWeights::n_bias();
        std::vector<float16> host(nw);
        FILE* f=fopen(path,"rb"); if(!f)return false;
        size_t nr = fread(host.data(),sizeof(float16),nw,f); fclose(f);
        if(nr != (size_t)nw){fprintf(stderr,"weight file too small\n");return false;}
        
        // Upload fp16 weights and biases
        xpu_memcpy(w16_,host.data(),nw*sizeof(float16),XPU_HOST_TO_DEVICE);
        xpu_wait();
        
        // Convert biases to float32
        int nfp16 = DenoiseWeights::n_fp16();
        std::vector<float> b32_host(nb);
        for(int i=0;i<nb;i++) b32_host[i]=(float)host[nfp16+i];
        xpu_memcpy(b32_,b32_host.data(),nb*sizeof(float),XPU_HOST_TO_DEVICE);
        xpu_wait();
        printf("Loaded weights from %s\n",path);
        return true;
    }
    
    void cleanup() {
        if(w16_){xpu_free(w16_);w16_=nullptr;}
        if(b32_){xpu_free(b32_);b32_=nullptr;}
        if(feat_){xpu_free(feat_);feat_=nullptr;}
        if(feat_skip_){xpu_free(feat_skip_);feat_skip_=nullptr;}
        if(rb_in_){xpu_free(rb_in_);rb_in_=nullptr;}
        if(rb_mid_){xpu_free(rb_mid_);rb_mid_=nullptr;}
        if(rb_out_){xpu_free(rb_out_);rb_out_=nullptr;}
        if(ctx_){xdnn::destroy_context(ctx_);ctx_=nullptr;}
    }
    
    bool forward(int H, int W, const float16* input, float16* output) {
        int fsz = H * W * MID_CH;
        auto ensure=[&](float16*&p,int n){if(!p&&xpu_malloc((void**)&p,n*sizeof(float16)))return false;return true;};
        if(!ensure(feat_,fsz)||!ensure(feat_skip_,fsz)||!ensure(rb_in_,fsz)||!ensure(rb_mid_,fsz)||!ensure(rb_out_,fsz))return false;
        
        int bn=1, gp=1;
        std::vector<int64_t> ks{KSIZE,KSIZE}, st{1,1}, pd{KSIZE/2,KSIZE/2}, dl{1,1};
        int wp=0, bp=0;
        auto wt = [&](int n){int r=wp;wp+=n;return w16_+r;};
        auto bi = [&](){return b32_+(bp++);};
        
        // conv_first: 3→32
        {
            float16* wgt = wt(3*3*3*32); float* b = bi();
            int ret = xdnn::conv2d_fusion<float16,float16,float16,signed char>(
                ctx_, input, wgt, feat_, bn, IN_CH, H, W, MID_CH,
                ks, st, pd, dl, gp, nullptr, nullptr, nullptr, true,
                b, nullptr, xdnn::Activation_t::LEAKY_RELU);
            if(ret){fprintf(stderr,"conv_first err=%d\n",ret);return false;} xpu_wait();
        }
        xpu_memcpy(feat_skip_, feat_, fsz*sizeof(float16), XPU_DEVICE_TO_DEVICE); xpu_wait();
        
        // 8 residual blocks
        for(int b=0; b<N_BLOCKS; b++){
            xpu_memcpy(rb_in_, feat_, fsz*sizeof(float16), XPU_DEVICE_TO_DEVICE); xpu_wait();
            // conv1: 32→32 LeakyReLU
            {float16* wgt=wt(3*3*32*32); float* bs=bi();
            int ret=xdnn::conv2d_fusion<float16,float16,float16,signed char>(
                ctx_,feat_,wgt,rb_mid_,bn,MID_CH,H,W,MID_CH,ks,st,pd,dl,gp,nullptr,nullptr,nullptr,true,bs,nullptr,xdnn::Activation_t::LEAKY_RELU);
            if(ret){fprintf(stderr,"rb%d_c1 err=%d\n",b,ret);return false;}xpu_wait();}
            // conv2: 32→32 LINEAR
            {float16* wgt=wt(3*3*32*32); float* bs=bi();
            int ret=xdnn::conv2d_fusion<float16,float16,float16,signed char>(
                ctx_,rb_mid_,wgt,rb_out_,bn,MID_CH,H,W,MID_CH,ks,st,pd,dl,gp,nullptr,nullptr,nullptr,true,bs,nullptr,xdnn::Activation_t::LINEAR);
            if(ret){fprintf(stderr,"rb%d_c2 err=%d\n",b,ret);return false;}xpu_wait();}
            // add
            {int ret=xdnn::add<float16>(ctx_,rb_in_,rb_out_,feat_,fsz);
            if(ret){fprintf(stderr,"rb%d_add err=%d\n",b,ret);return false;}xpu_wait();}
        }
        
        // conv_mid: 32→32 LINEAR
        {float16* wgt=wt(3*3*32*32); float* bs=bi();
        int ret=xdnn::conv2d_fusion<float16,float16,float16,signed char>(
            ctx_,feat_,wgt,rb_mid_,bn,MID_CH,H,W,MID_CH,ks,st,pd,dl,gp,nullptr,nullptr,nullptr,true,bs,nullptr,xdnn::Activation_t::LINEAR);
        if(ret){fprintf(stderr,"conv_mid err=%d\n",ret);return false;}xpu_wait();}
        
        // global skip
        {int ret=xdnn::add<float16>(ctx_,feat_skip_,rb_mid_,feat_,fsz);
        if(ret){fprintf(stderr,"global_add err=%d\n",ret);return false;}xpu_wait();}
        
        // conv_last: 32→3 LINEAR
        {float16* wgt=wt(3*3*32*3); float* bs=bi();
        int ret=xdnn::conv2d_fusion<float16,float16,float16,signed char>(
            ctx_,feat_,wgt,output,bn,MID_CH,H,W,OUT_CH,ks,st,pd,dl,gp,nullptr,nullptr,nullptr,true,bs,nullptr,xdnn::Activation_t::LINEAR);
        if(ret){fprintf(stderr,"conv_last err=%d\n",ret);return false;}xpu_wait();}
        
        return true;
    }
};

// --- Weight init (for testing) ---
void init_rand_weights(float16* h, int n) {
    srand(42);
    for(int i=0;i<n;i++) h[i]=float16(((float)rand()/RAND_MAX-0.5f)*0.1f);
}

// --- Main ---
int main(int argc,char** argv) {
    if(argc<5){fprintf(stderr,"Usage: %s <dev> <weights.bin|rand> <in.ppm> <out.ppm>\n",argv[0]);return 1;}
    int dev=atoi(argv[1]);
    const char *wp=argv[2],*ip=argv[3],*op=argv[4];
    
    int w,h; std::vector<uint8_t> img;
    if(!read_ppm(ip,img,w,h))return 1;
    printf("Input: %dx%d\n",w,h);
    
    std::vector<float16> in_fp(w*h*3);
    for(int i=0;i<w*h*3;i++) in_fp[i]=float16(img[i]/255.0f);
    
    float16 *gi,*go;
    xpu_set_device(dev);
    xpu_malloc((void**)&gi,w*h*3*sizeof(float16));
    xpu_malloc((void**)&go,w*h*3*sizeof(float16));
    xpu_memcpy(gi,in_fp.data(),w*h*3*sizeof(float16),XPU_HOST_TO_DEVICE);xpu_wait();
    
    DenoiseEngine eng(dev);
    if(!eng.init())return 1;
    
    if(!strcmp(wp,"rand")){
        int nw=DenoiseWeights::n_total_fp16();
        std::vector<float16> rw(nw); init_rand_weights(rw.data(),nw);
        FILE* f=fopen("/tmp/xpu_denoise_rand.bin","wb");
        fwrite(rw.data(),sizeof(float16),nw,f);fclose(f);
        eng.load_weights("/tmp/xpu_denoise_rand.bin");
    }else eng.load_weights(wp);
    
    printf("Running inference...\n");
    double t0=now_us();
    bool ok=eng.forward(h,w,gi,go);
    double t1=now_us();
    if(!ok){fprintf(stderr,"Inference failed\n");return 1;}
    printf("Inference: %.2f ms\n",(t1-t0)/1000.0);
    
    std::vector<float16> ofp(w*h*3);
    xpu_memcpy(ofp.data(),go,w*h*3*sizeof(float16),XPU_DEVICE_TO_HOST);xpu_wait();
    
    std::vector<uint8_t> oi(w*h*3);
    for(int i=0;i<w*h*3;i++){float v=(float)ofp[i];if(v<0)v=0;if(v>1)v=1;oi[i]=(uint8_t)(v*255.0f);}
    write_ppm(op,oi.data(),w,h);
    printf("Output: %s\n",op);
    
    xpu_free(gi);xpu_free(go);
    return 0;
}
