#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_TIME_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_TIME_H

#ifdef _MSC_VER
#define NOMINMAX
#include <winsock.h>
#else
#include <sys/time.h>
#endif

#if defined(_MSC_VER)
#include <windows.h>
#include <chrono>
static int gettimeofday(struct timeval* tp, struct timezone* tzp) {
    namespace sc = std::chrono;
    sc::system_clock::duration d = sc::system_clock::now().time_since_epoch();
    sc::seconds s = sc::duration_cast<sc::seconds>(d);
    tp->tv_sec = s.count();
    tp->tv_usec = sc::duration_cast<sc::microseconds>(d - s).count();
    return 0;
}
#endif // _MSC_VER

#endif
