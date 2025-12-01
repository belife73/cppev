#include <fcntl.h>

#include <thread>
#include <unordered_map>

#include "config.h"
#include "cppev/logger.h"
#include "cppev/tcp.h"

class filecache final
{
public:
    // 懒加载文件内容到内存缓冲区
    // 先找在加载
    const cppev::buffer *lazyload(const std::string &filename)
    {
        std::unique_lock<std::mutex> lock(lock_);
        if (hash_.count(filename) != 0)
        {
            return &(hash_[filename]->rbuffer());
        }
        LOG_INFO << "start loading file";
        int fd = open(filename.c_str(), O_RDONLY);
        // 找个专门的管理员（stream对象）管这个文件
        std::shared_ptr<cppev::stream> iops =
            std::make_shared<cppev::stream>(fd);
        iops->read_all(CHUNK_SIZE);
        close(fd);
        hash_[filename] = iops;
        LOG_INFO << "finish loading file";
        return &(iops->rbuffer());
    }

private:
    std::mutex lock_;

    std::unordered_map<std::string, std::shared_ptr<cppev::stream>> hash_;
};

cppev::reactor::tcp_event_handler on_read_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    LOG_INFO << "start callback : on_read_complete";
    // 尝试从读缓冲区获取所有数据转为字符串
    std::string filename = iopt->rbuffer().get_string(-1, false);、
    // 如果没有收到 \n，说明命令还没传完。函数直接 return，不做处理，等待下一次数据到达触发回调（届时缓冲区会有更多数据）。
    if (filename[filename.size() - 1] != '\n')
    {
        return;
    }
    // 清空读缓冲区，准备接收下一次数据
    iopt->rbuffer().clear();
    filename = filename.substr(0, filename.size() - 1);
    LOG_INFO << "client request file : " << filename;

    // 获取文件内容 (缓存机制)
    const cppev::buffer *bf =
        reinterpret_cast<filecache *>(cppev::reactor::external_data(iopt))
            ->lazyload(filename);

    // 把文件内容写入写缓冲区，准备发送给客户端
    iopt->wbuffer().put_string(bf->data(), bf->size());
    cppev::reactor::async_write(iopt);
    LOG_INFO << "end callback : on_read_complete";
};

cppev::reactor::tcp_event_handler on_write_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    LOG_INFO << "start callback : on_write_complete";
    cppev::reactor::safely_close(iopt);
    LOG_INFO << "end callback : on_write_complete";
};

int main()
{
    cppev::thread_block_signal(SIGINT);

    filecache cache;
    cppev::reactor::tcp_server server(3, false, &cache);
    server.set_on_read_complete(on_read_complete);
    server.set_on_write_complete(on_write_complete);
    server.listen(PORT, cppev::family::ipv4);
    server.run();

    cppev::thread_wait_for_signal(SIGINT);

    server.shutdown();

    LOG_INFO << "main thread exited";

    return 0;
}
