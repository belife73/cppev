#ifndef _cppev_dynamic_loader_h_6C0224787A17_
#define _cppev_dynamic_loader_h_6C0224787A17_

#include <dlfcn.h>
// dlopen: 打开动态库。
// dlsym: 获取函数或变量的地址（符号查找）。
// dlclose: 关闭动态库。
// dlerror: 获取最近一次的错误信息。

#include <string>

#include "cppev/common.h"
#include "cppev/utils.h"

namespace cppev
{

enum class CPPEV_PUBLIC dyld_mode
{
    // 惰性加载。只有当代码真正执行到某个符号（函数）时，系统才去解析它的地址。启动快，但如果符号不存在，程序会在运行中崩溃。
    lazy,
    // 立即加载。程序在加载动态库时，就会解析所有符号的地址。如果有符号不存在，程序会在启动时就报错退出。启动慢，但运行中不会因为找不到符号而崩溃。
    now,
};

class CPPEV_PUBLIC dynamic_loader
{
public:
    explicit dynamic_loader(const std::string &filename, dyld_mode mode);

    // 禁止拷贝和移动
    dynamic_loader(const dynamic_loader &) = delete;
    dynamic_loader &operator=(const dynamic_loader &) = delete;
    dynamic_loader(dynamic_loader &&other) = delete;
    dynamic_loader &operator=(dynamic_loader &&other) = delete;

    ~dynamic_loader() noexcept;

    template <typename Function>
    Function *load(const std::string &func) const
    {
        void *ptr = dlsym(handle_, func.c_str());
        if (ptr == nullptr)
        {
            // errno is set in macOS, but not in linux
            throw_runtime_error(
                std::string("dlsym error : ").append(dlerror()));
        }
        return reinterpret_cast<Function *>(ptr);
    }

private:
    void *handle_;
};

}  // namespace cppev

#endif  // _cppev_dynamic_loader_h_6C0224787A17_