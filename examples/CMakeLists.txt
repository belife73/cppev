function(compile_target target_name_)
    # 1. 创建可执行文件目标
    # 将 ${target_name_}.cc 编译成名为 ${target_name_} 的可执行程序
    add_executable(${target_name_} ${target_name_}.cc)
    # 2. 链接核心库
    # 将生成的可执行程序与 cppev 库进行链接
    # 这样程序才能调用 cppev::socktcp, cppev::reactor 等符号
    target_link_libraries(${target_name_} cppev)
    # 3. 继承编译选项 (关键行)
    # 这一行使用了 CMake 的生成器表达式 (Generator Expression)
    # $<TARGET_PROPERTY:cppev,INTERFACE_COMPILE_OPTIONS>: 
    #   意思是从 'cppev' 这个目标中，读取它的 'INTERFACE_COMPILE_OPTIONS' 属性。
    #   通常包含像 -std=c++17, -Wall, -O2 等编译标志。
    # target_compile_options(..., PRIVATE ...):
    #   将读取到的这些选项应用到当前的示例程序中。
    # 目的：确保示例代码和主库使用完全相同的编译器标志进行编译，防止因为标准版本不一致(如 C++11 vs C++17)导致 ABI 兼容性问题。
    target_compile_options(${target_name_}  PRIVATE $<TARGET_PROPERTY:cppev,INTERFACE_COMPILE_OPTIONS>)
endfunction(compile_target)

add_subdirectory(tcp_stress)
add_subdirectory(file_transfer)
add_subdirectory(io_evlp)

# MyProject/
#├── CMakeLists.txt          <-- (1) 根目录脚本：调用 add_subdirectory(src)
#├── src/
#│   ├── CMakeLists.txt      <-- (2) 子目录脚本：定义 add_library(MyLib ...)
#│   ├── core.cpp
#│   └── network.cpp
#└── examples/
#    ├── CMakeLists.txt      <-- (3) 子目录脚本：定义 add_executable(Demo ...)
#    └── demo.cpp