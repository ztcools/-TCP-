#include "../include/conn/connection_pool.h"
#include "../include/conn/connection.h"
#include "../include/util/log.h"
#include "../include/util/memory_pool.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>

void TestConnectionPoolBasic() {
  std::cout << "=== Test ConnectionPool Basic ===" << std::endl;
  
  auto& pool = conn::ConnectionPool::GetInstance();
  pool.Init(1000, 60000);
  
  std::cout << "Max connections: " << pool.GetMaxConnections() << std::endl;
  std::cout << "Connection count: " << pool.GetConnectionCount() << std::endl;
  
  pool.Shutdown();
  
  std::cout << std::endl;
}

void TestConnectionPoolAddRemove() {
  std::cout << "=== Test ConnectionPool Add/Remove ===" << std::endl;
  
  auto& pool = conn::ConnectionPool::GetInstance();
  pool.Init(1000, 60000);
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  
  std::cout << "Before add - count: " << pool.GetConnectionCount() << std::endl;
  
  for (int i = 0; i < 10; ++i) {
    auto conn = std::make_shared<conn::Connection>(
        100 + i, "192.168.1." + std::to_string(i + 1), 8000 + i, memory_pool);
    conn->SetState(conn::ConnectionState::CONNECTED);
    
    bool added = pool.AddConnection(conn);
    std::cout << "  Add fd=" << (100 + i) << ": " << (added ? "success" : "failed") << std::endl;
  }
  
  std::cout << "After add - count: " << pool.GetConnectionCount() << std::endl;
  
  for (int i = 0; i < 5; ++i) {
    bool removed = pool.RemoveConnection(100 + i);
    std::cout << "  Remove fd=" << (100 + i) << ": " << (removed ? "success" : "failed") << std::endl;
  }
  
  std::cout << "After remove - count: " << pool.GetConnectionCount() << std::endl;
  
  pool.Shutdown();
  
  std::cout << std::endl;
}

void TestConnectionPoolGet() {
  std::cout << "=== Test ConnectionPool Get ===" << std::endl;
  
  auto& pool = conn::ConnectionPool::GetInstance();
  pool.Init(1000, 60000);
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  
  auto conn1 = std::make_shared<conn::Connection>(200, "10.0.0.1", 9000, memory_pool);
  auto conn2 = std::make_shared<conn::Connection>(201, "10.0.0.2", 9001, memory_pool);
  
  pool.AddConnection(conn1);
  pool.AddConnection(conn2);
  
  auto get1 = pool.GetConnection(200);
  auto get2 = pool.GetConnection(201);
  auto get3 = pool.GetConnection(999);
  
  std::cout << "Get fd=200: " << (get1 ? "found" : "not found") << std::endl;
  if (get1) {
    std::cout << "  IP: " << get1->GetIp() << ", Port: " << get1->GetPort() << std::endl;
  }
  
  std::cout << "Get fd=201: " << (get2 ? "found" : "not found") << std::endl;
  std::cout << "Get fd=999: " << (get3 ? "found" : "not found") << std::endl;
  
  pool.Shutdown();
  
  std::cout << std::endl;
}

void TestConnectionPoolMaxLimit() {
  std::cout << "=== Test ConnectionPool Max Limit ===" << std::endl;
  
  auto& pool = conn::ConnectionPool::GetInstance();
  pool.Init(5, 60000);
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  
  int success_count = 0;
  for (int i = 0; i < 10; ++i) {
    auto conn = std::make_shared<conn::Connection>(
        300 + i, "172.16.0." + std::to_string(i + 1), 7000 + i, memory_pool);
    
    if (pool.AddConnection(conn)) {
      success_count++;
    }
  }
  
  std::cout << "Max connections: " << pool.GetMaxConnections() << std::endl;
  std::cout << "Successfully added: " << success_count << std::endl;
  std::cout << "Current count: " << pool.GetConnectionCount() << std::endl;
  
  pool.Shutdown();
  
  std::cout << std::endl;
}

