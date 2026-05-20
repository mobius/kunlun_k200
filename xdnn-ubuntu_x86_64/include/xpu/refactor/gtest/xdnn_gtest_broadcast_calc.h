#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_BROADCAST_CALC_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_BROADCAST_CALC_H

#include "xpu/xdnn.h"
#include "xpu/refactor/gtest/xdnn_gtest.h"

#define GTEST_BROADCAST_HELPER(type, ctx)                                         \
    gtest_broadcast<type>(ctx, GM, GM, {1}, {7890});                              \
    gtest_broadcast<type>(ctx, GM, GM, {3, 1, 3}, {3, 4, 3});                     \
    gtest_broadcast<type>(ctx, GM, GM, {7, 1, 9, 1, 11}, {7, 8, 9, 10, 11});      \
    gtest_broadcast<type>(ctx, GM, GM, {7, 1, 1, 1, 11}, {7, 8, 9, 10, 11});      \
    gtest_broadcast<type>(ctx, GM, GM, {7, 1, 1, 1, 11}, {7, 8, 1, 10, 11});      \
    gtest_broadcast<type>(ctx, GM, GM, {7, 1, 9, 1, 1}, {7, 8, 9, 10, 11});       \
    gtest_broadcast<type>(ctx, GM, GM, {2, 3, 4}, {2, 3, 4});                     \
    gtest_broadcast<type>(ctx, GM, GM, {81, 33, 1}, {81, 33, 33});                \
    gtest_broadcast<type>(ctx, GM, GM, {7, 6}, {21, 6});                          \
    gtest_broadcast<type>(ctx, GM, GM, {2, 3, 1}, {2, 600, 2});                   \
    gtest_broadcast<type>(ctx, GM, GM, {5, 32, 20}, {25, 128, 60});               \
    gtest_broadcast<type>(ctx, GM, GM, {1, 1}, {2239, 5120});                     \
    gtest_broadcast<type>(ctx, GM, GM, {1, 1}, {1181, 8192});                     \
    gtest_broadcast<type>(ctx, GM, GM, {1, 1}, {4096, 8192});


#define GTEST_BROADCAST_CALC_HELPER(op, type, ctx)                         \
    gtest_##op<type>(ctx, GM, GM, GM, {6}, {1});                           \
    gtest_##op<type>(ctx, GM, GM, GM, {1}, {6});                           \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 5}, {5});                        \
    gtest_##op<type>(ctx, GM, GM, GM, {5}, {6, 5});                        \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 1}, {6, 5});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 5}, {6, 1});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {16, 1578}, {1578});                 \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 5, 4, 3}, {5, 1, 1});            \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1, 1}, {6, 5, 4, 3});            \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 1}, {1, 6});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 64, 3, 1}, {2, 64, 1, 1});       \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 64, 1, 1}, {2, 64, 3, 1});       \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 1, 16, 17}, {1, 1, 1});          \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 16, 17}, {5, 1, 17});            \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 1}, {1, 1});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {1024}, {1024});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {4096}, {4096});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {6}, {1});                           \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 1024}, {2, 1024});               \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 4096}, {2, 4096});               \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 2048, 40, 128}, {1, 2048, 1, 128});


