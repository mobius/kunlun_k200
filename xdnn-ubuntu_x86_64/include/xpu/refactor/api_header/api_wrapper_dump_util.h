#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NEW_WRAPPER_DUMP_UTIL_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NEW_WRAPPER_DUMP_UTIL_H
namespace baidu {
namespace xpu {
namespace api {
// clang-format on
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_0(...) {/* do nothing */}
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p0) FUNC(ctx, p0)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_2(FUNC, ctx, p0, p1)                                                           \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p0)                                                                   \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p1)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_3(FUNC, ctx, p0, p1, p2)                                                       \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_2(FUNC, ctx, p0, p1)                                                               \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p2)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_4(FUNC, ctx, p0, p1, p2, p3)                                                   \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_3(FUNC, ctx, p0, p1, p2)                                                           \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p3)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_5(FUNC, ctx, p0, p1, p2, p3, p4)                                               \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_4(FUNC, ctx, p0, p1, p2, p3)                                                       \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p4)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_6(FUNC, ctx, p0, p1, p2, p3, p4, p5)                                           \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_5(FUNC, ctx, p0, p1, p2, p3, p4)                                                   \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p5)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_7(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6)                                       \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_6(FUNC, ctx, p0, p1, p2, p3, p4, p5)                                               \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p6)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_8(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7)                                   \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_7(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6)                                           \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p7)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_9(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8)                               \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_8(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7)                                       \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p8)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_10(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)                          \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_9(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8)                                   \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p9)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_11(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)                     \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_10(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)                              \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p10)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_12(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)                \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_11(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)                         \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p11)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_13(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)           \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_12(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)                    \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p12)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_14(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)      \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_13(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)               \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p13)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_15(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14) \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_14(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)          \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p14)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_16(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15)                                                                        \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_15(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14)     \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p15)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_17(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16)                                                                   \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_16(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15)                                                                            \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p16)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_18(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17)                                                              \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_17(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16)                                                                       \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p17)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_19(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18)                                                         \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_18(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17)                                                                  \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p18)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_20(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19)                                                    \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_19(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18)                                                             \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p19)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_21(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20)                                               \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_20(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19)                                                        \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p20)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_22(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21)                                          \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_21(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20)                                                   \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p21)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_23(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22)                                     \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_22(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21)                                              \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p22)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_24(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23)                                \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_23(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22)                                         \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p23)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_25(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24)                           \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_24(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23)                                    \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p24)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_26(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25)                      \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_25(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24)                               \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p25)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_27(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26)                 \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_26(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25)                          \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p26)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_28(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27)            \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_27(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26)                     \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p27)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_29(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28)       \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_28(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27)                \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p28)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_30(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29)  \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_29(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28)           \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p29)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_31(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,  \
                                           p30)                                                                        \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_30(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29)      \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p30)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_32(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,  \
                                           p30, p31)                                                                   \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_31(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30) \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p31)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_33(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,  \
                                           p30, p31, p32)                                                              \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_32(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, \
                                       p31)                                                                            \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p32)
#define _WRAPPER_NESTED_DEFINE_WITH_CTX_34(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, \
                                           p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,  \
                                           p30, p31, p32, p33)                                                         \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_33(FUNC, ctx, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,     \
                                       p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, \
                                       p31, p32)                                                                       \
    _WRAPPER_NESTED_DEFINE_WITH_CTX_1(FUNC, ctx, p33)

