#include "../../include/net/epoll.h"
#include "../../include/util/log.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdexcept>
#include <string>

namespace net {

Epoll::Epoll(int max_events)
    : epoll_fd_(-1),
      max_events_(max_events),
      events_(max_events),
      event_count_(0) {
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  CheckError(epoll_fd_, "epoll_create1");
  
  LOG_INFO("Epoll created: fd=" + std::to_string(epoll_fd_) + 
           ", max_events=" + std::to_string(max_events_));
}

Epoll::~Epoll() {
  if (epoll_fd_ != -1) {
    LOG_INFO("Epoll closed: fd=" + std::to_string(epoll_fd_));
    close(epoll_fd_);
  }
}

void Epoll::AddFd(int fd, uint32_t events) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.fd = fd;

  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
  CheckError(ret, "epoll_ctl(EPOLL_CTL_ADD)");
  
  LOG_DEBUG("Epoll add fd=" + std::to_string(fd) + ", events=" + std::to_string(events));
}

void Epoll::ModifyFd(int fd, uint32_t events) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.fd = fd;

  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
  CheckError(ret, "epoll_ctl(EPOLL_CTL_MOD)");
  
  LOG_DEBUG("Epoll modify fd=" + std::to_string(fd) + ", events=" + std::to_string(events));
}

void Epoll::RemoveFd(int fd) {
  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
  CheckError(ret, "epoll_ctl(EPOLL_CTL_DEL)");
  
  LOG_DEBUG("Epoll remove fd=" + std::to_string(fd));
}

int Epoll::Wait(int timeout_ms) {
  event_count_ = epoll_wait(epoll_fd_, events_.data(), max_events_, timeout_ms);
  
  if (event_count_ < 0) {
    if (errno == EINTR) {
      return 0;
    }
    CheckError(event_count_, "epoll_wait");
  }
  
  if (event_count_ > 0) {
    LOG_DEBUG("Epoll wait: " + std::to_string(event_count_) + " events ready");
  }
  
  return event_count_;
}

const struct epoll_event* Epoll::GetEvents() const {
  return events_.data();
}

int Epoll::GetEventCount() const {
  return event_count_;
}

int Epoll::GetFd() const {
  return epoll_fd_;
}

bool Epoll::IsValid() const {
  return epoll_fd_ != -1;
}

void Epoll::CheckError(int ret, const char* operation) {
  if (ret < 0) {
    std::string msg = std::string(operation) + " failed: " + strerror(errno);
    LOG_ERROR(msg);
    throw std::runtime_error(msg);
  }
}

}  // namespace net
