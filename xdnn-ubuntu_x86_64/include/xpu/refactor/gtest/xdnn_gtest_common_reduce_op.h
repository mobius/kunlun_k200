#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_REDUCE_OP_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_COMMON_REDUCE_OP_H

#include "xpu/xdnn.h"
#include "xpu/refactor/gtest/xdnn_gtest.h"

template<typename T> static void gtest_common_reduce_op(std::string funcname,
        std::function<int(api::Context*, const T*, T*, const std::vector<int64_t>&, const std::vector<int64_t>&)> func,
        float diff, api::DeviceType dev, const std::string& x_pos, const std::string& y_pos,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims, bool is_all_any_op = false,
        int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
        api::Dtype dt = api::CPPTypeToDtype<T>();
    int64_t lenx = vector_prod(xshape);
    int64_t leny = lenx;
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM4(x_pos, y_pos, xshape, rdims);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();
    for (int64_t i = 0; i < rdims.size(); ++i) {
        leny /= xshape[rdims[i]];
    }
    auto f = [is_all_any_op](float minval, float maxval, int64_t len) {
        if (is_all_any_op) {
            return api::randint(static_cast<int>(minval), static_cast<int>(maxval), len);
        } else {
            return api::randfloat(minval, maxval, len);
        }
    };

    float min_val = -10.f;
    float max_val = 10.f;
    if (funcname == "reduce_prod") {
        if(std::is_same<T, int>::value || std::is_same<T, int64_t>::value ){
            min_val = 1;
            max_val = 3;
        }else{
            min_val = 0.2f;
            max_val = 1.5f;
        }
    } else if (is_all_any_op) {
        min_val = 0.f;
        max_val = 1.f;
    }
    GTEST_INIT_TENSOR(api::kINPUT, x, T, lenx, f, min_val, max_val);
    GTEST_INIT_TENSOR(api::kOUTPUT, y, T, leny, f, min_val, max_val);
    GTEST_DEFINE_PTR(T, x_pos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR(T, y_pos, y0, y1, y0ptr, y1ptr);
    GTEST_XPU_START(&ctx_xpu);
    ASSERT_NE(0, func(&ctx_xpu, nullptr, y1ptr, xshape, rdims));
    ASSERT_NE(0, func(&ctx_xpu, x1ptr, nullptr, xshape, rdims));
    ASSERT_NE(0, func(&ctx_xpu, x1ptr, y1ptr, std::vector<int64_t>(0), rdims));
    ASSERT_NE(0, func(&ctx_xpu, x1ptr, y1ptr, xshape, std::vector<int64_t>(0)));
    ASSERT_EQ(0, func(&ctx_xpu, x1ptr, y1ptr, xshape, rdims));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, (lenx + leny) * sizeof(T), "%s profiling", funcname.c_str());
    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0ptr, y0ptr, xshape, rdims));
    GTEST_CPU_END(&ctx_cpu);
    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(y0, y1, diff, diff);
    GTEST_CHECK_END();
}

const std::vector<std::vector<std::vector<int64_t> > > plist_xpu1 = {
    {{1}, {0}},
    {{5, 1, 5}, {1}},
    {{1, 1, 1}, {0, 2}},
    {{50, 50}, {1}},
    {{2, 3, 4, 3}, {0, 2}}, //连续多次调用kernel，影响精度，稍后修改
    {{2, 3, 4, 3}, {2, 3}},
    {{2, 5, 3, 3, 4, 3}, {0, 1, 4}},
    {{2, 2, 3, 5, 4, 3}, {2, 3, 5}},
    {{2, 1, 3, 1, 4, 3}, {0, 1, 4}},
    {{2, 1, 3, 1, 4, 3}, {2, 3, 5}},
    {{1, 4096, 6}, {1}},   // 调用big_t + mtn, 且在big_t里没有更新全部元素
    {{1, 1024, 2}, {1}},   // 调用big_t
    // {{1, 2050, 3000},{1}}, // 调用big_t + mtn, 且在big_t里没有更新全部元素, t<n；单测耗时太长暂时注释掉
    {{1, 10, 20}, {1}},    // 调用big_t, t<n
    {{2, 20, 5}, {1}},     // 调用mtn, 连续下载多个n*t
    {{60, 3, 200}, {1}},   // 调用mtn
    {{5, 200, 1}, {1}},    // 调用mt
};

