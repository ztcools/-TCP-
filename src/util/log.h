#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <string>
#include <fstream>
#include <mutex>
#include <thread>
#include <memory>
#include <ctime>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <string_view>

namespace util {

enum class LogLevel {
  DEBUG = 0,
  INFO = 1,
  WARN = 2,
  ERROR = 3,
  FATAL = 4
};

class Logger {
 public:
  static Logger& GetInstance();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void SetLogLevel(LogLevel level);
  LogLevel GetLogLevel() const;

  void Debug(const std::string& message, const char* file, int line);
  void Info(const std::string& message, const char* file, int line);
  void Warn(const std::string& message, const char* file, int line);
  void Error(const std::string& message, const char* file, int line);
  void Fatal(const std::string& message, const char* file, int line);

  void Flush();
  void Shutdown();

 private:
  Logger();
  ~Logger();

  struct LogBuffer {
    std::vector<char> buffer;
    size_t size;
    std::mutex mutex;

    LogBuffer(size_t capacity) : buffer(capacity), size(0) {}
  };

  void Init();
  void InitLogFile();
  void WriteLog(LogLevel level, const std::string& message, const char* file, int line);
  std::string FormatTime(const std::time_t& time);
  std::string LevelToString(LogLevel level);
  void CreateLogDirectory();
  std::string GetLogFilePath();
  void LogThreadFunc();
  void SwapBuffers();
  void WriteToFile(const std::vector<char>& buffer, size_t size);
  void CheckLogFileSize();
  void RotateLogFile();

  LogLevel log_level_;
  std::atomic<bool> running_;
  std::unique_ptr<std::thread> log_thread_;
  std::condition_variable cv_;
  std::mutex cv_mutex_;

  std::unique_ptr<LogBuffer> active_buffer_;
  std::unique_ptr<LogBuffer> backup_buffer_;
  size_t buffer_capacity_;
  size_t max_log_file_size_;

  std::ofstream log_file_;
  std::string current_date_;
  std::string log_directory_;
  std::atomic<size_t> current_log_file_size_;
};

#define LOG_DEBUG(msg) util::Logger::GetInstance().Debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) util::Logger::GetInstance().Info(msg, __FILE__, __LINE__)
#define LOG_WARN(msg) util::Logger::GetInstance().Warn(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) util::Logger::GetInstance().Error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) util::Logger::GetInstance().Fatal(msg, __FILE__, __LINE__)

}  // namespace util

#endif  // UTIL_LOG_H
