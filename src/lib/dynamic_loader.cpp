#include "cppev/dynamic_loader.h"

namespace cppev
{

dynamic_loader::dynamic_loader(const std::string &filename, dyld_mode mode)
    : handle_(nullptr)
{
    // 1. 设置基础标志位：全局可见
    auto all_mode = RTLD_GLOBAL;

    // 2. 根据用户参数，组合加载模式 (按位或运算)
    if (mode == dyld_mode::lazy)
    {
        all_mode |= RTLD_LAZY; // 惰性加载
    }
    else
    {
        all_mode |= RTLD_NOW;  // 立即加载
    }

    // 3. 调用系统 API 打开动态库
    handle_ = dlopen(filename.c_str(), all_mode);

    // 4. 错误处理
    if (handle_ == nullptr)
    {
        // 获取具体的错误信息 (如 "file not found", "undefined symbol") 并抛出异常
        throw_runtime_error(std::string("dlopen error : ").append(dlerror()));
    }
}

dynamic_loader::~dynamic_loader() noexcept
{
    dlclose(handle_);
}

}  // namespace cppev
