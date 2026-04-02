#ifndef NET_EPOLL_H
#define NET_EPOLL_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <sys/epoll.h>

namespace net {

class Epoll {
 public:
  explicit Epoll(int max_events = 102400);
  ~Epoll();

  Epoll(const Epoll&) = delete;
  Epoll& operator=(const Epoll&) = delete;

  void AddFd(int fd, uint32_t events);
  void ModifyFd(int fd, uint32_t events);
  void RemoveFd(int fd);

  int Wait(int timeout_ms = -1);

  const struct epoll_event* GetEvents() const;
  int GetEventCount() const;

  int GetFd() const;
  bool IsValid() const;

 private:
  void CheckError(int ret, const char* operation);

  int epoll_fd_;
  int max_events_;
  std::vector<struct epoll_event> events_;
  int event_count_;
};

}  // namespace net

#endif  // NET_EPOLL_H
