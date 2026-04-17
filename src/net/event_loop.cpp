#include "net/event_loop.h"
#include "util/log.h"
#include "conn/connection_pool.h"
#include <cstring>
#include <errno.h>

namespace net {

EventLoop::EventLoop() 
    : running_(false), is_main_reactor_(false) {
  epoll_ = std::make_unique<Epoll>();
  thread_id_ = std::this_thread::get_id();
  LOG_DEBUG("EventLoop created in thread: " + std::to_string(std::hash<std::thread::id>{}(thread_id_)));
}

EventLoop::~EventLoop() {
  Stop();
}

void EventLoop::Loop() {
  // 更新thread_id_为当前线程ID
  thread_id_ = std::this_thread::get_id();
  
  running_ = true;
  LOG_INFO("EventLoop started in thread: " + std::to_string(std::hash<std::thread::id>{}(thread_id_)));

  while (running_) {
    int num_events = epoll_->Wait(-1);
    if (num_events < 0) {
      if (errno == EINTR) {
        continue;
      }
      LOG_ERROR("Epoll wait error: " + std::string(strerror(errno)));
      break;
    }

    HandleEvents();
  }

  LOG_INFO("EventLoop stopped in thread: " + std::to_string(std::hash<std::thread::id>{}(thread_id_)));
}

void EventLoop::Stop() {
  running_ = false;
}

bool EventLoop::IsInLoopThread() const {
  return std::this_thread::get_id() == thread_id_;
}

void EventLoop::AddFd(int fd, uint32_t events) {
  epoll_->AddFd(fd, events);
}

void EventLoop::ModifyFd(int fd, uint32_t events) {
  epoll_->ModifyFd(fd, events);
}

void EventLoop::RemoveFd(int fd) {
  epoll_->RemoveFd(fd);
}

void EventLoop::HandleEvents() {
  for (int i = 0; i < epoll_->GetEventCount(); ++i) {
    const struct epoll_event& event = epoll_->GetEvents()[i];
    int fd = event.data.fd;

    if (event.events & EPOLLERR || event.events & EPOLLHUP) {
      HandleError(fd);
    } else if (event.events & EPOLLIN) {
      // 主Reactor只处理监听socket的读事件（accept）
      // 从Reactor处理其他连接的读事件
      if (!IsMainReactor()) {
        HandleRead(fd);
      }
      // 主Reactor的accept事件由Server::MainEventLoop处理
    } else if (event.events & EPOLLOUT) {
      // 只有从Reactor处理写事件
      if (!IsMainReactor()) {
        HandleWrite(fd);
      }
    }
  }
}

void EventLoop::HandleRead(int fd) {
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

  ModifyFd(fd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT);
}

void EventLoop::HandleWrite(int fd) {
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
  ModifyFd(fd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT);
}

void EventLoop::HandleError(int fd) {
  auto& conn_pool = conn::ConnectionPool::GetInstance();
  auto conn = conn_pool.GetConnection(fd);

  if (conn) {
    LOG_INFO("Closing connection: " + conn->GetIp() + ":" +
             std::to_string(conn->GetPort()) + ", fd=" + std::to_string(fd));
  }

  RemoveFd(fd);
  conn_pool.RemoveConnection(fd);
}

}  // namespace net
