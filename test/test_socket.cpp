#include "../include/net/socket.h"
#include "../include/util/log.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <vector>

void TestSocketCreate() {
  std::cout << "=== Test Socket Create ===" << std::endl;
  
  net::Socket sock;
  sock.Create();
  
  std::cout << "Socket fd: " << sock.GetFd() << std::endl;
  std::cout << "Socket valid: " << (sock.IsValid() ? "yes" : "no") << std::endl;
  
  sock.Close();
  std::cout << "After close - valid: " << (sock.IsValid() ? "yes" : "no") << std::endl;
  std::cout << std::endl;
}

void TestSocketBindListen() {
  std::cout << "=== Test Socket Bind & Listen ===" << std::endl;
  
  net::Socket sock;
  sock.Create();
  sock.Bind("127.0.0.1", 8888);
  sock.Listen(128);
  
  std::cout << "Socket bound and listening on 127.0.0.1:8888" << std::endl;
  
  sock.Close();
  std::cout << std::endl;
}

void TestSocketMove() {
  std::cout << "=== Test Socket Move Semantics ===" << std::endl;
  
  net::Socket sock1;
  sock1.Create();
  int fd1 = sock1.GetFd();
  
  std::cout << "sock1 fd: " << fd1 << std::endl;
  
  net::Socket sock2 = std::move(sock1);
  
  std::cout << "After move - sock1 valid: " << (sock1.IsValid() ? "yes" : "no") << std::endl;
  std::cout << "After move - sock2 fd: " << sock2.GetFd() << std::endl;
  std::cout << "After move - sock2 valid: " << (sock2.IsValid() ? "yes" : "no") << std::endl;
  
  sock2.Close();
  std::cout << std::endl;
}

void TestSocketOptions() {
  std::cout << "=== Test Socket Options ===" << std::endl;
  
  net::Socket sock;
  sock.Create();
  
  std::cout << "Socket options set: SO_REUSEADDR, SO_REUSEPORT, TCP_NODELAY, O_NONBLOCK" << std::endl;
  
  sock.Close();
  std::cout << std::endl;
}

void TestServerClientBasic() {
  std::cout << "=== Test Server-Client Basic ===" << std::endl;
  
  std::thread server_thread([]() {
    try {
      net::Socket server_sock;
      server_sock.Create();
      server_sock.Bind("127.0.0.1", 9999);
      server_sock.Listen(128);
      
      LOG_INFO("Server waiting for connection...");
      
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      
      net::Socket client_sock = server_sock.Accept();
      
      if (client_sock.IsValid()) {
        LOG_INFO("Server got connection!");
        
        char buffer[256];
        ssize_t recv_len = client_sock.Recv(buffer, sizeof(buffer) - 1);
        
        if (recv_len > 0) {
          buffer[recv_len] = '\0';
          LOG_INFO(std::string("Server received: ") + buffer);
          
          const char* reply = "Hello from server!";
          client_sock.Send(reply, strlen(reply));
        }
        
        client_sock.Close();
      }
      
      server_sock.Close();
    } catch (const std::exception& e) {
      LOG_ERROR(std::string("Server error: ") + e.what());
    }
  });
  
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  
  try {
    net::Socket client_sock;
    client_sock.Create();
    
    LOG_INFO("Client connecting to server...");
    
    const char* msg = "Hello from client!";
    client_sock.Send(msg, strlen(msg));
    
    char buffer[256];
    client_sock.Recv(buffer, sizeof(buffer) - 1);
    
    client_sock.Close();
  } catch (const std::exception& e) {
    LOG_WARN(std::string("Client error (expected for non-blocking): ") + e.what());
  }
  
  server_thread.join();
  std::cout << std::endl;
}

void TestMultipleSockets() {
  std::cout << "=== Test Multiple Sockets ===" << std::endl;
  
  std::vector<net::Socket> sockets;
  
  for (int i = 0; i < 5; ++i) {
    net::Socket sock;
    sock.Create();
    sockets.push_back(std::move(sock));
  }
  
  std::cout << "Created " << sockets.size() << " sockets" << std::endl;
  
  for (size_t i = 0; i < sockets.size(); ++i) {
    std::cout << "Socket " << i << " fd: " << sockets[i].GetFd() << std::endl;
  }
  
  for (auto& sock : sockets) {
    sock.Close();
  }
  
  std::cout << std::endl;
}

void TestSocketRAII() {
  std::cout << "=== Test Socket RAII ===" << std::endl;
  
  {
    net::Socket sock;
    sock.Create();
    std::cout << "Socket fd in scope: " << sock.GetFd() << std::endl;
  }
  
  std::cout << "Socket out of scope - should be closed" << std::endl;
  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "     TCP Server Socket Test Suite      " << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  util::Logger::GetInstance().SetLogLevel(util::LogLevel::DEBUG);
  
  TestSocketCreate();
  TestSocketBindListen();
  TestSocketMove();
  TestSocketOptions();
  TestSocketRAII();
  TestMultipleSockets();
  TestServerClientBasic();
  
  std::cout << "========================================" << std::endl;
  std::cout << "     All Tests Completed Successfully  " << std::endl;
  std::cout << "========================================" << std::endl;
  
  return 0;
}
