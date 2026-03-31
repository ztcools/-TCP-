#include "../include/util/thread_pool.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>

void TestBasicTask() {
  std::cout << "=== Test Basic Task Execution ===" << std::endl;
  
  util::ThreadPool pool(4);
  
  auto result = pool.Enqueue([]() {
    std::cout << "Task executed in thread: " << std::this_thread::get_id() << std::endl;
    return 42;
  });
  
  std::cout << "Result: " << result.get() << std::endl;
  std::cout << std::endl;
}

void TestMultipleTasks() {
  std::cout << "=== Test Multiple Tasks ===" << std::endl;
  
  util::ThreadPool pool(4);
  std::vector<std::future<int>> results;
  
  for (int i = 0; i < 10; ++i) {
    results.emplace_back(pool.Enqueue([i]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      return i * i;
    }));
  }
  
  std::cout << "Results: ";
  int sum = 0;
  for (auto& result : results) {
    int value = result.get();
    std::cout << value << " ";
    sum += value;
  }
  std::cout << std::endl;
  std::cout << "Sum: " << sum << std::endl;
  std::cout << std::endl;
}

void TestTaskWithArguments() {
  std::cout << "=== Test Task With Arguments ===" << std::endl;
  
  util::ThreadPool pool(4);
  
  auto task = [](int a, int b, int c) {
    return a + b + c;
  };
  
  auto result = pool.Enqueue(task, 10, 20, 30);
  std::cout << "Sum(10, 20, 30) = " << result.get() << std::endl;
  std::cout << std::endl;
}

void TestConcurrentExecution() {
  std::cout << "=== Test Concurrent Execution ===" << std::endl;
  
  util::ThreadPool pool(4);
  std::vector<std::future<std::thread::id>> futures;
  
  for (int i = 0; i < 8; ++i) {
    futures.emplace_back(pool.Enqueue([]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      return std::this_thread::get_id();
    }));
  }
  
  std::cout << "Tasks executed in threads: " << std::endl;
  std::vector<std::thread::id> thread_ids;
  for (auto& future : futures) {
    thread_ids.push_back(future.get());
  }
  
  std::sort(thread_ids.begin(), thread_ids.end());
  auto unique_end = std::unique(thread_ids.begin(), thread_ids.end());
  size_t unique_count = std::distance(thread_ids.begin(), unique_end);
  
  std::cout << "Used " << unique_count << " different threads" << std::endl;
  std::cout << std::endl;
}

void TestGracefulShutdown() {
  std::cout << "=== Test Graceful Shutdown ===" << std::endl;
  
  {
    util::ThreadPool pool(4);
    
    for (int i = 0; i < 5; ++i) {
      pool.Enqueue([i]() {
        std::cout << "Task " << i << " starting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Task " << i << " completed" << std::endl;
      });
    }
    
    std::cout << "Tasks enqueued, waiting for completion..." << std::endl;
  }
  
  std::cout << "ThreadPool destroyed, all tasks completed" << std::endl;
  std::cout << std::endl;
}

void TestDefaultThreadCount() {
  std::cout << "=== Test Default Thread Count ===" << std::endl;
  
  util::ThreadPool pool;
  
  std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;
  std::cout << "Thread pool size: " << pool.GetThreadCount() << std::endl;
  std::cout << std::endl;
}

void TestQueueSize() {
  std::cout << "=== Test Queue Size ===" << std::endl;
  
  util::ThreadPool pool(2);
  
  std::cout << "Initial queue size: " << pool.GetQueueSize() << std::endl;
  
  std::vector<std::future<void>> futures;
  for (int i = 0; i < 10; ++i) {
    futures.emplace_back(pool.Enqueue([i]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }));
  }
  
  std::cout << "After enqueueing 10 tasks: " << pool.GetQueueSize() << std::endl;
  
  for (auto& future : futures) {
    future.get();
  }
  
  std::cout << "After completion: " << pool.GetQueueSize() << std::endl;
  std::cout << std::endl;
}

void TestExceptionHandling() {
  std::cout << "=== Test Exception Handling ===" << std::endl;
  
  util::ThreadPool pool(4);
  
  auto future = pool.Enqueue([]() {
    throw std::runtime_error("Test exception");
  });
  
  try {
    future.get();
    std::cout << "No exception thrown" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Caught exception: " << e.what() << std::endl;
  }
  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "     TCP Server ThreadPool Test Suite  " << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  TestBasicTask();
  TestMultipleTasks();
  TestTaskWithArguments();
  TestConcurrentExecution();
  TestGracefulShutdown();
  TestDefaultThreadCount();
  TestQueueSize();
  TestExceptionHandling();
  
  std::cout << "========================================" << std::endl;
  std::cout << "     All Tests Completed Successfully  " << std::endl;
  std::cout << "========================================" << std::endl;
  
  return 0;
}
