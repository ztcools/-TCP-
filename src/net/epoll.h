#ifndef NET_EPOLL_H
#define NET_EPOLL_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <sys/epoll.h>

namespace net {

/**
 * @brief Epoll 封装类，提供对 Linux epoll 机制的封装
 * 
 * 该类封装了 Linux epoll 的核心功能，支持 ET 边缘触发模式，
 * 用于高效处理大量并发连接的事件通知。
 */
class Epoll {
 public:
  /**
   * @brief 构造函数
   * @param max_events 最大事件数，默认 102400
   */
  explicit Epoll(int max_events = 102400);
  
  /**
   * @brief 析构函数
   */
  ~Epoll();

  // 禁止拷贝和赋值
  Epoll(const Epoll&) = delete;
  Epoll& operator=(const Epoll&) = delete;

  /**
   * @brief 添加文件描述符到 epoll
   * @param fd 文件描述符
   * @param events 事件类型，如 EPOLLIN、EPOLLOUT、EPOLLET 等
   */
  void AddFd(int fd, uint32_t events);
  
  /**
   * @brief 修改文件描述符的事件类型
   * @param fd 文件描述符
   * @param events 新的事件类型
   */
  void ModifyFd(int fd, uint32_t events);
  
  /**
   * @brief 从 epoll 中移除文件描述符
   * @param fd 文件描述符
   */
  void RemoveFd(int fd);

  /**
   * @brief 等待事件发生
   * @param timeout_ms 超时时间（毫秒），-1 表示无限等待
   * @return 就绪事件数量
   */
  int Wait(int timeout_ms = -1);

  /**
   * @brief 获取就绪事件数组
   * @return 就绪事件数组的指针
   */
  const struct epoll_event* GetEvents() const;
  
  /**
   * @brief 获取就绪事件数量
   * @return 就绪事件数量
   */
  int GetEventCount() const;

  /**
   * @brief 获取 epoll 文件描述符
   * @return epoll 文件描述符
   */
  int GetFd() const;
  
  /**
   * @brief 检查 epoll 是否有效
   * @return epoll 是否有效
   */
  bool IsValid() const;

 private:
  /**
   * @brief 检查操作是否出错
   * @param ret 操作返回值
   * @param operation 操作名称
   */
  void CheckError(int ret, const char* operation);

  int epoll_fd_;       // epoll 文件描述符
  int max_events_;     // 最大事件数
  std::vector<struct epoll_event> events_;  // 事件数组
  int event_count_;    // 就绪事件数量
};

}  // namespace net

#endif  // NET_EPOLL_H
