#ifndef CONN_CONNECTION_POOL_H
#define CONN_CONNECTION_POOL_H

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <condition_variable>
#include "connection.h"
#include "util/memory_pool.h"

namespace conn {

/**
 * @brief 连接池类，管理所有客户端连接
 * 
 * 该类采用单例模式，负责管理所有客户端连接，包括添加、删除、查找连接，
 * 以及定时心跳检测，确保连接的有效性。
 */
class ConnectionPool {
 public:
  /**
   * @brief 获取连接池实例（单例模式）
   * @return 连接池实例
   */
  static ConnectionPool& GetInstance();

  // 禁止拷贝和赋值
  ConnectionPool(const ConnectionPool&) = delete;
  ConnectionPool& operator=(const ConnectionPool&) = delete;

  /**
   * @brief 初始化连接池
   * @param max_connections 最大连接数
   * @param heartbeat_timeout_ms 心跳超时时间（毫秒）
   */
  void Init(size_t max_connections = 100000,
            int64_t heartbeat_timeout_ms = 60000);

  /**
   * @brief 关闭连接池
   */
  void Shutdown();

  /**
   * @brief 添加连接
   * @param conn 连接对象
   * @return 是否添加成功
   */
  bool AddConnection(std::shared_ptr<Connection> conn);
  
  /**
   * @brief 移除连接
   * @param fd 文件描述符
   * @return 是否移除成功
   */
  bool RemoveConnection(int fd);
  
  /**
   * @brief 获取连接
   * @param fd 文件描述符
   * @return 连接对象
   */
  std::shared_ptr<Connection> GetConnection(int fd);

  /**
   * @brief 获取当前连接数
   * @return 当前连接数
   */
  size_t GetConnectionCount() const;
  
  /**
   * @brief 获取最大连接数
   * @return 最大连接数
   */
  size_t GetMaxConnections() const;

  /**
   * @brief 开始心跳检测
   */
  void StartHeartbeatCheck();
  
  /**
   * @brief 停止心跳检测
   */
  void StopHeartbeatCheck();

 private:
  /**
   * @brief 构造函数
   */
  ConnectionPool() = default;
  
  /**
   * @brief 析构函数
   */
  ~ConnectionPool();

  /**
   * @brief 心跳检测循环
   */
  void HeartbeatCheckLoop();

  mutable std::mutex mutex_;  // 互斥锁，用于线程安全
  std::unordered_map<int, std::shared_ptr<Connection>> connections_;  // 连接映射表
  
  std::shared_ptr<util::MemoryPool> memory_pool_;  // 内存池
  
  size_t max_connections_;  // 最大连接数
  int64_t heartbeat_timeout_ms_;  // 心跳超时时间（毫秒）
  std::atomic<bool> running_;  // 运行状态
  std::atomic<bool> heartbeat_running_;  // 心跳检测运行状态
  
  std::thread heartbeat_thread_;  // 心跳检测线程
  std::condition_variable heartbeat_cv_;  // 心跳检测条件变量
  std::mutex heartbeat_mutex_;  // 心跳检测互斥锁
};

}  // namespace conn

#endif  // CONN_CONNECTION_POOL_H
