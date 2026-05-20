#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_GTEST_UTIL_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_GTEST_UTIL_H
#include <functional>
#include "xpu/refactor/util/vector_util.h"

#define TENSOR_ALLCLOSE(tensor1, tensor2, rtol, atol)   \
ASSERT_EQ(tensor1.dtype(), tensor2.dtype());    \
ASSERT_EQ(tensor1.numel(), tensor2.numel());    \
if (get_gtest_print_tensor_diff()) {                                                            \
    ASSERT_EQ(0, baidu::xpu::api::print_tensor_diff(tensor1, tensor2));     \
}                                                                           \
if (get_gtest_print_tensor()) {                                                          \
    print_tensor(tensor1);                                                  \
    print_tensor(tensor2);                                                  \
}                                                                           \
ASSERT_EQ(tensor1.numel(), baidu::xpu::api::count_allclose(tensor1, tensor2, rtol, atol));

#define TENSOR_MAXCLOSE(tensor1, tensor2, rtol, atol)   \
ASSERT_EQ(baidu::xpu::api::abs_max_close(tensor1, tensor2, rtol, atol), true);

#define QUANT_TENSOR_MAXCLOSE(qtensor1, qtensor2, rtol, atol)   \
ASSERT_EQ(1, baidu::xpu::api::max_allclose(qtensor1, qtensor2, rtol, atol));

#endif

// 将组合转换为字符串形式
inline std::string combinationToString(const std::vector<int64_t>& combination) {
    std::ostringstream oss;
    for (auto num : combination) {
        oss << num << ",";
    }
    return oss.str();
}

inline std::vector<int64_t> generateRandomCombination(const std::vector<int64_t>& dimension, int n, int64_t seed = 0) {
    std::default_random_engine gen(seed);
    std::vector<int64_t> combination;
    std::uniform_int_distribution<> dis(0, dimension.size() - 1);

    for (int i = 0; i < n; ++i) {
        // 随机选择n个数并添加到组合中
        combination.push_back(dimension[dis(gen)]);
    }

    return combination;
}

inline std::vector<std::vector<int64_t>> combinations_generator(const std::vector<int64_t>& dimension, int n, int nums_comb, int64_t seed = 0, int64_t size = 0, int sizeof_T = 0) {
    // 初始化随机数生成器
    std::set<std::string> uniqueCombinations;
    std::vector<std::vector<int64_t>> combinations;
    // 生成 n 组不同的组合
    while (combinations.size() < nums_comb) {
        auto combination = generateRandomCombination(dimension, n, seed);
        if (size > 0) {
            // 过滤掉不满足size的组合
            int64_t nums = sizeof_T;
            for (auto num : combination) {
                nums *= num;
            }
            if (nums > size) {
                continue;
            }
        }

        std::string comboStr = combinationToString(combination);

        // 检查是否已存在此组合
        if (uniqueCombinations.insert(comboStr).second) {
            combinations.push_back(combination);
        }
    }
    return combinations;
}

inline int getRandomNumberInRange(int min, int max, int64_t seed = 0) {
    // 静态用于性能优化，避免重复构造
    std::default_random_engine gen(seed);  // 用于获得随机数种子
    std::uniform_int_distribution<> dis(min, max); // 定义一个分布范围
    return dis(gen); // 返回一个在[min, max]范围内的随机数
}

inline float rand_float(int& seed, float min_val, float max_val) {
    std::default_random_engine e(seed);
    std::uniform_real_distribution<float> uf(min_val, max_val);
    std::uniform_int_distribution<int> ui(INT32_MIN, INT32_MAX);
    seed = ui(e);
    return uf(e);
}

inline int64_t rand_int(int& seed, int64_t min_val = INT32_MIN, int64_t max_val = INT32_MAX) {
    std::default_random_engine e(seed);
    std::uniform_int_distribution<int64_t> ui(min_val, max_val);
    std::uniform_int_distribution<int> useed(INT32_MIN, INT32_MAX);
    seed = useed(e);
    return ui(e);
}