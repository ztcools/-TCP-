#include "net/server.h"
#include <cstring>
#include <errno.h>
#include <algorithm>

namespace net {

std::atomic<bool> Server::stop_requested_(false);

Server& Server::GetInstance() {
  static Server instance;
  return instance;
}

Server::~Server() {
  Stop();
}

void Server::Init(const Config& config) {
  if (running_) {
    LOG_WARN("Server already initialized");
    return;
  }

  config_ = config;
  running_ = false;
  stop_requested_ = false;

  util::Logger::GetInstance().SetLogLevel(config_.log_level);

  LOG_INFO("========================================");
  LOG_INFO("  TCP Server Initializing");
  LOG_INFO("========================================");
  LOG_INFO("Listen IP: " + config_.ip);
  LOG_INFO("Listen Port: " + std::to_string(config_.port));
  LOG_INFO("Backlog: " + std::to_string(config_.backlog));
  LOG_INFO("Max Connections: " + std::to_string(config_.max_connections));
  LOG_INFO("Heartbeat Timeout: " + std::to_string(config_.heartbeat_timeout_ms) + "ms");
  LOG_INFO("========================================");

  listen_socket_ = std::make_unique<Socket>();
  listen_socket_->Create();
  listen_socket_->SetNonBlocking();
  listen_socket_->SetReuseAddr();
  listen_socket_->SetReusePort();
  listen_socket_->Bind(config_.ip, config_.port);
  listen_socket_->Listen(config_.backlog);

  LOG_INFO("Server listening on " + config_.ip + ":" + std::to_string(config_.port));

  epoll_ = std::make_unique<Epoll>();
  epoll_->AddFd(listen_socket_->GetFd(), EPOLLIN | EPOLLET);
  LOG_INFO("Epoll initialized");

  memory_pool_ = std::make_shared<util::MemoryPool>(
      config_.memory_pool_block_size, config_.memory_pool_block_count);
  LOG_INFO("Memory pool initialized: block_size=" +
           std::to_string(config_.memory_pool_block_size) +
           ", block_count=" + std::to_string(config_.memory_pool_block_count));

  auto& conn_pool = conn::ConnectionPool::GetInstance();
  conn_pool.Init(config_.max_connections, config_.heartbeat_timeout_ms);
  conn_pool.StartHeartbeatCheck();
  LOG_INFO("Connection pool initialized");

  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);
  LOG_INFO("Signal handlers registered");

  LOG_INFO("Server initialized successfully");
}

void Server::Start() {
  if (running_) {
    LOG_WARN("Server already running");
    return;
  }

  running_ = true;
  LOG_INFO("Server starting...");

  event_loop_thread_ = std::thread(&Server::EventLoop, this);
  LOG_INFO("Server started");
}

void Server::Stop() {
  if (!running_) {
    return;
  }

  LOG_INFO("Server stopping...");
  running_ = false;

  if (event_loop_thread_.joinable()) {
    event_loop_thread_.join();
  }

  auto& conn_pool = conn::ConnectionPool::GetInstance();
  conn_pool.StopHeartbeatCheck();
  conn_pool.Shutdown();
  LOG_INFO("Connection pool shutdown");

  if (epoll_) {
    epoll_.reset();
  }

  if (listen_socket_) {
    listen_socket_->Close();
    listen_socket_.reset();
  }

  LOG_INFO("Server stopped");
}

void Server::Wait() {
  if (event_loop_thread_.joinable()) {
    event_loop_thread_.join();
  }
}

void Server::SignalHandler(int signal) {
  LOG_INFO("Received signal " + std::to_string(signal));
  stop_requested_ = true;
}

