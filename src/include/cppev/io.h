#pragma once
#ifndef CPPEV_ENENT_LOOP_H
#define CPPEV_ENENT_LOOP_H

#include <sys/socket.h>
#include <unistd.h>

#include <climits>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cppev/buffer.h"
#include "cppev/common.h"
#include "cppev/utils.h"

namespace cppev
{
    // 底层IO类
    class IO;
    class stream;
    class sock;
    class sockudp;
    class socktcp;
    // 事件循环类
    class event_loop;

    // 网络协议族
    enum class CPPEV_PUBLIC family
    {
        ipv4,
        ipv6,
        local,
    };

    // 简单工厂模式
    namespace io_factory
    {
        // 主程序和网络系统都会用到IO对象，所以用shared_ptr管理
        CPPEV_PUBLIC std::shared_ptr<socktcp> get_socktcp(family f);
        CPPEV_PUBLIC std::shared_ptr<sockudp> get_sockudp(family f);
        CPPEV_PUBLIC std::vector<std::shared_ptr<stream>> get_pipes();
        CPPEV_PUBLIC std::vector<std::shared_ptr<stream>> get_fifos(const std::string &str);
    }; // namespace io_factory

    class CPPEV_PUBLIC io
    {
    public:
        // block 是否阻塞模式
        explicit io(int fd, bool block = false);

        // 避免拷贝双重持有fd
        io(const io &) = delete;
        io &operator=(const io &) = delete;
        // 移动构造函数 
        io(io &&other) noexcept;
        io &operator=(io &&other) noexcept;

        virtual ~io() noexcept;

        //文件描述
        int fd() const noexcept;

        // 读缓冲区
        const buffer &rbuffer() const noexcept;
        buffer &rbuffer() noexcept;

        // 写缓冲区
        const buffer &wbuffer() const noexcept;
        buffer &wbuffer() noexcept;

        // 查询所属的事件循环
        const event_loop &evlp() const noexcept;
        event_loop &evlp() noexcept;

        // 设置所属的事件循环
        void set_evlp(event_loop *evlp) noexcept;

        // 是否已关闭
        bool is_closed() const noexcept;

        // 关闭IO
        void close() noexcept;

        // 设置为非阻塞模式
        void set_io_nonblock();
        // 设置为阻塞模式
        void set_io_block();

    protected:
        // 文件描述符
        int fd_;
        // 是否阻塞模式
        bool block_;
        // 是否已关闭
        bool closed_;
        // 读缓冲区
        buffer rbuffer_;
        // 写缓冲区
        buffer wbuffer_;
        // 所属的事件循环
        event_loop *evlp_;
        // 移动构造函数实现
        void move(io &&othre) noexcept;    
    };

    // 虚继承避免菱形继承问题
    class CPPEV_PUBLIC sock : public virtual io
    {
        //通过工厂函数创建
        friend std::shared_ptr<socktcp> io_factory::get_socktcp(family);
        friend std::shared_ptr<sockudp> io_factory::get_sockudp(family);

    public:
    
        sock(int fd, family f);
        sock(sock &&other) noexcept;

        // 不能被复制（没有拷贝构造函数），但可以被转移（Move）
        sock &operator=(sock &&other) noexcept;
        // 保证子类可以正确析构
        virtual ~sock();
        
        // sock的协议族
        family sockfamily() const noexcept;

        // bind 单个ipv4 / ipv6
        void bind(const char* p, size_t port);
        // bind 所有ipv4 / ipv6
        void bind(int port);

        // 不走网卡，速度极快，会在磁盘生成文件。
        // 绑定文件路径，remove表示是否删除已存在的文件
        void bind_unix(const char* path, bool remove = false);;
        // 绑定文件路径，remove表示是否删除已存在的文件
        void bind_unix(const std::string& path, bool remove = false);

        // 关掉程序后，无需等待 2 分钟即可立刻重启。
        void set_reuseaddr(bool enable = true);
        // 允许多个线程抢占同一个端口，提升并发性能。
        void get_reuseaddr() const;

        // 设置是否允许端口重用
        bool get_so_reuseport() const;

        // 设置接受缓冲区大小
        // actually set to size*2 in Linux
        // 因为内核也要在这个空间里存一些管理信息（比如这个包裹是谁寄的、寄到哪、有没有损坏）。
        // Linux 为了防止你觉得“怎么我申请了 10 平米只能放 5 平米的货？”，它干脆直接把分配的内存翻倍，保证你实际能用的空间足够大
        void set_so_rcvbuf(int size);
        // 获取接受缓冲区大小
        int get_so_rcvbuf() const;

        // 设置发送缓冲区大小
        // actually set to size*2 in Linux
        void set_so_sndbuf(int size);
        // 获取发送缓冲区大小
        int get_so_sndbuf() const;

        // 设置触发可读事件的低水位标记
        // 为了减少系统频繁通知你的次数，让你一次能把大量数据塞进去，提高效率。
        void set_so_rcvlowat(int size);
        // 获取触发可读事件的低水位标记
        int get_so_rcvlowat() const;

