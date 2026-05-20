#!/bin/bash
set -ex
xdnn_path=${XDNN_PATH:-"../../"}
xre_path=${XRE_PATH:-"../../runtime/output"}
if [ -f /opt/compiler/gcc-8.2/bin/g++ ]; then
    gcc_path=${GCC_PATH:-"/opt/compiler/gcc-8.2/"}
else
    gcc_path=${GCC_PATH:-"/usr/"}
fi
link_type=${LINK_TYPE:-"dynamic"}

XDNN_PATH=${xdnn_path} XRE_PATH=${xre_path} CXX=${gcc_path}/bin/g++ LINK_TYPE=${link_type} make
