#pragma once
#ifndef CPPEV_UTILS_H
#define CPPEV_UTILS_H

#include <cassert>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <ctime>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "cppev/common.h"

namespace cppev
{
    // 如果需要将枚举值当作整数使用（例如记录到日志或进行数值计算），必须使用 static_cast进行显式转换
    enum class CPPEV_PUBLIC priority
    {
        highest = 100,  // 不要使用！因为它被内部保留
        p0 = 20,
        p1 = 19,
        p2 = 18,
        p3 = 17,
        p4 = 16,
        p5 = 15,
        p6 = 14,
        lowest = 1,  // 不要使用！因为它被内部保留
    };
// clang-format off
// 关闭格式化 

    // 解决C++标准（在C++14之前）中，std::unordered_map等哈希容器无法直接使用枚举类型作为键的问题
    struct CPPEV_PRIVATE enum_hash
    {
        // size_t 哈希标准返回值
        template <typename T> std::size_t operator()(const T &t) const noexcept
        {
            // 将枚举值转换为整数进行哈希计算
            return std::hash<int>()(static_cast<int>(t));
        }
    };

    // 用于对元组中特定索引的元素进行升序比较的函数对象
    template <typename T, size_t I>
    struct CPPEV_PRIVATE tuple_less
    {
        bool operator()(const T &lhs, const T &rhs) const noexcept
        {
            return std::get<I>(lhs) < std::get<I>(rhs);
        }
    };

    // 用于对元组中特定索引的元素进行降序比较的函数对象
    template <typename T, size_t I>
    struct CPPEV_PRIVATE tuple_greater
    {
        bool operator()(const T &lhs, const T &rhs) const noexcept
        {
            return std::get<I>(lhs) > std::get<I>(rhs);
        }
    };
    // clang-format on
    // 打开格式化

    /** chrono **/
    // 获取格式化的时间戳字符串
    CPPEV_PUBLIC std::string timestamp(time_t t = -1, const char* format = nullptr);

    // 指定时钟类型，默认为 std::chrono::system_clock
    template <typename Clock = std::chrono::system_clock>
    // 自己定义的sleep_until函数，用于将时间点向下取整到最近的秒
    CPPEV_PUBLIC void sleep_until(const std::chrono::nanoseconds& stamp)
    {
        std::this_thread::sleep_until(
            std::chrono::duration_cast<typename Clock::duration>(stamp));
    }

