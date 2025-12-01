- # 示例教程

  

  

  ### 1. TCP 压力测试

  

  TCP 服务端启动监听线程，以支持 IPv4 / IPv6 / Unix 协议族。

  TCP 客户端启动连接线程，以连接至服务端。

  服务端和客户端均初始化线程池来处理 TCP 连接。

  - **用法**

    Bash

    ```
    $ cd examples/tcp_stress
    $ ./simple_server       # 终端窗口-1
    $ ./simple_client       # 终端窗口-2
    ```

  

  ### 2. 大文件传输

  

  TCP 客户端发送请求的文件名。

  TCP 服务端缓存并传输所请求的文件。

  TCP 客户端接收文件并将其存储到磁盘。

  - **用法**

    Bash

    ```
    $ cd example/file_transfer
    $ touch /tmp/test_cppev_file_transfer_6C0224787A17.file
    $ # 向文件中写入数据使其变大，例如 20MB 或更大 #
    $ ./file_transfer_server        # 终端窗口-1
    $ ./file_transfer_client        # 终端窗口-2
    $ openssl md5 /tmp/test_cppev_file_transfer_6C0224787A17.file*
    ```

  

  ### 3. IO 事件循环

  

  使用原生事件循环通过 TCP / UDP 进行连接。

  - **用法**

    Bash

    ```
    $ cd examples/io_evlp
    $ ./tcp_server      # 终端窗口-1
    $ ./tcp_client      # 终端窗口-2
    $ ./udp_server      # 终端窗口-3
    $ ./udp_client      # 终端窗口-4
    ```