        // 设置触发可写事件的低水位标记
        // Linux 内核比较固执，它通常强制认为：只要有空间（通常是 1 字节），就是可写的。它不太支持你去改这个“发送低水位”的值，或者说改了也没用。
        // 这个函数虽然写在这里是为了代码库的完整性
        void set_so_sndlowat(int size);
        // 获取触发可写事件的低水位标记
        int get_so_sndlowat() const;
    
    protected:
        family family_;
        // 当这个 Socket 对象被销毁（析构）时，系统可能需要根据在这个路径去把那个临时文件删掉，免得下次启动报错“文件已存在”
        std::string unix_path_;
        // move_base 表示是否移动基类 io 的成员变量
        //“移动构造函数”（出生时搬家）和 “移动赋值操作符”（半路搬家）的逻辑有 90% 是重复的。为了不写两遍重复的代码
        // 我们把重复的代码单独封装到一个叫 move 的私有成员函数里
        void move(sock &&other, bool move_base) noexcept;
    };

    enmu class CPPEV_PUBLIC shutdown_mode
    {
        // 关闭读端
        shutdown_rd,
        // 关闭写端
        shutdown_wr,
        // 关闭读写端
        shutdown_rdwr,
    };

    class CPPEV_PUBLIC socktcp final : public sock, public stream
    {
    public:
        socktcp(int sockfd, family f);
        socktcp(socktcp &&other) noexcept;
        socktcp &operator=(socktcp &&other) noexcept;
        ~socktcp();

        // backlog(排队区) 默认值为系统最大值
        void listen(int backlog = SOMAXCONN);

        bool connect(const char* ip, size_t port);
        bool connect(const std::string& ip, size_t port);

        bool connect_unix(const char* path);
        bool connect_unix(const std::string& path);

        // 自己的地址信息
        std::tuple<std::string, int, family> get_sock_name() const;
        // 对端的地址信息
        std::tuple<std::string, int, family> get_peer_name() const;
        // 获取连接目标的uri信息
        std::tuple<std::string, int> get_target_uri() const;

        // 设置 周期性心跳检测
        void set_so_keepalive(bool enable = true);
        // 获取 周期性心跳检测 状态
        bool get_so_keepalive() const;

        // 设置 关闭连接时长
        // enable 是否启用该选项
        // timeout 关闭连接的超时时间，单位秒
        // （false）
        // 你调用 close()，函数立刻返回，你的程序继续往下跑。
        // 操作系统接管剩下的烂摊子：它会在后台默默地把缓冲区里剩下的数据发给对方，发完了再挥手再见（FIN）。

        // （true）
        //你调用 close()。
        //操作系统直接丢弃缓冲区里所有没发完的数据。
        //给对方发一个 RST (Reset) 包，而不是正常的 FIN 包。
        //效果：对方会收到一个报错“Connection Reset by Peer”。

        // （true， 5）
        //  你调用 close()，你的线程会被卡住（阻塞）。
        //  系统尝试把剩余数据发完。
        //结果：
        //如果 5 秒内发完了，close() 成功返回。
        //如果 5 秒到了还没发完，强行断开（同姿势 B）。
        void set_so_linger(bool enable = false, int timeout = 0);

        std::pair<bool, int> get_so_linger() const;

        // 设置 Nagle 算法
        void set_tcp_nodelay(bool disable = true);
        // 获取 Nagle 算法 状态
        bool get_tcp_nodelay() const;
        // 获取 连接错误 状态
        // 这个错误码藏在内核深处，而且读取它有一个副作用：读完一次，内核里的这个错误码就会被清零。所以这相当于是一次性的“拆信封”查看结果。
        int get_so_error() const;


    private:
        // string: IP int: port
        std::tuple<std::string, int> conn_uri_;

        void move(socktcp &&other, bool move_base) noexcept;
    };


    // 发送时：如果你要发 2KB 的数据，但系统缓冲区只剩 1KB 了，剩下的可能就被丢了，或者需要你应用层去处理分片。
    // 接收时：如果对方发来一个 1KB 的包，但你只准备了 512 字节的缓冲区去接，剩下的 512 字节通常会被操作系统直接扔掉（截断），找都找不回来。
    class CPPEV_PUBLIC sockudp final : public sock
    {
    public:
        sockudp(int sockfd, family f);
        sockudp(sockudp &&other) noexcept;
        sockudp &operator=(sockudp &&other) noexcept;
        ~sockudp();

        std::tuple<std::string, int, family> recv() const;
        void send(const char* ip, size_t port);
        void send(const std::string& ip, size_t port);
        void send_unix(const char* path);
        void send_unix(const std::string& path);
        void set_broadcast(bool enable = true);
        bool get_broadcast() const;
    private:
        void move(sockudp &&other, bool move_base) noexcept;
    };
} // namespace cppev

#endif // _cppev_io_h_6C0224787A17_