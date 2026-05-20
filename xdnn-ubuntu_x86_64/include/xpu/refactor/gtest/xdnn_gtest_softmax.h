#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_SOFTMAX_AND_GRAD_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_SOFTMAX_AND_GRAD_H

#define GTEST_SOFTMAX_BERT(dev, T) \
    gtest_softmax<T>(api::dev, GM, GM, {32, 12, 128, 128}, 3);\
    gtest_softmax<T>(api::dev, GM, GM, {32, 2}, 1);           \
    gtest_softmax<T>(api::dev, GM, GM, {456, 30528}, 1);      \
    gtest_softmax<T>(api::dev, GM, GM, {488, 30528}, 1);      \
    gtest_softmax<T>(api::dev, GM, GM, {496, 30528}, 1);      \
    gtest_softmax<T>(api::dev, GM, GM, {504, 30528}, 1);      \
    gtest_softmax<T>(api::dev, GM, GM, {512, 30528}, 1);      \
    gtest_softmax<T>(api::dev, GM, GM, {528, 30528}, 1);      \
    gtest_softmax<T>(api::dev, GM, GM, {536, 30528}, 1);      \
    gtest_softmax<T>(api::dev, GM, GM, {544, 30528}, 1);

#define GTEST_SOFTMAX_GRAD_BERT(dev, T) \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {32, 12, 128, 128}, 3);

#define GTEST_SOFTMAX_TRANSFORMER(dev, T) \
    gtest_softmax<T>(api::dev, GM, GM, {104, 30, 38512}, 2);  \
    gtest_softmax<T>(api::dev, GM, GM, {104, 38, 38512}, 2);  \
    gtest_softmax<T>(api::dev, GM, GM, {128, 32, 38512}, 2);  \
    gtest_softmax<T>(api::dev, GM, GM, {40, 26, 38512}, 2);  \
    gtest_softmax<T>(api::dev, GM, GM, {64, 63, 38512}, 2);  \
    gtest_softmax<T>(api::dev, GM, GM, {64, 64, 38512}, 2);  \
    gtest_softmax<T>(api::dev, GM, GM, {72, 54, 38512}, 2);  \
    gtest_softmax<T>(api::dev, GM, GM, {88, 43, 38512}, 2);  \
    gtest_softmax<T>(api::dev, GM, GM, {88, 44, 38512}, 2);  \
    gtest_softmax<T>(api::dev, L3, GM, {104, 8, 30, 30}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {104, 8, 30, 37}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {104, 8, 34, 34}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {104, 8, 37, 37}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {104, 8, 38, 34}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {104, 8, 38, 38}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {104, 8, 38, 39}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {104, 8, 39, 39}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {128, 8, 32, 32}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {40, 8, 26, 26}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {40, 8, 26, 87}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {40, 8, 87, 87}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {64, 8, 23, 23}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {64, 8, 53, 53}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {64, 8, 63, 53}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {64, 8, 63, 63}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {64, 8, 64, 23}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {64, 8, 64, 64}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {72, 8, 28, 28}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {72, 8, 54, 28}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {72, 8, 54, 54}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {88, 8, 40, 40}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {88, 8, 42, 42}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {88, 8, 43, 40}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {88, 8, 43, 43}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {88, 8, 44, 42}, 3); \
    gtest_softmax<T>(api::dev, L3, GM, {88, 8, 44, 44}, 3);

#define GTEST_SOFTMAX_GRAD_TRANSFORMER(dev, T) \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {104, 8, 30, 30}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {104, 8, 30, 37}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {104, 8, 34, 34}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {104, 8, 37, 37}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {104, 8, 38, 34}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {104, 8, 38, 38}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {104, 8, 38, 39}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {104, 8, 39, 39}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {128, 8, 32, 32}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {40, 8, 26, 26}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {40, 8, 26, 87}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {40, 8, 87, 87}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {64, 8, 23, 23}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {64, 8, 53, 53}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {64, 8, 63, 53}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {64, 8, 63, 63}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {64, 8, 64, 23}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {64, 8, 64, 64}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {72, 8, 28, 28}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {72, 8, 54, 28}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {72, 8, 54, 54}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {88, 8, 40, 40}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {88, 8, 42, 42}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {88, 8, 43, 40}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {88, 8, 43, 43}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {88, 8, 44, 42}, 3); \
    gtest_softmax_grad<T>(api::dev, GM, GM, GM, {88, 8, 44, 44}, 3);

#define GTEST_SOFTMAX_TESTCASE_TRANSFORMER(dev, T) \
    gtest_softmax<T>(api::dev, GM, GM, {3072, 128}, 1);  \
    gtest_softmax<T>(api::dev, GM, GM, {24576, 128}, 1);

#endif

