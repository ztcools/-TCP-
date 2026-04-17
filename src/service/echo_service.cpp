#include "service/echo_service.h"

namespace service {

EchoService& EchoService::GetInstance() {
  static EchoService instance;
  return instance;
}

EchoService::~EchoService() {
  Shutdown();
}

void EchoService::Init(size_t thread_pool_size) {
  if (running_) {
    LOG_WARN("EchoService already initialized");
    return;
  }

  if (thread_pool_size == 0) {
    thread_pool_size = std::thread::hardware_concurrency() * 2;
    if (thread_pool_size == 0) {
      thread_pool_size = 4;
    }
  }

  thread_pool_ = std::make_unique<util::ThreadPool>(thread_pool_size);
  running_ = true;

  LOG_INFO("EchoService initialized: thread_pool_size=" + std::to_string(thread_pool_size));
}

void EchoService::Shutdown() {
  if (!running_) {
    return;
  }

  running_ = false;

  if (thread_pool_) {
    thread_pool_->Shutdown();
    thread_pool_.reset();
  }

  LOG_INFO("EchoService shutdown");
}

void EchoService::HandleConnection(std::shared_ptr<conn::Connection> conn) {
  if (!running_) {
    LOG_WARN("EchoService not running, cannot handle connection");
    return;
  }

  size_t data_len = conn->GetReadBufferSize();
  if (data_len == 0) {
    return;
  }

  const char* data = conn->GetReadBuffer();
  std::string buffer(data, data_len);

  conn->ConsumeReadBuffer(data_len);

  LOG_DEBUG("EchoService: received " + std::to_string(data_len) + 
            " bytes from " + conn->GetIp() + ":" + std::to_string(conn->GetPort()));

  if (thread_pool_) {
    thread_pool_->Enqueue(&EchoService::ProcessData, this, conn, buffer);
  } else {
    ProcessData(conn, buffer);
  }
}

void EchoService::ProcessData(std::shared_ptr<conn::Connection> conn, const std::string& data) {
  std::vector<std::string> lines = SplitLines(data);

  for (const std::string& line : lines) {
    if (line.empty()) {
      continue;
    }

    LOG_INFO("EchoService: echoing - " + line);

    std::string response = line + "\n";
    conn->AppendWriteBuffer(response.c_str(), response.size());

    LOG_DEBUG("EchoService: sent " + std::to_string(response.size()) + 
              " bytes to " + conn->GetIp() + ":" + std::to_string(conn->GetPort()));
  }

  conn->Send();
}

std::vector<std::string> EchoService::SplitLines(const std::string& data) {
  std::vector<std::string> lines;
  std::string line;

  for (char c : data) {
    if (c == '\n' || c == '\r') {
      if (!line.empty()) {
        lines.push_back(line);
        line.clear();
      }
    } else {
      line += c;
    }
  }

  if (!line.empty()) {
    lines.push_back(line);
  }

  return lines;
}

}  // namespace service
