#ifndef _cppev_ipc_h_6C0224787A17_
#define _cppev_ipc_h_6C0224787A17_

#include <semaphore.h>
#include <sys/time.h>

#include <string>

#include "cppev/common.h"
#include "cppev/utils.h"

namespace cppev
{

class CPPEV_PUBLIC shared_memory final
{
public:
    shared_memory(const std::string &name, int size, mode_t mode = 0600);
    //系统资源类的封装通常都禁止拷贝。
    shared_memory(const shared_memory &) = delete;
    shared_memory &operator=(const shared_memory &) = delete;
    shared_memory(shared_memory &&other) noexcept;
    shared_memory &operator=(shared_memory &&other) noexcept;

    ~shared_memory() noexcept;

    template <typename SharedClass, typename... Args>
    SharedClass *construct(Args &&...args)
    {
        SharedClass *object =
            // 不要分配内存！ 我手里已经有一块地了（就是 ptr_，指向那块共享内存）。
            // 请你直接在这块地上盖房子（调用构造函数）。
            new (ptr_) SharedClass(std::forward<Args>(args)...);
        if (object == nullptr)
        {
            throw_runtime_error("placement new error");
        }
        return object;
    }

    void unlink();

    void *ptr() const noexcept;

    int size() const noexcept;

    bool creator() const noexcept;

private:
    void move(shared_memory &&other) noexcept;

    std::string name_;

    int size_;

    void *ptr_;

    bool creator_;
};

class CPPEV_PUBLIC semaphore final
{
public:
    // 创建和初始化信号量的地方
    explicit semaphore(const std::string &name, mode_t mode = 0600);

    // 系统资源（如文件描述符或系统信号量句柄）是独占的，不适合进行浅拷贝或自动深拷贝。
    semaphore(const semaphore &) = delete;
    semaphore &operator=(const semaphore &) = delete;
    semaphore(semaphore &&other) noexcept;
    semaphore &operator=(semaphore &&other) noexcept;

    ~semaphore() noexcept;
    // 非阻塞
    bool try_acquire();
    // 阻塞
    void acquire(int count = 1);

    void release(int count = 1);
    // 销毁或删除系统中具名信号量
    void unlink();

    bool creator() const noexcept;

private:
    void move(semaphore &&other) noexcept;

    // 信号量的名称，用于在系统中查找或创建信号量
    std::string name_;
    // 这是 POSIX 信号量 API（如 sem_open）返回的句柄
    sem_t *sem_;
    // 记录当前对象是否是信号量的创建者
    bool creator_;
};

}  // namespace cppev

#endif  // ipc.h
