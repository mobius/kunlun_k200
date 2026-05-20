// P2P DMA test between PD0 and PD1 on same K200 card
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <xpu/runtime.h>

static inline double us() {
    auto n = std::chrono::high_resolution_clock::now();
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(n.time_since_epoch()).count();
}
int main() {
    int dc;
    xpu_device_count(&dc);
    printf("Devices: %d\n", dc);
    if (dc < 2) { printf("Need >=2 devices\n"); return 1; }

    struct { size_t sz; const char* n; int runs; } s[] = {
        {1ULL*1024*1024,      "1MB",   50},
        {4ULL*1024*1024,      "4MB",   50},
        {16ULL*1024*1024,    "16MB",   30},
        {64ULL*1024*1024,    "64MB",   20},
        {256ULL*1024*1024,  "256MB",   10},
        {512ULL*1024*1024,  "512MB",    5},
    };
    int ns = sizeof(s)/sizeof(s[0]);

    printf("=== P2P Memcpy: PD0 -> PD1 (same card) ===\n");
    printf("%-8s %10s %10s %10s\n", "Size", "P2P GB/s", "D2D GB/s", "Status");

    for (int i=0; i<ns; i++) {
        void *src, *dst;
        xpu_set_device(0);
        xpu_malloc(&src, s[i].sz);
        xpu_set_device(1);
        xpu_malloc(&dst, s[i].sz);

        /* Init src on dev0 */
        xpu_set_device(0);
        void* hbuf = aligned_alloc(4096, s[i].sz);
        memset(hbuf, 0xAB, s[i].sz);
        xpu_memcpy(src, hbuf, s[i].sz, XPU_HOST_TO_DEVICE);
        xpu_wait();
        free(hbuf);

        /* P2P copy: dev0 -> dev1 via xpu_memcpy_peer */
        for (int r=0; r<3; r++) {
            xpu_memcpy_peer(1, dst, 0, src, s[i].sz);
            xpu_wait();
        }
        double t0 = us();
        for (int r=0; r<s[i].runs; r++) {
            xpu_memcpy_peer(1, dst, 0, src, s[i].sz);
            xpu_wait();
        }
        double t1 = us();
        double p2p_bw = (double)(s[i].sz*s[i].runs)/((t1-t0)*1e-6)/(1.0*1024*1024*1024);

        /* D2D baseline on dev0 */
        void* src2, *dst2;
        xpu_set_device(0);
        xpu_malloc(&src2, s[i].sz);
        xpu_malloc(&dst2, s[i].sz);
        xpu_memcpy(src2, src, s[i].sz, XPU_DEVICE_TO_DEVICE);
        for (int r=0; r<3; r++) {
            xpu_memcpy(dst2, src2, s[i].sz, XPU_DEVICE_TO_DEVICE);
            xpu_wait();
        }
        t0 = us();
        for (int r=0; r<s[i].runs; r++) {
            xpu_memcpy(dst2, src2, s[i].sz, XPU_DEVICE_TO_DEVICE);
            xpu_wait();
        }
        t1 = us();
        double d2d_bw = (double)(s[i].sz*s[i].runs)/((t1-t0)*1e-6)/(1.0*1024*1024*1024);

        printf("%-8s %10.2f %10.2f %s\n", s[i].n, p2p_bw, d2d_bw,
               p2p_bw > 0.1 ? "OK" : "TIMEOUT?");

        xpu_free(src); xpu_free(dst); xpu_free(src2); xpu_free(dst2);
    }

    /* Also test cross-card P2P */
    printf("\n=== P2P Memcpy: PD0 -> PD2 (cross-card) ===\n");
    if (dc >= 3) {
        void *src, *dst;
        xpu_set_device(0);
        xpu_malloc(&src, 64*1024*1024);
        xpu_set_device(2);
        xpu_malloc(&dst, 64*1024*1024);
        void* h = aligned_alloc(4096, 64*1024*1024);
        memset(h, 0xAB, 64*1024*1024);
        xpu_memcpy(src, h, 64*1024*1024, XPU_HOST_TO_DEVICE);
        xpu_wait();
        free(h);
        for(int r=0;r<3;r++){xpu_memcpy_peer(2,dst,0,src,64*1024*1024);xpu_wait();}
        double t0=us();
        for(int r=0;r<10;r++){xpu_memcpy_peer(2,dst,0,src,64*1024*1024);xpu_wait();}
        double t1=us();
        printf("  64MB cross-card: %.2f GB/s\n", (double)(64*1024*1024*10)/((t1-t0)*1e-6)/(1.0*1024*1024*1024));
        xpu_free(src); xpu_free(dst);
    }

    printf("\nDone.\n");
    return 0;
}
