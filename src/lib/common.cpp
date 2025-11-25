#include "cppev/common.h"

namespace cppev
{
    namespace sysconfig
    {
        // 缓存池大小
        int udp_buffer_size = 1500;

        // 每个epoll / kevent的文件描述符数量
        int event_number = 2048;

        // stream的读写默认批量大小
        int buffer_io_step = 1024;

        // reactor关闭的超时时间，单位毫秒
        int reactor_shutdown_timeout = 5000;
    } // namespace sysconfig
} // namespace cppev