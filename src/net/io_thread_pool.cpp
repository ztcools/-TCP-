#include "net/io_thread_pool.h"
#include "util/log.h"
#include <algorithm>

namespace net {

IOThreadPool::IOThreadPool(size_t thread_num) 
    : next_loop_idx_(0) {
  if (thread_num == 0) {
    thread_num = 1;
  }
  
  loops_.reserve(thread_num);
  threads_.reserve(thread_num);
  
  LOG_INFO("Creating IO thread pool with " + std::to_string(thread_num) + " threads");
  
  for (size_t i = 0; i < thread_num; ++i) {
    loops_.emplace_back(std::make_unique<EventLoop>());
  }
}

IOThreadPool::~IOThreadPool() {
  Stop();
}

void IOThreadPool::Start() {
  for (size_t i = 0; i < loops_.size(); ++i) {
    auto& loop = loops_[i];
    if (i == 0 && main_reactor_func_) {
      // 为主Reactor线程调用主Reactor回调函数
      threads_.emplace_back(std::make_unique<std::thread>([this]() {
        this->main_reactor_func_();
      }));
      LOG_INFO("Main reactor thread started");
    } else {
      // 为从Reactor线程调用Loop()方法
      threads_.emplace_back(std::make_unique<std::thread>([loop = loop.get()]() {
        loop->Loop();
      }));
      LOG_INFO("IO thread " + std::to_string(i) + " started");
    }
  }
}

void IOThreadPool::Stop() {
  for (auto& loop : loops_) {
    loop->Stop();
  }
  
  for (auto& thread : threads_) {
    if (thread->joinable()) {
      thread->join();
    }
  }
  
  threads_.clear();
}

EventLoop* IOThreadPool::GetNextEventLoop() {
  if (loops_.empty()) {
    return nullptr;
  }
  
  // 轮询方式获取下一个EventLoop
  EventLoop* loop = loops_[next_loop_idx_].get();
  next_loop_idx_ = (next_loop_idx_ + 1) % loops_.size();
  return loop;
}

}  // namespace net
