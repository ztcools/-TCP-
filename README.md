# 企业级高并发 TCP 服务器

一个基于 Linux Epoll 的高性能、企业级 TCP 服务器实现。

## 项目特性

- ✅ 高并发设计（支持 10万+ 连接）
- ✅ ET 边缘触发 Epoll 模型
- ✅ 内存池管理（减少内存碎片）
- ✅ 线程池异步处理
- ✅ 连接池管理
- ✅ 心跳超时检测
- ✅ 优雅关闭
- ✅ 完整的日志系统
- ✅ Echo 业务示例

## 模块架构

### 1. 日志模块 (util/log)
- 分级日志（DEBUG/INFO/WARN/ERROR/FATAL）
- 线程安全
- 双输出（控制台 + 文件）
- 按天自动分割日志

### 2. 线程池模块 (util/thread_pool)
- C++11 thread/mutex/condition_variable
- 任务队列
- 线程池大小可配置
- 优雅关闭
- 返回 std::future

### 3. 内存池模块 (util/memory_pool)
- 固定大小内存块
- 内存块链管理
- 预分配内存
- 线程安全
- 无锁优化

### 4. Socket 封装模块 (net/socket)
- Linux TCP Socket 封装
- 非阻塞模式
- SO_REUSEADDR/SO_REUSEPORT
- TCP_NODELAY
- 错误统一处理

### 5. Epoll 封装模块 (net/epoll)
- ET 边缘触发模式
- epoll_create1/epoll_ctl/epoll_wait
- 事件类型管理

### 6. 连接管理模块 (conn/connection)
- 客户端连接封装
- 读写缓冲区
- 心跳超时检测
- 连接状态管理

### 7. 连接池模块 (conn/connection_pool)
- 单例模式
- 连接添加/删除/查找
- 最大连接数限制
- 定时心跳检查
- 线程安全

### 8. Echo 业务模块 (service/echo_service)
- 读取客户端数据原样返回
- 基于连接对象读取缓冲区
- 线程池异步处理
- 处理粘包/半包（按行分割）

### 9. 主服务器模块 (net/server)
- 整合所有模块
- ET 模式事件循环
- 处理：新连接 accept、读事件、写事件、错误事件
- 新连接加入连接池
- 读事件交给线程池+业务层处理
- 优雅关闭（Ctrl+C）
- 支持配置端口、监听队列、最大连接数

## 快速开始

### 编译项目

#### 方式一：使用 build.sh (推荐)
```bash
# Release 模式（默认）
./build.sh

# Debug 模式
./build.sh Debug
```

#### 方式二：手动编译
```bash
# 创建构建目录
mkdir -p build
cd build

# Release 模式（默认）
cmake ..

# Debug 模式
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 编译
make -j$(nproc)
```

### 运行服务器

```bash
# 使用默认配置
./build/bin/tcp_server

# 自定义端口
./build/bin/tcp_server -p 9999

# 详细日志
./build/bin/tcp_server -v

# 完整配置
./build/bin/tcp_server \
    -p 8888 \
    -i 0.0.0.0 \
    -b 128 \
    -c 100000 \
    -t 60000 \
    -w 8
```

### 查看帮助
```bash
./build/bin/tcp_server -h
```

### 测试服务器

```bash
# 使用 telnet
telnet 127.0.0.1 8888

# 或使用 nc
nc 127.0.0.1 8888
```

## 命令行参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-h, --help` | 显示帮助 | - |
| `-p, --port` | 监听端口 | 8888 |
| `-i, --ip` | 监听 IP | 0.0.0.0 |
| `-b, --backlog` | 监听队列 | 128 |
| `-c, --max-conn` | 最大连接数 | 100000 |
| `-t, --timeout` | 心跳超时(ms) | 60000 |
| `-w, --workers` | 工作线程数 | 自动 |
| `-v, --verbose` | 详细日志 | INFO |

## 项目结构

```
tcp_server/
├── CMakeLists.txt          # 根目录构建文件
├── build.sh                # 编译脚本
├── README.md               # 项目文档
├── include/                # 头文件目录
│   ├── util/
│   │   ├── log.h           # 日志模块
│   │   ├── thread_pool.h   # 线程池模块
│   │   └── memory_pool.h   # 内存池模块
│   ├── net/
│   │   ├── socket.h        # Socket 封装
│   │   ├── epoll.h         # Epoll 封装
│   │   └── server.h        # 服务器核心
│   ├── conn/
│   │   ├── connection.h    # 连接管理
│   │   └── connection_pool.h  # 连接池
│   └── service/
│       └── echo_service.h  # Echo 业务
├── src/                    # 源文件目录
│   ├── util/
│   │   ├── log.cpp
│   │   ├── thread_pool.cpp
│   │   └── memory_pool.cpp
│   ├── net/
│   │   ├── socket.cpp
│   │   ├── epoll.cpp
│   │   └── server.cpp
│   ├── conn/
│   │   ├── connection.cpp
│   │   └── connection_pool.cpp
│   ├── service/
│   │   └── echo_service.cpp
│   └── main.cpp
└── test/                   # 测试代码
    ├── CMakeLists.txt
    ├── test_log.cpp
    ├── test_thread_pool.cpp
    ├── test_memory_pool.cpp
    ├── test_socket.cpp
    ├── test_epoll.cpp
    ├── test_connection.cpp
    ├── test_connection_pool.cpp
    └── test_echo_service.cpp
```

## 构建系统特性

- CMake 3.22.1+
- C++17 标准
- Debug/Release 编译模式
- 自动遍历源文件
- 链接 pthread、rt 库
- 编译警告：-Wall -Wextra -Wpedantic

## 技术栈

- **操作系统**: Linux
- **编程语言**: C++17
- **构建系统**: CMake
- **核心技术**: Epoll ET 模式
- **并发模型**: Reactor 模式

## 性能优化

1. **内存池**: 减少内存分配/释放开销
2. **线程池**: 避免频繁创建/销毁线程
3. **Epoll ET**: 边缘触发减少系统调用
4. **非阻塞 IO**: 充分利用系统资源
5. **连接池**: 复用连接对象

## 许可证

MIT License