const std::vector<std::vector<std::vector<int64_t> > > plist_xpu2 = plist_xpu1;

const std::vector<std::vector<std::vector<int64_t> > > plist_int_xpu2 = {
    {{2, 4, 8}, {1}},        // 调用mtn
    {{8, 10, 5}, {1}},        // 调用mtn
    {{2, 8}, {1}},          // 调用mt
    {{4, 10}, {1}},          // 调用mt
    {{1, 10, 20}, {1}},    // 调用big_t, t<n
    {{2, 3, 4, 3}, {0, 2}}, //连续多次调用kernel，影响精度，稍后修改
    {{2, 3, 4, 3}, {2, 3}},
    {{2, 5, 3, 3, 4, 3}, {0, 1, 4}},
    {{2, 2, 3, 5, 4, 3}, {2, 3, 5}},
    {{2, 1, 3, 1, 4, 3}, {0, 1, 4}},
    {{2, 1, 3, 1, 4, 3}, {2, 3, 5}},
};

const std::vector<std::vector<std::vector<int64_t> > > plist_xpu3_acc = {
    // 小规模测试
    {{1}, {0}},
    {{2}, {0}},
    {{1, 2}, {1}},
    {{2, 4, 8}, {1}},
    {{8, 10, 5}, {1}},
    {{2, 8}, {1}},
    {{4, 10}, {1}},
    {{1, 10, 20}, {1}},
    {{2, 8, 2, 1}, {0, 1, 3}},
    {{2, 3, 4, 3}, {0, 2}},
    {{2, 3, 4, 3}, {2, 3}},
    {{2, 5, 3, 3, 4, 3}, {0, 1, 4}},
    {{2, 2, 3, 5, 4, 3}, {2, 3, 5}},
    {{2, 1, 3, 1, 4, 3}, {0, 1, 4}},
    {{2, 1, 3, 1, 4, 3}, {2, 3, 5}},
};

const std::vector<std::vector<std::vector<int64_t> > > plist_xpu3 = {
    // reduce系列算子根据输入规模不同，调用不同的kernel。为了提高单测的覆盖率，需要保证测试规模覆盖每个kernel。
    // 输入张量的shape用[M, T, N]表示。

    // 调用reduce_mt，对应输入规模为[M, T, 1]
    {{12, 1024, 1}, {1}},                           //--|-----
    {{12, 4096, 1}, {1}},                           //  | 边界对齐
                                                    //--|-----
    {{12 + 1, 1024 - 1, 1}, {1}},                   //  | 边界不对齐
    {{12 + 1, 4096 - 1, 1}, {1}},                   //--|-----

    // 调用reduce_mt，对应输入规模为[M, T, 1]
    {{1024, 128, 1}, {1}},
    {{4096, 128, 1}, {1}},
    {{1024 - 1, 128 + 1, 1}, {1}},
    {{4096 + 1, 128 - 1, 1}, {1}},

    // 调用reduce_mtn，对应输入规模为[M, T, N]
    {{2, 1024, 1024}, {1}},                         //--|-----
    {{2, 4096, 4096}, {1}},                         //  | 边界对齐
                                                    //--|-----
    {{2, 1024 + 1, 1024 + 1}, {1}},                 //  | 边界不对齐
    {{2, 4096 + 1, 4096 - 1}, {1}},                 //--|-----

    // 调用reduce_tn_big_t与reduce_mtn，对应输入规模为[1, T, N]，且 T > 2048
    {{1, 4096, 1024},{1}},                          //--|-----
    {{1, 4096, 4096},{1}},                          //  | 边界对齐
                                                    //--|-----        
    {{1, 4096 + 1, 1024 + 1},{1}},                  //  | 边界不对齐
    {{1, 4096 + 1, 4096 + 1},{1}},                  //--|-----

    // 调用reduce_tn_big_t，对应输入规模为[1, T, N]，且 T <= 2048
    {{1, 1024 * 2, 1024}, {1}},                     //--|-----
    {{1, 1024 * 2, 4096}, {1}},                     //  | 边界对齐
                                                    //--|-----
    {{1, 1024 * 2 - 1, 1024 - 1}, {1}},             //  | 边界不对齐
    {{1, 1024 * 2 - 1, 4096 - 1}, {1}},             //--|-----

    // 调用reduce_mt_big_t，对应输入规模为[M, T, 1]，且 t > 5000000
    {{12, 8192 * 12 * 64, 1}, {1}},                 // 边界对齐        
    {{12 + 1, 8192 * 12 * 64 + 1, 1}, {1}},         // 边界不对齐

    // 调用reduce_tn_256_n
    {{8, 256, 768}, {0, 1}},
    {{32, 512, 768}, {0, 1}},
    {{8, 256, 512}, {0, 1}},
    {{32, 512, 512}, {0, 1}},
    {{8, 256, 256}, {0, 1}},
    {{32, 512, 256}, {0, 1}},
};

