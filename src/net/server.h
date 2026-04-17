#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <string>
#include <memory>
#include <atomic>
#include <signal.h>
#include "util/log.h"
#include "util/memory_pool.h"
#include "net/socket.h"
#include "net/epoll.h"
#include "conn/connection.h"
#include "conn/connection_pool.h"

namespace net {

/**
 * @brief 主服务器类，整合所有模块，提供事件循环和连接管理
 *
 * 该类是整个服务器的核心，负责初始化各个模块、启动事件循环、
 * 处理连接请求和管理连接生命周期。采用单例模式实现。
 */
class Server {
 public:
  /**
   * @brief 服务器配置结构体
   */
  struct Config {
    std::string ip = "0.0.0.0";              // 监听 IP 地址
    uint16_t port = 8888;                     // 监听端口
    int backlog = 128;                        // 监听队列大小
    size_t max_connections = 100000;          // 最大连接数
    int64_t heartbeat_timeout_ms = 60000;     // 心跳超时时间（毫秒）
    size_t memory_pool_block_size = 4096;     // 内存池块大小
    size_t memory_pool_block_count = 10000;   // 内存池块数量
    util::LogLevel log_level = util::LogLevel::INFO;  // 日志级别
  };

  /**
   * @brief 获取服务器实例（单例模式）
   * @return 服务器实例
   */
  static Server& GetInstance();

  // 禁止拷贝和赋值
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  /**
   * @brief 初始化服务器
   * @param config 服务器配置
   */
  void Init(const Config& config);

  /**
   * @brief 启动服务器
   */
  void Start();

  /**
   * @brief 停止服务器
   */
  void Stop();

  /**
   * @brief 等待服务器停止
   */
  void Wait();

  /**
   * @brief 检查服务器是否运行
   * @return 服务器是否运行
   */
  bool IsRunning() const { return running_; }

 private:
  /**
   * @brief 构造函数
   */
  Server() = default;

  /**
   * @brief 析构函数
   */
  ~Server();

  /**
   * @brief 信号处理函数
   * @param signal 信号
   */
  static void SignalHandler(int signal);

  /**
   * @brief 事件循环
   */
  void EventLoop();

  /**
   * @brief 处理新连接
   */
  void HandleAccept();

  /**
   * @brief 处理读事件
   * @param fd 文件描述符
   */
  void HandleRead(int fd);

  /**
   * @brief 处理写事件
   * @param fd 文件描述符
   */
  void HandleWrite(int fd);

  /**
   * @brief 处理错误事件
   * @param fd 文件描述符
   */
  void HandleError(int fd);

  Config config_;                          // 服务器配置
  std::unique_ptr<Socket> listen_socket_;  // 监听 socket
  std::unique_ptr<Epoll> epoll_;          // epoll 实例
  std::shared_ptr<util::MemoryPool> memory_pool_;  // 内存池
  std::thread event_loop_thread_;          // 事件循环线程
  std::atomic<bool> running_;              // 运行状态
  static std::atomic<bool> stop_requested_;  // 停止请求标志
};

}  // namespace net

#endif  // NET_SERVER_H