#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

void Logger::log(const std::string& message, LogLevel level) {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    // Format time
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    
    // Get level string
    std::string level_str;
    switch (level) {
        case LogLevel::INFO:
            level_str = "INFO";
            break;
        case LogLevel::WARNING:
            level_str = "WARNING";
            break;
        case LogLevel::ERROR:
            level_str = "ERROR";
            break;
    }
    
    // Output formatted log message
    std::cout << "[" << ss.str() << "] [" << level_str << "] " << message << std::endl;
}
