// xpu_host_alloc smoke test
#include <stdio.h>
#include <string.h>
#include <xpu/runtime.h>

int main() {
    int dc = 0;
    xpu_device_count(&dc);
    xpu_set_device(0);

    void *ptr = nullptr;
    size_t sz = 64 * 1024 * 1024;

    printf("=== xpu_host_alloc test (%zu MB) ===\n", sz / (1024 * 1024));
    int ret = xpu_host_alloc(&ptr, sz, 0);
    printf("xpu_host_alloc ret=%d ptr=%p\n", ret, ptr);
    if (ret != 0 || !ptr)
        return 1;

    memset(ptr, 0x5A, sz);
    unsigned char *p = (unsigned char *)ptr;
    int ok = (p[0] == 0x5A && p[sz - 1] == 0x5A);
    printf("pattern check: %s\n", ok ? "PASS" : "FAIL");

    ret = xpu_host_free(ptr);
    printf("xpu_host_free ret=%d\n", ret);
    return (ok && ret == 0) ? 0 : 1;
}