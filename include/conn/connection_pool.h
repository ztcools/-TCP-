#ifndef CONN_CONNECTION_POOL_H
#define CONN_CONNECTION_POOL_H

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <condition_variable>
#include "../../include/conn/connection.h"
#include "../../include/util/memory_pool.h"

namespace conn {

class ConnectionPool {
 public:
  static ConnectionPool& GetInstance();

  ConnectionPool(const ConnectionPool&) = delete;
  ConnectionPool& operator=(const ConnectionPool&) = delete;

  void Init(size_t max_connections = 100000,
            int64_t heartbeat_timeout_ms = 60000);

  void Shutdown();

  bool AddConnection(std::shared_ptr<Connection> conn);
  bool RemoveConnection(int fd);
  std::shared_ptr<Connection> GetConnection(int fd);

  size_t GetConnectionCount() const;
  size_t GetMaxConnections() const;

  void StartHeartbeatCheck();
  void StopHeartbeatCheck();

 private:
  ConnectionPool() = default;
  ~ConnectionPool();

  void HeartbeatCheckLoop();

  mutable std::mutex mutex_;
  std::unordered_map<int, std::shared_ptr<Connection>> connections_;
  
  std::shared_ptr<util::MemoryPool> memory_pool_;
  
  size_t max_connections_;
  int64_t heartbeat_timeout_ms_;
  std::atomic<bool> running_;
  std::atomic<bool> heartbeat_running_;
  
  std::thread heartbeat_thread_;
  std::condition_variable heartbeat_cv_;
  std::mutex heartbeat_mutex_;
};

}  // namespace conn

#endif  // CONN_CONNECTION_POOL_H
