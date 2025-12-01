#ifndef _cppev_event_loop_h_6C0224787A17_
#define _cppev_event_loop_h_6C0224787A17_

#include <unistd.h>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <tuple>
#include <unordered_map>

#include "cppev/common.h"
#include "cppev/io.h"
#include "cppev/logger.h"
#include "cppev/utils.h"

namespace cppev
{
// 这是一个位掩码（Bitmask），允许通过位运算（|, &）同时监听读和写事件
enum class CPPEV_PUBLIC fd_event
{
    fd_readable = 1 << 0,
    fd_writable = 1 << 1,
};

// Level Trigger (LT): 只要缓冲区有数据，就会一直触发事件。编程简单，不易丢数据。
// Edge Trigger (ET): 只有状态变化（从无数据到有数据）时触发一次。性能更高
// 但要求程序员必须一次性把数据读完（read_all），否则会卡死。特别建议配合 io::read_all 使用。
enum class CPPEV_PUBLIC fd_event_mode
{
    level_trigger = 1 << 0, // 水平触发 (LT) - 默认
    edge_trigger = 1 << 1,  // 边缘触发 (ET) - 高性能
    oneshot = 1 << 2,       // 单次触发 (oneshot)
};

// 在 C++ 中，enum class（强类型枚举）默认是不支持位运算的。
// 你不能直接写 fd_readable | fd_writable，编译器会报错，因为它们不是整数。
// 但是，在网络编程中，我们经常需要同时监听读和写（例如：EPOLLIN | EPOLLOUT）。
CPPEV_PRIVATE fd_event operator&(fd_event lhs, fd_event rhs);

CPPEV_PRIVATE fd_event operator|(fd_event lhs, fd_event rhs);

CPPEV_PRIVATE fd_event operator^(fd_event lhs, fd_event rhs);

CPPEV_PRIVATE void operator&=(fd_event &lhs, fd_event rhs);

CPPEV_PRIVATE void operator|=(fd_event &lhs, fd_event rhs);

CPPEV_PRIVATE void operator^=(fd_event &lhs, fd_event rhs);

// 功能：定义了一个哈希表，将 fd_event 枚举值映射为人类可读的字符串（const char *）。
// 用途：主要用于日志记录 (Logging) 和 调试。
// 当程序打印日志时，它不会打印冷冰冰的数字（比如 3），而是打印 "READABLE | WRITABLE"，方便开发者快速排查问题。
CPPEV_PRIVATE extern const std::unordered_map<fd_event, const char *>
    fd_event_to_string;

// 回调函数定义：当某个文件描述符（fd）上的事件被触发时，
// 这个函数类型的回调函数会被调用。
// 它接受一个指向 io 对象的智能指针作为参数，
// 允许用户在事件发生时执行自定义的逻辑（比如读取数据、写入数据等）。
using fd_event_handler = std::function<void(const std::shared_ptr<io> &)>;

struct CPPEV_PRIVATE fd_event_hash
{
    std::size_t operator()(const std::tuple<int, fd_event> &ev) const noexcept;
};

namespace cppev
{

class CPPEV_PUBLIC event_loop
{
public:
    explicit event_loop(void *data = nullptr, void *owner = nullptr);

    // 禁止拷贝和移动构造与赋值
    event_loop(const event_loop &) = delete;
    event_loop &operator=(const event_loop &) = delete;
    event_loop(event_loop &&) = delete;
    event_loop &operator=(event_loop &&) = delete;

    virtual ~event_loop() noexcept;

    // 获取 Event Loop 的外部自定义数据指针。
    void *data() noexcept;

    // 获取 Event Loop 的外部自定义数据指针（const）。
    const void *data() const noexcept;

    // 获取拥有此 Event Loop 的外部对象指针。
    void *owner() noexcept;

    // 获取拥有此 Event Loop 的外部对象指针（const）。
    const void *owner() const noexcept;

    // 获取当前 Event Loop 监控的文件描述符（FD）负载数量。
    int ev_loads() const noexcept;

    // 设置 FD 的事件触发模式（如 LT/ET），应在激活前调用，否则将使用默认模式。
    // 注意：不要尝试为同一个 FD 的不同事件设置不同的模式。
    //       epoll 和 kqueue 对此的处理方式不同（epoll 通常要求同一 FD 模式一致）。
    // 提示：如果用户想要原子性地设置模式并激活，需要自行通过互斥锁实现。
    // @param iop       IO 智能指针。
    // @param ev_mode   事件模式 (Level Trigger / Edge Trigger)。
    void fd_set_mode(const std::shared_ptr<io> &iop, fd_event_mode ev_mode);

    // 将 FD 事件注册到用户态的事件轮询器中，但在系统 IO 多路复用层（内核态）暂不激活。
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型 (读/写)。
    // @param handler   FD 事件回调函数。
    // @param prio      事件优先级。
    void fd_register(const std::shared_ptr<io> &iop, fd_event ev_type,
                     const fd_event_handler &handler = fd_event_handler(),
                     priority prio = priority::p0);

    // 激活 FD 事件（调用系统 API 开始监听）。
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型。
    void fd_activate(const std::shared_ptr<io> &iop, fd_event ev_type);

