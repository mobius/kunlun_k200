// Pageable H2D/D2H pattern integrity (S7 regression gate).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <xpu/runtime.h>

int main() {
    const size_t sz = 64ULL << 20;
    void *d = nullptr;
    void *h = nullptr;
    void *h2 = nullptr;

    xpu_set_device(0);
    if (xpu_malloc(&d, sz) != 0) {
        printf("xpu_malloc FAIL\n");
        return 1;
    }
    h = aligned_alloc(4096, sz);
    h2 = aligned_alloc(4096, sz);
    if (!h || !h2) {
        printf("host alloc FAIL\n");
        return 1;
    }

    for (size_t i = 0; i < sz; i++)
        ((unsigned char *)h)[i] = (unsigned char)(i * 131u + 7u);
    memset(h2, 0, sz);

    if (xpu_memcpy(d, h, sz, XPU_HOST_TO_DEVICE) != 0) {
        printf("H2D FAIL\n");
        return 1;
    }
    xpu_wait();
    if (xpu_memcpy(h2, d, sz, XPU_DEVICE_TO_HOST) != 0) {
        printf("D2H FAIL\n");
        return 1;
    }
    xpu_wait();

    if (memcmp(h, h2, sz) != 0) {
        printf("pageable 64MB pattern: FAIL\n");
        return 1;
    }
    printf("pageable 64MB pattern: PASS\n");

    xpu_free(d);
    free(h);
    free(h2);
    return 0;
}