#define _WRAPPER_NESTED_DEFINE_0(...) {/* do nothing */}
#define _WRAPPER_NESTED_DEFINE_1(FUNC, T0) FUNC(T0)
#define _WRAPPER_NESTED_DEFINE_2(FUNC, T0, T1)                                                                         \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T1)
#define _WRAPPER_NESTED_DEFINE_3(FUNC, T0, T1, T2)                                                                     \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_2(FUNC, T1, T2)
#define _WRAPPER_NESTED_DEFINE_4(FUNC, T0, T1, T2, T3)                                                                 \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_3(FUNC, T1, T2, T3)
#define _WRAPPER_NESTED_DEFINE_5(FUNC, T0, T1, T2, T3, T4)                                                             \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_4(FUNC, T1, T2, T3, T4)
#define _WRAPPER_NESTED_DEFINE_6(FUNC, T0, T1, T2, T3, T4, T5)                                                         \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_5(FUNC, T1, T2, T3, T4, T5)
#define _WRAPPER_NESTED_DEFINE_7(FUNC, T0, T1, T2, T3, T4, T5, T6)                                                     \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_6(FUNC, T1, T2, T3, T4, T5, T6)
#define _WRAPPER_NESTED_DEFINE_7(FUNC, T0, T1, T2, T3, T4, T5, T6)                                                     \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_6(FUNC, T1, T2, T3, T4, T5, T6)
#define _WRAPPER_NESTED_DEFINE_8(FUNC, T0, T1, T2, T3, T4, T5, T6, T7)                                                 \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_7(FUNC, T1, T2, T3, T4, T5, T6, T7)
#define _WRAPPER_NESTED_DEFINE_9(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8)                                             \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_8(FUNC, T1, T2, T3, T4, T5, T6, T7, T8)
#define _WRAPPER_NESTED_DEFINE_10(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)                                        \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_9(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9)
#define _WRAPPER_NESTED_DEFINE_11(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)                                   \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_10(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)
#define _WRAPPER_NESTED_DEFINE_12(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)                              \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_11(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)
#define _WRAPPER_NESTED_DEFINE_13(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)                         \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_12(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)
#define _WRAPPER_NESTED_DEFINE_14(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)                    \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_13(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)
#define _WRAPPER_NESTED_DEFINE_15(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)               \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_14(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)
#define _WRAPPER_NESTED_DEFINE_16(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)          \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_15(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)
#define _WRAPPER_NESTED_DEFINE_17(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)     \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_16(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)
#define _WRAPPER_NESTED_DEFINE_18(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17)                                                                                 \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_17(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)
#define _WRAPPER_NESTED_DEFINE_19(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18)                                                                            \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_18(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)
#define _WRAPPER_NESTED_DEFINE_20(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19)                                                                       \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_19(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19)
#define _WRAPPER_NESTED_DEFINE_21(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20)                                                                  \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_20(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20)
#define _WRAPPER_NESTED_DEFINE_22(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21)                                                             \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_21(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21)
#define _WRAPPER_NESTED_DEFINE_23(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22)                                                        \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_22(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22)
#define _WRAPPER_NESTED_DEFINE_24(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23)                                                   \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_23(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23)
#define _WRAPPER_NESTED_DEFINE_25(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24)                                              \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_24(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24)
#define _WRAPPER_NESTED_DEFINE_26(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25)                                         \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_25(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25)
#define _WRAPPER_NESTED_DEFINE_27(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25, T26)                                    \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_26(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25, T26)
#define _WRAPPER_NESTED_DEFINE_28(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27)                               \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_27(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25, T26, T27)
#define _WRAPPER_NESTED_DEFINE_29(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28)                          \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_28(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25, T26, T27, T28)
#define _WRAPPER_NESTED_DEFINE_30(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29)                     \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_29(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29)
#define _WRAPPER_NESTED_DEFINE_31(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30)                \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_30(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30)
#define _WRAPPER_NESTED_DEFINE_32(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30, T31)           \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_31(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30, T31)
#define _WRAPPER_NESTED_DEFINE_33(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30, T31, T32)      \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_32(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30, T31, T32)
#define _WRAPPER_NESTED_DEFINE_34(FUNC, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16,     \
                                  T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30, T31, T32, T33) \
    _WRAPPER_NESTED_DEFINE_1(FUNC, T0)                                                                                \
    _WRAPPER_NESTED_DEFINE_33(FUNC, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18,   \
                              T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30, T31, T32, T33)

// arg nums
#define VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19,     \
                         _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, __N, ...)          \
    __N

#define VA_NUM_ARGS(...)                                                                                               \
    VA_NUM_ARGS_IMPL(__VA_ARGS__, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14,  \
                     13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define VA_NUM_ARGS_WRAPPPER(...) VA_NUM_ARGS(1, ##__VA_ARGS__)

#define __CONNECT2(__0, __1) __0##__1
#define CONNECT2(__0, __1)   __CONNECT2(__0, __1)
#define WRAPPER_NESTED_DEFINE_WITH_CTX(FUNC, ctx, ...)                                                                 \
    CONNECT2(_WRAPPER_NESTED_DEFINE_WITH_CTX_, VA_NUM_ARGS_WRAPPPER(__VA_ARGS__))                                      \
    (FUNC, ctx, __VA_ARGS__)
#define WRAPPER_NESTED_DEFINE(FUNC, ...)                                                                               \
    CONNECT2(_WRAPPER_NESTED_DEFINE_, VA_NUM_ARGS_WRAPPPER(__VA_ARGS__))                                               \
    (FUNC, __VA_ARGS__)
// clang-format on
} // namespace api
} // namespace xpu
} // namespace baidu
#endif 