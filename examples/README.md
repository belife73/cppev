示例教程
1. TCP 压力测试
TCP 服务器启动监听线程，以支持 IPv4 / IPv6 / Unix 域套接字协议族。

TCP 客户端启动连接线程以连接到服务器。

服务器和客户端均启动线程池来处理 TCP 连接。

用法

Bash

$ cd examples/tcp_stress
$ ./simple_server       # Shell-1 (终端窗口 1)
$ ./simple_client       # Shell-2 (终端窗口 2)
2. 大文件传输
TCP 客户端发送所需文件的名称。

TCP 服务器缓存并传输所需的文件。

TCP 客户端接收文件并将其存储到磁盘。

用法

Bash

$ cd example/file_transfer
$ touch /tmp/test_cppev_file_transfer_6C0224787A17.file
$ # 向文件写入数据使其变大，例如 20MB 或更多 #
$ ./file_transfer_server        # Shell-1 (终端窗口 1)
$ ./file_transfer_client        # Shell-2 (终端窗口 2)
$ openssl md5 /tmp/test_cppev_file_transfer_6C0224787A17.file*
3. IO 事件循环
使用原始事件循环通过 TCP / UDP 进行连接。

用法

Bash

$ cd examples/io_evlp
$ ./tcp_server      # Shell-1 (终端窗口 1)
$ ./tcp_client      # Shell-2 (终端窗口 2)
$ ./udp_server      # Shell-3 (终端窗口 3)
$ ./udp_client      # Shell-4 (终端窗口 4)