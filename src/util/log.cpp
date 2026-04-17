#include "log.h"

#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace util {

Logger& Logger::GetInstance() {
  static Logger instance;
  return instance;
}

Logger::Logger()
    : log_level_(LogLevel::DEBUG),
      running_(true),
      buffer_capacity_(1024 * 1024),  // 1MB buffer
      max_log_file_size_(100 * 1024 * 1024),  // 100MB per file
      log_directory_("./log/"),
      current_log_file_size_(0) {
  Init();
}

Logger::~Logger() {
  Shutdown();
}

void Logger::Init() {
  CreateLogDirectory();
  InitLogFile();
  
  // Initialize buffers
  active_buffer_ = std::make_unique<LogBuffer>(buffer_capacity_);
  backup_buffer_ = std::make_unique<LogBuffer>(buffer_capacity_);
  
  // Start log thread
  log_thread_ = std::make_unique<std::thread>(&Logger::LogThreadFunc, this);
}

void Logger::SetLogLevel(LogLevel level) {
  log_level_ = level;
}

LogLevel Logger::GetLogLevel() const {
  return log_level_;
}

void Logger::Debug(const std::string& message, const char* file, int line) {
  WriteLog(LogLevel::DEBUG, message, file, line);
}

void Logger::Info(const std::string& message, const char* file, int line) {
  WriteLog(LogLevel::INFO, message, file, line);
}

void Logger::Warn(const std::string& message, const char* file, int line) {
  WriteLog(LogLevel::WARN, message, file, line);
}

void Logger::Error(const std::string& message, const char* file, int line) {
  WriteLog(LogLevel::ERROR, message, file, line);
}

void Logger::Fatal(const std::string& message, const char* file, int line) {
  WriteLog(LogLevel::FATAL, message, file, line);
  Shutdown();
  std::abort();
}

void Logger::Flush() {
  SwapBuffers();
  {
    std::lock_guard<std::mutex> lock(cv_mutex_);
    cv_.notify_one();
  }
}

void Logger::Shutdown() {
  running_ = false;
  Flush();
  if (log_thread_ && log_thread_->joinable()) {
    log_thread_->join();
  }
  if (log_file_.is_open()) {
    log_file_.close();
  }
}

void Logger::WriteLog(LogLevel level, const std::string& message, const char* file, int line) {
  if (level < log_level_) {
    return;
  }
  
  std::time_t now = std::time(nullptr);
  std::string timestamp = FormatTime(now);
  std::string level_str = LevelToString(level);
  std::thread::id thread_id = std::this_thread::get_id();
  
  std::ostringstream oss;
  oss << "[" << timestamp << "] "
      << "[Thread:" << thread_id << "] "
      << "[" << level_str << "] "
      << "[" << file << ":" << line << "] "
      << message << "\n";
  
  std::string log_line = oss.str();
  size_t log_line_size = log_line.size();
  
  // Write to active buffer with minimal blocking
  {
    std::lock_guard<std::mutex> lock(active_buffer_->mutex);
    if (active_buffer_->size + log_line_size > active_buffer_->buffer.size()) {
      // Buffer full, swap and notify
      SwapBuffers();
      {
        std::lock_guard<std::mutex> lock(cv_mutex_);
        cv_.notify_one();
      }
      
      // Re-acquire lock for new active buffer
      std::lock_guard<std::mutex> lock2(active_buffer_->mutex);
      if (active_buffer_->size + log_line_size > active_buffer_->buffer.size()) {
        // Still not enough space, truncate or handle error
        return;
      }
    }
    
    // Use memcpy for fast memory copy
    memcpy(active_buffer_->buffer.data() + active_buffer_->size, log_line.c_str(), log_line_size);
    active_buffer_->size += log_line_size;
  }
  
  // For FATAL level, we need to flush immediately
  if (level == LogLevel::FATAL) {
    Flush();
  }
}

void Logger::SwapBuffers() {
  std::unique_ptr<LogBuffer> temp = std::move(backup_buffer_);
  backup_buffer_ = std::move(active_buffer_);
  active_buffer_ = std::move(temp);
  active_buffer_->size = 0;
}

void Logger::LogThreadFunc() {
  while (running_) {
    // Wait for either 3 seconds or notification
    std::unique_lock<std::mutex> lock(cv_mutex_);
    cv_.wait_for(lock, std::chrono::seconds(3));
    
    // Swap buffers to get data to write
    SwapBuffers();
    
    // Write backup buffer to file
    if (backup_buffer_->size > 0) {
      WriteToFile(backup_buffer_->buffer, backup_buffer_->size);
      backup_buffer_->size = 0;
    }
  }
  
  // Write any remaining data before exiting
  if (active_buffer_->size > 0) {
    WriteToFile(active_buffer_->buffer, active_buffer_->size);
  }
  if (backup_buffer_->size > 0) {
    WriteToFile(backup_buffer_->buffer, backup_buffer_->size);
  }
}

void Logger::WriteToFile(const std::vector<char>& buffer, size_t size) {
  if (size == 0) return;
  
  std::time_t now = std::time(nullptr);
  std::tm* tm_info = std::localtime(&now);
  char date_buffer[11];
  std::strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", tm_info);
  std::string current_date = date_buffer;
  
  // Check if date has changed
  if (current_date != current_date_) {
    RotateLogFile();
    current_date_ = current_date;
  }
  
  // Check if log file size exceeds limit
  CheckLogFileSize();
  
  // Write to file
  if (log_file_.is_open()) {
    log_file_.write(buffer.data(), size);
    log_file_.flush();
    current_log_file_size_ += size;
  }
  
  // Also write to console
  std::cout.write(buffer.data(), size);
  std::cout.flush();
}

void Logger::InitLogFile() {
  std::string log_file_path = GetLogFilePath();
  log_file_.open(log_file_path, std::ios::app);
  if (!log_file_.is_open()) {
    std::cerr << "Failed to open log file: " << log_file_path << std::endl;
  }
  
  // Get current file size
  log_file_.seekp(0, std::ios::end);
  current_log_file_size_ = log_file_.tellp();
}

void Logger::CheckLogFileSize() {
  if (current_log_file_size_ >= max_log_file_size_) {
    RotateLogFile();
  }
}

void Logger::RotateLogFile() {
  if (log_file_.is_open()) {
    log_file_.close();
  }
  
  std::string log_file_path = GetLogFilePath();
  log_file_.open(log_file_path, std::ios::app);
  if (!log_file_.is_open()) {
    std::cerr << "Failed to open log file: " << log_file_path << std::endl;
  }
  current_log_file_size_ = 0;
}

std::string Logger::FormatTime(const std::time_t& time) {
  std::tm* tm_info = std::localtime(&time);
  char buffer[26];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
  return std::string(buffer);
}

std::string Logger::LevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARN:
      return "WARN";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::FATAL:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

void Logger::CreateLogDirectory() {
  struct stat st;
  if (stat(log_directory_.c_str(), &st) != 0) {
    if (mkdir(log_directory_.c_str(), 0755) != 0) {
      std::cerr << "Failed to create log directory: " << log_directory_ << std::endl;
    }
  }
}

std::string Logger::GetLogFilePath() {
  std::time_t now = std::time(nullptr);
  std::tm* tm_info = std::localtime(&now);
  char date_buffer[11];
  std::strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", tm_info);
  
  return log_directory_ + "tcp_server_" + std::string(date_buffer) + ".log";
}

}  // namespace util