const std::vector<std::vector<std::vector<int64_t> > > plist_xpu3_daily = {
    // reduce系列算子根据输入规模不同，调用不同的kernel。为了提高单测的覆盖率，需要保证测试规模覆盖每个kernel。
    // 输入张量的shape用[M, T, N]表示。

    // 调用reduce_mt，对应输入规模为[M, T, 1]
    {{2, 1024 * 64, 1}, {1}},                   //--|-----
    {{2, 2048 * 64, 1}, {1}},                   //  |
    {{2, 1024 * 64 * 12, 1}, {1}},              //  | 边界对齐
    {{2, 2048 * 64 * 12, 1}, {1}},              //  |
                                                //--|-----
    {{2, 1024 * 64 - 1, 1}, {1}},               //  |
    {{2, 2048 * 64 + 1, 1}, {1}},               //  | 边界不对齐
    {{2, 1024 * 64 * 12 - 1, 1}, {1}},          //  |
    {{2, 2048 * 64 * 12 + 1, 1}, {1}},          //--|-----

    // 调用reduce_mt_big_mt，对应输入规模为[M, T, 1]
    {{1024 * 12, 1024 * 12, 1}, {1}},
    {{1024 * 16, 1024 * 16, 1}, {1}},
    {{1024 * 12 - 1, 1024 * 12 - 1, 1}, {1}},
    {{1024 * 16 - 1, 1024 * 16 - 1, 1}, {1}},

    // 调用reduce_mt_small_t，对应输入规模为[M, T, 1]
    {{1024 * 12, 768, 1}, {1}},
    {{1024 * 16, 512, 1}, {1}},
    {{1024 * 32, 256, 1}, {1}},
    {{1024 * 64, 128, 1}, {1}},

    {{1024 * 12 - 1, 768 - 1, 1}, {1}},
    {{1024 * 16 - 1, 512 - 1, 1}, {1}},
    {{1024 * 32 - 1, 256 - 1, 1}, {1}},
    {{1024 * 64 - 1, 128 - 1, 1}, {1}},

    // 调用reduce_mtn，对应输入规模为[M, T, N]
    {{2, 1024, 1024 * 64}, {1}},                  //--|-----
    {{2, 2048, 2048 * 64}, {1}},                  //  | 边界对齐
                                                  //--|-----
    {{2, 1024 + 1, 1024 * 64 + 1}, {1}},          //  | 边界不对齐
    {{2, 2048 + 1, 2048 * 64 - 1}, {1}},          //--|-----

    // 调用reduce_tn_big_t与reduce_mtn，对应输入规模为[1, T, N]，且 T > 2048
    {{1, 4096, 1024 * 64}, {1}},              //--|-----
    {{1, 4096, 2048 * 64}, {1}},              //  | 边界对齐
                                              //--|-----
    {{1, 4096 - 1, 1024 * 64 - 1}, {1}},      //  | 边界不对齐
    {{1, 4096 - 1, 2048 * 64 - 1}, {1}},      //--|-----

    // 调用reduce_tn_big_t，对应输入规模为[1, T, N]，且 T <= 2048
    {{1, 1024 * 2, 1024 * 64}, {1}},              //--|-----
    {{1, 1024 * 2, 2048 * 64}, {1}},              //  | 边界对齐
                                                  //--|-----
    {{1, 1024 * 2 - 1, 1024 * 64 - 1}, {1}},      //  |边界不对齐
    {{1, 1024 * 2 - 1, 2048 * 64 - 1}, {1}},      //--|-----

    // 调用reduce_mt_big_t，对应输入规模为[M, T, 1]，且 t > 5000000
    {{12, 8192 * 12 * 64, 1}, {1}},             //--|-----           
    {{16, 8192 * 12 * 64, 1}, {1}},             //  | 边界对齐
                                                //--|-----
    {{12 - 1, 8192 * 12 * 64 + 1, 1}, {1}},     //  | 边界不对齐
    {{16 - 1, 8192 * 12 * 64 + 1, 1}, {1}},     //--|-----
};

