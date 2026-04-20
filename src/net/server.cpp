#include "net/server.h"
#include "../conn/connection_pool.h"
#include <cstring>
#include <errno.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <thread>

namespace net {

std::atomic<bool> Server::stop_requested_(false);
int Server::signal_fd_(-1);

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

  main_loop_ = std::make_unique<EventLoop>();
  main_loop_->SetMainReactor(true);
  main_loop_->AddFd(listen_socket_->GetFd(), EPOLLIN | EPOLLET);
  LOG_INFO("Main EventLoop created in main thread");

  // 创建内存池
  memory_pool_ = std::make_shared<util::MemoryPool>(
      config_.memory_pool_block_size, config_.memory_pool_block_count);
  LOG_INFO("Memory pool initialized: block_size=" +
           std::to_string(config_.memory_pool_block_size) +
           ", block_count=" + std::to_string(config_.memory_pool_block_count));

  // 计算IO线程数：CPU核心数 - 1（不包括主Reactor线程）
  size_t cpu_count = std::thread::hardware_concurrency();
  size_t io_thread_num = (cpu_count > 1) ? (cpu_count - 1) : 1;
  LOG_INFO("CPU cores: " + std::to_string(cpu_count) + ", IO threads: " + std::to_string(io_thread_num));

  // 创建IO线程池
  io_thread_pool_ = std::make_unique<IOThreadPool>(io_thread_num);
  LOG_INFO("IO thread pool initialized");

  // 初始化连接池
  auto& conn_pool = conn::ConnectionPool::GetInstance();
  conn_pool.Init(config_.max_connections, config_.heartbeat_timeout_ms);
  conn_pool.StartHeartbeatCheck();
  LOG_INFO("Connection pool initialized");

  // 创建 eventfd 用于中断 epoll_wait
  signal_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (signal_fd_ == -1) {
    LOG_ERROR("Failed to create eventfd");
  } else {
    main_loop_->AddFd(signal_fd_, EPOLLIN | EPOLLET);
    LOG_INFO("Eventfd created for interrupting epoll_wait");
  }

  // 注册信号处理
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

  // 启动IO线程池（从Reactor）
  io_thread_pool_->Start();
  LOG_INFO("IO thread pool started");

  // 主Reactor在主线程中运行
  LOG_INFO("Main EventLoop will run in main thread");

  // 运行主Reactor的事件循环
  MainEventLoop();

  LOG_INFO("Server started");
}

void Server::Stop() {
  if (!running_) {
    return;
  }

  LOG_INFO("Server stopping...");
  running_ = false;

  // 停止IO线程池（包括主Reactor线程）
  if (io_thread_pool_) {
    io_thread_pool_->Stop();
  }

  // 关闭连接池
  auto& conn_pool = conn::ConnectionPool::GetInstance();
  conn_pool.StopHeartbeatCheck();
  conn_pool.Shutdown();
  LOG_INFO("Connection pool shutdown");

  // 关闭 eventfd
  if (signal_fd_ != -1) {
    close(signal_fd_);
    signal_fd_ = -1;
  }

  // 关闭监听socket
  if (listen_socket_) {
    listen_socket_->Close();
    listen_socket_.reset();
  }

  LOG_INFO("Server stopped");
}

void Server::Wait() {
  // 由于主Reactor现在在IO线程池中运行，不需要单独等待
  // IO线程池的Stop()方法会等待所有线程结束
}

void Server::SignalHandler(int signal) {
  LOG_INFO("Received signal " + std::to_string(signal));
  stop_requested_ = true;
  // 向 eventfd 写入数据以中断 epoll_wait
  if (signal_fd_ != -1) {
    uint64_t tmp = 1;
    [[maybe_unused]] ssize_t ret = write(signal_fd_, &tmp, sizeof(tmp));
  }
}

void Server::MainEventLoop() {
  LOG_INFO("Main EventLoop started");
  
  // 检查main_loop_是否有效
  if (!main_loop_) {
    LOG_ERROR("Main EventLoop: main_loop_ is null");
    return;
  }
  
  // 检查listen_socket_是否有效
  if (!listen_socket_) {
    LOG_ERROR("Main EventLoop: listen_socket_ is null");
    return;
  }
  
  // 检查epoll是否有效
  auto epoll = main_loop_->GetEpoll();
  if (!epoll) {
    LOG_ERROR("Main EventLoop: epoll is null");
    return;
  }
  
  LOG_INFO("Main EventLoop: all pointers are valid");
  LOG_INFO("Main EventLoop: listen_fd=" + std::to_string(listen_socket_->GetFd()));

  while (running_ && !stop_requested_) {
    int num_events = epoll->Wait(-1);
    if (num_events < 0) {
      if (errno == EINTR) {
        continue;
      }
      LOG_ERROR("Epoll wait error: " + std::string(strerror(errno)));
      break;
    }

    LOG_DEBUG("Main EventLoop: num_events=" + std::to_string(num_events));

    for (int i = 0; i < num_events; ++i) {
      const struct epoll_event& event = epoll->GetEvents()[i];
      int fd = event.data.fd;

      LOG_DEBUG("Main EventLoop: handling event for fd=" + std::to_string(fd));

      if (fd == signal_fd_) {
        uint64_t tmp;
        [[maybe_unused]] ssize_t ret = read(signal_fd_, &tmp, sizeof(tmp));
        LOG_INFO("Main EventLoop: received stop signal from eventfd");
        running_ = false;
        break;
      } else if (fd == listen_socket_->GetFd()) {
        if (event.events & EPOLLIN) {
          LOG_DEBUG("Main EventLoop: accept event");
          HandleAccept();
        }
      }
    }
  }

  LOG_INFO("Main EventLoop stopped");
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

    // 轮询获取一个从Reactor
    EventLoop* sub_loop = io_thread_pool_->GetNextEventLoop();
    if (!sub_loop) {
      LOG_ERROR("No available IO thread");
      client_socket.Close();
      conn_pool.RemoveConnection(client_fd);
      continue;
    }

    // 将连接添加到从Reactor
    sub_loop->AddFd(client_fd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT);
    LOG_DEBUG("Connection " + std::to_string(client_fd) + " assigned to IO thread");

    client_socket.Release();
  }
}

}  // namespace net