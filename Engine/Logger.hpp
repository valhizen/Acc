#pragma once

#include <chrono>
#include <iostream>
#include <time.h>
#include <utility>
#include <ctime>

enum class LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {
public:
  template <typename... Args> void log(LogLevel level, Args &&...args) {
    auto now = std::chrono::system_clock::now();
    const std::time_t timeInfo = std::chrono::system_clock::to_time_t(now);

    time_t result = time(nullptr);

    char buffer[100]{};
    ctime_s(buffer, sizeof(buffer), &result);

    std::cout << buffer << levelToString(level) << ' ';

    (std::cout << ... << std::forward<Args>(args));

    std::cout << '\n';
  }

private:
  static const char *levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARNING:
      return "WARNING";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::CRITICAL:
      return "CRITICAL";
    default:
      return "UNKNOWN";
    }
  }
};

inline Logger logger{};

template <typename... Args> void logInfo(Args &&...args) {
  logger.log(LogLevel::INFO, std::forward<Args>(args)...);
}

template <typename... Args> void logWarning(Args &&...args) {
  logger.log(LogLevel::WARNING, std::forward<Args>(args)...);
}

template <typename... Args> void logCritical(Args &&...args) {
  logger.log(LogLevel::CRITICAL, std::forward<Args>(args)...);
}

template <typename... Args> void logDebug(Args &&...args) {
  logger.log(LogLevel::DEBUG, std::forward<Args>(args)...);
}

template <typename... Args> void logError(Args &&...args) {
  logger.log(LogLevel::ERROR, std::forward<Args>(args)...);
}
