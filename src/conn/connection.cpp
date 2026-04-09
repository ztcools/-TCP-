#include "../../include/conn/connection.h"
#include "../../include/util/log.h"

#include <algorithm>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

namespace conn {

Connection::Connection(int fd, const std::string &ip, uint16_t port,
                       std::shared_ptr<util::MemoryPool> memory_pool)
    : fd_(fd), ip_(ip), port_(port), state_(ConnectionState::CONNECTING),
      read_idx_(0), write_idx_(0), memory_pool_(std::move(memory_pool)) {
  read_buffer_.reserve(4096);
  write_buffer_.reserve(4096);
  last_heartbeat_ = std::chrono::steady_clock::now();

  LOG_INFO("Connection created: " + ip_ + ":" + std::to_string(port_) +
           ", fd=" + std::to_string(fd_));
}

Connection::~Connection() { Close(); }

int Connection::GetFd() const { return fd_; }

const std::string &Connection::GetIp() const { return ip_; }

uint16_t Connection::GetPort() const { return port_; }

ConnectionState Connection::GetState() const {
  return state_.load(std::memory_order_relaxed);
}

void Connection::SetState(ConnectionState state) {
  state_.store(state, std::memory_order_relaxed);

  const char *state_str;
  switch (state) {
  case ConnectionState::CONNECTING:
    state_str = "CONNECTING";
    break;
  case ConnectionState::CONNECTED:
    state_str = "CONNECTED";
    break;
  case ConnectionState::CLOSED:
    state_str = "CLOSED";
    break;
  default:
    state_str = "UNKNOWN";
    break;
  }

  LOG_DEBUG("Connection state changed: " + ip_ + ":" + std::to_string(port_) +
            " -> " + state_str);
}

void Connection::UpdateHeartbeat() {
  std::lock_guard<std::mutex> lock(mutex_);
  last_heartbeat_ = std::chrono::steady_clock::now();
}

bool Connection::IsTimeout(int64_t timeout_ms) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - last_heartbeat_);
  return duration.count() > timeout_ms;
}

int64_t Connection::GetLastHeartbeatMs() const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - last_heartbeat_);
  return duration.count();
}

ssize_t Connection::Recv() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (state_.load(std::memory_order_relaxed) == ConnectionState::CLOSED) {
    return -1;
  }

  CheckAndResizeReadBuffer();

  size_t available = read_buffer_.size() - read_idx_;
  ssize_t ret = recv(fd_, read_buffer_.data() + read_idx_, available, 0);

  if (ret > 0) {
    read_idx_ += ret;
    last_heartbeat_ = std::chrono::steady_clock::now();
    LOG_DEBUG("Connection recv: " + ip_ + ":" + std::to_string(port_) +
              ", bytes=" + std::to_string(ret));
  } else if (ret == 0) {
    LOG_INFO("Connection closed by peer: " + ip_ + ":" + std::to_string(port_));
    state_.store(ConnectionState::CLOSED, std::memory_order_relaxed);
  } else {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG_ERROR("Connection recv error: " + ip_ + ":" + std::to_string(port_) +
                ", errno=" + std::to_string(errno) + " (" + strerror(errno) +
                ")");
      state_.store(ConnectionState::CLOSED, std::memory_order_relaxed);
    }
  }

  return ret;
}

ssize_t Connection::Send() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (state_.load(std::memory_order_relaxed) == ConnectionState::CLOSED) {
    return -1;
  }

  if (write_idx_ == 0) {
    return 0;
  }

  ssize_t ret = send(fd_, write_buffer_.data(), write_idx_, 0);

  if (ret > 0) {
    memmove(write_buffer_.data(), write_buffer_.data() + ret, write_idx_ - ret);
    write_idx_ -= ret;
    last_heartbeat_ = std::chrono::steady_clock::now();
    LOG_DEBUG("Connection send: " + ip_ + ":" + std::to_string(port_) +
              ", bytes=" + std::to_string(ret));
  } else if (ret == 0) {
    LOG_INFO("Connection closed by peer: " + ip_ + ":" + std::to_string(port_));
    state_.store(ConnectionState::CLOSED, std::memory_order_relaxed);
  } else {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG_ERROR("Connection send error: " + ip_ + ":" + std::to_string(port_) +
                ", errno=" + std::to_string(errno) + " (" + strerror(errno) +
                ")");
      state_.store(ConnectionState::CLOSED, std::memory_order_relaxed);
    }
  }

  return ret;
}

bool Connection::HasPendingData() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return write_idx_ > 0;
}

size_t Connection::GetReadBufferSize() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return read_idx_;
}

size_t Connection::GetWriteBufferSize() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return write_idx_;
}

const char *Connection::GetReadBuffer() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return read_buffer_.data();
}

void Connection::ConsumeReadBuffer(size_t len) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (len >= read_idx_) {
    read_idx_ = 0;
  } else {
    memmove(read_buffer_.data(), read_buffer_.data() + len, read_idx_ - len);
    read_idx_ -= len;
  }
}

void Connection::AppendWriteBuffer(const char *data, size_t len) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (write_buffer_.size() < write_idx_ + len) {
    size_t new_size = std::max(write_buffer_.size() * 2, write_idx_ + len);
    write_buffer_.resize(new_size);
  }

  memcpy(write_buffer_.data() + write_idx_, data, len);
  write_idx_ += len;

  LOG_DEBUG("Connection append write buffer: " + ip_ + ":" +
            std::to_string(port_) + ", bytes=" + std::to_string(len) +
            ", total=" + std::to_string(write_idx_));
}

void Connection::Close() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (state_.load(std::memory_order_relaxed) != ConnectionState::CLOSED) {
    state_.store(ConnectionState::CLOSED, std::memory_order_relaxed);

    if (fd_ != -1) {
      LOG_INFO("Connection closed: " + ip_ + ":" + std::to_string(port_) +
               ", fd=" + std::to_string(fd_));
      close(fd_);
      fd_ = -1;
    }
  }
}

void Connection::CheckAndResizeReadBuffer() {
  if (read_buffer_.size() - read_idx_ < 1024) {
    size_t new_size = std::max(read_buffer_.size() * 2, read_idx_ + 4096);
    read_buffer_.resize(new_size);
  }
}

void Connection::CheckAndResizeWriteBuffer() {
  if (write_buffer_.size() - write_idx_ < 1024) {
    size_t new_size = std::max(write_buffer_.size() * 2, write_idx_ + 4096);
    write_buffer_.resize(new_size);
  }
}

} // namespace conn