const std::vector<std::vector<std::vector<int64_t> > > plist_xpu3_weekly = {

    // reduce_mt
    {{64, 1024 * 64 * 12, 1}, {1}},
    {{64, 2048 * 64 * 12, 1}, {1}},
    {{128, 1024 * 64 * 12 - 1, 1}, {1}},
    {{128, 2048 * 64 * 12 + 1, 1}, {1}},

    // reduce_mt_big_mt
    {{1024 * 12, 1024 * 12, 1}, {1}},
    {{1024 * 16, 1024 * 16, 1}, {1}},
    {{1024 * 12 - 1, 1024 * 12 - 1, 1}, {1}},
    {{1024 * 16 - 1, 1024 * 16 - 1, 1}, {1}},

    // reduce_mt_small_t
    {{1024 * 64 * 12, 768, 1}, {1}},
    {{2048 * 64 * 12, 512, 1}, {1}},
    {{4096 * 64 * 12, 256, 1}, {1}},
    {{8192 * 64 * 12, 128, 1}, {1}},

    {{1024 * 64 * 12 - 1, 768 - 1, 1}, {1}},
    {{2048 * 64 * 12 - 1, 512 - 1, 1}, {1}},
    {{4096 * 64 * 12 - 1, 256 - 1, 1}, {1}},
    {{8192 * 64 * 12 - 1, 128 - 1, 1}, {1}},

    // reduce_mtn
    {{2, 1024, 1024 * 64 * 12}, {1}},
    {{2, 1024, 2048 * 64 * 12}, {1}},
    {{2, 1024 - 1, 1024 * 64 * 12 + 1}, {1}}, 
    {{2, 1024 - 1, 2048 * 64 * 12 - 1}, {1}},

    // reduce_tn_big_t
    {{1, 4096, 1024 * 64 * 12}, {1}},
    {{1, 4096, 2048 * 64 * 12}, {1}},
    {{1, 4096 - 1, 1024 * 64 * 12 - 1}, {1}},
    {{1, 4096 - 1, 2048 * 64 * 12 - 1}, {1}},

    // reduce_tn
    {{1, 1024 * 2, 1024 * 64 * 12}, {1}},
    {{1, 1024 * 2, 2048 * 64 * 12}, {1}},
    {{1, 1024 * 2 - 1, 1024 * 64 * 12 - 1}, {1}},
    {{1, 1024 * 2 - 1, 2048 * 64 * 12 - 1}, {1}},

    // reduce_mt_big_t
    {{64, 8192 * 12 * 64, 1}, {1}},   
    {{128, 8192 * 12 * 64, 1}, {1}},

    {{64 - 1, 8192 * 12 * 64 + 1, 1}, {1}},
    {{128 - 1, 8192 * 12 * 64 + 1, 1}, {1}},

};
#endif