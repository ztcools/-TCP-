#ifndef SERVICE_ECHO_SERVICE_H
#define SERVICE_ECHO_SERVICE_H

#include <string>
#include <memory>
#include "../conn/connection.h"
#include "util/thread_pool.h"
#include "util/log.h"

namespace service {

/**
 * @brief Echo 服务类，实现数据回显功能
 * 
 * 该类是一个业务服务示例，负责处理客户端发送的数据并原样返回，
 * 支持按行分割数据，处理粘包/半包问题。
 */
class EchoService {
 public:
  /**
   * @brief 获取 EchoService 实例（单例模式）
   * @return EchoService 实例
   */
  static EchoService& GetInstance();

  // 禁止拷贝和赋值
  EchoService(const EchoService&) = delete;
  EchoService& operator=(const EchoService&) = delete;

  /**
   * @brief 初始化 EchoService
   * @param thread_pool_size 线程池大小，0 表示自动
   */
  void Init(size_t thread_pool_size = 0);
  
  /**
   * @brief 关闭 EchoService
   */
  void Shutdown();

  /**
   * @brief 处理连接数据
   * @param conn 连接对象
   */
  void HandleConnection(std::shared_ptr<conn::Connection> conn);

 private:
  /**
   * @brief 构造函数
   */
  EchoService() = default;
  
  /**
   * @brief 析构函数
   */
  ~EchoService();

  /**
   * @brief 处理数据
   * @param conn 连接对象
   * @param data 数据
   */
  void ProcessData(std::shared_ptr<conn::Connection> conn, const std::string& data);
  
  /**
   * @brief 按行分割数据
   * @param data 数据
   * @return 分割后的行列表
   */
  std::vector<std::string> SplitLines(const std::string& data);

  std::unique_ptr<util::ThreadPool> thread_pool_;  // 线程池
  bool running_;  // 运行状态
};

}  // namespace service

#endif  // SERVICE_ECHO_SERVICE_H