#include "utils/logger.h"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "utils/setup.h"
#include "utils/time_util.h"

namespace fs = std::filesystem;

std::string Logger::GetLogLevelString(const LogLevel level, bool color) {
  std::ostringstream oss;
  const int8_t width = 5;

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

void Logger::Log(const std::string& message, const LogLevel level) {
  Logger::TerminalLog(message, level);
  Logger::SystemLog(message, level);
}

void Logger::TerminalLog(const std::string& message, const LogLevel level) {
  std::string prefix = GetLogLevelString(level, true) + " ";
  std::istringstream msgStream(message);
  std::string line;

  if (std::getline(msgStream, line)) {
    std::cout << prefix << line << std::endl;
  }

  std::string indent(8, ' ');
  while (std::getline(msgStream, line)) {
    std::cout << indent << line << std::endl;
  }
}

void Logger::SystemLog(const std::string& message, const LogLevel level) {
  std::string base_dir = Setup::GetAppDataPath() + "/.logs";
  fs::create_directories(base_dir);
  std::string log_file_path = base_dir + "/sys.log";

  std::ofstream logFile(log_file_path, std::ios::app);
  std::string logEntry = TimeUtil::GetCurrentTimestamp() + " - " +
                         GetLogLevelString(level) + " " + message;

  if (logFile.is_open()) {
    logFile << logEntry << std::endl;
    logFile.close();
  }
}
