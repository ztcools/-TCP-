#include "net/epoll.h"
#include "util/log.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdexcept>
#include <string>

namespace net {

/**
 * @brief 构造函数，初始化 epoll 实例
 * 
 * @param max_events 最大事件数
 * 
 * 步骤：
 * 1. 初始化成员变量
 * 2. 调用 epoll_create1 创建 epoll 实例，使用 EPOLL_CLOEXEC 标志
 * 3. 检查创建是否成功
 * 4. 记录日志
 */
Epoll::Epoll(int max_events)
    : epoll_fd_(-1),
      max_events_(max_events),
      events_(max_events),
      event_count_(0) {
  // 创建 epoll 实例，使用 EPOLL_CLOEXEC 标志自动关闭文件描述符
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  CheckError(epoll_fd_, "epoll_create1");
  
  LOG_INFO("Epoll created: fd=" + std::to_string(epoll_fd_) + 
           ", max_events=" + std::to_string(max_events_));
}

/**
 * @brief 析构函数，关闭 epoll 文件描述符
 */
Epoll::~Epoll() {
  if (epoll_fd_ != -1) {
    LOG_INFO("Epoll closed: fd=" + std::to_string(epoll_fd_));
    close(epoll_fd_);
  }
}

/**
 * @brief 添加文件描述符到 epoll
 * 
 * @param fd 文件描述符
 * @param events 事件类型
 * 
 * 步骤：
 * 1. 初始化 epoll_event 结构体
 * 2. 设置事件类型和文件描述符
 * 3. 调用 epoll_ctl 添加文件描述符
 * 4. 检查操作是否成功
 * 5. 记录日志
 */
void Epoll::AddFd(int fd, uint32_t events) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.fd = fd;

  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
  CheckError(ret, "epoll_ctl(EPOLL_CTL_ADD)");
  
  LOG_DEBUG("Epoll add fd=" + std::to_string(fd) + ", events=" + std::to_string(events));
}

/**
 * @brief 修改文件描述符的事件类型
 * 
 * @param fd 文件描述符
 * @param events 新的事件类型
 * 
 * 步骤：
 * 1. 初始化 epoll_event 结构体
 * 2. 设置新的事件类型和文件描述符
 * 3. 调用 epoll_ctl 修改文件描述符
 * 4. 检查操作是否成功
 * 5. 记录日志
 */
void Epoll::ModifyFd(int fd, uint32_t events) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.fd = fd;

  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
  CheckError(ret, "epoll_ctl(EPOLL_CTL_MOD)");
  
  LOG_DEBUG("Epoll modify fd=" + std::to_string(fd) + ", events=" + std::to_string(events));
}

/**
 * @brief 从 epoll 中移除文件描述符
 * 
 * @param fd 文件描述符
 * 
 * 步骤：
 * 1. 调用 epoll_ctl 移除文件描述符
 * 2. 检查操作是否成功
 * 3. 记录日志
 */
void Epoll::RemoveFd(int fd) {
  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
  CheckError(ret, "epoll_ctl(EPOLL_CTL_DEL)");
  
  LOG_DEBUG("Epoll remove fd=" + std::to_string(fd));
}

/**
 * @brief 等待事件发生
 * 
 * @param timeout_ms 超时时间（毫秒）
 * @return 就绪事件数量
 * 
 * 步骤：
 * 1. 调用 epoll_wait 等待事件
 * 2. 处理中断信号
 * 3. 检查操作是否成功
 * 4. 记录日志
 * 5. 返回就绪事件数量
 */
int Epoll::Wait(int timeout_ms) {
  event_count_ = epoll_wait(epoll_fd_, events_.data(), max_events_, timeout_ms);
  
  if (event_count_ < 0) {
    if (errno == EINTR) {
      // 处理中断信号，返回 0 表示没有就绪事件
      return 0;
    }
    CheckError(event_count_, "epoll_wait");
  }
  
  if (event_count_ > 0) {
    LOG_DEBUG("Epoll wait: " + std::to_string(event_count_) + " events ready");
  }
  
  return event_count_;
}

/**
 * @brief 获取就绪事件数组
 * 
 * @return 就绪事件数组的指针
 */
const struct epoll_event* Epoll::GetEvents() const {
  return events_.data();
}

/**
 * @brief 获取就绪事件数量
 * 
 * @return 就绪事件数量
 */
int Epoll::GetEventCount() const {
  return event_count_;
}

/**
 * @brief 获取 epoll 文件描述符
 * 
 * @return epoll 文件描述符
 */
int Epoll::GetFd() const {
  return epoll_fd_;
}

/**
 * @brief 检查 epoll 是否有效
 * 
 * @return epoll 是否有效
 */
bool Epoll::IsValid() const {
  return epoll_fd_ != -1;
}

/**
 * @brief 检查操作是否出错
 * 
 * @param ret 操作返回值
 * @param operation 操作名称
 * 
 * 步骤：
 * 1. 检查返回值是否小于 0
 * 2. 如果出错，构造错误消息
 * 3. 记录错误日志
 * 4. 抛出运行时异常
 */
void Epoll::CheckError(int ret, const char* operation) {
  if (ret < 0) {
    std::string msg = std::string(operation) + " failed: " + strerror(errno);
    LOG_ERROR(msg);
    throw std::runtime_error(msg);
  }
}

}  // namespace net
