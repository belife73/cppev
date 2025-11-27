#include "cppev/utils.h"

// ​进程调度相关（系统调用）
#include <sched.h>

#include <cstring>
#include <exception>
#include <string>
#include <system_error>
#include <numeric>

namespace cppev
{
    // 输入的时间戳t。如果为负数，则自动获取当前系统时间。
    // 格式化字符串format，默认为"%F %T %Z"（即"年-月-日 时:分:秒 时区"格式）。
    std::string timestamp(time_t t, const char *format)
    {
        // 编译期断言time_t 类型是否是有符号的（signed）。
        static_assert(std::is_signed<time_t>::value, "time_t is not signed!");
        // 一个负数的 time_t 值通常表示时间在 Unix 纪元（1970年1月1日）之前
        if (t < 0)
        {
            // 获取当前的日历时间
            t = time(nullptr);
            if (t == -1)
            {
                throw_system_error("time error");
            }
        }
        //判断: 检查是否提供了自定义的格式字符串（format 是否为空指针）。
        //默认值: 如果没有提供，则使用默认格式字符串 "%F %T %Z"。
        if (format == nullptr)
        {
            format = "%F %T %Z";
        }
        // 声明一个 struct tm 结构体，用于保存日期和时间的各个组成部分（如年份、月份、小时等）。
        tm s_tm;
        // 将 time_t 类型的原始时间值（通常是自 Unix 纪元以来的秒数）转换为本地时区的日历时间，并将结果填充到 s_tm 结构体中。
        localtime_r(&t, &s_tm);
        char buf[1024];
        memset(buf, 0, 1024);
        // 该函数尝试将日历时间结构体 s_tm 按照 format 格式化后，写入到大小为 1024 的缓冲区 buf 中。
        if (0 == strftime(buf, 1024, format, &s_tm))
        {
            throw_system_error("strftime error");
        }
        return buf;
    }

    // 计算最小公倍数
    int64_t least_common_multiple(int64_t p, int64_t r)
    {
        assert(p != 0 && r != 0);
        return (p / greatest_common_divisor(p, r)) * r;
        // C++17 标准库实现
        // return std::lcm(p, r);
    }
    // 计算最小公倍数
    int64_t least_common_multiple(const std::vector<int64_t> &nums)
    {
        assert(nums.size() >= 2);
        int64_t prev = nums[0];
        for (size_t i = 1; i < nums.size(); ++i)
        {
            prev = least_common_multiple(prev, nums[i]);
        }
        return prev;
    }

    int64_t greatest_common_divisor(int64_t p, int64_t r)
    {
        assert(p != 0 && r != 0);
        int64_t remain;
        while (true)
        {
            remain = p % r;
            p = r;
            r = remain;
            if (remain == 0)
            {
                break;
            }
        }
        return p;

        // C++17 标准库实现
        // return std::gcd(p, r);
    }

    // 计算最大公约数
    int64_t greatest_common_divisor(const std::vector<int64_t> &nums)
    {
        assert(nums.size() >= 2);
        int64_t prev = nums[0];
        for (size_t i = 1; i < nums.size(); ++i)
        {
            prev = greatest_common_divisor(prev, nums[i]);
        }
        return prev;
    }

    // 异常保护
    bool exception_guard(const std::function<void()> &func)
    {
        try
        {
            func();
            return true;
        }
        catch(const std::exception &e)
        {
            return false;
        }
    }

    void ignore_signal(int sig)
    {
        // 设置信号处理函数为忽略该信号
        handle_signal(sig, SIG_IGN);
    }

    // 撤销所有先前对信号 sig 的自定义处理，并将其恢复为默认行为
    void reset_signal(int sig)
    {
        handle_signal(sig, SIG_DFL);
    }

    void handle_signal(int sig, sig_t handler)
    {
        struct sigaction sigact;
        memset(&sigact, 0, sizeof(sigact));
        sigact.sa_handler = handler;
        if (sigaction(sig, &sigact, nullptr) == -1)
        {
            throw_system_error("sigaction error");
        }
    }

    void send_signal(pid_t pid, int sig)
    {
        if (sig == 0)
        {
            throw_logic_error("pid or pgid check is not supported");
        }
        if (kill(pid, sig) != 0)
        {

        }
    }

