#!/bin/bash
set -ex
gcc_path=${GCC_PATH:-"/path_to_gcc"}
cmake_path=${CMAKE_PATH:-"/path_to_cmake/"}
xtdk_path=${XTDK_PATH:-"/path_to_xtdk/"}
xre_path=${XRE_PATH:-"/path_to_xre/"}
source ./script/dependency.sh
source ./script/get_platformInfo.sh
#############################################################################################################
with_dependency_cache=${WITH_DEPENDENCY_CACHE:-OFF}

function set_arch() {
    host_arch=""
    if [[ $(uname -m) == x86_64 ]]; then
        host_arch="x86_64-baidu-linux-gnu"
    elif [[ $(uname -m) == aarch64 || $(uname -m) == arm64 ]]; then
        host_arch="aarch64-linux-gnu"
    fi
    if [[ ${host_arch} == "" ]]; then
        echo "error host_arch"
        exit -1
    fi
    if [[ ${target_arch} == "" ]]; then
        target_arch=${host_arch}
    fi
    if [[ ${target_arch} =~ "x86" ]]; then
        target_arch="x86_64-baidu-linux-gnu"
    fi
    if [[ ${target_arch} =~ "aarch" ]]; then
        target_arch="aarch64-linux-gnu"
    fi
}
function set_gcc_path() {
    if [ -f ${gcc_path}/bin/g++ ]; then
        export PATH=${gcc_path}/bin/:$PATH
        return
    fi
    if [[ ${host_arch} =~ "x86" ]] && [[ ${target_arch} =~ "aarch64" ]];then
        gcc_path=/opt/compiler/gcc-linaro-5.4.1-2017.01-x86_64_aarch64-linux-gnu
        export PATH=$PATH:${gcc_path}/bin/
        export CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++
        export CROSS_PREFIX=aarch64-linux-gnu-
        return
    fi
    if [ -f /opt/compiler/gcc-8.2/bin/g++ ] && [[ ${os_release} =~ "centos" ]]; then
        gcc_path=/opt/compiler/gcc-8.2
        export PATH=${gcc_path}/bin/:$PATH
        return
    fi
    gcc_path=/
}

function set_cmake_path() {
    if [ -f ${cmake_path}/bin/cmake ]; then
        return
    fi
    if [[ ${with_dependency_cache} = OFF ]];then
        rm -rf ../cmake-3.20.1-linux-x86_64/
        cmake_url='http://gzbh-aip-paddlecloud140.gzbh:8080/miaotianxiang/public/cmake-3.20.1-linux-x86_64.tar.gz'
        wget --no-check-certificate -q -O - ${cmake_url} | tar -zxf - -C ../
    fi
    # set_cmake_path
    cmake_path=`readlink -f "../cmake-3.20.1-linux-x86_64/"`
}
function set_xtdk_path() {
    if [ -f ${xtdk_path}/bin/clang++ ]; then
        return
    fi
    xtdk_fallback_path="../xtdk_output/"
    if [[ ${with_dependency_cache} = OFF ]];then
        rm -rf ${xtdk_fallback_path}
        mkdir ${xtdk_fallback_path}
        pushd ${xtdk_fallback_path}
        wget -q --no-check-certificate https://klx-sdk-release-public.su.bcebos.com/xtdk_llvm15/dev/${XTDK_VERSION}/${xtdk_download}.tar.gz && tar zxf ${xtdk_download}.tar.gz
    else
        pushd ${xtdk_fallback_path}
    fi
    xtdk_path=`readlink -f ${xtdk_download}`
    popd
}
function set_xre_path() {
    if [ -f ${xre_path}/output/so/libxpurt.so ]; then
        return
    fi
    xre_fallback_path="../runtime"
    if [[ ${with_dependency_cache} = OFF ]];then
        rm -rf ${xre_fallback_path}
        mkdir ${xre_fallback_path}
        pushd ${xre_fallback_path}
        wget --no-check-certificate https://klx-sdk-release-public.su.bcebos.com/xre/release/${XRE_VERSION}/${xre_download}.tar.gz && tar zxf ${xre_download}.tar.gz
        mv ${xre_download} output
        popd
    fi
    xre_path=`readlink -f ${xre_fallback_path}`
}
function set_gtest_path() {
    if [[ ${with_dependency_cache} = OFF ]];then
        rm -rf ./third_party/googletest-release-1.10.0/build
        mkdir -p ./third_party/googletest-release-1.10.0/build
        pushd ./third_party/googletest-release-1.10.0/build
        ${cmake_path}/bin/cmake .. -DBUILD_GMOCK=OFF
        make
        popd
    fi
}
function set_gflags_path() {
    if [[ ${with_dependency_cache} = OFF ]];then
        rm -rf ./third_party/gflags-v2.2.2/build
        mkdir -p ./third_party/gflags-v2.2.2/build
        pushd ./third_party/gflags-v2.2.2/build
        ${cmake_path}/bin/cmake -DGFLAGS_NAMESPACE=google ..
        make
        popd
    fi
}
function build(){
    rm -rf src/lib
    rm -rf ./build
    mkdir build
    pushd build
    ${cmake_path}/bin/cmake ../ -DCLANG_PATH=${xtdk_path} -DHOST_SYSROOT=${gcc_path} && make -j32
    popd
}


###main entry###
set_arch
set_gcc_path
set_cmake_path
set_xtdk_path
set_xre_path
set_gtest_path
set_gflags_path
echo ${cmake_path}
echo ${xtdk_path}
echo ${xre_path}

build

