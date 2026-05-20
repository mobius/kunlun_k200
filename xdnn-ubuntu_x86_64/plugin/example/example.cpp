/*!\file gemm_test.cpp
 *
 * \brief A simple demo
 *
 * \author kunlun_api@baidu.com
 *
 * \copyright (C) 2022 KUNLUNXIN, Inc
 */
#include <assert.h>
#include "xpu/xdnn.h"
#include "xdnn_plugin.h"

namespace xdnn = baidu::xpu::api;

void print_matrix(int m, int n, std::vector<float>& matrix) {
    for (int i = 0; i < m; i++) {
        printf("    ");
        for (int j = 0; j < n; j++) {
            printf("%.1f ", matrix[i * n + j]);
        }
        puts("");
    }
}
int main() {
    int m = 2;
    int n = 3;
    int k = 2;
    int ret = 0;
    float* A = nullptr;
    ret = xpu_malloc((void**)(&A), m * k * sizeof(float));
    assert(ret == 0);
    float* B = nullptr;
    ret = xpu_malloc((void**)(&B), k * n * sizeof(float));
    assert(ret == 0);
    float* C = nullptr;
    ret = xpu_malloc((void**)(&C), m * n * sizeof(float));
    assert(ret == 0);
    std::vector<float> A_cpu = {1, 2, 2, 1};
    A_cpu.resize(m * k);
    std::vector<float> B_cpu = {0, 1, 2, 3, 4, 5};
    B_cpu.resize(k * n);
    std::vector<float> C_cpu(m * n, 0.0f);
    xpu_memcpy((void*)A, (void*) & (A_cpu[0]), m * k * sizeof(float), XPUMemcpyKind::XPU_HOST_TO_DEVICE);
    xpu_memcpy((void*)B, (void*) & (B_cpu[0]), k * n * sizeof(float), XPUMemcpyKind::XPU_HOST_TO_DEVICE);
    auto ctx = xdnn::create_context();

    ret = plugin::add2(ctx, C, C, m * n);
    assert(ret == 0);
    ret = plugin::sub2(ctx, C, C, m * n);
    assert(ret == 0);
    xpu_memcpy((void*) & (C_cpu[0]), (void*)C, 2 * sizeof(float), XPUMemcpyKind::XPU_DEVICE_TO_HOST);
    printf("A(%p):\n", A);
    print_matrix(m, k, A_cpu);
    printf("B(%p):\n", B);
    print_matrix(k, n, B_cpu);
    printf("C(%p) = A * B:\n", C);
    print_matrix(m, n, C_cpu);

    ret = xdnn::fc<float, float, float, int16_t>(ctx, A, B, C, m, n, k, false, false, nullptr, nullptr, nullptr);
    assert(ret == 0);
    xpu_memcpy((void*) & (C_cpu[0]), (void*)C, m * n * sizeof(float), XPUMemcpyKind::XPU_DEVICE_TO_HOST);
    printf("A(%p):\n", A);
    print_matrix(m, k, A_cpu);
    printf("B(%p):\n", B);
    print_matrix(k, n, B_cpu);
    printf("C(%p) = A * B:\n", C);
    print_matrix(m, n, C_cpu);
    ret = xpu_free(A);
    assert(ret == 0);
    ret = xpu_free(B);
    assert(ret == 0);
    ret = xpu_free(C);
    assert(ret == 0);
    destroy_context(ctx);
    return 0;
}
