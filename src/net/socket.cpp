#include "net/socket.h"
#include "util/log.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <stdexcept>
#include <string>

namespace net {

Socket::Socket() : fd_(-1) {}

Socket::Socket(int fd) : fd_(fd) {}

Socket::~Socket() {
  Close();
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
  other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
  if (this != &other) {
    Close();
    fd_ = other.fd_;
    other.fd_ = -1;
  }
  return *this;
}

void Socket::Create() {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  CheckError(fd_, "socket");
  
  SetNonBlocking();
  SetReuseAddr();
  SetReusePort();
  SetTcpNoDelay();
  
  LOG_INFO("Socket created: fd=" + std::to_string(fd_));
}

void Socket::Bind(const std::string& ip, uint16_t port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  
  if (ip.empty() || ip == "0.0.0.0") {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
      LOG_ERROR("Invalid IP address: " + ip);
      throw std::runtime_error("Invalid IP address");
    }
  }
  
  int ret = bind(fd_, (struct sockaddr*)&addr, sizeof(addr));
  CheckError(ret, "bind");
  
  LOG_INFO("Socket bound: " + (ip.empty() ? "0.0.0.0" : ip) + ":" + std::to_string(port));
}

void Socket::Listen(int backlog) {
  int ret = listen(fd_, backlog);
  CheckError(ret, "listen");
  
  LOG_INFO("Socket listening: backlog=" + std::to_string(backlog));
}

Socket Socket::Accept() {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  
  int client_fd = accept(fd_, (struct sockaddr*)&client_addr, &client_len);
  
  if (client_fd == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return Socket(-1);
    }
    CheckError(client_fd, "accept");
  }
  
  Socket client_socket(client_fd);
  client_socket.SetNonBlocking();
  client_socket.SetTcpNoDelay();
  
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
  LOG_INFO("New connection accepted: " + std::string(ip) + ":" + std::to_string(ntohs(client_addr.sin_port)) + ", fd=" + std::to_string(client_fd));
  
  return client_socket;
}

void Socket::Connect(const std::string& ip, uint16_t port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  
  if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
    LOG_ERROR("Invalid IP address: " + ip);
    throw std::runtime_error("Invalid IP address");
  }
  
  int ret = connect(fd_, (struct sockaddr*)&addr, sizeof(addr));
  if (ret == -1) {
    if (errno != EINPROGRESS) {
      CheckError(ret, "connect");
    }
  }
  
  LOG_INFO("Socket connected to: " + ip + ":" + std::to_string(port));
}

ssize_t Socket::Send(const void* data, size_t len) {
  ssize_t ret = send(fd_, data, len, 0);
  if (ret == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    CheckError(ret, "send");
  }
  return ret;
}

ssize_t Socket::Recv(void* data, size_t len) {
  ssize_t ret = recv(fd_, data, len, 0);
  if (ret == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    CheckError(ret, "recv");
  }
  return ret;
}

int Socket::GetFd() const {
  return fd_;
}

bool Socket::IsValid() const {
  return fd_ != -1;
}

void Socket::Close() {
  if (fd_ != -1) {
    LOG_INFO("Socket closed: fd=" + std::to_string(fd_));
    close(fd_);
    fd_ = -1;
  }
}

int Socket::Release() {
  int fd = fd_;
  fd_ = -1;
  return fd;
}

void Socket::SetNonBlocking() {
  int flags = fcntl(fd_, F_GETFL, 0);
  CheckError(flags, "fcntl(F_GETFL)");
  
  flags |= O_NONBLOCK;
  int ret = fcntl(fd_, F_SETFL, flags);
  CheckError(ret, "fcntl(F_SETFL)");
}

void Socket::SetReuseAddr() {
  int opt = 1;
  int ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  CheckError(ret, "setsockopt(SO_REUSEADDR)");
}

void Socket::SetReusePort() {
  int opt = 1;
  int ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
  CheckError(ret, "setsockopt(SO_REUSEPORT)");
}

void Socket::SetTcpNoDelay() {
  int opt = 1;
  int ret = setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
  CheckError(ret, "setsockopt(TCP_NODELAY)");
}

std::string Socket::GetPeerIp() const {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  if (getpeername(fd_, (struct sockaddr*)&addr, &len) == 0) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
  }
  return "0.0.0.0";
}

uint16_t Socket::GetPeerPort() const {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  if (getpeername(fd_, (struct sockaddr*)&addr, &len) == 0) {
    return ntohs(addr.sin_port);
  }
  return 0;
}

std::string Socket::GetLocalIp() const {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  if (getsockname(fd_, (struct sockaddr*)&addr, &len) == 0) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
  }
  return "0.0.0.0";
}

uint16_t Socket::GetLocalPort() const {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  if (getsockname(fd_, (struct sockaddr*)&addr, &len) == 0) {
    return ntohs(addr.sin_port);
  }
  return 0;
}

void Socket::CheckError(int ret, const char* operation) {
  if (ret == -1) {
    std::string msg = std::string(operation) + " failed: " + strerror(errno);
    LOG_ERROR(msg);
    throw std::runtime_error(msg);
  }
}

}  // namespace net
