#ifndef SERVICE_ECHO_SERVICE_H
#define SERVICE_ECHO_SERVICE_H

#include <string>
#include <memory>
#include "conn/connection.h"
#include "util/thread_pool.h"
#include "util/log.h"

namespace service {

class EchoService {
 public:
  static EchoService& GetInstance();

  EchoService(const EchoService&) = delete;
  EchoService& operator=(const EchoService&) = delete;

  void Init(size_t thread_pool_size = 0);
  void Shutdown();

  void HandleConnection(std::shared_ptr<conn::Connection> conn);

 private:
  EchoService() = default;
  ~EchoService();

  void ProcessData(std::shared_ptr<conn::Connection> conn, const std::string& data);
  std::vector<std::string> SplitLines(const std::string& data);

  std::unique_ptr<util::ThreadPool> thread_pool_;
  bool running_;
};

}  // namespace service

#endif  // SERVICE_ECHO_SERVICE_H