#define GTEST_KL3_BROADCAST_CALC_HELPER(op, type, ctx)                    \
    gtest_##op<type>(ctx, GM, GM, GM, {1024}, {1024});                      \
    gtest_##op<type>(ctx, GM, GM, GM, {4096}, {4096});                      \
    gtest_##op<type>(ctx, GM, GM, GM, {177}, {177});                      \
    gtest_##op<type>(ctx, GM, GM, GM, {13145}, {13145});                  \
    gtest_##op<type>(ctx, GM, GM, GM, {177}, {1});                        \
    gtest_##op<type>(ctx, GM, GM, GM, {1}, {177});                        \
    gtest_##op<type>(ctx, GM, GM, GM, {1024}, {1});                        \
    gtest_##op<type>(ctx, GM, GM, GM, {1}, {4096});                        \
    gtest_##op<type>(ctx, GM, GM, GM, {17712}, {1});                      \
    gtest_##op<type>(ctx, GM, GM, GM, {1}, {14511});                      \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 177}, {5, 1});                  \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1}, {5, 177});                  \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1}, {5, 1024});                  \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 4096}, {5, 1});                  \
    gtest_##op<type>(ctx, GM, GM, GM, {1024, 1}, {1024, 177});            \
    gtest_##op<type>(ctx, GM, GM, GM, {4096, 177}, {4096, 1});            \
    gtest_##op<type>(ctx, GM, GM, GM, {1115, 1}, {1115, 177});            \
    gtest_##op<type>(ctx, GM, GM, GM, {1115, 177}, {1115, 1});            \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1177}, {5, 1});                 \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1}, {5, 1177});                 \
    gtest_##op<type>(ctx, GM, GM, GM, {3, 3, 177}, {3, 3, 1});            \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 23, 1}, {5, 23, 177});          \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 5, 177}, {115, 5, 1});        \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 5, 1}, {115, 5, 177});        \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 7, 1}, {5, 7, 1177});           \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 7, 1177}, {5, 7, 1});           \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 7, 1}, {115, 7, 1177});       \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 7, 1177}, {115, 7, 1});       \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1, 177}, {5, 3, 177});          \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 3, 177}, {5, 1, 177});          \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 1, 177}, {115, 3, 177});      \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 1, 1177}, {2, 3, 1177});        \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 7, 1177}, {2, 1, 1177});


#define GTEST_BROADCAST_CALC_2D_HELPER(op, type, ctx, cluster_num, core_num)                             \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 1024*core_num}, {1, 1024*core_num});                           \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 4096*core_num}, {1, 4096*core_num});                           \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 1024*core_num*cluster_num}, {1, 1024*core_num*cluster_num});   \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 4096*core_num*cluster_num}, {1, 4096*core_num*cluster_num});


#define GTEST_BROADCAST_CALC_HELPER_DAILY(op, type, ctx)                         \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 64*1024}, {2, 64*1024});               \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 64*4096}, {2, 64*4096});               \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 12*64*1024}, {2, 12*64*1024});         \
    gtest_##op<type>(ctx, GM, GM, GM, {2, 12*64*4096}, {2, 12*64*4096});         