    template <typename Clock = std::chrono::system_clock>
    CPPEV_PRIVATE typename Clock::time_point ceil_time_point(const typename Clock::time_point &point)
    {
        auto stamp = std::chrono::nanoseconds(point.time_since_epoch()).count();
        int64_t ceil_stamp_nsec = (stamp / 1'000'000'000 + 1) * 1'000'000'000;
        auto ceil_stamp = std::chrono::duration_cast<typename Clock::duration>(
           std::chrono::nanoseconds(ceil_stamp_nsec));
        return typename Clock::time_point(ceil_stamp);
    }

    // 使用 C++17 标准库的简洁实现
    // template <typename Clock = std::chrono::system_clock>
    // CPPEV_PRIVATE typename Clock::time_point ceil_time_point(
    //     const typename Clock::time_point& point)
    // {
    //     return std::chrono::ceil<std::chrono::seconds>(point);
    // }

    /** 算法 **/
    // 计算最小公倍数
    CPPEV_PUBLIC int64_t least_common_multiple(int64_t p, int64_t r);

    // 计算最小公倍数
    CPPEV_PUBLIC int64_t least_common_multiple(const std::vector<int64_t> &nums);

    // 计算最大公约数
    CPPEV_PUBLIC int64_t greatest_common_divisor(int64_t p, int64_t r);

    // 计算最大公约数
    CPPEV_PUBLIC int64_t greatest_common_divisor(const std::vector<int64_t> &nums);

    // 异常处理
    // 从C++14开始，标准库为类型萃取工具提供了更简洁的别名模板
    // using errno_type = std::remove_reference_t<decltype(errno)>;
    using errno_type = std::remove_reference<decltype(errno)>::type;


    // 模板会先匹配更具体的版本，再匹配通用版本
    template <typename T>
    CPPEV_PRIVATE std::ostringstream oss_writer(T err_code)
    {
        std::ostringstream oss;
        oss << " : errno " << err_code << " ";
        return oss;
    }

    template <typename Prev, typename... Args>
    CPPEV_PRIVATE std::ostringstream oss_writer(Prev prev, Args... args)
    {
        std::ostringstream oss;
        oss << prev;
        oss << oss_writer(args...).str();
        return oss;
    }

    // C++17完美转发与折叠表达式结合实现的日志拼接
    // template <typename... Args>
    // CPPEV_PRIVATE std::ostringstream oss_writer(Args&&... args) // 注意：万能引用
    // {
    //     std::ostringstream oss;
    //     (oss << ... << std::forward<Args>(args)); // 完美转发
    //     return oss;
    // }

    template <typename T>
    CPPEV_PRIVATE errno_type errno_getter(T err_code)
    {
        return err_code;
    }

    // 在抛出系统错误的上下文中，错误码（errno）通常作为最具体、最核心的信息最后被提供
    template <typename Prev, typename... Args>
    CPPEV_PRIVATE errno_type errno_getter(Prev, Args... args)
    {
        return errno_getter(args...);
    }

    // 使用C++17if constexpr实现的errno_getter，编译期递归终止
    // template <typename T, typename... Args>
    // CPPEV_PRIVATE errno_type errno_getter(T first, Args... args) {
    //     if constexpr (sizeof...(args) > 0) {
    //         // 如果还有剩余参数，递归处理剩余部分
    //         return errno_getter(args...);
    //     } else {
    //         // 如果这是最后一个参数，返回它
    //         return first;
    //     }
    // }

    template <typename... Args>
    CPPEV_PUBLIC void throw_system_error_with_specific_errno(Args... args)
    {
        // 使用折叠表达式拼接日志信息
        ostringstream oss = oss_writer(args...);

        // 提取最后一个参数作为错误码（因为最后一个参数通常是最具体、最核心的信息）
        errno_type err_code = errno_getter(args...);

        // std::error_code这是一个轻量级类，它将一个简单的整数错误码（如 errno的值）与一个错误类别​ 绑定在一起。
        // 这解决了传统上只使用一个全局整数（如 errno）无法区分错误来源的问题。

        //err_code：这是一个整数，代表具体的错误值，通常来自系统调用
        //std::system_category()：这是一个函数，返回一个代表操作系统错误类别的单例对象。
        //它告诉 std::error_code，这个 err_code是一个由底层操作系统（如POSIX或Windows系统）定义的错误码。这样，相同的整数值在不同的错误类别下会被解释为不同的错误。

        // std::system_error：这是一个特殊的异常类，派生自 std::runtime_error。它的主要特点是**内部封装了一个 std::error_code对象
        // 通过继承的 what()方法，可以获取一个描述错误的字符串信息，这个信息通常包含了错误码对应的文本描述，方便调试和日志记录。
        throw std::system_error(std::error_code(err_code, std::system_category()),
                                oss.str());
    }

    // Args... args: 可变参数模板，可以接受任意数量、任意类型的参数
    // args..., errno: 将传入的所有参数与当前的errno值一起传递给底层函数
    // errno: 全局变量，存储最近一次系统调用的错误代码
    // 封装后自动处理errno：
    template <typename... Args>
    CPPEV_PUBLIC void throw_system_error(Args... args)
    {
        throw_system_error_with_specific_errno(args..., errno);
    }

    // 拼接参数并抛出运行时错误异常
    template <typename... Args>
    CPPEV_PUBLIC void throw_logic_error(Args... args)
    {
        std::ostringstream oss;
        (oss << ... << args);
        // 抛出一个逻辑错误异常，包含拼接后的错误信息
        throw std::logic_error(oss.str());
    }

    // 拓展runtime_error异常
    template <typename... Args>
    CPPEV_PUBLIC void throw_runtime_error(Args... args)
    {
        std::ostringstream oss;
        (oss << ... << args);
        throw std::runtime_error(oss.str());
    }

    // 异常保护
    // 让try-catch块更简洁， 不需要每次都写try-catch
    bool CPPEV_PUBLIC exception_guard(const std::function<void()> &func);

/*
 * 进程级信号处理。
 */

    // 信号处理
    CPPEV_PUBLIC void ignore_signal(int sig);
    // 重置信号处理为默认行为
    CPPEV_PUBLIC void reset_signal(int sig);
    // 定义信号处理函数类型
    CPPEV_PUBLIC void handle_signal(int sig, sig_t handler = [](int) {});
    // 发送信号给指定进程
    CPPEV_PUBLIC void send_signal(pid_t pid, int sig);
    // 检查进程是否存在
    CPPEV_PUBLIC bool check_process(pid_t pid);
    // 检查进程组是否存在
    CPPEV_PUBLIC bool check_process_group(pid_t pgid);

/*
 * 线程级信号处理。
 */
    // 向当前线程发送信号
    CPPEV_PUBLIC void thread_raise_signal(int sig);
    // 阻塞当前线程的指定信号
    CPPEV_PUBLIC void thread_block_signal(int sig);
    // 阻塞当前线程的多个指定信号
    CPPEV_PUBLIC void thread_block_signal(const std::vector<int> &sigs);
    // 解除阻塞当前线程的指定信号
    CPPEV_PUBLIC void thread_unblock_signal(int sig);
    // 解除阻塞当前线程的多个指定信号
    CPPEV_PUBLIC void thread_unblock_signal(const std::vector<int> &sigs);
    // 挂起当前线程，直到接收到指定信号
    CPPEV_PUBLIC void thread_suspend_for_signal(int sig);
    // 挂起当前线程，直到接收到多个指定信号中的任意一个
    CPPEV_PUBLIC void thread_suspend_for_signal(const std::vector<int> &sigs);
    // 等待当前线程接收到指定信号
    CPPEV_PUBLIC void thread_wait_for_signal(int sig);
    // 等待当前线程接收到多个指定信号中的任意一个，返回接收到的信号编号
    CPPEV_PUBLIC int thread_wait_for_signal(const std::vector<int> &sigs);
    // 检查指定信号是否在当前线程的信号掩码中被阻塞
    CPPEV_PUBLIC bool thread_check_signal_mask(int sig);
    // 检查指定信号是否在当前线程的待处理信号集中
    CPPEV_PUBLIC bool thread_check_signal_pending(int sig);

/*
* 字符串处理
*/
    // 使用指定分隔符连接字符串数组
    CPPEV_PUBLIC std::string join(const std::vector<std::string> &str_arr,
                                  const std::string &sep) noexcept;
    // 去除字符串两端的指定字符
    CPPEV_PUBLIC std::string strip(const std::string &str,
                                   const std::string &chars);
    // 去除字符串左端的指定字符
    CPPEV_PUBLIC std::string lstrip(const std::string &str,
                                    const std::string &chars);
    // 去除字符串右端的指定字符
    CPPEV_PUBLIC std::string rstrip(const std::string &str,
                                    const std::string &chars);
    // 使用指定分隔符分割字符串
    CPPEV_PUBLIC std::vector<std::string> split(const std::string &str,
                                            const std::string &sep);
}
    
#endif // CPPEV_UTILS_H