#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "macros.h"

#define LOG_INFO(logmsgFormat, ...)                   \
  do {                                                \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    Tnet::Logger::log_info(buf);                      \
  } while (0)

#define LOG_WARN(logmsgFormat, ...)                   \
  do {                                                \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    Tnet::Logger::log_warn(buf);                      \
  } while (0)

#define LOG_ERROR(logmsgFormat, ...)                  \
  do {                                                \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    Tnet::Logger::log_error(buf);                     \
  } while (0)

#define LOG_CRITICAL(logmsgFormat, ...)               \
  do {                                                \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    Tnet::Logger::log_critical(buf);                  \
  } while (0)

#define LOG_DEBUG(logmsgFormat, ...)                  \
  do {                                                \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    Tnet::Logger::log_debug(buf);                     \
  } while (0)

namespace Tnet {

class Logger {
 public:
  Logger() = default;

  DISALLOW_COPY(Logger);

  static const uint16_t CONSOLE_OUTPUT = 1;  // 终端输出
  static const uint16_t FILE_OUTPUT = 2;     // 文件输出
  static const uint16_t ALL_OUTPUT = 3;      // 终端输出与文件输出

  void init(uint16_t output_level = 3) {
    if (output_level & 0X1) {
      // 创建一个彩色控制台日志接收器
      auto console_sink =
          std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_level(spdlog::level::trace);
      console_sink->set_pattern(
          "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l](%@): %v");
      sinks.push_back(console_sink);
    }

    if (output_level & 0X10) {
      // 创建一个基本文件日志接收器
      auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
          "logs/Tnet.log", true);
      file_sink->set_level(spdlog::level::trace);
      file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l](%@): %v");
      sinks.push_back(file_sink);
    }

    if (!sinks.empty()) {
      auto logger = std::make_shared<spdlog::logger>(
          "multi_sink", sinks.begin(), sinks.end());
      logger->set_level(spdlog::level::trace);
      spdlog::set_default_logger(logger);

      spdlog::flush_on(spdlog::level::info);
    }
  }

  static void log_info(const std::string& message) { spdlog::info(message); }

  static void log_warn(const std::string& message) { spdlog::warn(message); }

  static void log_error(const std::string& message) { spdlog::error(message); }

  static void log_critical(const std::string& message) {
    spdlog::critical(message);
  }

  static void log_debug(const std::string& message) { spdlog::debug(message); }

 private:
  std::vector<spdlog::sink_ptr> sinks;
};

}  // namespace Tnet
