## 简介
xdnn plugin 机制是xdnn库的扩展， 由算子库官方提供的一套源码及脚本，使用这套源码和脚本，用户就可以基于xdnn的公开头文件&lib/so 快速进行自定义接口的开发与调用。
xdnn plugin 将所有开发需要的头文件，lib/so 打包到xdnn 产出中的 output/plugin 目录下。
xdnn plugin 算子开发完成后，用户可以链接生成的 libxpuplugin.so 使用对应算子。
### 目录结构
目录以及主要文件
```
output/plugin/                              
├── CMakeLists.txt                              // plugin 模块的CMakefilelist
├── example                                     // plugin 算子使用例子
│   ├── build.sh
│   ├── example.cpp
│   └── Makefile
├── include                                     // plugin 头文件. 
│   └── xdnn_plugin.h
├── script                                      // plugin 算子开发所需脚本
│   ├── cmake_build.sh                          // 编译脚本
│   ├── dependency.sh                           // plugin 依赖
│   ├── get_output.py                           
│   ├── get_platformInfo.sh
│   └── linker.specs
├── src                                         // plugin 算子源代码目录
│   ├── kernel                                  // kernel 层代码
│   │   ├── include                             // kernel 开发自定义头文件目录
│   │   └── kunlun2cpp                          // 算子实际 kernel 代码
│   └── wrapper                                 // wrapper 层代码
│       ├── add2.cpp                            // example, 基于xtdk 头文件
│       └── sub2.cpp                            // example, 基于xdnn 头文件
├── tests                                       // plugin 算子单测目录
│   └── unittest    
│       ├── refactor                            
|       │   ├── test_add2.cpp                   // 对应两个plugin 算子例子
│       │   └── test_sub2.cpp
│       └── test_main.cpp
└── third_party                                 // 第三方库目录
    ├── googletest-release-1.10.0
    │   └── build
    └── third_party
        ├── gflags-v2.2.2
        └── googletest-release-1.10.0
```
## plugin 算子的开发
1. 源码编译获取最新产出： 
```
sh script/cmake_build.sh
```
2. 进入产出目录
```
cd output/plugin
```
3. 新增plugin算子定义，新增kernel，wrappe, unittest文件,
3.0 新增plugin算子定义
在include/plugin.h 中新增算子的定义。如提供的两个例子
```
// y[i] = x[i] + 2
DLL_EXPORT int add2(baidu::xpu::api::Context* ctx, const float* x, float* y, int len);
// y[i] = x[i] - 2
DLL_EXPORT int sub1(baidu::xpu::api::Context* ctx, const float* x, float* y, int len);
```
3.1 在src/kernel 目录下新增kernel 文件
提供了两个例子
```
add1.xpu, 基于xtdk 头文件进行开发
sub1.xpu, 基于xdnn 头文件进行开发， 推荐
```
3.2 在 src/wrapper 目录下增加 wrappr文件
对应于两个例子
```
src/wrapper/test_add2.cpp
src/wrapper/test_sub2.cpp
```
3.3 在tests/unittest/refactor 目录下新增单测
```
tests/unittest/refactor/test_add2.cpp
tests/unittest/refactor/test_sub2.cpp
```
3.4 编译并执行测试，确认开发算子功能正确
编译
```
cd output/plugin
bash script/cmake_build.sh
会在
test_refactor 文件
```
执行测试
```
test_refactor --gtest_filter=test*add2*
test_refactor --gtest_filter=test*sub2*
```

## plugin算子的使用
plugin 算子开发完成后，编译后会在 output/plugin/so目录下 生成libxpuplugin.so 文件。
用户按正常的使用lib库的方式即可调用开发好的plugin算子。
example目录下提供了一个简单的例子可供用户参考。