#define GTEST_KL3_BROADCAST_CALC_HELPER_DAILY(op, type, ctx)                  \
    gtest_##op<type>(ctx, GM, GM, GM, {1024 * 12}, {1024 * 12});                    \
    gtest_##op<type>(ctx, GM, GM, GM, {4096 * 12}, {4096 * 12});                    \
    gtest_##op<type>(ctx, GM, GM, GM, {4096 * 12 * 64}, {4096 * 12 * 64});                    \
    gtest_##op<type>(ctx, GM, GM, GM, {1024 * 12 * 64}, {1024 * 12 * 64});                    \
    gtest_##op<type>(ctx, GM, GM, GM, {312177}, {312177});                    \
    gtest_##op<type>(ctx, GM, GM, GM, {1024 * 12}, {1});                         \
    gtest_##op<type>(ctx, GM, GM, GM, {1}, {4096 * 12});                         \
    gtest_##op<type>(ctx, GM, GM, GM, {4096 * 12 * 64}, {1});                         \
    gtest_##op<type>(ctx, GM, GM, GM, {1}, {1024 * 12 * 64});                         \
    gtest_##op<type>(ctx, GM, GM, GM, {177321}, {1});                         \
    gtest_##op<type>(ctx, GM, GM, GM, {1}, {177321});                         \
    gtest_##op<type>(ctx, GM, GM, GM, {1024 * 12, 177}, {1024 * 12, 1});                \
    gtest_##op<type>(ctx, GM, GM, GM, {4096 * 12, 1}, {4096 * 12, 177});                \
    gtest_##op<type>(ctx, GM, GM, GM, {1521, 512}, {1521, 1});                \
    gtest_##op<type>(ctx, GM, GM, GM, {1521, 1}, {1521, 1024});               \
    gtest_##op<type>(ctx, GM, GM, GM, {1521, 1177}, {1521, 1});               \
    gtest_##op<type>(ctx, GM, GM, GM, {1521, 1}, {1521, 1237});               \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1025}, {5, 1});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1}, {5, 1101});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {35, 223, 177}, {35, 223, 1});        \
    gtest_##op<type>(ctx, GM, GM, GM, {35, 223, 512}, {35, 223, 1});        \
    gtest_##op<type>(ctx, GM, GM, GM, {35, 223, 1}, {35, 223, 257});        \
    gtest_##op<type>(ctx, GM, GM, GM, {35, 123, 1}, {35, 123, 1024});       \
    gtest_##op<type>(ctx, GM, GM, GM, {1115, 15, 155}, {1115, 1, 155});       \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 1, 133}, {115, 177, 133});        \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 1, 512}, {115, 177, 512});        \
    gtest_##op<type>(ctx, GM, GM, GM, {315, 47, 1177}, {315, 1, 1177});       \
    gtest_##op<type>(ctx, GM, GM, GM, {315, 1, 1023}, {315, 47, 1023});       \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 1117, 127}, {115, 1, 127});       \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 1, 129}, {115, 1117, 129});       \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1, 1023}, {5, 1123, 1023});         \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 223, 1025}, {5, 1, 1025});          \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1, 256}, {5, 223, 256});            \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 1, 177}, {115, 223, 177});        \
    gtest_##op<type>(ctx, GM, GM, GM, {115, 1, 512}, {115, 223, 512});        \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 1, 2177}, {5, 1147, 2177});         \
    gtest_##op<type>(ctx, GM, GM, GM, {135, 1, 2177}, {135, 147, 2177});      \
    gtest_##op<type>(ctx, GM, GM, GM, {135, 1, 2048}, {135, 147, 2048});      \
    gtest_##op<type>(ctx, GM, GM, GM, {135, 147, 2177}, {135, 1, 2177});      \
    gtest_##op<type>(ctx, GM, GM, GM, {5, 317, 2177}, {5, 1, 2177});


#define GTEST_BROADCAST_CALC_GRAD_HELPER(op, type, ctx)                          \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {10, 3, 12}, {10, 1, 12});     \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {6}, {1});                     \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1}, {6});                     \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {6, 5}, {5});                  \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {5}, {6, 5});                  \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {6, 1}, {6, 5});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {6, 5}, {6, 1});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {6, 5, 4, 3}, {5, 4, 3});


#define GTEST_KL3_BROADCAST_CALC_GRAD_HELPER(op, type, ctx)                                      \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1024}, {1024});                      \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {4096}, {4096});                      \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {177}, {177});                      \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {1024}, {1});                        \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {1}, {4096});                        \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {17712}, {1});                      \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1}, {14511});                      \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {5, 177}, {5, 1});                  \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {5, 1}, {5, 177});                  \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {5, 1}, {5, 1024});                  \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {5, 4096}, {5, 1});                  \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1024, 1}, {1024, 177});            \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {4096, 177}, {4096, 1});            \
	gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {165}, {165});                                 \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {127}, {127});                             \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {13145}, {13145});                             \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {13145}, {13145});                         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {177}, {1});                                   \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {177}, {1});                               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1}, {177});                                   \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {1}, {177});                               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {17712}, {1});                                 \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {1}, {14511});                             \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {5, 177}, {5, 1});                             \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {5, 1}, {5, 177});                         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {115, 1}, {115, 177});                         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {115, 177}, {115, 1});                     \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {5, 1177}, {5, 1});                            \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {5, 1}, {5, 1177});                        \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {5, 1}, {5, 1177});                        \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {5, 3, 177}, {5, 3, 1});                   \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {5, 3, 1}, {5, 3, 177});                   \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {115, 3, 1}, {115, 3, 177});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {5, 7, 1}, {5, 7, 1177});                  \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {5, 7, 1177}, {5, 7, 1});                      \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {15, 7, 1}, {15, 7, 1177});                \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {15, 7, 1177}, {15, 7, 1});                \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {5, 1, 177}, {5, 3, 177});                     \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {2, 3, 177}, {2, 1, 177});                 \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {15, 1, 177}, {15, 3, 177});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {2, 1, 1177}, {2, 7, 1177});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {2, 7, 1177}, {2, 1, 1177});


