#include <fcntl.h>

#include <mutex>
#include <thread>
#include <unordered_map>

#include "config.h"
#include "cppev/cppev.h"

class fdcache final
{
public:
    // 存入映射
    void setfd(int conn, int file_descriptor)
    {
        std::unique_lock<std::mutex> lock(lock_);
        hash_[conn] = std::make_shared<cppev::stream>(file_descriptor);
    }

    // 获取映射
    std::shared_ptr<cppev::stream> getfd(int conn)
    {
        std::unique_lock<std::mutex> lock(lock_);
        return hash_[conn];
    }

private:
    std::unordered_map<int, std::shared_ptr<cppev::stream>> hash_;

    std::mutex lock_;
};

cppev::reactor::tcp_event_handler on_connect =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    // 发送请求协议
    iopt->wbuffer().put_string(FILENAME);
    iopt->wbuffer().put_string("\n");
    cppev::reactor::async_write(iopt);
    LOG_INFO << "request file " << FILENAME;

    // 生成本地唯一文件名
    std::stringstream file_copy_name;
    file_copy_name << FILENAME << "." << iopt->fd() << "." << std::showbase
                   << std::hex << std::this_thread::get_id() << ".copy";
    int fd = open(file_copy_name.str().c_str(), O_WRONLY | O_CREAT | O_APPEND,
                  S_IRWXU);
    if (fd < 0)
    {
        cppev::throw_system_error("open error");
    }
    // 关联 Socket 与本地文件
    fdcache *cache =
        reinterpret_cast<fdcache *>(cppev::reactor::external_data(iopt));
    // 当后续 Socket 收到数据（触发 on_read 事件）时，程序可以查这个表，知道要把收到的数据写入哪个本地文件。
    cache->setfd(iopt->fd(), fd);
    LOG_INFO << "create file " << file_copy_name.str();
};

cppev::reactor::tcp_event_handler on_read_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    iopt->read_all();
    // 我们需要用这个 cache 来查表，看看这个 Socket 对应哪个文件
    fdcache *cache =
        reinterpret_cast<fdcache *>(cppev::reactor::external_data(iopt));
    // 查表拿到文件流，然后把收到的数据写入文件
    auto iops = cache->getfd(iopt->fd());
    iops->wbuffer().put_string(iopt->rbuffer().get_string());
    iops->write_all();
    LOG_INFO << "writing chunk to file complete";
};

cppev::reactor::tcp_event_handler on_closed =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{ LOG_INFO << "receiving file complete"; };

int main(int argc, char **argv)
{
    // 告诉操作系统：“如果用户按了 Ctrl+C (发送 SIGINT 信号)，先别急着杀我，也不要打断我的任何子线程
    cppev::thread_block_signal(SIGINT);

    // 初始化核心引擎
    fdcache cache;
    // 创建 TCP 客户端，传入外部数据指针（fdcache）
    // 开启 6 个 IO 工作线程（负责读写数据）
    // 开启 1 个分发线程（在 Client 模式下通常用于处理连接建立）
    cppev::reactor::tcp_client client(6, 1, &cache);

    // 连接通了干嘛？ -> 发文件名，建文件
    client.set_on_connect(on_connect);
    // 有数据了干嘛？ -> 写入磁盘
    client.set_on_read_complete(on_read_complete);
    // 断开了干嘛？ -> 打日志
    client.set_on_closed(on_closed);

    client.add("127.0.0.1", PORT, cppev::family::ipv4, CONCURRENCY);
    client.run();

    // 配合第一行的 thread_block_signal 使用的
    // 一旦你按下 Ctrl+C，主线程苏醒，继续往下执行
    cppev::thread_wait_for_signal(SIGINT);

    // 它会等待所有线程安全退出（join）
    client.shutdown();

    LOG_INFO << "main thread exited";

    return 0;
}
