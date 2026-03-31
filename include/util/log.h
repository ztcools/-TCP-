#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <memory>
#include <ctime>
#include <iomanip>

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

 private:
  Logger();
  ~Logger();

  void InitLogFile();
  void WriteLog(LogLevel level, const std::string& message, const char* file, int line);
  std::string FormatTime(const std::time_t& time);
  std::string LevelToString(LogLevel level);
  void CreateLogDirectory();
  std::string GetLogFilePath();

  LogLevel log_level_;
  std::mutex mutex_;
  std::ofstream log_file_;
  std::string current_date_;
  std::string log_directory_;
};

#define LOG_DEBUG(msg) util::Logger::GetInstance().Debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) util::Logger::GetInstance().Info(msg, __FILE__, __LINE__)
#define LOG_WARN(msg) util::Logger::GetInstance().Warn(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) util::Logger::GetInstance().Error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) util::Logger::GetInstance().Fatal(msg, __FILE__, __LINE__)

}  // namespace util

#endif  // UTIL_LOG_H
