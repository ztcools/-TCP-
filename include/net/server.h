#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <string>
#include <memory>
#include <atomic>
#include <signal.h>
#include "../../include/util/log.h"
#include "../../include/util/thread_pool.h"
#include "../../include/util/memory_pool.h"
#include "../../include/net/socket.h"
#include "../../include/net/epoll.h"
#include "../../include/conn/connection.h"
#include "../../include/conn/connection_pool.h"
#include "../../include/service/echo_service.h"

namespace net {

class Server {
 public:
  struct Config {
    std::string ip = "0.0.0.0";
    uint16_t port = 8888;
    int backlog = 128;
    size_t max_connections = 100000;
    int64_t heartbeat_timeout_ms = 60000;
    size_t thread_pool_size = 0;
    size_t memory_pool_block_size = 4096;
    size_t memory_pool_block_count = 10000;
    util::LogLevel log_level = util::LogLevel::INFO;
  };

  static Server& GetInstance();

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  void Init(const Config& config);
  void Start();
  void Stop();
  void Wait();

  bool IsRunning() const { return running_; }

 private:
  Server() = default;
  ~Server();

  static void SignalHandler(int signal);

  void EventLoop();
  void HandleAccept();
  void HandleRead(int fd);
  void HandleWrite(int fd);
  void HandleError(int fd);

  Config config_;
  std::unique_ptr<Socket> listen_socket_;
  std::unique_ptr<Epoll> epoll_;
  std::shared_ptr<util::MemoryPool> memory_pool_;
  std::thread event_loop_thread_;
  std::atomic<bool> running_;
  static std::atomic<bool> stop_requested_;
};

}  // namespace net

#endif  // NET_SERVER_H
