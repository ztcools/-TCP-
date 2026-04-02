#include "../include/net/epoll.h"
#include "../include/net/socket.h"
#include "../include/util/log.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

void TestEpollCreate() {
  std::cout << "=== Test Epoll Create ===" << std::endl;
  
  net::Epoll epoll;
  
  std::cout << "Epoll fd: " << epoll.GetFd() << std::endl;
  std::cout << "Epoll valid: " << (epoll.IsValid() ? "yes" : "no") << std::endl;
  std::cout << std::endl;
}

void TestEpollAddModifyRemove() {
  std::cout << "=== Test Epoll Add/Modify/Remove ===" << std::endl;
  
  net::Epoll epoll;
  net::Socket sock;
  sock.Create();
  
  std::cout << "Adding socket fd " << sock.GetFd() << " to epoll..." << std::endl;
  epoll.AddFd(sock.GetFd(), EPOLLIN | EPOLLET);
  
  std::cout << "Modifying socket events..." << std::endl;
  epoll.ModifyFd(sock.GetFd(), EPOLLIN | EPOLLOUT | EPOLLET);
  
  std::cout << "Removing socket from epoll..." << std::endl;
  epoll.RemoveFd(sock.GetFd());
  
  sock.Close();
  std::cout << std::endl;
}

void TestEpollWithCustomMaxEvents() {
  std::cout << "=== Test Epoll With Custom Max Events ===" << std::endl;
  
  net::Epoll epoll(2048);
  
  std::cout << "Epoll created with max_events=2048" << std::endl;
  std::cout << "Epoll fd: " << epoll.GetFd() << std::endl;
  std::cout << std::endl;
}

void TestEpollWaitTimeout() {
  std::cout << "=== Test Epoll Wait Timeout ===" << std::endl;
  
  net::Epoll epoll;
  
  auto start = std::chrono::high_resolution_clock::now();
  int event_count = epoll.Wait(100);
  auto end = std::chrono::high_resolution_clock::now();
  
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  std::cout << "Epoll wait returned " << event_count << " events" << std::endl;
  std::cout << "Timeout duration: " << duration.count() << "ms" << std::endl;
  std::cout << std::endl;
}

void TestEpollMultipleSockets() {
  std::cout << "=== Test Epoll Multiple Sockets ===" << std::endl;
  
  net::Epoll epoll;
  std::vector<net::Socket> sockets;
  
  for (int i = 0; i < 5; ++i) {
    net::Socket sock;
    sock.Create();
    epoll.AddFd(sock.GetFd(), EPOLLIN | EPOLLET);
    sockets.push_back(std::move(sock));
  }
  
  std::cout << "Added " << sockets.size() << " sockets to epoll" << std::endl;
  
  for (size_t i = 0; i < sockets.size(); ++i) {
    epoll.RemoveFd(sockets[i].GetFd());
    sockets[i].Close();
  }
  
  std::cout << "Removed all sockets from epoll" << std::endl;
  std::cout << std::endl;
}

void TestEpollServerClient() {
  std::cout << "=== Test Epoll Server-Client ===" << std::endl;
  
  std::thread server_thread([]() {
    try {
      net::Epoll epoll;
      net::Socket server_sock;
      
      server_sock.Create();
      server_sock.Bind("127.0.0.1", 8888);
      server_sock.Listen(128);
      
      epoll.AddFd(server_sock.GetFd(), EPOLLIN | EPOLLET);
      
      LOG_INFO("Epoll server started on port 8888");
      
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      
      epoll.RemoveFd(server_sock.GetFd());
      server_sock.Close();
      
      LOG_INFO("Epoll server stopped");
    } catch (const std::exception& e) {
      LOG_ERROR(std::string("Server error: ") + e.what());
    }
  });
  
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  
  try {
    net::Socket client_sock;
    client_sock.Create();
    client_sock.Close();
  } catch (const std::exception& e) {
    LOG_WARN(std::string("Client error (expected): ") + e.what());
  }
  
  server_thread.join();
  std::cout << std::endl;
}

void TestEpollEvents() {
  std::cout << "=== Test Epoll Event Types ===" << std::endl;
  
  std::cout << "Available event types:" << std::endl;
  std::cout << "  EPOLLIN = " << EPOLLIN << std::endl;
  std::cout << "  EPOLLOUT = " << EPOLLOUT << std::endl;
  std::cout << "  EPOLLRDHUP = " << EPOLLRDHUP << std::endl;
  std::cout << "  EPOLLPRI = " << EPOLLPRI << std::endl;
  std::cout << "  EPOLLERR = " << EPOLLERR << std::endl;
  std::cout << "  EPOLLHUP = " << EPOLLHUP << std::endl;
  std::cout << "  EPOLLET = " << EPOLLET << std::endl;
  std::cout << "  EPOLLONESHOT = " << EPOLLONESHOT << std::endl;
  std::cout << std::endl;
}

void TestEpollGetEvents() {
  std::cout << "=== Test Epoll GetEvents ===" << std::endl;
  
  net::Epoll epoll;
  net::Socket sock;
  sock.Create();
  
  epoll.AddFd(sock.GetFd(), EPOLLIN | EPOLLET);
  
  int event_count = epoll.Wait(10);
  std::cout << "Wait returned: " << event_count << " events" << std::endl;
  
  if (event_count > 0) {
    const struct epoll_event* events = epoll.GetEvents();
    for (int i = 0; i < event_count; ++i) {
      std::cout << "  Event " << i << ": fd=" << events[i].data.fd 
                << ", events=" << events[i].events << std::endl;
    }
  }
  
  epoll.RemoveFd(sock.GetFd());
  sock.Close();
  
  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "     TCP Server Epoll Test Suite       " << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  util::Logger::GetInstance().SetLogLevel(util::LogLevel::DEBUG);
  
  TestEpollCreate();
  TestEpollEvents();
  TestEpollAddModifyRemove();
  TestEpollWithCustomMaxEvents();
  TestEpollWaitTimeout();
  TestEpollMultipleSockets();
  TestEpollGetEvents();
  TestEpollServerClient();
  
  std::cout << "========================================" << std::endl;
  std::cout << "     All Tests Completed Successfully  " << std::endl;
  std::cout << "========================================" << std::endl;
  
  return 0;
}