void TestConnectionPoolDuplicateFd() {
  std::cout << "=== Test ConnectionPool Duplicate FD ===" << std::endl;
  
  auto& pool = conn::ConnectionPool::GetInstance();
  pool.Init(1000, 60000);
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  
  auto conn1 = std::make_shared<conn::Connection>(400, "1.1.1.1", 1000, memory_pool);
  auto conn2 = std::make_shared<conn::Connection>(400, "2.2.2.2", 2000, memory_pool);
  
  bool add1 = pool.AddConnection(conn1);
  bool add2 = pool.AddConnection(conn2);
  
  std::cout << "Add first fd=400: " << (add1 ? "success" : "failed") << std::endl;
  std::cout << "Add duplicate fd=400: " << (add2 ? "success" : "failed") << std::endl;
  std::cout << "Connection count: " << pool.GetConnectionCount() << std::endl;
  
  pool.Shutdown();
  
  std::cout << std::endl;
}

void TestConnectionPoolShutdown() {
  std::cout << "=== Test ConnectionPool Shutdown ===" << std::endl;
  
  auto& pool = conn::ConnectionPool::GetInstance();
  pool.Init(1000, 60000);
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  
  for (int i = 0; i < 20; ++i) {
    auto conn = std::make_shared<conn::Connection>(
        500 + i, "192.168.100." + std::to_string(i + 1), 6000 + i, memory_pool);
    pool.AddConnection(conn);
  }
  
  std::cout << "Before shutdown - count: " << pool.GetConnectionCount() << std::endl;
  
  pool.Shutdown();
  
  std::cout << "After shutdown - count: " << pool.GetConnectionCount() << std::endl;
  
  auto conn = std::make_shared<conn::Connection>(9999, "0.0.0.0", 0, memory_pool);
  bool added = pool.AddConnection(conn);
  std::cout << "Add after shutdown: " << (added ? "success" : "failed (expected)") << std::endl;
  
  std::cout << std::endl;
}

void TestConnectionPoolThreadSafety() {
  std::cout << "=== Test ConnectionPool Thread Safety ===" << std::endl;
  
  auto& pool = conn::ConnectionPool::GetInstance();
  pool.Init(10000, 60000);
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 10000);
  
  std::vector<std::thread> threads;
  std::atomic<int> add_count(0);
  std::atomic<int> remove_count(0);
  
  for (int t = 0; t < 10; ++t) {
    threads.emplace_back([&pool, &memory_pool, t, &add_count, &remove_count]() {
      for (int i = 0; i < 100; ++i) {
        int fd = t * 1000 + i + 6000;
        
        auto conn = std::make_shared<conn::Connection>(
            fd, "10.10." + std::to_string(t) + "." + std::to_string(i),
            5000 + fd, memory_pool);
        
        if (pool.AddConnection(conn)) {
          add_count++;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        
        if (pool.RemoveConnection(fd)) {
          remove_count++;
        }
      }
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  std::cout << "Add count: " << add_count << std::endl;
  std::cout << "Remove count: " << remove_count << std::endl;
  std::cout << "Final connection count: " << pool.GetConnectionCount() << std::endl;
  
  pool.Shutdown();
  
  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "  TCP Server ConnectionPool Test Suite  " << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  util::Logger::GetInstance().SetLogLevel(util::LogLevel::DEBUG);
  
  TestConnectionPoolBasic();
  TestConnectionPoolAddRemove();
  TestConnectionPoolGet();
  TestConnectionPoolMaxLimit();
  TestConnectionPoolDuplicateFd();
  TestConnectionPoolShutdown();
  TestConnectionPoolThreadSafety();
  
  std::cout << "========================================" << std::endl;
  std::cout << "     All Tests Completed Successfully  " << std::endl;
  std::cout << "========================================" << std::endl;
  
  return 0;
}
