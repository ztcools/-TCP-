#ifndef CONN_CONNECTION_H
#define CONN_CONNECTION_H

#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>
#include "net/socket.h"
#include "util/memory_pool.h"

namespace conn {

/**
 * @brief 连接状态枚举
 */
enum class ConnectionState {
  CONNECTING,  // 连接中
  CONNECTED,   // 已连接
  CLOSED       // 已关闭
};

/**
 * @brief 连接管理类，封装客户端连接
 * 
 * 该类封装了客户端连接，提供了读写缓冲区、心跳检测、
 * 状态管理等功能，是服务器处理客户端连接的核心类。
 */
class Connection {
 public:
  /**
   * @brief 构造函数
   * @param fd 文件描述符
   * @param ip 客户端 IP 地址
   * @param port 客户端端口号
   * @param memory_pool 内存池
   */
  Connection(int fd, const std::string& ip, uint16_t port,
             std::shared_ptr<util::MemoryPool> memory_pool);
  
  /**
   * @brief 析构函数
   */
  ~Connection();

  // 禁止拷贝和赋值
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  /**
   * @brief 获取文件描述符
   * @return 文件描述符
   */
  int GetFd() const;
  
  /**
   * @brief 获取客户端 IP 地址
   * @return 客户端 IP 地址
   */
  const std::string& GetIp() const;
  
  /**
   * @brief 获取客户端端口号
   * @return 客户端端口号
   */
  uint16_t GetPort() const;
  
  /**
   * @brief 获取连接状态
   * @return 连接状态
   */
  ConnectionState GetState() const;
  
  /**
   * @brief 设置连接状态
   * @param state 连接状态
   */
  void SetState(ConnectionState state);

  /**
   * @brief 更新心跳时间
   */
  void UpdateHeartbeat();
  
  /**
   * @brief 检查连接是否超时
   * @param timeout_ms 超时时间（毫秒）
   * @return 连接是否超时
   */
  bool IsTimeout(int64_t timeout_ms) const;
  
  /**
   * @brief 获取最后心跳时间（毫秒）
   * @return 最后心跳时间（毫秒）
   */
  int64_t GetLastHeartbeatMs() const;

  /**
   * @brief 接收数据
   * @return 接收的字节数
   */
  ssize_t Recv();
  
  /**
   * @brief 发送数据
   * @return 发送的字节数
   */
  ssize_t Send();

  /**
   * @brief 检查是否有待发送数据
   * @return 是否有待发送数据
   */
  bool HasPendingData() const;
  
  /**
   * @brief 获取读缓冲区大小
   * @return 读缓冲区大小
   */
  size_t GetReadBufferSize() const;
  
  /**
   * @brief 获取写缓冲区大小
   * @return 写缓冲区大小
   */
  size_t GetWriteBufferSize() const;

  /**
   * @brief 获取读缓冲区指针
   * @return 读缓冲区指针
   */
  const char* GetReadBuffer() const;
  
  /**
   * @brief 消费读缓冲区数据
   * @param len 消费的长度
   */
  void ConsumeReadBuffer(size_t len);
  
  /**
   * @brief 追加数据到写缓冲区
   * @param data 数据指针
   * @param len 数据长度
   */
  void AppendWriteBuffer(const char* data, size_t len);

  /**
   * @brief 关闭连接
   */
  void Close();

 private:
  /**
   * @brief 检查并调整读缓冲区大小
   */
  void CheckAndResizeReadBuffer();
  
  /**
   * @brief 检查并调整写缓冲区大小
   */
  void CheckAndResizeWriteBuffer();

  int fd_;                    // 文件描述符
  std::string ip_;            // 客户端 IP 地址
  uint16_t port_;             // 客户端端口号
  std::atomic<ConnectionState> state_;  // 连接状态

  std::chrono::steady_clock::time_point last_heartbeat_;  // 最后心跳时间

  std::vector<char> read_buffer_;   // 读缓冲区
  std::vector<char> write_buffer_;  // 写缓冲区
  size_t read_idx_;                 // 读缓冲区索引（单线程访问）
  std::atomic<size_t> write_idx_;   // 写缓冲区索引（atomic，无需锁）

  std::shared_ptr<util::MemoryPool> memory_pool_;  // 内存池
};

}  // namespace conn

#endif  // CONN_CONNECTION_H
