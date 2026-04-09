#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace util {
extern bool g_is_log_enable; //日志开关

// 控制日志开关的函数
inline void EnableLog() { g_is_log_enable = true; }
inline void DisableLog() { g_is_log_enable = false; }
inline bool IsLogEnabled() { return g_is_log_enable; }

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4 };

class Logger {
public:
  static Logger &GetInstance();

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  void SetLogLevel(LogLevel level);
  LogLevel GetLogLevel() const;

  void Debug(const std::string &message, const char *file, int line);
  void Info(const std::string &message, const char *file, int line);
  void Warn(const std::string &message, const char *file, int line);
  void Error(const std::string &message, const char *file, int line);
  void Fatal(const std::string &message, const char *file, int line);

  void Flush();

private:
  Logger();
  ~Logger();

  void InitLogFile();
  void WriteLog(LogLevel level, const std::string &message, const char *file,
                int line);
  std::string FormatTime(const std::time_t &time);
  std::string LevelToString(LogLevel level);
  void CreateLogDirectory();
  std::string GetLogFilePath();

  LogLevel log_level_;
  std::mutex mutex_;
  std::ofstream log_file_;
  std::string current_date_;
  std::string log_directory_;
};

// 日志宏定义，支持运行时开关
#define LOG_DEBUG(msg)                                                         \
  if (util::g_is_log_enable)                                                   \
    util::Logger::GetInstance().Debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg)                                                          \
  if (util::g_is_log_enable)                                                   \
    util::Logger::GetInstance().Info(msg, __FILE__, __LINE__)
#define LOG_WARN(msg)                                                          \
  if (util::g_is_log_enable)                                                   \
    util::Logger::GetInstance().Warn(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg)                                                         \
  if (util::g_is_log_enable)                                                   \
    util::Logger::GetInstance().Error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg)                                                         \
  if (util::g_is_log_enable)                                                   \
    util::Logger::GetInstance().Fatal(msg, __FILE__, __LINE__)

} // namespace util

#endif // UTIL_LOG_H
