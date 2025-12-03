#include "cppev/ipc.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>

namespace cppev
{

shared_memory::shared_memory(const std::string &name, int size, mode_t mode)
    : name_(name), size_(size), ptr_(nullptr), creator_(false)
{
    int fd = shm_open(name_.c_str(), O_RDWR, mode);
    if (fd < 0)
    {
        // ENOENT（对象不存在）
        if (errno == ENOENT)
        {
            // O_EXCL 保证了操作的原子性：如果创建成功，当前进程就是创建者（creator_ = true）。
            // mode指定了新创建的共享内存对象的访问权限
            fd = shm_open(name_.c_str(), O_RDWR | O_CREAT | O_EXCL, mode);
            if (fd < 0)
            {
                // 被其他进程抢先创建
                if (errno == EEXIST)
                {
                    fd = shm_open(name_.c_str(), O_RDWR, mode);
                    if (fd < 0)
                    {
                        throw_system_error("shm_open error");
                    }
                }
                else
                {
                    throw_system_error("shm_open error");
                }
            }
            else
            {
                creator_ = true;
            }
        }
        else
        {
            throw_system_error("shm_open error");
        }
    }

    if (creator_)
    {
        // 使用 ftruncate 函数来设置其大小（size_）
        //只有在设置了正确的大小后，后续的 mmap 调用才能成功地将这块内存区域映射到进程的地址空间。
        int ret = ftruncate(fd, size_);
        if (ret == -1)
        {
            throw_system_error("ftruncate error");
        }
    }
    ptr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr_ == MAP_FAILED)
    {
        throw_system_error("mmap error");
    }
    close(fd);
    if (creator_)
    {
        memset(ptr_, 0, size_);
    }
}

shared_memory::shared_memory(shared_memory &&other) noexcept
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<shared_memory>(other));
}

shared_memory &shared_memory::operator=(shared_memory &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<shared_memory>(other));
    return *this;
}

// 个完整的共享内存清理流程通常是：先在析构函数中调用 munmap（解除映射）
// 然后在创建者进程的生命周期结束时调用 shm_unlink个完整的共享内存清理流程通常是：
// 先在析构函数中调用 munmap（解除映射），然后在创建者进程的生命周期结束时调用 shm_unlink
// 在对象生命周期结束时，解除内存映射，从而释放当前进程对共享内存区域的访问权。
shared_memory::~shared_memory() noexcept
{
    if (ptr_ != nullptr && size_ != 0)
    {
        munmap(ptr_, size_);
    }
}

//  解除映射
void shared_memory::unlink()
{
    if (name_.size() && (shm_unlink(name_.c_str()) == -1))
    {
        throw_system_error("shm_unlink error");
    }
}

void *shared_memory::ptr() const noexcept
{
    return ptr_;
}

int shared_memory::size() const noexcept
{
    return size_;
}

bool shared_memory::creator() const noexcept
{
    return creator_;
}

void shared_memory::move(shared_memory &&other) noexcept
{
    this->name_ = other.name_;
    this->size_ = other.size_;
    this->ptr_ = other.ptr_;
    this->creator_ = other.creator_;

    other.name_ = "";
    other.size_ = 0;
    other.ptr_ = nullptr;
    other.creator_ = false;
}

semaphore::semaphore(const std::string &name, mode_t mode)
    : name_(name), sem_(nullptr), creator_(false)
{
    sem_ = sem_open(name_.c_str(), 0);
    if (sem_ == SEM_FAILED)
    {
        if (errno == ENOENT)
        {
            sem_ = sem_open(name_.c_str(), O_CREAT | O_EXCL, mode, 0);
            if (sem_ == SEM_FAILED)
            {
                // 解决竞态问题，被其他进程抢先创建
                if (errno == EEXIST)
                {
                    sem_ = sem_open(name_.c_str(), 0);
                    if (sem_ == SEM_FAILED)
                    {
                        throw_system_error("sem_open error");
                    }
                }
                else
                {
                    throw_system_error("sem_open error");
                }
            }
            else
            {
                creator_ = true;
            }
        }
        else
        {
            throw_system_error("sem_open error");
        }
    }
}

semaphore::semaphore(semaphore &&other) noexcept
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<semaphore>(other));
}

semaphore &semaphore::operator=(semaphore &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<semaphore>(other));
    return *this;
}

semaphore::~semaphore() noexcept
{
    if (sem_ != SEM_FAILED)
    {
        sem_close(sem_);
    }
}

bool semaphore::try_acquire()
{
    if (sem_trywait(sem_) == -1)
    {
        // sem_trywait() 返回失败的最常见原因，表示信号量计数当前为 0。由于 try_acquire 是非阻塞的，它不能等待。
        // sem_trywait() 在完成前被一个信号中断。由于 EAGAIN 是非阻塞的，通常也将其视为未能成功获取资源
        if (errno == EINTR || errno == EAGAIN)
        {
            return false;
        }
        else
        {
            throw_system_error("sem_trywait error");
        }
    }
    return true;
}

void semaphore::acquire(int count)
{
    for (int i = 0; i < count; ++i)
    {
        // 阻塞等待三次
        if (sem_wait(sem_) == -1)
        {
            throw_system_error("sem_wait error");
        }
    }
}

void semaphore::release(int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (sem_post(sem_) == -1)
        {
            throw_system_error("sem_post error");
        }
    }
}

void semaphore::unlink()
{
    if (name_.size() && (sem_unlink(name_.c_str()) == -1))
    {
        throw_system_error("sem_unlink error");
    }
}

bool semaphore::creator() const noexcept
{
    return creator_;
}

void semaphore::move(semaphore &&other) noexcept
{
    this->name_ = other.name_;
    this->sem_ = other.sem_;
    this->creator_ = other.creator_;

    other.name_ = "";
    other.sem_ = SEM_FAILED;
    other.creator_ = false;
}

}  // namespace cppev
