#ifndef LOGGER_H
#define LOGGER_H

#include <string>

enum class LogLevel { INFO, WARNING, ERROR };

namespace Logger {
std::string GetLogLevelString(const LogLevel level, bool color = false);
void TerminalLog(const std::string& message,
                 const LogLevel level = LogLevel::INFO);
void SystemLog(const std::string& message,
               const LogLevel level = LogLevel::INFO);
};  // namespace Logger

#endif  // LOGGER_H