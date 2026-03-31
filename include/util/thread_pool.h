#ifndef UTIL_THREAD_POOL_H
#define UTIL_THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

namespace util {

class ThreadPool {
 public:
  explicit ThreadPool(size_t num_threads = 0);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  template<typename F, typename... Args>
  auto Enqueue(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type>;

  void Shutdown();

  size_t GetThreadCount() const;
  size_t GetQueueSize() const;
  bool IsRunning() const;

 private:
  void WorkerThread();

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;

  mutable std::mutex queue_mutex_;
  std::condition_variable condition_;

  std::atomic<bool> stop_;
  std::atomic<bool> shutdown_requested_;
  size_t num_threads_;
};

template<typename F, typename... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
  using return_type = typename std::result_of<F(Args...)>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...)
  );

  std::future<return_type> result = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    if (stop_ || shutdown_requested_) {
      throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
    }

    tasks_.emplace([task]() { (*task)(); });
  }

  condition_.notify_one();
  return result;
}

}  // namespace util

#endif  // UTIL_THREAD_POOL_H
