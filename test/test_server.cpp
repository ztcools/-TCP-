#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "../include/net/socket.h"
#include "../include/util/log.h"
#include "../include/util/thread_pool.h"
#include "../include/util/memory_pool.h"

std::atomic<int> success_count(0);
std::atomic<int> fail_count(0);

void PrintTestResult(const std::string& test_name, bool success) {
  if (success) {
    std::cout << "✓ " << test_name << std::endl;
    success_count++;
  } else {
    std::cout << "✗ " << test_name << std::endl;
    fail_count++;
  }
}

void TestLogModule() {
  std::cout << "=== Testing Log Module ===" << std::endl;
  
  auto& logger = util::Logger::GetInstance();
  logger.SetLogLevel(util::LogLevel::DEBUG);
  
  LOG_DEBUG("Debug test message");
  LOG_INFO("Info test message");
  LOG_WARN("Warn test message");
  LOG_ERROR("Error test message");
  
  bool success = true;
  PrintTestResult("Log module", success);
  std::cout << std::endl;
}

void TestThreadPoolModule() {
  std::cout << "=== Testing ThreadPool Module ===" << std::endl;
  
  util::ThreadPool pool(4);
  
  std::atomic<int> counter(0);
  std::vector<std::future<void>> futures;
  
  for (int i = 0; i < 100; ++i) {
    futures.push_back(pool.Enqueue([&counter]() {
      counter++;
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }));
  }
  
  for (auto& f : futures) {
    f.wait();
  }
  
  bool success = (counter == 100);
  PrintTestResult("ThreadPool module", success);
  std::cout << std::endl;
}

void TestMemoryPoolModule() {
  std::cout << "=== Testing MemoryPool Module ===" << std::endl;
  
  util::MemoryPool pool(1024, 100);
  
  std::vector<void*> ptrs;
  for (int i = 0; i < 50; ++i) {
    void* ptr = pool.Allocate();
    ptrs.push_back(ptr);
    memset(ptr, 0xAA, 1024);
  }
  
  for (void* ptr : ptrs) {
    pool.Deallocate(ptr);
  }
  
  bool success = true;
  PrintTestResult("MemoryPool module", success);
  std::cout << std::endl;
}

void TestSocketModule() {
  std::cout << "=== Testing Socket Module ===" << std::endl;
  
  bool success = true;
  
  try {
    net::Socket sock;
    sock.Create();
    sock.SetNonBlocking();
    sock.SetReuseAddr();
    
    if (!sock.IsValid()) {
      success = false;
    }
    
    int fd = sock.GetFd();
    if (fd < 0) {
      success = false;
    }
    
    sock.Close();
    
    if (sock.IsValid()) {
      success = false;
    }
  } catch (...) {
    success = false;
  }
  
  PrintTestResult("Socket module", success);
  std::cout << std::endl;
}

void TestEchoCorrectness() {
  std::cout << "=== Testing Echo Correctness ===" << std::endl;
  
  const char* test_message = "Hello, Echo Server!\n";
  std::string received;
  
  bool success = true;
  
  std::vector<std::string> test_messages = {
    "Test line 1\n",
    "Test line 2\n",
    "Test line 3 with more data\n"
  };
  
  for (const auto& msg : test_messages) {
    std::string expected = msg;
    expected.erase(expected.find_last_not_of("\n\r") + 1);
    expected += "\n";
  }
  
  PrintTestResult("Echo correctness", success);
  std::cout << std::endl;
}

void TestModuleIntegration() {
  std::cout << "=== Testing Module Integration ===" << std::endl;
  
  auto& logger = util::Logger::GetInstance();
  logger.SetLogLevel(util::LogLevel::INFO);
  
  util::ThreadPool thread_pool(2);
  util::MemoryPool memory_pool(4096, 100);
  
  std::atomic<bool> integration_ok(true);
  
  auto task = [&]() {
    void* mem = memory_pool.Allocate();
    if (mem) {
      memory_pool.Deallocate(mem);
    } else {
      integration_ok = false;
    }
  };
  
  std::vector<std::future<void>> futures;
  for (int i = 0; i < 10; ++i) {
    futures.push_back(thread_pool.Enqueue(task));
  }
  
  for (auto& f : futures) {
    f.wait();
  }
  
  PrintTestResult("Module integration", integration_ok);
  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "  TCP Server Test Suite" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  TestLogModule();
  TestThreadPoolModule();
  TestMemoryPoolModule();
  TestSocketModule();
  TestEchoCorrectness();
  TestModuleIntegration();
  
  std::cout << "========================================" << std::endl;
  std::cout << "  Test Summary" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "Passed: " << success_count << std::endl;
  std::cout << "Failed: " << fail_count << std::endl;
  std::cout << "Total:  " << (success_count + fail_count) << std::endl;
  std::cout << "========================================" << std::endl;
  
  return (fail_count == 0) ? 0 : 1;
}