void Server::EventLoop() {
  LOG_INFO("Event loop started");

  while (running_ && !stop_requested_) {
    int num_events = epoll_->Wait(100);
    if (num_events < 0) {
      if (errno == EINTR) {
        continue;
      }
      LOG_ERROR("Epoll wait error: " + std::string(strerror(errno)));
      break;
    }

    for (int i = 0; i < num_events; ++i) {
      const struct epoll_event& event = epoll_->GetEvents()[i];
      int fd = event.data.fd;

      if (fd == listen_socket_->GetFd()) {
        if (event.events & EPOLLIN) {
          HandleAccept();
        }
      } else {
        if (event.events & EPOLLERR || event.events & EPOLLHUP) {
          HandleError(fd);
        } else if (event.events & EPOLLIN) {
          HandleRead(fd);
        } else if (event.events & EPOLLOUT) {
          HandleWrite(fd);
        }
      }
    }
  }

  LOG_INFO("Event loop stopped");
  Stop();
}

void Server::HandleAccept() {
  while (true) {
    Socket client_socket = listen_socket_->Accept();
    if (!client_socket.IsValid()) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      LOG_ERROR("Accept error: " + std::string(strerror(errno)));
      break;
    }

    std::string client_ip = client_socket.GetPeerIp();
    uint16_t client_port = client_socket.GetPeerPort();
    int client_fd = client_socket.GetFd();

    LOG_INFO("New connection accepted: " + client_ip + ":" + std::to_string(client_port) +
             ", fd=" + std::to_string(client_fd));

    client_socket.SetNonBlocking();
    client_socket.SetTcpNoDelay();

    auto& conn_pool = conn::ConnectionPool::GetInstance();

    auto conn = std::make_shared<conn::Connection>(
        client_fd, client_ip, client_port, memory_pool_);
    conn->SetState(conn::ConnectionState::CONNECTED);

    if (!conn_pool.AddConnection(conn)) {
      LOG_WARN("Connection rejected: max connections reached");
      client_socket.Close();
      continue;
    }

    client_socket.Release();

    epoll_->AddFd(client_fd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT);
    LOG_DEBUG("Connection added to epoll: fd=" + std::to_string(client_fd));
  }
}

void Server::HandleRead(int fd) {
  auto& conn_pool = conn::ConnectionPool::GetInstance();
  auto conn = conn_pool.GetConnection(fd);

  if (!conn) {
    LOG_WARN("Connection not found for read: fd=" + std::to_string(fd));
    return;
  }

  ssize_t recv_len = conn->Recv();
  if (recv_len < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG_ERROR("Recv error: " + std::string(strerror(errno)));
      HandleError(fd);
    }
    return;
  }

  if (recv_len == 0) {
    LOG_INFO("Connection closed by peer: " + conn->GetIp() + ":" +
             std::to_string(conn->GetPort()));
    HandleError(fd);
    return;
  }

  conn->UpdateHeartbeat();

  size_t data_len = conn->GetReadBufferSize();
  if (data_len == 0) {
    return;
  }

  const char* data = conn->GetReadBuffer();
  conn->AppendWriteBuffer(data, data_len);
  conn->ConsumeReadBuffer(data_len);

  ssize_t send_len = conn->Send();
  if (send_len < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG_ERROR("Send error: " + std::string(strerror(errno)));
      HandleError(fd);
      return;
    }
  }

  epoll_->ModifyFd(fd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT);
}

void Server::HandleWrite(int fd) {
  auto& conn_pool = conn::ConnectionPool::GetInstance();
  auto conn = conn_pool.GetConnection(fd);

  if (!conn) {
    LOG_WARN("Connection not found for write: fd=" + std::to_string(fd));
    return;
  }

  ssize_t send_len = conn->Send();
  if (send_len < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG_ERROR("Send error: " + std::string(strerror(errno)));
      HandleError(fd);
    }
    return;
  }

  conn->UpdateHeartbeat();
  epoll_->ModifyFd(fd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT);
}

void Server::HandleError(int fd) {
  auto& conn_pool = conn::ConnectionPool::GetInstance();
  auto conn = conn_pool.GetConnection(fd);

  if (conn) {
    LOG_INFO("Closing connection: " + conn->GetIp() + ":" +
             std::to_string(conn->GetPort()) + ", fd=" + std::to_string(fd));
  }

  epoll_->RemoveFd(fd);
  conn_pool.RemoveConnection(fd);
}

}  // namespace net