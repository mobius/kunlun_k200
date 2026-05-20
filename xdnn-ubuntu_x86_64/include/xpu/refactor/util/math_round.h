#ifndef BAIDU_XPU_API_INCLUDE_XPU_UTIL_MATH_ROUND_H
#define BAIDU_XPU_API_INCLUDE_XPU_UTIL_MATH_ROUND_H

template <typename TX, typename TB> inline TX roundup_div(TX x, TB base) {
    if (base < static_cast<TB>(1)) {
        return 0;
    }
    return (x + static_cast<TX>(base) - 1) / static_cast<TX>(base);
}
template <typename TX, typename TB> inline TX roundup(TX x, TB base) {
    return roundup_div(x, base) * static_cast<TX>(base);
}
template <typename TX, typename TB> inline TX rounddown_div(TX x, TB base) {
    if (x < static_cast<TX>(1) || base < static_cast<TB>(1)) {
        return 0;
    }
    return x / static_cast<TX>(base);
}
template <typename TX, typename TB> inline TX rounddown(TX x, TB base) {
    return rounddown_div(x, base) * static_cast<TX>(base);
}
template <typename T> inline T roundup512(T n) {
    return roundup(n, 512);
}
template <typename T> inline T roundup128(T n) {
    return roundup(n, 128);
}
template <typename T> inline T roundup64(T n) {
    return roundup(n, 64);
}
template <typename T> inline T roundup32(T n) {
    return roundup(n, 32);
}
template <typename T> inline T roundup16(T n) {
    return roundup(n, 16);
}
#endif // BAIDU_XPU_API_INCLUDE_XPU_UTIL_MATH_ROUND_H
