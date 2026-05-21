// P2P data verification test
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
    int dc; xpu_device_count(&dc);

    size_t sz = 16*1024*1024; // 16 MB
    void *sdev0, *ddev1, *ddev0;

    // Alloc on dev0
    xpu_set_device(0);
    xpu_malloc(&sdev0, sz);
    xpu_malloc(&ddev0, sz);  // local D2D destination on dev0

    // Alloc on dev1
    xpu_set_device(1);
    xpu_malloc(&ddev1, sz);

    // Fill source with known pattern on dev0
    uint32_t *hbuf = (uint32_t*)aligned_alloc(4096, sz);
    for (size_t i=0; i<sz/4; i++) hbuf[i] = (uint32_t)(i & 0xFFFFFFFF);
    xpu_set_device(0);
    xpu_memcpy(sdev0, hbuf, sz, XPU_HOST_TO_DEVICE); xpu_wait();

    // --- Test 1: Local D2D copy on dev0 ---
    printf("=== Test 1: Local D2D (dev0 -> dev0) ===\n");
    xpu_memcpy(ddev0, sdev0, sz, XPU_DEVICE_TO_DEVICE); xpu_wait();

    // Verify D2D
    uint32_t *verify = (uint32_t*)aligned_alloc(4096, sz);
    memset(verify, 0, sz);
    xpu_memcpy(verify, ddev0, sz, XPU_DEVICE_TO_HOST); xpu_wait();

    int d2d_ok = 1;
    for (size_t i=0; i<sz/4; i++) {
        if (verify[i] != (uint32_t)(i & 0xFFFFFFFF)) { printf("  D2D FAIL at %zu: expected 0x%x got 0x%x\n", i, (uint32_t)i, verify[i]); d2d_ok = 0; break; }
    }
    printf("  D2D verification: %s\n\n", d2d_ok ? "PASS" : "FAIL");

    // --- Test 2: P2P copy dev0 -> dev1 ---
    printf("=== Test 2: P2P (dev0 -> dev1) ===\n");
    for (int r=0; r<3; r++) {
        xpu_memcpy_peer(1, ddev1, 0, sdev0, sz); xpu_wait();
    }

    // Read back from dev1
    memset(verify, 0xFF, sz);  // fill with 0xFF to detect no-copy
    xpu_set_device(1);
    xpu_memcpy(verify, ddev1, sz, XPU_DEVICE_TO_HOST); xpu_wait();

    int p2p_ok = 1;
    for (size_t i=0; i<sz/4; i++) {
        if (verify[i] != (uint32_t)(i & 0xFFFFFFFF)) {
            printf("  P2P FAIL at %zu: expected 0x%x got 0x%x\n", i, (uint32_t)(i&0xFFFFFFFF), verify[i]);
            p2p_ok = 0;
            if (i > 5) break;
        }
    }
    if (p2p_ok) printf("  P2P verification: PASS\n");
    else        printf("  P2P verification: FAIL\n");

    // --- Test 3: P2P copy dev0 -> dev1 (different pattern) ---
    printf("\n=== Test 3: P2P 2nd pattern ===\n");
    for (size_t i=0; i<sz/4; i++) hbuf[i] = 0xDEADBEEF;
    xpu_set_device(0);
    xpu_memcpy(sdev0, hbuf, sz, XPU_HOST_TO_DEVICE); xpu_wait();

    xpu_memcpy_peer(1, ddev1, 0, sdev0, sz); xpu_wait();

    memset(verify, 0, sz);
    xpu_set_device(1);
    xpu_memcpy(verify, ddev1, sz, XPU_DEVICE_TO_HOST); xpu_wait();

    int p2p2_ok = 1;
    for (size_t i=0; i<sz/4; i++) {
        if (verify[i] != 0xDEADBEEF) { printf("  P2P2 FAIL at %zu: 0x%x\n", i, verify[i]); p2p2_ok = 0; break; }
    }
    printf("  P2P2 verification: %s\n", p2p2_ok ? "PASS" : "FAIL");

    free(hbuf);
    free(verify);
    xpu_free(sdev0); xpu_free(ddev0); xpu_free(ddev1);

    printf("\nDone.\n");
    return 0;
}
