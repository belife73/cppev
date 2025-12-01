#ifndef _cppev_file_transfer_config_h_6C0224787A17_
#define _cppev_file_transfer_config_h_6C0224787A17_

// 定义 TCP 连接的端口号
const int PORT = 8891;

// 定义每次读写文件或发送网络包的缓冲区大小
const int CHUNK_SIZE = 10 * 1024 * 1024;  // 10MB

// 指定要传输的文件的绝对路径
// /tmp/ 是 Linux 的临时目录，重启会清空，适合存放测试生成的垃圾文件
const char *FILENAME = "/tmp/test_cppev_file_transfer_6C0224787A17.file";

// 定义并发连接的数量
const int CONCURRENCY = 10;

#endif  // config.h
