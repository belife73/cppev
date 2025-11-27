#pragma once
#ifndef CPPEV_BUFFER_H_
#define CPPEV_BUFFER_H_

#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>

#include "cppev/common.h"
#include "cppev/utils.h"

namespace cppev
{
    /*
    *原型设计模式
    */ 
    // 1. 定义原型接口 (Abstract Prototype)

    template <typename Char>
    class BufferPrototype
    {
    public:
        virtual ~BufferPrototype() = default;
        // 核心方法：克隆
        virtual std::unique_ptr<BufferPrototype<Char>> clone() const = 0;
    };

    template <typename Char>
    // 禁止继承
    class CPPEV_PUBLIC basic_buffer final : public BufferPrototype<Char>
    {
    public:
        basic_buffer() noexcept : basic_buffer(1){}

        explict basic_buffer(int cap) noexcept : cap_(cap), start_(0), offset_(0)
        {
            // 确保容量至少为1
            if (cap_ < 1)
            {
                cap_ = 1;
            }
            buffer_ = std::make_unique<Char[]>(cap_);
            // 初始化缓冲区为0
            memset(buffer_.get(), 0, cap_ * sizeof(Char));
        }

        // 深拷贝
        basic_buffer(const basic_buffer &other) noexcept
        {
            copy_from_other(other);
        }

        basic_buffer &operator=(const basic_buffer &other) noexcept
        {
            if (this != &other) {
                copy_from_other(other);
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
            return std::make_unique<basic_buffer>(*this);
        }

        // 提供强类型的 clone 方法，方便使用
        // 避免多次static_cast转换为具体类型
        std::unique_ptr<basic_buffer<Char>> clone_self() const
        {
            return std::make_unique<basic_buffer>(*this);
        }

        // -----------------------------------------------------------
        // 业务逻辑方法 (保持不变)
        // -----------------------------------------------------------

        Char &operator[](int i) noexcept
        {
            return buffer_[start_ + i];
        }

        // Char at(int i) const noexcept
        // {
        //     return buffer_[start_ + i];
        // }
        // 增加越界检查
        Char at(int i) 
        {
            if (i < 0 || i >= size())
            {
                throw std::out_of_range("Index out of range");
            }
            return buffer_[start_ + i];
        }

        // 太大要清理buffer_
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

        // 获得和设置 start_ 和 offset_ 的方法
        int get_start() const noexcept { return start_; }
        void set_start(int start) noexcept { start_ = start; }
        int &get_start_ref() noexcept { return start_; }

        int get_offset() const noexcept { return offset_; }
        void set_offset(int offset) noexcept { offset_ = offset; }
        int &get_offset_ref() noexcept { return offset_; }

        // 获取底层缓冲区指针
        const Char *ptr() const noexcept { return buffer_.get(); }
        Char *ptr() noexcept { return buffer_.get(); }

        const Char *data() const noexcept { return buffer_.get() + start_; }
        Char *data() noexcept { return buffer_.get() + start_; }

        void resize(int cap) noexcept
        {
            if (cap_ >= cap) return;
            while (cap_ < cap) cap_ *= 2;
            std::unique_ptr<Char[]> nbuffer = std::make_unique<Char[]>(cap_);
            memset(nbuffer.get(), 0, cap_ * sizeof(Char));
            // 复制现有还未消费的数据
            for (int i = start_; i < offset_; ++i)
            {
                nbuffer[i] = buffer_[i];
            }
            buffer_ = std::move(nbuffer);
        }

        void tiny_compact() noexcept
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
            // 扩大容量以容纳新数据
            resize(offset_ + len);
            // 优化：对于 char 类型，memcpy 通常比循环快
            std::memcpy(buffer_.get() + offset_, ptr, len * sizeof(Char));
            offset_ += len;
        }

        // basic_string = string
        void put_string(const std::basic_string<Char> &str) noexcept
        {
            put_string(str.c_str(), str.size());
        }

        std::basic_string<Char> get_string(int len = -1, bool consume = true) noexcept
        {
            if (len < 0 || len > size())
            {
                len = size();
            }

            std::basic_string<Char> result(buffer_.get() + start_, len);
            if (consume)
            {
                start_ += len;
            }
            return result;
        }

    private:
        // 堆容量
        int cap_;
        // 缓冲区起始位置，该字节包含在内
        int start_;
        // 缓冲区结束位置，该字节不包含在内
        int offset_;

        // 堆缓冲区
        std::unique_ptr<Char[]> buffer_;

        void copy_from_other(const basic_buffer &other) noexcept
        {
            if (&other != this)
            {
                this->cap_ = other.cap_;
                this->start_ = other.start_;
                this->offset_ = other.offset_;
                this->buffer_ = std::make_unique<Char[]>(cap_);
                memcpy(this->buffer_.get(), other.buffer_.get(), cap_ * sizeof(Char));
            }            
        }
    };

} // namespace cppev

#endif // CPPEV_BUFFER_H_