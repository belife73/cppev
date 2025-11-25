#pragma 
#ifndef _cppev_tcp_h_3F5D2C1A9B4E_
#define _cppev_tcp_h_3F5D2C1A9B4E

// 防止和其他库重复包含
#define CPPEV_PUBLIC __attribute__((visibility("default")))
#define CPPEV_INTERNAL __attribute((visibility("default")))
#define CPPEV_PRIVATE __attribute((visibility("hidden")))

namespace cppev
{
    namespace systemconfig
    {
        // udp缓冲区大小
        CPPEV_PUBLIC extern int udp_buffer_size;
        // 每个epoll / kevent的文件描述符数量
        CPPEV_PUBLIC extern int event_number;

        // stream的读写默认批量大小
        CPPEV_PUBLIC extern int buffer_io_step; 

        // reactor关闭的超时时间，单位毫秒
        CPPEV_PUBLIC extern int reactor_shutdown_timeout; 
    }
}

#endif  // cppev_tcp_h_3F5D2C1A9B4E_