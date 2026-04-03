#include "../../include/conn/connection_pool.h"
#include "../../include/util/log.h"

namespace conn {

ConnectionPool& ConnectionPool::GetInstance() {
  static ConnectionPool instance;
  return instance;
}

ConnectionPool::~ConnectionPool() {
  Shutdown();
}

void ConnectionPool::Init(size_t max_connections, int64_t heartbeat_timeout_ms) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  max_connections_ = max_connections;
  heartbeat_timeout_ms_ = heartbeat_timeout_ms;
  running_ = true;
  
  memory_pool_ = std::make_shared<util::MemoryPool>(4096, max_connections_);
  
  LOG_INFO("ConnectionPool initialized: max_connections=" + std::to_string(max_connections_) + 
           ", heartbeat_timeout=" + std::to_string(heartbeat_timeout_ms_) + "ms");
}

void ConnectionPool::Shutdown() {
  StopHeartbeatCheck();
  
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (!running_.load(std::memory_order_relaxed)) {
    return;
  }
  
  running_.store(false, std::memory_order_relaxed);
  
  size_t count = connections_.size();
  for (auto& pair : connections_) {
    pair.second->Close();
  }
  connections_.clear();
  
  LOG_INFO("ConnectionPool shutdown: closed " + std::to_string(count) + " connections");
}

bool ConnectionPool::AddConnection(std::shared_ptr<Connection> conn) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (!running_.load(std::memory_order_relaxed)) {
    LOG_WARN("ConnectionPool is not running, cannot add connection");
    return false;
  }
  
  if (connections_.size() >= max_connections_) {
    LOG_ERROR("ConnectionPool: max connections reached (" + std::to_string(max_connections_) + ")");
    return false;
  }
  
  int fd = conn->GetFd();
  if (connections_.find(fd) != connections_.end()) {
    LOG_WARN("ConnectionPool: connection with fd=" + std::to_string(fd) + " already exists");
    return false;
  }
  
  connections_[fd] = conn;
  
  LOG_DEBUG("ConnectionPool: added connection " + conn->GetIp() + ":" + 
            std::to_string(conn->GetPort()) + ", fd=" + std::to_string(fd) + 
            ", total=" + std::to_string(connections_.size()));
  
  return true;
}

bool ConnectionPool::RemoveConnection(int fd) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = connections_.find(fd);
  if (it == connections_.end()) {
    LOG_WARN("ConnectionPool: connection with fd=" + std::to_string(fd) + " not found");
    return false;
  }
  
  std::shared_ptr<Connection> conn = it->second;
  std::string ip = conn->GetIp();
  uint16_t port = conn->GetPort();
  
  conn->Close();
  connections_.erase(it);
  
  LOG_DEBUG("ConnectionPool: removed connection " + ip + ":" + std::to_string(port) + 
            ", fd=" + std::to_string(fd) + ", total=" + std::to_string(connections_.size()));
  
  return true;
}

std::shared_ptr<Connection> ConnectionPool::GetConnection(int fd) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = connections_.find(fd);
  if (it == connections_.end()) {
    return nullptr;
  }
  
  return it->second;
}

size_t ConnectionPool::GetConnectionCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return connections_.size();
}

size_t ConnectionPool::GetMaxConnections() const {
  return max_connections_;
}

void ConnectionPool::StartHeartbeatCheck() {
  std::lock_guard<std::mutex> lock(heartbeat_mutex_);
  
  if (heartbeat_running_.load(std::memory_order_relaxed)) {
    LOG_WARN("ConnectionPool: heartbeat check already running");
    return;
  }
  
  heartbeat_running_.store(true, std::memory_order_relaxed);
  
  heartbeat_thread_ = std::thread(&ConnectionPool::HeartbeatCheckLoop, this);
  
  LOG_INFO("ConnectionPool: heartbeat check started");
}

void ConnectionPool::StopHeartbeatCheck() {
  {
    std::lock_guard<std::mutex> lock(heartbeat_mutex_);
    
    if (!heartbeat_running_.load(std::memory_order_relaxed)) {
      return;
    }
    
    heartbeat_running_.store(false, std::memory_order_relaxed);
  }
  
  heartbeat_cv_.notify_all();
  
  if (heartbeat_thread_.joinable()) {
    heartbeat_thread_.join();
  }
  
  LOG_INFO("ConnectionPool: heartbeat check stopped");
}

void ConnectionPool::HeartbeatCheckLoop() {
  LOG_DEBUG("ConnectionPool: heartbeat check loop started");
  
  while (heartbeat_running_.load(std::memory_order_relaxed)) {
    {
      std::unique_lock<std::mutex> lock(heartbeat_mutex_);
      
      heartbeat_cv_.wait_for(lock, std::chrono::seconds(10), [this]() {
        return !heartbeat_running_.load(std::memory_order_relaxed);
      });
    }
    
    if (!heartbeat_running_.load(std::memory_order_relaxed)) {
      break;
    }
    
    std::vector<int> timeout_fds;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      
      for (const auto& pair : connections_) {
        if (pair.second->IsTimeout(heartbeat_timeout_ms_)) {
          timeout_fds.push_back(pair.first);
        }
      }
    }
    
    for (int fd : timeout_fds) {
      LOG_WARN("ConnectionPool: connection fd=" + std::to_string(fd) + " timed out, removing");
      RemoveConnection(fd);
    }
    
    if (!timeout_fds.empty()) {
      LOG_INFO("ConnectionPool: removed " + std::to_string(timeout_fds.size()) + " timeout connections");
    }
  }
  
  LOG_DEBUG("ConnectionPool: heartbeat check loop stopped");
}

}  // namespace conn
