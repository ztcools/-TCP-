#include "../../include/util/log.h"

#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

namespace util {

Logger& Logger::GetInstance() {
  static Logger instance;
  return instance;
}

Logger::Logger()
    : log_level_(LogLevel::DEBUG),
      log_directory_("./log/") {
  CreateLogDirectory();
  InitLogFile();
}

Logger::~Logger() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (log_file_.is_open()) {
    log_file_.flush();
    log_file_.close();
  }
}

void Logger::SetLogLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(mutex_);
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
}

void Logger::Flush() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (log_file_.is_open()) {
    log_file_.flush();
  }
  std::cout.flush();
}

void Logger::InitLogFile() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::time_t now = std::time(nullptr);
  std::tm* tm_info = std::localtime(&now);
  char date_buffer[11];
  std::strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", tm_info);
  current_date_ = date_buffer;
  
  std::string log_file_path = GetLogFilePath();
  log_file_.open(log_file_path, std::ios::app);
  if (!log_file_.is_open()) {
    std::cerr << "Failed to open log file: " << log_file_path << std::endl;
  }
}

void Logger::WriteLog(LogLevel level, const std::string& message, const char* file, int line) {
  if (level < log_level_) {
    return;
  }
  
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::time_t now = std::time(nullptr);
  std::tm* tm_info = std::localtime(&now);
  char date_buffer[11];
  std::strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", tm_info);
  std::string current_date = date_buffer;
  
  if (current_date != current_date_) {
    if (log_file_.is_open()) {
      log_file_.close();
    }
    current_date_ = current_date;
    std::string log_file_path = GetLogFilePath();
    log_file_.open(log_file_path, std::ios::app);
  }
  
  std::string timestamp = FormatTime(now);
  std::string level_str = LevelToString(level);
  std::thread::id thread_id = std::this_thread::get_id();
  
  std::ostringstream oss;
  oss << "[" << timestamp << "] "
      << "[Thread:" << thread_id << "] "
      << "[" << level_str << "] "
      << "[" << file << ":" << line << "] "
      << message;
  
  std::string log_line = oss.str();
  
  std::cout << log_line << std::endl;
  
  if (log_file_.is_open()) {
    log_file_ << log_line << std::endl;
  }
  
  if (level == LogLevel::FATAL) {
    if (log_file_.is_open()) {
      log_file_.flush();
      log_file_.close();
    }
    std::abort();
  }
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
