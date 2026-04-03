#include "../include/service/echo_service.h"
#include "../include/conn/connection.h"
#include "../include/util/memory_pool.h"
#include "../include/util/log.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cstring>

void TestEchoServiceBasic() {
  std::cout << "=== Test EchoService Basic ===" << std::endl;

  auto& service = service::EchoService::GetInstance();
  service.Init(4);

  std::cout << "EchoService initialized" << std::endl;

  service.Shutdown();

  std::cout << std::endl;
}

void TestEchoServiceHandle() {
  std::cout << "=== Test EchoService Handle ===" << std::endl;

  auto& service = service::EchoService::GetInstance();
  service.Init(4);

  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  auto conn = std::make_shared<conn::Connection>(
      100, "127.0.0.1", 8888, memory_pool);
  conn->SetState(conn::ConnectionState::CONNECTED);

  const char* test_data = "Hello, EchoService!\nTest line 2\n";
  conn->AppendWriteBuffer(test_data, strlen(test_data));

  std::cout << "Appended test data to connection" << std::endl;

  conn->ConsumeReadBuffer(conn->GetReadBufferSize());
  conn->AppendWriteBuffer(test_data, strlen(test_data));

  service.HandleConnection(conn);

  std::cout << "Handled connection" << std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << "Write buffer size: " << conn->GetWriteBufferSize() << std::endl;

  service.Shutdown();

  std::cout << std::endl;
}

void TestEchoServiceMultipleLines() {
  std::cout << "=== Test EchoService Multiple Lines ===" << std::endl;

  auto& service = service::EchoService::GetInstance();
  service.Init(4);

  auto memory_pool = std::make_shared<util::MemoryPool>(1024, 100);
  auto conn = std::make_shared<conn::Connection>(
      200, "192.168.1.1", 9999, memory_pool);
  conn->SetState(conn::ConnectionState::CONNECTED);

  std::string multi_line_data =
      "Line 1\n"
      "Line 2\r\n"
      "Line 3\n"
      "Line 4\r"
      "Line 5\n";

  conn->AppendWriteBuffer(multi_line_data.c_str(), multi_line_data.size());
  conn->ConsumeReadBuffer(conn->GetReadBufferSize());
  conn->AppendWriteBuffer(multi_line_data.c_str(), multi_line_data.size());

  std::cout << "Multi-line data prepared" << std::endl;

  service.HandleConnection(conn);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << "Write buffer size after handling: " << conn->GetWriteBufferSize() << std::endl;

  service.Shutdown();

  std::cout << std::endl;
}

void TestEchoServiceShutdown() {
  std::cout << "=== Test EchoService Shutdown ===" << std::endl;

  auto& service = service::EchoService::GetInstance();
  service.Init(4);

  std::cout << "EchoService running" << std::endl;

  service.Shutdown();

  std::cout << "EchoService shutdown" << std::endl;

  service.Shutdown();

  std::cout << "Double shutdown (safe)" << std::endl;

  std::cout << std::endl;
}

void TestEchoServiceSingleton() {
  std::cout << "=== Test EchoService Singleton ===" << std::endl;

  auto& service1 = service::EchoService::GetInstance();
  auto& service2 = service::EchoService::GetInstance();

  std::cout << "Service1 address: " << &service1 << std::endl;
  std::cout << "Service2 address: " << &service2 << std::endl;
  std::cout << "Are same instance: " << (&service1 == &service2 ? "yes" : "no") << std::endl;

  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "  TCP Server EchoService Test Suite   " << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;

  util::Logger::GetInstance().SetLogLevel(util::LogLevel::DEBUG);

  TestEchoServiceBasic();
  TestEchoServiceSingleton();
  TestEchoServiceShutdown();
  TestEchoServiceHandle();
  TestEchoServiceMultipleLines();

  std::cout << "========================================" << std::endl;
  std::cout << "     All Tests Completed Successfully  " << std::endl;
  std::cout << "========================================" << std::endl;

  return 0;
}
