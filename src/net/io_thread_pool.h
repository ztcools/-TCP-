#ifndef NET_IO_THREAD_POOL_H
#define NET_IO_THREAD_POOL_H

#include <vector>
#include <memory>
#include <thread>
#include "net/event_loop.h"

namespace net {

/**
 * @brief IO线程池类，管理从Reactor线程
 * 
 * 每个线程都有一个独立的EventLoop，负责处理已连接客户端的读写事件和业务逻辑
 */
class IOThreadPool {
public:
  /**
   * @brief 构造函数
   * @param thread_num 线程数量
   */
  explicit IOThreadPool(size_t thread_num);
  
  /**
   * @brief 析构函数
   */
  ~IOThreadPool();

  /**
   * @brief 启动线程池
   */
  void Start();

  /**
   * @brief 停止线程池
   */
  void Stop();

  /**
   * @brief 轮询获取下一个EventLoop
   * @return EventLoop指针
   */
  EventLoop* GetNextEventLoop();

  /**
   * @brief 获取指定索引的EventLoop
   * @param index 索引
   * @return EventLoop指针
   */
  EventLoop* GetEventLoop(size_t index) {
    if (index < loops_.size()) {
      return loops_[index].get();
    }
    return nullptr;
  }

  /**
   * @brief 获取线程池大小
   * @return 线程池大小
   */
  size_t Size() const {
    return loops_.size();
  }

private:
  std::vector<std::unique_ptr<EventLoop>> loops_;      // EventLoop列表
  std::vector<std::unique_ptr<std::thread>> threads_;  // 线程列表
  size_t next_loop_idx_;                               // 下一个要使用的EventLoop索引
};

}  // namespace net

#endif  // NET_IO_THREAD_POOL_H
