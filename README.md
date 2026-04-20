# 企业级高并发 TCP 服务器

一个基于 Linux Epoll 的高性能、企业级 TCP 服务器实现，采用 **主从 Reactor 架构**。

## 项目特性

- ✅ **主从 Reactor 架构** - 主 Reactor 处理连接accept，IO Reactor 处理读写事件
- ✅ 高并发设计（支持 10万+ 连接）
- ✅ ET 边缘触发 Epoll 模型
- ✅ 内存池管理（减少内存碎片）
- ✅ 连接池 + 心跳检测
- ✅ **半包粘包处理**（按行分割 / 定长分割 / 特殊分隔符）
- ✅ 优雅关闭（Ctrl+C / signal）
- ✅ 完整的日志系统
- ✅ Echo 业务示例
- ✅ 完善的部署流程

## 架构设计

### 主从 Reactor 模型

```
┌─────────────────────────────────────────────────────────────┐
│                         Client                              │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Main Reactor (主 Reactor)                  │
│                   accept() 新连接                             │
│              0.0.0.0:9999 - 监听 socket                     │
└─────────────────────────────────────────────────────────────┘
                              │
                    accept() 新连接
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Connection Pool (连接池)                        │
│         管理所有客户端连接，10万+ 连接                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│               IO Reactors (IO Reactor 线程池)                │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐      ┌─────────┐      │
│  │Thread 1 │ │Thread 2 │ │Thread 3 │ ...  │Thread N │      │
│  │Epoll    │ │Epoll    │ │Epoll    │      │Epoll    │      │
│  │         │ │         │ │         │      │         │      │
│  └─────────┘ └─────────┘ └─────────┘      └─────────┘      │
│                 处理读写事件、业务逻辑                        │
└─────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 说明 |
|------|------|
| **Main Reactor** | 主线程运行，仅处理新连接 accept |
| **IO Reactor Pool** | N 个 IO 线程，每个线程独立 Epoll 处理已连接 socket 的读写事件 |
| **Connection Pool** | 单例管理所有连接，线程安全 |
| **Memory Pool** | 预分配内存块，减少 malloc 开销 |
| **EventLoop** | 每个线程一个 EventLoop，封装 Epoll 操作 |
| **WakeupFD (eventfd)** | 用于优雅关闭时中断所有线程的 epoll_wait |

### 线程配置

- **IO 线程数**：CPU 核心数 - 1（如 16 核机器 = 15 个 IO 线程）
- **主 Reactor**：主线程专属，运行在主线程
- **内存池**：10000 个内存块，每块 4096 字节

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

### 6. EventLoop 模块 (net/event_loop)
- 每个 IO 线程一个 EventLoop
- 封装 Epoll 操作
- WakeupFD 支持优雅关闭中断

### 7. IO 线程池模块 (net/io_thread_pool)
- 管理所有 IO Reactor 线程
- 轮询分配新连接给 IO 线程
- 支持优雅关闭

### 8. 连接管理模块 (conn/connection)
- 客户端连接封装
- 读写缓冲区
- 心跳超时检测
- 连接状态管理

### 9. 连接池模块 (conn/connection_pool)
- 单例模式
- 连接添加/删除/查找
- 最大连接数限制
- 定时心跳检查
- 线程安全

### 10. Echo 业务模块 (service/echo_service)
- 读取客户端数据原样返回
- 基于连接对象读取缓冲区
- 线程池异步处理
- **处理粘包/半包（按行分割）**

### 11. 主服务器模块 (net/server)
- 单例模式，整合所有模块
- 主 Reactor 事件循环
- IO Reactor 线程池管理
- 优雅关闭（signal 处理）

## 核心亮点

### 1. 半包粘包处理

TCP 是流式协议，数据可能被打散（半包）或合并（粘包）。本服务器实现多种分包策略：

| 分包策略 | 说明 | 适用场景 |
|---------|------|---------|
| **按行分割** | 以 `\n` 或 `\r\n` 为结束标志 | 文本协议，如日志、HTTP |
| **定长分割** | 每次读取指定长度 | 二进制协议，数据块固定 |
| **特殊分隔符** | 自定义分隔符（如 `\|`、`\0`） | 自定义协议 |

```cpp
// 按行分割示例
// 客户端发送: "Hello\nWorld\n"
// 服务器分两次接收: "Hello" 和 "World"
```

### 2. 内存池技术

传统的 malloc/free 会造成内存碎片和性能开销。本服务器采用内存池技术：

- **预分配**：启动时分配 10000 个内存块
- **固定大小**：每个块 4096 字节，适合网络缓冲区
- **无锁优化**：每个线程独立内存池，减少锁竞争
- **高性能**：避免频繁系统调用，内存分配 O(1)

### 3. 心跳检测机制

- **定时检测**：每隔一段时间检查连接是否存活
- **超时断开**：超过指定时间无响应自动断开
- **资源回收**：及时释放无效连接，节省资源

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
# 使用默认配置（端口 9999）
./build/bin/tcp_server

# 自定义端口
./build/bin/tcp_server -p 9999

# 详细日志
./build/bin/tcp_server -v

# 完整配置
./build/bin/tcp_server \
    -p 9999 \
    -i 0.0.0.0 \
    -b 128 \
    -c 100000 \
    -t 60000
```

### 查看帮助
```bash
./build/bin/tcp_server -h
```

### 测试服务器

```bash
# 使用 telnet
telnet 127.0.0.1 9999

# 或使用 nc
nc 127.0.0.1 9999
```

