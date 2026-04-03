#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <thread>

void TestSingleConnection(const std::string& server_ip, int server_port) {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
    std::cerr << "Invalid server address" << std::endl;
    close(client_fd);
    return;
  }

  if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    std::cerr << "Failed to connect to server" << std::endl;
    close(client_fd);
    return;
  }

  std::cout << "Connected to server" << std::endl;

  std::string test_msg = "Hello, server!";
  ssize_t sent = send(client_fd, test_msg.c_str(), test_msg.length(), 0);
  if (sent == -1) {
    std::cerr << "Failed to send message" << std::endl;
    close(client_fd);
    return;
  }
  std::cout << "Sent: " << test_msg << std::endl;

  char buffer[1024];
  ssize_t recv_len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (recv_len > 0) {
    buffer[recv_len] = '\0';
    std::cout << "Received: " << buffer << std::endl;
  }

  close(client_fd);
  std::cout << "Connection closed" << std::endl;
}

int main(int argc, char* argv[]) {
  std::string server_ip = "127.0.0.1";
  int server_port = 8888;

  if (argc > 1) {
    server_ip = argv[1];
  }
  if (argc > 2) {
    server_port = atoi(argv[2]);
  }

  std::cout << "Testing TCP Server: " << server_ip << ":" << server_port << std::endl;

  TestSingleConnection(server_ip, server_port);

  return 0;
}
