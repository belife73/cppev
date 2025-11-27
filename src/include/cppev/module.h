#ifndef _cppev_buffer_h_
#define _cppev_buffer_h_

#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>
#include <string>
#include <iostream> // 用于演示

#include "cppev/common.h" // 假设这些文件存在
#include "cppev/utils.h"

namespace cppev
{

// 1. 定义原型接口 (Abstract Prototype)
// 即使目前只有一个类，定义接口也有助于未来的扩展（如 CircularBuffer, SharedBuffer 等）
template <typename Char>
class BufferPrototype {
public:
    virtual ~BufferPrototype() = default;
    // 核心方法：克隆
    virtual std::unique_ptr<BufferPrototype<Char>> clone() const = 0;
};

template <typename Char>
class CPPEV_PUBLIC basic_buffer final : public BufferPrototype<Char>
{
public:
    basic_buffer() noexcept : basic_buffer(1)
    {
    }

    explicit basic_buffer(int cap) noexcept : cap_(cap), start_(0), offset_(0)
    {
        if (cap_ < 1)
        {
            cap_ = 1;
        }
        buffer_ = std::make_unique<Char[]>(cap_);
        memset(buffer_.get(), 0, cap_);
    }

    // 拷贝构造函数（保持现有逻辑，用于深拷贝）
    basic_buffer(const basic_buffer &other) noexcept
    {
        copy_from(other);
    }

    basic_buffer &operator=(const basic_buffer &other) noexcept
    {
        if (this != &other) {
            copy_from(other);
        }
        return *this;
    }

    // 移动构造和赋值
    basic_buffer(basic_buffer &&other) noexcept = default;
    basic_buffer &operator=(basic_buffer &&other) = default;

    ~basic_buffer() override = default;

    // -----------------------------------------------------------
    // 2. 实现原型模式的 Clone 方法
    // -----------------------------------------------------------
    std::unique_ptr<BufferPrototype<Char>> clone() const override
    {
        // 利用拷贝构造函数创建一个新的对象，并封装在 unique_ptr 中返回
        return std::make_unique<basic_buffer<Char>>(*this);
    }

    // 为了方便使用，也可以提供一个返回具体类型的强类型 clone
    std::unique_ptr<basic_buffer<Char>> clone_self() const
    {
        return std::make_unique<basic_buffer<Char>>(*this);
    }

    // -----------------------------------------------------------
    // 业务逻辑方法 (保持不变)
    // -----------------------------------------------------------

    Char &operator[](int i) noexcept
    {
        return buffer_[start_ + i];
    }

    Char at(int i) const noexcept
    {
        return buffer_[start_ + i];
    }

    int waste() const noexcept
    {
        return start_;
    }

    int size() const noexcept
    {
        return offset_ - start_;
    }

    int capacity() const noexcept
    {
        return cap_;
    }

    // Getters and Setters
    int get_start() const noexcept { return start_; }
    void set_start(int start) noexcept { start_ = start; }
    int &get_start_ref() noexcept { return start_; }

    int get_offset() const noexcept { return offset_; }
    void set_offset(int offset) noexcept { offset_ = offset; }
    int &get_offset_ref() noexcept { return offset_; }

    const Char *ptr() const noexcept { return buffer_.get(); }
    Char *ptr() noexcept { return buffer_.get(); }

    const Char *data() const noexcept { return buffer_.get() + start_; }
    Char *data() noexcept { return buffer_.get() + start_; }

    void resize(int cap) noexcept
    {
        if (cap_ >= cap) return;
        while (cap_ < cap) cap_ *= 2;
        
        auto nbuffer = std::make_unique<Char[]>(cap_);
        memset(nbuffer.get(), 0, cap_);
        
        // 优化：使用 memcpy 处理 POD 类型更高效，但循环更通用
        // 这里保持原逻辑
        for (int i = start_; i < offset_; ++i)
        {
            nbuffer[i] = buffer_[i];
        }
        buffer_ = std::move(nbuffer);
    }

    void tiny() noexcept
    {
        if (start_ == 0) return;
        int len = offset_ - start_;
        // 移动有效数据到头部
        std::memmove(buffer_.get(), buffer_.get() + start_, len * sizeof(Char));
        // 清理剩余空间
        memset(buffer_.get() + len, 0, (cap_ - len) * sizeof(Char));
        start_ = 0;
        offset_ = len;
    }

    void clear() noexcept
    {
        memset(buffer_.get(), 0, cap_);
        start_ = 0;
        offset_ = 0;
    }

    void put_string(const Char *ptr, int len) noexcept
    {
        resize(offset_ + len);
        // 优化：对于 char 类型，memcpy 通常比循环快
        std::memcpy(buffer_.get() + offset_, ptr, len * sizeof(Char));
        offset_ += len;
    }

    void put_string(const std::basic_string<Char> &str) noexcept
    {
        put_string(str.c_str(), str.size());
    }

    
    std::basic_string<Char> get_string(int len = -1, bool consume = true) noexcept
    {
        if (len == -1) len = size();
        if (start_ + len > offset_) len = size(); // 安全检查

        std::basic_string<Char> str(buffer_.get() + start_, len);
        if (consume)
        {
            start_ += len;
        }
        return str;
    }

private:
    int cap_;
    int start_;
    int offset_;
    std::unique_ptr<Char[]> buffer_;

    // 提取出的私有辅助函数，用于统一深拷贝逻辑
    void copy_from(const basic_buffer &other) noexcept
    {
        this->cap_ = other.cap_;
        this->start_ = other.start_;
        this->offset_ = other.offset_;
        this->buffer_ = std::make_unique<Char[]>(cap_);
        // 确保深拷贝内容
        std::memcpy(this->buffer_.get(), other.buffer_.get(), cap_ * sizeof(Char));
    }
};

using buffer = basic_buffer<char>;

}  // namespace cppev

#endif  // _cppev_buffer_h_