## 命令行参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-h, --help` | 显示帮助 | - |
| `-p, --port` | 监听端口 | 9999 |
| `-i, --ip` | 监听 IP | 0.0.0.0 |
| `-b, --backlog` | 监听队列 | 128 |
| `-c, --max-conn` | 最大连接数 | 100000 |
| `-t, --timeout` | 心跳超时(ms) | 60000 |
| `-w, --workers` | 工作线程数 | 自动 |
| `-v, --verbose` | 详细日志 | INFO |

## 性能测试

### 测试环境

- **压测工具**：TitanBench（高性能网络压测工具）
- **测试场景**：TCP Echo 响应
- **并发数**：500
- **线程数**：8
- **时长**：30 秒

### WSL 本地测试结果（自研测压工具titanbench）

```
./titanbench-x64-linux -c 500 -T 8 -t 30 -p tcp -h 127.0.0.1 -P 9999
```

| 指标 | 数值 |
|------|------|
| **QPS** | 64,756 ~ 70,000+ |
| **平均延迟** | 2.42 us |
| **P99 延迟** | 7.00 us |
| **成功率** | 100% |
| **30秒处理请求** | 1,944,289 |

### 性能对比图

！[本地实测性能](./assets/test.png)   ！[本地实测性能](./assets/test1.png)








### 物理机部署性能

实际物理机部署可达 **100,000+ QPS**

## 项目结构

```
tcp_server/
├── CMakeLists.txt          # 根目录构建文件
├── build.sh                # 编译脚本
├── README.md               # 项目文档
├── src/                    # 源文件目录
│   ├── util/
│   │   ├── log.cpp         # 日志模块
│   │   ├── thread_pool.cpp # 线程池模块
│   │   └── memory_pool.cpp # 内存池模块
│   ├── net/
│   │   ├── socket.cpp      # Socket 封装
│   │   ├── epoll.cpp       # Epoll 封装
│   │   ├── event_loop.cpp  # EventLoop 封装
│   │   ├── io_thread_pool.cpp  # IO 线程池
│   │   └── server.cpp      # 服务器核心
│   ├── conn/
│   │   ├── connection.cpp  # 连接管理
│   │   └── connection_pool.cpp  # 连接池
│   ├── service/
│   │   └── echo_service.cpp # Echo 业务
│   └── main.cpp
└── test/                   # 测试代码
```

## 构建系统特性

- CMake 3.16+
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
- **并发模型**: 主从 Reactor 模式

## 性能优化

1. **主从 Reactor 架构**：主 Reactor 处理 accept，IO Reactor 并行处理读写，充分利用多核
2. **内存池**：预分配内存块，减少 malloc/free 开销
3. **Epoll ET 边缘触发**：减少系统调用次数
4. **非阻塞 IO**：充分利用系统资源
5. **连接池**：复用连接对象，避免频繁创建/销毁
6. **WakeupFD 中断**：优雅关闭时立即中断所有 epoll_wait

## 优雅关闭流程

```
Ctrl+C (SIGINT)
    ↓
Signal Handler (设置 stop_requested_)
    ↓
向所有 EventLoop 的 wakeup_fd 写入数据
    ↓
所有 epoll_wait 立即返回
    ↓
IO 线程池优雅停止
    ↓
关闭连接池、监听 socket
    ↓
服务器完全退出
```

## 部署指南

### 环境要求

- Linux 系统（CentOS 7+ / Ubuntu 18+ / Debian 10+）
- GCC 9+ 或 Clang 10+
- CMake 3.16+

### 部署步骤

#### 1. 安装依赖

```bash
# CentOS / RHEL
sudo yum install -y gcc-c++ cmake make

# Ubuntu / Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake
```

#### 2. 编译项目

```bash
# 下载代码
git clone <repository_url>
cd tcp_server

# 编译
./build.sh
```

#### 3. 配置与运行

```bash
# 创建运行目录
sudo mkdir -p /opt/tcp_server
sudo cp -r build/bin /opt/tcp_server/
sudo cp config.json /opt/tcp_server/  # 如有配置文件

# 创建启动用户
sudo useradd -r -s /sbin/nologin tcp_server

# 设置权限
sudo chown -R tcp_server:tcp_server /opt/tcp_server

# 启动服务
sudo su - tcp_server -c "/opt/tcp_server/bin/tcp_server -p 9999"
```

#### 4. 配置 systemd 服务

```bash
# 创建服务文件
sudo vim /etc/systemd/system/tcp_server.service
```

```ini
[Unit]
Description=TCP Server
After=network.target

[Service]
Type=simple
User=tcp_server
WorkingDirectory=/opt/tcp_server
ExecStart=/opt/tcp_server/bin/tcp_server -p 9999 -c 100000
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

```bash
# 启用服务
sudo systemctl daemon-reload
sudo systemctl enable tcp_server
sudo systemctl start tcp_server

# 检查状态
sudo systemctl status tcp_server
```

#### 5. 配置防火墙

```bash
# 开放端口
sudo firewall-cmd --permanent --add-port=9999/tcp
sudo firewall-cmd --reload

# 或使用 iptables
sudo iptables -A INPUT -p tcp --dport 9999 -j ACCEPT
```

### 常用命令

```bash
# 启动服务
sudo systemctl start tcp_server

# 停止服务
sudo systemctl stop tcp_server

# 重启服务
sudo systemctl restart tcp_server

# 查看日志
sudo journalctl -u tcp_server -f

# 查看进程
ps aux | grep tcp_server
```

## 许可证

MIT License
