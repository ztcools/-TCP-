#include "../include/util/log.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

void TestBasicLog() {
  std::cout << "=== Test Basic Logging ===" << std::endl;
  
  util::Logger::GetInstance().SetLogLevel(util::LogLevel::DEBUG);
  
  LOG_DEBUG("This is a DEBUG message");
  LOG_INFO("This is an INFO message");
  LOG_WARN("This is a WARN message");
  LOG_ERROR("This is an ERROR message");
  
  std::cout << std::endl;
}

void TestMultiThreadLog() {
  std::cout << "=== Test Multi-threaded Logging ===" << std::endl;
  
  std::vector<std::thread> threads;
  
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back([i]() {
      for (int j = 0; j < 10; ++j) {
        std::ostringstream oss;
        oss << "Thread " << i << " - Log message " << j;
        
        switch (j % 5) {
          case 0:
            LOG_DEBUG(oss.str());
            break;
          case 1:
            LOG_INFO(oss.str());
            break;
          case 2:
            LOG_WARN(oss.str());
            break;
          case 3:
            LOG_ERROR(oss.str());
            break;
          case 4:
            LOG_DEBUG(oss.str());
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  std::cout << "Multi-threaded test completed" << std::endl;
  std::cout << std::endl;
}

void TestLogLevelFilter() {
  std::cout << "=== Test Log Level Filtering ===" << std::endl;
  
  std::cout << "Setting log level to WARN..." << std::endl;
  util::Logger::GetInstance().SetLogLevel(util::LogLevel::WARN);
  
  LOG_DEBUG("This DEBUG should not appear");
  LOG_INFO("This INFO should not appear");
  LOG_WARN("This WARN should appear");
  LOG_ERROR("This ERROR should appear");
  
  std::cout << "Resetting log level to DEBUG..." << std::endl;
  util::Logger::GetInstance().SetLogLevel(util::LogLevel::DEBUG);
  
  std::cout << std::endl;
}

void TestLogFileRotation() {
  std::cout << "=== Test Log File Creation ===" << std::endl;
  
  for (int i = 0; i < 5; ++i) {
    std::ostringstream oss;
    oss << "Log entry " << i << " for file rotation test";
    LOG_INFO(oss.str());
  }
  
  std::cout << "Check ./log/ directory for log files" << std::endl;
  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "     TCP Server Logger Test Suite      " << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  TestBasicLog();
  
  TestMultiThreadLog();
  
  TestLogLevelFilter();
  
  TestLogFileRotation();
  
  std::cout << "========================================" << std::endl;
  std::cout << "     All Tests Completed Successfully  " << std::endl;
  std::cout << "========================================" << std::endl;
  
  return 0;
}