#define GTEST_BROADCAST_CALC_GRAD_HELPER_DAILY(op, type, ctx)                                \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {2, 64*1024}, {2, 64*1024});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {2, 64*4096}, {2, 64*4096});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {2, 12*64*1024}, {2, 12*64*1024});         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {2, 12*64*4096}, {2, 12*64*4096});         


#define GTEST_KL3_BROADCAST_CALC_GRAD_HELPER_DAILY(op, type, ctx)                                \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1024 * 12}, {1024 * 12});                    \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {4096 * 12}, {4096 * 12});                    \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {4096 * 12 * 64}, {4096 * 12 * 64});                    \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {1024 * 12 * 64}, {1024 * 12 * 64});                    \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1024 * 12}, {1});                         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {1}, {4096 * 12});                         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {4096 * 12 * 64}, {1});                         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {1}, {1024 * 12 * 64});                         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {1024 * 12, 177}, {1024 * 12, 1});                \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {4096 * 12, 1}, {4096 * 12, 177});                \
	gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {3212377}, {3212377});                         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {3212377}, {3212377});                     \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {3212377}, {3212377});                     \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {121277}, {1});                                \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {121277}, {1});                            \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {121277}, {1});                            \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1}, {121277});                                \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {1}, {121277});                            \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {1}, {121277});                            \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1579, 177}, {1579, 1});                       \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {1579, 133}, {1579, 1});                   \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {1579, 179}, {1579, 1});                   \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {1579, 1}, {1579, 186});                       \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {1579, 1}, {1579, 277});                   \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {1579, 1}, {1579, 377});                   \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {785, 1177}, {785, 1});                        \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {785, 1077}, {785, 1});                    \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {785, 1274}, {785, 1});                    \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {785, 1}, {785, 1163});                        \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {785, 1}, {785, 1063});                    \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {785, 1}, {785, 1164});                    \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {115, 1, 132}, {115, 123, 132});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {115, 1, 154}, {115, 123, 154});           \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {115, 1, 217}, {115, 123, 217});           \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {115, 223, 192}, {115, 1, 192});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {115, 223, 169}, {115, 1, 169});           \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {115, 223, 317}, {115, 1, 317});           \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {115, 1, 1018}, {115, 123, 1018});             \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {115, 1, 1111}, {115, 123, 1111});         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {115, 1, 1025}, {115, 123, 1025});         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {115, 223, 1133}, {115, 1, 1133});             \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {115, 223, 1117}, {115, 1, 1117});         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {115, 223, 1017}, {115, 1, 1017});         \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {15, 777, 1025}, {15, 1, 1025});               \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {15, 523, 1023}, {15, 1, 1023});           \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {15, 523, 1023}, {15, 1, 1023});           \
	gtest_##op<type>(ctx, GM, GM, GM, GM, GM, GM, {15, 1, 1214}, {15, 1217, 1214});              \
    gtest_##op<type>(ctx, GM, GM, GM, GM, GM, "NULL", {15, 1, 1319}, {15, 579, 1319});           \
    gtest_##op<type>(ctx, GM, GM, GM, GM, "NULL", GM, {15, 1, 1319}, {15, 421, 1319});


