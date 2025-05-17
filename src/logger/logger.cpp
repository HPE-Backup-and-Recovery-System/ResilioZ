#include "logger.h"
#include <iostream>
#include <fstream>
#include <ctime>

void Logger::log(const std::string &message, LogLevel level) {
    std::ofstream logFile("../logs/system.log", std::ios::app);
    std::time_t now = std::time(nullptr);
    std::string logLevel;

    switch (level) {
        case LogLevel::INFO:
            logLevel = "INFO";
            break;
        case LogLevel::WARNING:
            logLevel = "WARNING";
            break;
        case LogLevel::ERROR:
            logLevel = "ERROR";
            break;
    }

    std::string logEntry = std::string(std::ctime(&now)) + " - " + logLevel + ": " + message;

    if (logFile.is_open()) {
        logFile << logEntry << std::endl;
        logFile.close();
    }
}
