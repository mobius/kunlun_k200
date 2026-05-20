#include "xpu/refactor/gtest/xdnn_gtest.h"
#include <gflags/gflags.h>

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, false);
    return xdnn_gtest_main();
}