#define GTEST_BROADCAST_COMPARE_HELPER(op, type, ctx)                      \
    gtest_##op<type>(ctx, GM, GM, GM, {6}, {1});                           \
    gtest_##op<type>(ctx, GM, GM, GM, {1}, {6});                           \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 5}, {5});                        \
    gtest_##op<type>(ctx, GM, GM, GM, {5}, {6, 5});                        \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 1}, {6, 5});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 5}, {6, 1});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {16, 1578}, {1578});                 \
    gtest_##op<type>(ctx, GM, GM, GM, {6, 5, 4, 3}, {5, 1, 1});            \
    gtest_##op<type>(ctx, GM, GM, GM, {1024}, {1024});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {4096}, {4096});                     \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 1, 4096, 4096}, {1});

#define GTEST_BROADCAST_COMPARE_2D_HELPER(op, type, ctx, cluster_num, core_num)                             \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 1024*core_num}, {1, 1024*core_num});                              \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 4096*core_num}, {1, 4096*core_num});                              \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 1024*core_num*cluster_num}, {1, 1024*core_num*cluster_num});      \
    gtest_##op<type>(ctx, GM, GM, GM, {1, 4096*core_num*cluster_num}, {1, 4096*core_num*cluster_num});   

static std::vector<int64_t> get_zshape(const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape) {
    std::vector<int64_t> ret;
    int64_t shape_len_max = std::max<int64_t>(xshape.size(), yshape.size());
    for (int64_t i = 0; i < shape_len_max; i++) {
        int64_t xi = i + xshape.size() - shape_len_max;
        int64_t yi = i + yshape.size() - shape_len_max;
        xi = ((xi < 0) ? 1 : xshape[xi]);
        yi = ((yi < 0) ? 1 : yshape[yi]);
        ret.push_back(std::max<int64_t>(xi, yi));
    }
    return ret;
}

static int get_sign(float v) {
    if (v >= 0) {
        return 1;
    } else {
        return -1;
    }
}

template<typename T> static void gtest_broadcast(api::DeviceType dev, std::string xpos, std::string ypos,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
        api::Dtype dt = api::CPPTypeToDtype<T>();
    int64_t xlen = vector_prod(xshape);
    int64_t ylen = vector_prod(yshape);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1("broadcast", T);
    GTEST_GEN_HASH_PARAM4(xpos, ypos, xshape, yshape);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();
    GTEST_INIT_TENSOR(api::kINPUT, x, T, xlen, api::randfloat, -10.f, 10.f);
    GTEST_INIT_TENSOR(api::kOUTPUT, y, T, ylen, api::randfloat, -10.f, 10.f);
    GTEST_DEFINE_PTR(T, xpos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR(T, ypos, y0, y1, y0ptr, y1ptr);

    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, api::broadcast<T>(&ctx_xpu, x1ptr, y1ptr, xshape, yshape));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, (xlen + ylen) * sizeof(T), "broadcast, rlen(%ld) wlen(%ld)", xlen, ylen);

    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, api::broadcast<T>(&ctx_cpu, x0ptr, y0ptr, xshape, yshape));
    GTEST_CPU_END(&ctx_cpu);

    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(y0, y1, 0, 0);
    GTEST_CHECK_END();
}

