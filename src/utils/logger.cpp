#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR "."
#endif

#include "utils/logger.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

std::string Logger::GetLogLevelString(const LogLevel level, bool color) {
  std::ostringstream oss;
  const int width = 5;

  if (color) {
    switch (level) {
      case LogLevel::INFO:
        oss << "\033[32m[" << std::left << std::setw(width) << "INFO"
            << "]\033[0m";
        break;
      case LogLevel::WARNING:
        oss << "\033[33m[" << std::left << std::setw(width) << "WARN"
            << "]\033[0m";
        break;
      case LogLevel::ERROR:
        oss << "\033[31m[" << std::left << std::setw(width) << "ERROR"
            << "]\033[0m";
        break;
      default:
        oss << "\033[36m[" << std::left << std::setw(width) << "N.A."
            << "]\033[0m";
        break;
    }
  } else {
    oss << "[";
    switch (level) {
      case LogLevel::INFO:
        oss << std::left << std::setw(width) << "INFO";
        break;
      case LogLevel::WARNING:
        oss << std::left << std::setw(width) << "WARN";
        break;
      case LogLevel::ERROR:
        oss << std::left << std::setw(width) << "ERROR";
        break;
      default:
        oss << std::left << std::setw(width) << "N.A.";
        break;
    }
    oss << "]";
  }

  return oss.str();
}

void Logger::TerminalLog(const std::string& message, const LogLevel level) {
  std::cout << GetLogLevelString(level, true) << " " << message << std::endl;
}

void Logger::SystemLog(const std::string& message, const LogLevel level) {
  std::ofstream logFile(std::string(PROJECT_ROOT_DIR) + "/logs/sys.log",
                        std::ios::app);
  std::time_t now = std::time(nullptr);
  std::string logEntry = std::string(std::ctime(&now)) + " - " +
                         GetLogLevelString(level) + " " + message;

  if (logFile.is_open()) {
    logFile << logEntry << std::endl;
    logFile.close();
  }
}