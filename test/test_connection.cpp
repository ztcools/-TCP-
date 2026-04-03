#include "../include/conn/connection.h"
#include "../include/util/log.h"
#include "../include/util/memory_pool.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cstring>

void TestConnectionBasic() {
  std::cout << "=== Test Connection Basic ===" << std::endl;
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  conn::Connection conn(-1, "127.0.0.1", 8080, memory_pool);
  
  std::cout << "Connection ip: " << conn.GetIp() << std::endl;
  std::cout << "Connection port: " << conn.GetPort() << std::endl;
  std::cout << "Connection fd: " << conn.GetFd() << std::endl;
  
  std::cout << std::endl;
}

void TestConnectionState() {
  std::cout << "=== Test Connection State ===" << std::endl;
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  conn::Connection conn(-1, "192.168.1.1", 12345, memory_pool);
  
  std::cout << "Initial state: ";
  switch (conn.GetState()) {
    case conn::ConnectionState::CONNECTING: std::cout << "CONNECTING"; break;
    case conn::ConnectionState::CONNECTED: std::cout << "CONNECTED"; break;
    case conn::ConnectionState::CLOSED: std::cout << "CLOSED"; break;
  }
  std::cout << std::endl;
  
  conn.SetState(conn::ConnectionState::CONNECTED);
  std::cout << "After SetState(CONNECTED): ";
  switch (conn.GetState()) {
    case conn::ConnectionState::CONNECTING: std::cout << "CONNECTING"; break;
    case conn::ConnectionState::CONNECTED: std::cout << "CONNECTED"; break;
    case conn::ConnectionState::CLOSED: std::cout << "CLOSED"; break;
  }
  std::cout << std::endl;
  
  conn.SetState(conn::ConnectionState::CLOSED);
  std::cout << "After SetState(CLOSED): ";
  switch (conn.GetState()) {
    case conn::ConnectionState::CONNECTING: std::cout << "CONNECTING"; break;
    case conn::ConnectionState::CONNECTED: std::cout << "CONNECTED"; break;
    case conn::ConnectionState::CLOSED: std::cout << "CLOSED"; break;
  }
  std::cout << std::endl;
  
  std::cout << std::endl;
}

void TestConnectionHeartbeat() {
  std::cout << "=== Test Connection Heartbeat ===" << std::endl;
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  conn::Connection conn(-1, "10.0.0.1", 54321, memory_pool);
  
  conn.UpdateHeartbeat();
  std::cout << "Heartbeat updated" << std::endl;
  std::cout << "Time since last heartbeat: " << conn.GetLastHeartbeatMs() << "ms" << std::endl;
  
  std::cout << "Is timeout (100ms): " << (conn.IsTimeout(100) ? "yes" : "no") << std::endl;
  std::cout << "Is timeout (1ms): " << (conn.IsTimeout(1) ? "yes" : "no") << std::endl;
  
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  std::cout << "After 50ms - Is timeout (100ms): " << (conn.IsTimeout(100) ? "yes" : "no") << std::endl;
  std::cout << "After 50ms - Is timeout (30ms): " << (conn.IsTimeout(30) ? "yes" : "no") << std::endl;
  
  std::cout << std::endl;
}

void TestConnectionBuffers() {
  std::cout << "=== Test Connection Buffers ===" << std::endl;
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  conn::Connection conn(-1, "172.16.0.1", 9999, memory_pool);
  
  std::cout << "Initial read buffer size: " << conn.GetReadBufferSize() << std::endl;
  std::cout << "Initial write buffer size: " << conn.GetWriteBufferSize() << std::endl;
  std::cout << "Has pending data: " << (conn.HasPendingData() ? "yes" : "no") << std::endl;
  
  const char* test_data = "Hello, Connection!";
  conn.AppendWriteBuffer(test_data, strlen(test_data));
  std::cout << "After append write buffer - size: " << conn.GetWriteBufferSize() << std::endl;
  std::cout << "Has pending data: " << (conn.HasPendingData() ? "yes" : "no") << std::endl;
  
  conn.AppendWriteBuffer("More data", 9);
  std::cout << "After second append - write buffer size: " << conn.GetWriteBufferSize() << std::endl;
  
  std::cout << std::endl;
}

void TestConnectionClose() {
  std::cout << "=== Test Connection Close ===" << std::endl;
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  conn::Connection conn(-1, "127.0.0.1", 10000, memory_pool);
  
  conn.SetState(conn::ConnectionState::CONNECTED);
  std::cout << "State before close: ";
  switch (conn.GetState()) {
    case conn::ConnectionState::CONNECTING: std::cout << "CONNECTING"; break;
    case conn::ConnectionState::CONNECTED: std::cout << "CONNECTED"; break;
    case conn::ConnectionState::CLOSED: std::cout << "CLOSED"; break;
  }
  std::cout << std::endl;
  
  conn.Close();
  std::cout << "State after close: ";
  switch (conn.GetState()) {
    case conn::ConnectionState::CONNECTING: std::cout << "CONNECTING"; break;
    case conn::ConnectionState::CONNECTED: std::cout << "CONNECTED"; break;
    case conn::ConnectionState::CLOSED: std::cout << "CLOSED"; break;
  }
  std::cout << std::endl;
  
  std::cout << std::endl;
}

void TestMultipleConnections() {
  std::cout << "=== Test Multiple Connections ===" << std::endl;
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  
  std::vector<std::unique_ptr<conn::Connection>> connections;
  
  for (int i = 0; i < 5; ++i) {
    std::string ip = "192.168.0." + std::to_string(i + 1);
    uint16_t port = 8000 + i;
    connections.push_back(std::make_unique<conn::Connection>(
        -1, ip, port, memory_pool));
  }
  
  std::cout << "Created " << connections.size() << " connections" << std::endl;
  
  for (const auto& conn : connections) {
    std::cout << "  " << conn->GetIp() << ":" << conn->GetPort() << std::endl;
  }
  
  for (auto& conn : connections) {
    conn->Close();
  }
  
  std::cout << std::endl;
}

void TestConnectionThreadSafety() {
  std::cout << "=== Test Connection Thread Safety ===" << std::endl;
  
  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  auto conn = std::make_shared<conn::Connection>(-1, "127.0.0.1", 11111, memory_pool);
  
  conn->SetState(conn::ConnectionState::CONNECTED);
  
  std::vector<std::thread> threads;
  
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back([conn, i]() {
      for (int j = 0; j < 20; ++j) {
        conn->UpdateHeartbeat();
        std::string data = "Data from thread " + std::to_string(i) + " " + std::to_string(j);
        conn->AppendWriteBuffer(data.c_str(), data.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  std::cout << "Final write buffer size: " << conn->GetWriteBufferSize() << std::endl;
  std::cout << "Last heartbeat: " << conn->GetLastHeartbeatMs() << "ms ago" << std::endl;
  
  conn->Close();
  
  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "   TCP Server Connection Test Suite     " << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  util::Logger::GetInstance().SetLogLevel(util::LogLevel::DEBUG);
  
  TestConnectionBasic();
  TestConnectionState();
  TestConnectionHeartbeat();
  TestConnectionBuffers();
  TestConnectionClose();
  TestMultipleConnections();
  TestConnectionThreadSafety();
  
  std::cout << "========================================" << std::endl;
  std::cout << "     All Tests Completed Successfully  " << std::endl;
  std::cout << "========================================" << std::endl;
  
  return 0;
}
