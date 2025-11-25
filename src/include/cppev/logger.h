#pragma
#ifndef _LOGGER_H_
#define _LOGGER_H_

// 添加时间戳（毫秒级）和线程 ID 信息，确保日志的上下文完整性。
#include <chrono>
#include <thread>

// 供 log_message 类用于处理 可变参数列表 (va_list)，以支持 printf 风格的格式化日志。
#include <cstdarg>

// 用于构建流式日志内容 (message_buffer_) 和控制时间戳的格式。
#include <sstream>
#include <iomanip>

// std::mutex 确保线程安全；std::unordered_map 用于管理多日志级别/多输出目标。
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>

// 包含库的通用定义，特别是 CPPEV_PUBLIC 和 CPPEV_INTERNAL 宏（用于控制符号可见性，提高编译和链接效率）。
#include "cppev/common.h"

namespace cppev {
// 定义日志颜色
// ANSI 转义序列说明
// \033[：ANSI 转义序列开始
// 0m：重置所有属性
// 34m：设置前景色为蓝色
#define RESET_COLOR "\033[0m"     // 默认颜色 重置颜色，确保后续输出不受影响
#define DEBUG_COLOR "\033[34m"    // 亮蓝 DEBUG
#define INFO_COLOR "\033[32m"     // 绿色 用于 INFO 消息
#define WARNING_COLOR "\033[33m"  // 黄色 用于 WARNING 消息
#define ERROR_COLOR "\033[31m"    // 红色 用于 ERROR 消息
#define FATAL_COLOR "\033[35m"    // 紫色 用于 FATAL 消息

// 日志级别枚举 公开接口
enum class CPPEV_PUBLIC log_level {
    debug = 1 << 0,     // 调试级别，详细信息，通常用于开发和调试阶段
    info = 1 << 1,      // 一般信息，记录程序运行的正常操作
    warning = 1 << 2,   // 警告信息，表示可能的问题或非预期的情况
    error = 1 << 3,     // 错误信息，表示程序运行中出现的问题，但不致命
    fatal = 1 << 4,     // 致命错误，表示严重问题，通常会导致程序终止
};


// 单例日志记录器类 公开接口
class CPPEV_PUBLIC logger
{
public:
    // 获取单例实例
    static logger &get_instance();

    // 获取当前日志级别
    log_level get_log_level() const;
    // 设置日志级别
    void set_log_level(log_level level);

    // 输出流添加到所有日志级别
    void add_output_stream(std::ostream &output);
    // 输出流添加到指定日志级别
    void add_output_stream(log_level level, std::ostream &output);

    // 核心日志写入方法
    void write_log(log_level level, const std::string &file, int line,
                   const std::string &message); 

private:
// 私有析构函数
    logger();
    ~logger();

    // 私有构造函数，防止外部实例化
    std::string level_to_string(log_level level) const;
    // 为日志添加颜色
    void add_color(std::ostream &os, log_level level);
    // 添加时间戳（毫秒级）
    void add_timestamp(std::ostream &os);
    // 添加线程 ID
    void add_thread_id(std::ostream &os);
    // 重置颜色
    void reset_color(std::ostream &os);


    // 当前日志级别
    log_level current_level_;
    // 互斥锁，确保线程安全
    std::mutex mutex_;
    // 日志级别到输出流的映射
    std::unordered_map<log_level, std::vector<std::ostream *>> output_streams_;
};

// 日志消息辅助类 内部使用
class CPPEV_INTERNAL log_message
{
public:
    // 用于流式日志 (LOG_INFO << "...") 的构造。它初始化日志级别、文件和行号信息。
    log_message(log_level level, const char *file, int line);
    // log_message 临时对象生命周期结束时（即日志语句末尾的分号 ; 处），它自动被调用，并负责调用
    log_message(log_level level, const char *file, int line, const char *format,
                ...);
    // 提供对内部 message_buffer_ 的引用，允许用户使用 << 运算符向其写入数据。
    ~log_message();

    // 访问日志消息流
    std::ostringstream &stream();
private:
    // 使用可变参数列表格式化消息
    void format_message(const char *format, va_list args);
    
    // 日志级别
    log_level message_level_;

    // 源文件名
    const char *source_file_;

    // 行号
    int line_number_;

    // 消息缓冲区
    std::ostringstream message_buffer_;
};
} // namespace cppev


// 日志宏定义 公开接口
#define LOG_BASE(level)                                        \           
    if (level < cppev::logger::get_instance().get_log_level()) \
        ;                                                      \
    else                                                       \
        cppev::log_message(level, __FILE__, __LINE__).stream() // .stream流式接口


// 格式化写法 公开接口
#define LOG_FMT_BASE(level, format, ...)                       \
    if (level < cppev::logger::get_instance().get_log_level()) \
        ;                                                      \
    else                                                       \
        cppev::log_message(level, __FILE__, __LINE__, format, ##__VA_ARGS__)

// 具体日志级别宏定义 
#define LOG_DEBUG LOG_BASE(cppev::log_level::debug)
#define LOG_INFO LOG_BASE(cppev::log_level::info)
#define LOG_WARNING LOG_BASE(cppev::log_level::warning)
#define LOG_ERROR LOG_BASE(cppev::log_level::error)
#define LOG_FATAL LOG_BASE(cppev::log_level::fatal)

// 格式化写法具体日志级别宏定义 
#define LOG_DEBUG_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::debug, format, ##__VA_ARGS__)
#define LOG_INFO_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::info, format, ##__VA_ARGS__)
#define LOG_WARNING_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::warning, format, ##__VA_ARGS__)
#define LOG_ERROR_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::error, format, ##__VA_ARGS__)
#define LOG_FATAL_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::fatal, format, ##__VA_ARGS__)
#endif // _LOGGER_H_