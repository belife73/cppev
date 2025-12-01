# Cppev

Cppev 是一个高性能的 C++ 异步 I/O、多线程及多进程库。

## 架构 (Architecture)

### I/O

- **支持多种 I/O 操作**：包括磁盘文件、管道 (pipe)、命名管道 (fifo) 以及套接字 (socket)。
- **套接字协议类型支持**：TCP / UDP。
- **协议族支持**：IPv4 / IPv6 / Unix 域 (unix-domain)。
- **支持 I/O 事件监听**：基于 I/O 多路复用技术。
  - **事件类型**：可读 (readable) / 可写 (writable)。
  - **事件模式**：水平触发 (level-trigger) / 边缘触发 (edge-trigger) / 单次触发 (oneshot)。

### 多线程 (Multithreading)

- 支持子线程 (subthread) 和线程池 (threadpool)。

### 进程间通信 (Interprocess Communication)

- 支持信号量 (semaphore) 和共享内存 (shared-memory)。

### 线程与进程同步 (Thread and Process Synchronization)

- 支持线程/进程级别的信号处理 (signal-handling)、互斥锁 (mutex)、条件变量 (condition-variable) 以及读写锁 (read-write-lock)。

### 二进制文件加载 (Binary File Loading)

- 支持通过子进程加载可执行文件。
- 支持在运行时加载动态库。

### Reactor 模型 (Reactor)

- 支持 TCP 服务端和客户端。
- 采用多线程、非阻塞 I/O 以及水平触发事件监听的方式，兼具高性能与健壮性。

## 使用方法 (Usage)

### 前置条件 (Prerequisite)

- **操作系统**：Linux / macOS
- **依赖项**：googletest

### 使用 CMake 构建 (Build with cmake)

**构建 (Build)**

```Bash
$ mkdir build && cd build
$ cmake .. && make
```

**安装 (Install)**

```Bash
$ make install
```

**运行单元测试 (Run Unittest)**

```Bash
$ cd unittest && ctest
```

### 使用 Bazelisk 构建 (Build with bazelisk)

**构建 (Build)**

```Bash
$ bazel build //...
```

**运行单元测试 (Run Unittest)**

```Bash
$ bazel test //...
```

## 快速开始 (Getting Started)

请参阅示例 (examples) 以及相关教程。