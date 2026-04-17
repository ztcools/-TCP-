#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <string>
#include <cstddef>
#include <cstdint>

namespace net {

class Socket {
 public:
  Socket();
  explicit Socket(int fd);
  ~Socket();

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  void Create();
  void Bind(const std::string& ip, uint16_t port);
  void Listen(int backlog = 128);
  Socket Accept();

  void Connect(const std::string& ip, uint16_t port);

  ssize_t Send(const void* data, size_t len);
  ssize_t Recv(void* data, size_t len);

  int GetFd() const;
  bool IsValid() const;
  void Close();
  int Release();

  void SetNonBlocking();
  void SetReuseAddr();
  void SetReusePort();
  void SetTcpNoDelay();

  std::string GetPeerIp() const;
  uint16_t GetPeerPort() const;
  std::string GetLocalIp() const;
  uint16_t GetLocalPort() const;

 private:
  void CheckError(int ret, const char* operation);

  int fd_;
};

}  // namespace net

#endif  // NET_SOCKET_H