    bool check_process(pid_t pid)
    {
        // kill 函数的第二个参数（信号编号）设置为 0 时，系统不会发送任何信号，而是只执行错误检查
        // 成功返回 0: 如果 kill(pid, 0) 返回 0，则表示：
        // 目标进程（或进程组）存在。
        // 调用进程有权限向其发送信号。
        if (kill(pid, 0) == 0)
        {
            return true;
        }
        else
        {
            if (errno == ESRCH)
            {
                return false;
            }
            throw_system_error("kill error");
        }

        return true;
    }

    // 发送信号
    void thread_raise_signal(int sig)
    {
        if (raise(sig) != 0)
        {
            throw_system_error("raise error");
        }
    }

    //线程主动等待信号sig的到来
    void thread_wait_for_signal(int sig)
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);
        int ret_sig;
        // sigwait(&set, &ret_sig)：这是核心函数，它会阻塞当前线程，直到集合set中的任何一个信号到达
        // 当信号到达时，sigwait会将实际的信号值存入ret_sig变量
        //  在 Linux 系统上，sigwait函数的第二个参数（即存储接收到信号编号的整型指针）必须是一个非空的有效指针。
        if (sigwait(&set, &ret_sig) != 0)
        {
            throw_system_error("sigwait error");
        }
    }

    int thread_wait_for_signal(const std::vector<int> &sigs)
    {
        sigset_t set;
        sigemptyset(&set);
        for (auto sig : sigs)
        {
            sigaddset(&set, sig);
        }

        int ret_sig;
        if (sigwait(&set, &ret_sig) != 0)
        {
            throw_system_error("sigwait error");
        }
        return ret_sig;
    }
    
    void thread_suspend_for_signal(int sig)
    {
        // 创建一个包含除目标信号外所有信号的屏蔽集
        sigset_t set;
        // 创建一个包含了所有已知信号的集合 set
        sigfillset(&set);
        sigdelset(&set, sig);
        // 原子操作
        // 临时替换信号屏蔽字：将当前线程的信号屏蔽字替换为参数 set指定的信号集
        //进入休眠：线程挂起，等待信号到来
        //信号处理：只有指定的信号 sig能唤醒线程（因为其他信号都被屏蔽了）
        //恢复原状：信号处理完成后，恢复原来的信号屏蔽字
        //函数返回：sigsuspend返回 -1（如注释所示）
        sigsuspend(&set);
    }

    void thread_block_signal(int sig)
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);

        // 阻塞指定信号
        // nullptr 表示不需要保存之前的信号屏蔽字
        int ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
        if (ret != 0)
        {
            throw_system_error("pthread_sigmask error", ret);
        }
    }

    void thread_block_signal(const std::vector<int> &sigs)
    {
        sigset_t set;
        sigemptyset(&set);
        for (auto sig : sigs)
        {
            sigaddset(&set, sig);
        }
        int ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
        if (ret != 0)
        {
            throw_system_error("pthread_sigmask error", ret);
        }
    }

    void thread_unblock_signal(int sig)
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);
        int ret = pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
        if (ret != 0)
        {
            throw_system_error("pthread_sigmask error", ret);
        }
    }

    void thread_unblock_signal(const std::vector<int> &sigs)
    {
        sigset_t set;
        sigemptyset(&set);
        for (auto sig : sigs)
        {
            sigaddset(&set, sig);
        }
        int ret = pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
        if (ret != 0)
        {
            throw_system_error("pthread_sigmask error", ret);
        }
    }

    bool thread_check_signal_mask(int sig)
    {
        sigset_t set;
        sigemptyset(&set);

        // 获取当前线程的信号屏蔽字
        int ret = pthread_sigmask(SIG_SETMASK, nullptr, &set);
        if (ret != 0)
        {
            throw_system_error("pthread_sigmask error", ret);
        }
        return sigismember(&set, sig) == 1;
    }

    bool thread_check_signal_pending(int sig)
    {
        sigset_t set;
        sigemptyset(&set);
        // 将所有待处理的信号存入 set 集合中
        if (sigpending(&set) != 0)
        {
            throw_system_error("sigpending error");
        }
        return sigismember(&set, sig) == 1;
    }

    // std::vector<std::string> split(const std::string &str, const std::string &sep)
    // {
    //     if (sep.empty())
    //     {
    //         throw_runtime_error("cannot split string with empty seperator");
    //     }

    //     if (sep.size() > str.size())
    //     {
    //         return {str};
    //     }

    //     std::vector<int> sep_index;

    //     sep_index.push_back(0 - sep.size());

    //     for (size_t i = 0; i <= str.size() - sep.size();)
    //     {
    //         bool is_sep = true;
    //         for (size_t j = i; j < i + sep.size(); ++j)
    //         {
    //             if (str[j] != sep[j - i])
    //             {
    //                 is_sep = false;
    //                 break;
    //             }
    //         }
    //         if (is_sep)
    //         {
    //             sep_index.push_back(static_cast<int>(i));
    //             i += sep.size();
    //         }
    //         else
    //         {
    //             ++i;
    //         }
    //     }

    //     sep_index.push_back(str.size());

    //     if (sep_index.size() == 2)
    //     {
    //         return {str};
    //     }

    //     std::vector<std::string> substrs;

    //     for (size_t i = 1; i < sep_index.size(); ++i)
    //     {
    //         int begin = sep_index[i - 1] + sep.size();
    //         int end = sep_index[i];

    //         substrs.push_back(std::string(str, begin, end - begin));
    //     }
    //     return substrs;
    // }

    // 更加简洁高效的 split 实现
    // 插入分隔符sep，将字符串str拆分成多个子字符串，并返回这些子字符串的集合
    std::vector<std::string> split(const std::string &str, const std::string &sep)
    {
        if (sep.empty()) {
            // 使用标准异常抛出
            throw std::runtime_error("cannot split string with empty separator");
        }

        std::vector<std::string> substrs;

        // 预留一部分空间，避免 vector 频繁扩容（可选优化）
        // 假设平均每10个字符有一个分隔符，这是一个经验值，不加也没关系
        // substrs.reserve(str.size() / 10); 

        std::string::size_type start = 0;
        std::string::size_type end = str.find(sep);

        while (end != std::string::npos) // npos 表示“没找到”
        {
            // 截取当前段并放入结果
            substrs.emplace_back(str.substr(start, end - start));

            // 更新起点，跳过分隔符
            start = end + sep.size();

            // 继续查找下一个分隔符
            end = str.find(sep, start);
        }

        // 处理最后一个分隔符后面的剩余部分
        // 例如 "a,b,c" -> 处理完 "b" 后，start 指向 "c"，循环结束，这里把 "c" 加入
        // 例如 "a,b,"  -> 处理完 "b" 后，start 指向末尾，这里加入空字符串 ""
        substrs.emplace_back(str.substr(start));

        return substrs;
    }

    // 更加简洁高效的 join 实现
    std::string join(const std::vector<std::string> &str_arr, const std::string &seq)
    {
        if (str_arr.empty()) {return "";}

        size_t total_size = std::accumulate(
            str_arr.begin(), str_arr.end(), 0ULL,
            [](size_t sum, const std::string &s) 
            { return sum + s.size(); });

        total_size += seq.size() * (str_arr.size() - 1);
        std::string result;
        result.reserve(total_size);

        for (size_t i = 0; i < str_arr.size(); ++i)
        {
            result.append(str_arr[i]);
            if (i != str_arr.size() - 1)
            {
                result.append(seq);
            }
        }

        return result;
    }

    // 去除字符串左端端的指定字符
    static constexpr int STRIP_LEFT = 0x01;
    // 去除字符串右端的指定字符
    static constexpr int STRIP_RIGHT = 0x10;

    std::string do_strip(const std::string &str, const std::string & chars, const int type)
    {
        if (chars.empty())
        {
            throw_runtime_error("cannot strip string with empty chars");
        }

        std::string::size_type p = 0;
        std::string::size_type r = str.size();
        const size_t seq_len = chars.size();

        if (type & STRIP_LEFT)
        {
            while (r - p >= seq_len)
            {
                if (str.compare(p, seq_len, chars) == 0)
                {
                    p += seq_len;
                }
                else
                {
                    break;
                }
            }
        }

        if (type & STRIP_RIGHT)
        {
            while (r - p >= seq_len)
            {
                if (str.compare(r - seq_len, seq_len, chars) == 0)
                {
                    r -= seq_len;
                }
                else
                {
                    break;
                }
            }
        }
        return str.substr(p, r - p);
    }

    std::string strip(const std::string &str, const std::string &chars)
    {
        return do_strip(str, chars, STRIP_LEFT | STRIP_RIGHT);
    }

    std::string lstrip(const std::string &str, const std::string &chars)
    {
        return do_strip(str, chars, STRIP_LEFT);
    }

    std::string rstrip(const std::string &str, const std::string &chars)
    {
        return do_strip(str, chars, STRIP_RIGHT);
    }
} // namespace cppev