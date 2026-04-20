#ifndef NET_EVENT_LOOP_H
#define NET_EVENT_LOOP_H

#include <memory>
#include <thread>
#include <atomic>
#include "net/epoll.h"

namespace net {

/**
 * @brief 事件循环类，每个线程一个
 * 
 * 负责管理一个线程的事件循环，包括epoll的创建、事件的注册和处理
 */
class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  /**
   * @brief 启动事件循环
   */
  void Loop();

  /**
   * @brief 停止事件循环
   */
  void Stop();

  /**
   * @brief 检查当前线程是否是创建此EventLoop的线程
   * @return 是否是创建此EventLoop的线程
   */
  bool IsInLoopThread() const;

  /**
   * @brief 添加文件描述符到epoll
   * @param fd 文件描述符
   * @param events 事件类型
   */
  void AddFd(int fd, uint32_t events);

  /**
   * @brief 修改文件描述符的事件
   * @param fd 文件描述符
   * @param events 事件类型
   */
  void ModifyFd(int fd, uint32_t events);

  /**
   * @brief 从epoll中移除文件描述符
   * @param fd 文件描述符
   */
  void RemoveFd(int fd);

  /**
   * @brief 获取epoll实例
   * @return epoll实例
   */
  Epoll* GetEpoll() {
    return epoll_.get();
  }

  /**
   * @brief 获取当前线程ID
   * @return 线程ID
   */
  std::thread::id GetThreadId() const {
    return thread_id_;
  }

  /**
   * @brief 设置是否为主Reactor
   * @param is_main 是否为主Reactor
   */
  void SetMainReactor(bool is_main) {
    is_main_reactor_ = is_main;
  }

  /**
   * @brief 检查是否为主Reactor
   * @return 是否为主Reactor
   */
  bool IsMainReactor() const {
    return is_main_reactor_;
  }

private:
  /**
   * @brief 处理事件
   */
  void HandleEvents();

  /**
   * @brief 处理读事件
   * @param fd 文件描述符
   */
  void HandleRead(int fd);

  /**
   * @brief 处理写事件
   * @param fd 文件描述符
   */
  void HandleWrite(int fd);

  /**
   * @brief 处理错误事件
   * @param fd 文件描述符
   */
  void HandleError(int fd);

  std::unique_ptr<Epoll> epoll_;       // epoll实例
  std::atomic<bool> running_;           // 运行状态
  std::thread::id thread_id_;           // 创建此EventLoop的线程ID
  bool is_main_reactor_;                // 是否为主Reactor
  int wakeup_fd_;                       // wakeup fd，用于 Stop() 时中断 epoll_wait
};

}  // namespace net

#endif  // NET_EVENT_LOOP_H
