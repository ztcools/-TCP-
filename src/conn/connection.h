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

enum class ConnectionState {
  CONNECTING,
  CONNECTED,
  CLOSED
};

class Connection {
 public:
  Connection(int fd, const std::string& ip, uint16_t port,
             std::shared_ptr<util::MemoryPool> memory_pool);
  ~Connection();

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  int GetFd() const;
  const std::string& GetIp() const;
  uint16_t GetPort() const;
  ConnectionState GetState() const;
  void SetState(ConnectionState state);

  void UpdateHeartbeat();
  bool IsTimeout(int64_t timeout_ms) const;
  int64_t GetLastHeartbeatMs() const;

  ssize_t Recv();
  ssize_t Send();

  bool HasPendingData() const;
  size_t GetReadBufferSize() const;
  size_t GetWriteBufferSize() const;

  const char* GetReadBuffer() const;
  void ConsumeReadBuffer(size_t len);
  void AppendWriteBuffer(const char* data, size_t len);

  void Close();

 private:
  void CheckAndResizeReadBuffer();
  void CheckAndResizeWriteBuffer();

  mutable std::mutex mutex_;

  int fd_;
  std::string ip_;
  uint16_t port_;
  std::atomic<ConnectionState> state_;

  std::chrono::steady_clock::time_point last_heartbeat_;

  std::vector<char> read_buffer_;
  std::vector<char> write_buffer_;
  size_t read_idx_;
  size_t write_idx_;

  std::shared_ptr<util::MemoryPool> memory_pool_;
};

}  // namespace conn

#endif  // CONN_CONNECTION_H