    // 注册 FD 事件到事件轮询器，并立即在系统 IO 多路复用层激活它。
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型。
    // @param handler   FD 事件回调函数。
    // @param prio      事件优先级。
    void fd_register_and_activate(
        const std::shared_ptr<io> &iop, fd_event ev_type,
        const fd_event_handler &handler = fd_event_handler(),
        priority prio = priority::p0);

    // 从用户态事件轮询器中移除 FD 事件，但不从系统 IO 多路复用层取消激活。
    // （通常用于逻辑删除，实际内核监听可能还在，需谨慎使用）
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型。
    void fd_remove(const std::shared_ptr<io> &iop, fd_event ev_type);

    // 取消激活 FD 事件（通知内核停止监听该事件）。
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型。
    void fd_deactivate(const std::shared_ptr<io> &iop, fd_event ev_type);

    // 从事件轮询器中移除 FD 事件，并在系统 IO 多路复用层取消激活（彻底移除）。
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型。
    void fd_remove_and_deactivate(const std::shared_ptr<io> &iop,
                                  fd_event ev_type);

    // 删除并取消激活该 FD 的所有事件，清理所有相关数据。
    // @param iop       IO 智能指针。
    void fd_clean(const std::shared_ptr<io> &iop);

    // 等待事件，只循环一次。
    // @param timeout   超时时间（毫秒），-1 表示无限等待。
    void loop_once(int timeout = -1);

    // 等待事件，无限循环。
    // @param timeout   每次 wait 的超时时间（毫秒），-1 表示无限等待。
    void loop_forever(int timeout = -1);

    // 停止循环。
    void stop_loop();

    // 带超时地停止循环。
    // @param timeout   超时时间（毫秒）。
    // @return          true: 其他线程的循环已成功停止；false: 超时。
    bool stop_loop(int timeout);

private:
    // 辅助函数：将 FD 事件注册到事件轮询器（非线程安全版本 / NTS）。
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型。
    // @param handler   FD 事件回调函数。
    // @param prio      事件优先级。
    void fd_register_nts(const std::shared_ptr<io> &iop, fd_event ev_type,
                         const fd_event_handler &handler, priority prio);

    // 辅助函数：从事件轮询器中移除 FD 事件（非线程安全版本）。
    // @param iop       IO 智能指针。
    void fd_remove_nts(const std::shared_ptr<io> &iop, fd_event ev_type);

    // 辅助函数：创建 IO 多路复用文件描述符（如 epoll_create）。
    // 具体实现取决于平台（Linux/macOS）。
    void fd_io_multiplexing_create_nts();

    // 辅助函数：添加 FD 事件监听（调用 epoll_ctl ADD/MOD）。
    // 具体实现取决于平台。
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型。
    void fd_io_multiplexing_add_nts(const std::shared_ptr<io> &iop,
                                    fd_event ev_type);

    // 辅助函数：删除 FD 事件监听（调用 epoll_ctl DEL）。
    // 具体实现取决于平台。
    // @param iop       IO 智能指针。
    // @param ev_type   事件类型。
    void fd_io_multiplexing_del_nts(const std::shared_ptr<io> &iop,
                                    fd_event ev_type);

    // 辅助函数：等待事件触发（调用 epoll_wait / kevent）。
    // 具体实现取决于平台。
    // @param timeout   超时时间（毫秒），-1 表示无限等待。
    // @return          触发了事件的 FD 列表，同一个 FD 的不同事件是分开存储的。
    std::vector<std::tuple<int, fd_event>> fd_io_multiplexing_wait_ts(
        int timeout);

    using waiter_type = std::function<bool(std::unique_lock<std::mutex> &)>;

    // 辅助函数：停止事件循环（线程安全 / TS）。
    // @param waiter    等待函数。
    // @return          true: 其他线程的循环已停止；false: 超时。
    bool stop_loop_ts_wl(waiter_type waiter);

    // 保护内部数据结构，以保证 "注册 / 移除 / 循环" 操作的线程安全。
    std::mutex lock_;

    // 用于停止循环时的线程同步。
    // 虽然可以使用阻塞 IO 来实现，但作者曾在 OSX 上目睹读取阻塞 IO 导致 CPU 100% 占用，
    // 因此这里使用条件变量。
    std::condition_variable cond_;

    // 事件监听器 FD (如 epoll fd 或 kqueue fd)。
    int ev_fd_;

    // Event Loop 的外部自定义数据指针。
    void *data_;

    // 拥有此 Event Loop 的外部对象指针。
    void *owner_;

    // 哈希表：(fd, event) --> (优先级, IO对象, 回调函数)。
    std::unordered_map<std::tuple<int, fd_event>,
                       std::tuple<priority, std::shared_ptr<io>,
                                  std::shared_ptr<fd_event_handler>>,
                       fd_event_hash>
        fd_event_datas_;

    // 哈希表：fd --> fd_event (当前该 FD 注册了哪些事件掩码)。
    std::unordered_map<int, fd_event> fd_event_masks_;

    // 哈希表：fd --> fd_event_mode (事件模式)。
    // 根据系统 API，epoll 要求同一个 FD 必须使用相同的事件模式，kqueue 似乎没有此限制。
    std::unordered_map<int, fd_event_mode> fd_event_modes_;

    // 循环是否应该停止的标志位。
    bool stop_;

    // 默认的 FD 事件模式。
    static const fd_event_mode fd_event_mode_default_;
};

}  // namespace cppev

#endif  // event_loop.h
