#ifndef LOGGER_H
#define LOGGER_H

#include <string>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static void log(const std::string &message, LogLevel level = LogLevel::INFO);
};

#endif 