#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_REDUCE_TESTCASE_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_REDUCE_TESTCASE_H

#define GTEST_REDUCE_SUM_BERT(dev, T) \
    gtest_reduce_sum<T>(api::dev, GM, GM, {32, 128, 3072}, {0, 1}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {32, 128, 768}, {0, 1}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {32, 2}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {32, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {456, 1}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {456, 30528}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {456, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {488, 1}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {488, 30528}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {488, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {496, 1}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {496, 30528}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {496, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {504, 1}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {504, 30528}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {504, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {512, 1}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {512, 30528}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {512, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {528, 1}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {528, 30528}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {528, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {536, 1}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {536, 30528}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {536, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {544, 1}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {544, 30528}, {0});   \
    gtest_reduce_sum<T>(api::dev, GM, GM, {544, 768}, {0});   \
    gtest_reduce_sum<T>(api::dev, L3, GM, {8, 768}, {0});   \

#define GTEST_REDUCE_SUM_TRANSFORMER(dev, T) \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 30, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 30, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 30, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 34, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 34, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 37, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 37, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 38, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 38, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 38, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 39, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {104, 39, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {128, 32, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {128, 32, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {128, 32, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {40, 26, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {40, 26, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {40, 26, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {40, 87, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {40, 87, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 23, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 23, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 53, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 53, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 63, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 63, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 63, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 64, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 64, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {64, 64, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {72, 28, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {72, 28, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {72, 54, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {72, 54, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {72, 54, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 40, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 40, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 42, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 42, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 43, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 43, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 43, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 44, 1}, {0, 1, 2}); \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 44, 2048}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, GM, GM, {88, 44, 512}, {0, 1});  \
    gtest_reduce_sum<T>(api::dev, L3, GM, {8, 512}, {0});    \

#define GTEST_REDUCE_SUM_MEG32(dev, T) \
    gtest_reduce_sum<T>(api::dev, GM, L3, {1, 510, 1}, {1}); \
    gtest_reduce_sum<T>(api::dev, L3, GM, {1, 510, 312}, {1});  \

#define GTEST_REDUCE_MEAN_SIMULATOR(dev, T) \
    gtest_reduce_mean<T>(api::dev, GM, GM, {1}, {0}); \
    gtest_reduce_mean<T>(api::dev, GM, GM, {5, 1, 5}, {1}); \
    gtest_reduce_mean<T>(api::dev, GM, GM, {1, 1, 1}, {0, 2});   \
    gtest_reduce_mean<T>(api::dev, GM, GM, {50, 50}, {1});   \
    gtest_reduce_mean<T>(api::dev, GM, GM, {2, 3, 4, 3}, {0, 2});  \
    gtest_reduce_mean<T>(api::dev, GM, GM, {2, 3, 4, 3}, {2, 3});   \
    gtest_reduce_mean<T>(api::dev, GM, GM, {2, 5, 3, 3, 4, 3}, {0, 1, 4});   \
    gtest_reduce_mean<T>(api::dev, GM, GM, {2, 2, 3, 5, 4, 3}, {2, 3, 5});  \
    gtest_reduce_mean<T>(api::dev, GM, GM, {2, 1, 3, 1, 4, 3}, {0, 1, 4});   \
    gtest_reduce_mean<T>(api::dev, GM, GM, {2, 1, 3, 1, 4, 3}, {2, 3, 5});   \
    gtest_reduce_mean<T>(api::dev, GM, GM, {1, 4096, 6},{1});  \
    gtest_reduce_mean<T>(api::dev, GM, GM, {1, 1024, 2},{1});   \
    gtest_reduce_mean<T>(api::dev, GM, GM, {2, 20, 5},{1});   \
    gtest_reduce_mean<T>(api::dev, GM, GM, {60, 3, 200},{1});  \
    gtest_reduce_mean<T>(api::dev, GM, GM, {5, 200, 1},{1});

#define GTEST_REDUCE_MEAN_BERT(dev, T) \
    gtest_reduce_mean<T>(api::dev, GM, GM, {32, 1}, {0, 1});

#endif