template<typename T> static void gtest_broadcast_calc(std::string funcname,
        std::function<int(api::Context*, const T*, const T*, T*,
                const std::vector<int64_t>&, const std::vector<int64_t>&)> func, float diff,
        api::DeviceType dev, std::string xpos, std::string ypos, std::string zpos,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape,
        float x_minv, float x_maxv, float y_minv, float y_maxv, float eps, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
        api::Dtype dt = api::CPPTypeToDtype<T>();
    std::vector<int64_t> zshape = get_zshape(xshape, yshape);
    int64_t xlen = vector_prod(xshape);
    int64_t ylen = vector_prod(yshape);
    int64_t zlen = vector_prod(zshape);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM5(xpos, ypos, zpos, xshape, yshape);
    GTEST_GEN_HASH_PARAM5(x_minv, x_maxv, y_minv, y_maxv, eps);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();
    auto f = [eps](float minval, float maxval, int64_t len) {
        api::Tensor ret = api::randfloat(minval, maxval, len);
        for (size_t i = 0; i < ret.numel(); ++i) {
            ret.data<float>()[i] += get_sign(ret.data<float>()[i]) * eps;
        }
        return ret;
    };
    GTEST_INIT_TENSOR(api::kINPUT, x, T, xlen, f, x_minv, x_maxv);
    GTEST_INIT_TENSOR(api::kINPUT, y, T, ylen, f, y_minv, y_maxv);
    GTEST_INIT_TENSOR(api::kOUTPUT, z, T, zlen, api::randfloat, -10.0f, 10.f);
    GTEST_REUSE_PTR_DEFINE();
    GTEST_DEFINE_PTR_WO(T, zpos, z0, z1, z0ptr, z1ptr);
    GTEST_DEFINE_PTR_RO(T, xpos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR_RO(T, ypos, y0, y1, y0ptr, y1ptr);

    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func(&ctx_xpu, x1ptr, y1ptr, z1ptr, xshape, yshape));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, (xlen + ylen + zlen) * sizeof(T), "%s, rlen(%ld) wlen(%ld)", funcname.c_str(),
            xlen + ylen, zlen);
    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0ptr, y0ptr, z0ptr, xshape, yshape));
    GTEST_CPU_END(&ctx_cpu);

    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(z0, z1, diff, diff);
    GTEST_CHECK_END();
}

template<typename T> static void gtest_broadcast_calc_grad(std::string funcname,
        std::function<int(api::Context*, const T*, const T*, const T*, const T*, T*, T*,
                const std::vector<int64_t>&, const std::vector<int64_t>&)> func, float diff,
        api::DeviceType dev, std::string xpos, std::string ypos, std::string zpos,
        std::string dxpos, std::string dypos, std::string dzpos,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape,
        float minv, float maxv, float eps, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
        std::vector<int64_t> zshape = get_zshape(xshape, yshape);
    int64_t xlen = vector_prod(xshape);
    int64_t ylen = vector_prod(yshape);
    int64_t zlen = vector_prod(zshape);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM6(xpos, ypos, zpos, dxpos, dypos, dzpos);
    GTEST_GEN_HASH_PARAM5(xshape, yshape, minv, maxv, eps);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();
    auto f = [eps](float minval, float maxval, int64_t len) {
        api::Tensor ret = api::randfloat(minval, maxval, len);
        for (size_t i = 0; i < ret.numel(); ++i) {
            ret.data<float>()[i] += get_sign(ret.data<float>()[i]) * eps;
        }
        return ret;
    };
    GTEST_INIT_TENSOR(api::kINPUT, x, T, xlen, f, minv, maxv);
    GTEST_INIT_TENSOR(api::kINPUT, y, T, ylen, f, minv, maxv);
    GTEST_INIT_TENSOR(api::kINPUT, z, T, zlen, f, minv, maxv);
    GTEST_INIT_TENSOR(api::kOUTPUT, dx, T, xlen, api::randfloat, minv, maxv);
    GTEST_INIT_TENSOR(api::kOUTPUT, dy, T, ylen, api::randfloat, minv, maxv);
    GTEST_INIT_TENSOR(api::kINPUT, dz, T, zlen, f, minv, maxv);
    GTEST_REUSE_PTR_DEFINE();
    GTEST_DEFINE_PTR_WO(T, dxpos, dx0, dx1, dx0ptr, dx1ptr);
    GTEST_DEFINE_PTR_WO(T, dypos, dy0, dy1, dy0ptr, dy1ptr);
    GTEST_DEFINE_PTR_RO(T, xpos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR_RO(T, ypos, y0, y1, y0ptr, y1ptr);
    GTEST_DEFINE_PTR_RO(T, zpos, z0, z1, z0ptr, z1ptr);
    GTEST_DEFINE_PTR_RO(T, dzpos, dz0, dz1, dz0ptr, dz1ptr);

    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func(&ctx_xpu, x1ptr, y1ptr, z1ptr, dz1ptr, dy1ptr, dx1ptr, xshape, yshape));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, (2 * xlen + 2 * ylen + zlen) * sizeof(T),
            "%s, rlen(%ld) wlen(%ld)", funcname.c_str(), xlen + ylen + zlen, xlen + ylen);

    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0ptr, y0ptr, z0ptr, dz0ptr, dy0ptr, dx0ptr, xshape, yshape));
    GTEST_CPU_END(&ctx_cpu);

    GTEST_CHECK_START();
    if (dxpos != NIL) {
        GTEST_TENSOR_ALLCLOSE(dx0, dx1, diff, diff);
    }
    if (dypos != NIL) {
        GTEST_TENSOR_ALLCLOSE(dy0, dy1, diff, diff);
    }
    GTEST_CHECK_END();
}

