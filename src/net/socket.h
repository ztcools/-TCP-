#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <string>
#include <cstddef>
#include <cstdint>

namespace net {

/**
 * @brief Socket 封装类，提供对 Linux TCP Socket 的封装
 * 
 * 该类封装了 Linux TCP Socket 的核心功能，支持非阻塞模式、
 * 地址重用、端口重用和 TCP_NODELAY 等特性，用于网络通信。
 */
class Socket {
 public:
  /**
   * @brief 默认构造函数
   */
  Socket();
  
  /**
   * @brief 构造函数，使用已有的文件描述符
   * @param fd 文件描述符
   */
  explicit Socket(int fd);
  
  /**
   * @brief 析构函数
   */
  ~Socket();

  // 禁止拷贝和赋值
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  // 支持移动语义
  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  /**
   * @brief 创建 socket
   */
  void Create();
  
  /**
   * @brief 绑定 IP 和端口
   * @param ip IP 地址
   * @param port 端口号
   */
  void Bind(const std::string& ip, uint16_t port);
  
  /**
   * @brief 开始监听
   * @param backlog 监听队列大小
   */
  void Listen(int backlog = 128);
  
  /**
   * @brief 接受连接
   * @return 新的 Socket 对象
   */
  Socket Accept();

  /**
   * @brief 连接到服务器
   * @param ip 服务器 IP 地址
   * @param port 服务器端口号
   */
  void Connect(const std::string& ip, uint16_t port);

  /**
   * @brief 发送数据
   * @param data 数据指针
   * @param len 数据长度
   * @return 发送的字节数
   */
  ssize_t Send(const void* data, size_t len);
  
  /**
   * @brief 接收数据
   * @param data 数据缓冲区指针
   * @param len 缓冲区长度
   * @return 接收的字节数
   */
  ssize_t Recv(void* data, size_t len);

  /**
   * @brief 获取文件描述符
   * @return 文件描述符
   */
  int GetFd() const;
  
  /**
   * @brief 检查 socket 是否有效
   * @return socket 是否有效
   */
  bool IsValid() const;
  
  /**
   * @brief 关闭 socket
   */
  void Close();
  
  /**
   * @brief 释放文件描述符
   * @return 文件描述符
   */
  int Release();

  /**
   * @brief 设置非阻塞模式
   */
  void SetNonBlocking();
  
  /**
   * @brief 设置地址重用
   */
  void SetReuseAddr();
  
  /**
   * @brief 设置端口重用
   */
  void SetReusePort();
  
  /**
   * @brief 设置 TCP_NODELAY
   */
  void SetTcpNoDelay();

  /**
   * @brief 获取对端 IP 地址
   * @return 对端 IP 地址
   */
  std::string GetPeerIp() const;
  
  /**
   * @brief 获取对端端口号
   * @return 对端端口号
   */
  uint16_t GetPeerPort() const;
  
  /**
   * @brief 获取本地 IP 地址
   * @return 本地 IP 地址
   */
  std::string GetLocalIp() const;
  
  /**
   * @brief 获取本地端口号
   * @return 本地端口号
   */
  uint16_t GetLocalPort() const;

 private:
  /**
   * @brief 检查操作是否出错
   * @param ret 操作返回值
   * @param operation 操作名称
   */
  void CheckError(int ret, const char* operation);

  int fd_;  // socket 文件描述符
};

}  // namespace net

#endif  // NET_SOCKET_H
