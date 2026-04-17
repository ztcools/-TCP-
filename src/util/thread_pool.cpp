#include "thread_pool.h"

#include <stdexcept>
#include <thread>
#include <chrono>

namespace util {

ThreadPool::ThreadPool(size_t num_threads)
    : stop_(false),
      shutdown_requested_(false),
      num_threads_(num_threads) {
  if (num_threads_ == 0) {
    num_threads_ = std::thread::hardware_concurrency();
    if (num_threads_ == 0) {
      num_threads_ = 4;
    }
    num_threads_ *= 2;
  }

  for (size_t i = 0; i < num_threads_; ++i) {
    workers_.emplace_back(&ThreadPool::WorkerThread, this);
  }
}

ThreadPool::~ThreadPool() {
  Shutdown();
}

void ThreadPool::Shutdown() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (shutdown_requested_) {
      return;
    }
    shutdown_requested_ = true;
  }

  condition_.notify_all();

  for (std::thread& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

size_t ThreadPool::GetThreadCount() const {
  return num_threads_;
}

size_t ThreadPool::GetQueueSize() const {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  return tasks_.size();
}

bool ThreadPool::IsRunning() const {
  return !stop_ && !shutdown_requested_;
}

void ThreadPool::WorkerThread() {
  while (true) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);

      condition_.wait(lock, [this] {
        return stop_ || shutdown_requested_ || !tasks_.empty();
      });

      if ((stop_ || shutdown_requested_) && tasks_.empty()) {
        return;
      }

      task = std::move(tasks_.front());
      tasks_.pop();
    }

    task();
  }
}

}  // namespace util