template<typename T> static void gtest_broadcast_compare(std::string funcname,
        std::function<int(api::Context*, const T*, const T*, bool*,
                const std::vector<int64_t>&, const std::vector<int64_t>&)> func,
        api::DeviceType dev, std::string xpos, std::string ypos, std::string zpos,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape,
        float minv, float maxv, int l3size = 0) {
    api::Context ctx_cpu(api::kCPU);
    api::Context ctx_xpu(dev);
        api::Dtype dt = api::CPPTypeToDtype<T>();
    std::vector<int64_t> zshape = get_zshape(xshape, yshape);
    int64_t xlen = vector_prod(xshape);
    int64_t ylen = vector_prod(yshape);
    int64_t zlen = vector_prod(zshape);
    GTEST_INIT_CTX(&ctx_xpu, l3size);
    GTEST_GEN_HASH_FUNCTION_T1(funcname, T);
    GTEST_GEN_HASH_PARAM5(xpos, ypos, zpos, xshape, yshape);
    GTEST_GEN_HASH_PARAM1(l3size);
    GTEST_GEN_HASH_END();

    GTEST_INIT_TENSOR(api::kINPUT, x, T, xlen, api::randfloat, -1.0f, 1.0f);
    GTEST_INIT_TENSOR(api::kINPUT, y, T, ylen, api::randfloat, -1.0f, 1.0f);
    GTEST_INIT_TENSOR(api::kOUTPUT, z, bool, zlen, api::randfloat, -10, 10);

    GTEST_DEFINE_PTR(T, xpos, x0, x1, x0ptr, x1ptr);
    GTEST_DEFINE_PTR(T, ypos, y0, y1, y0ptr, y1ptr);
    GTEST_DEFINE_PTR(bool, zpos, z0, z1, z0ptr, z1ptr);

    GTEST_XPU_START(&ctx_xpu);
    ASSERT_EQ(0, func(&ctx_xpu, x1ptr, y1ptr, z1ptr, xshape, yshape));
    GTEST_XPU_END_DMA_FMT(&ctx_xpu, (xlen + ylen) * sizeof(T) + zlen, "%s, rlen(%ld) wlen(%ld)", funcname.c_str(),
            xlen + ylen, zlen);

    GTEST_CPU_START(&ctx_cpu);
    ASSERT_EQ(0, func(&ctx_cpu, x0ptr, y0ptr, z0ptr, xshape, yshape));
    GTEST_CPU_END(&ctx_cpu);

    GTEST_CHECK_START();
    GTEST_TENSOR_ALLCLOSE(z0, z1, 0, 0);
    GTEST_CHECK_END();
}

#endif
