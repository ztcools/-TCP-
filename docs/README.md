# TCP Server

高性能 TCP 服务器项目

## 目录结构

```
tcp_server/
├── CMakeLists.txt          # 总构建文件
├── src/                    # 源码
│   ├── util/              # 工具模块
│   ├── net/               # 网络核心模块
│   ├── conn/              # 连接管理模块
│   ├── service/           # 业务模块
│   └── main.cpp           # 主函数
├── include/               # 头文件
│   ├── util/
│   ├── net/
│   ├── conn/
│   └── service/
├── log/                   # 日志输出目录
├── test/                  # 测试代码
└── docs/                  # 开发文档
```

## 构建说明

```bash
mkdir build && cd build
cmake ..
make
```

## 运行

```bash
./bin/tcp_server
```
