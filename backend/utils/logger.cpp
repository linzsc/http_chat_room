#include "logger.h"

// 初始化日志文件
void Logger::init(const std::string& logFile) {
    this->logFile.open(logFile, std::ios::out | std::ios::app);
    if (!this->logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
    }
}

// 记录日志
void Logger::log(const std::string& message, LogLevel level) {
    // 获取当前时间戳
    std::string timestamp = getCurrentTime();

    // 日志级别字符串
    const char* levelStr;
    switch (level) {
        case DEBUG: levelStr = "[DEBUG]"; break;
        case INFO: levelStr = "[INFO]"; break;
        case WARNING: levelStr = "[WARNING]"; break;
        case ERROR: levelStr = "[ERROR]"; break;
        default: levelStr = "[UNKNOWN]"; break;
    }

    // 构建日志消息
    std::string logMessage = timestamp + " " + levelStr + " " + message + "\n";

    // 输出到控制台
    std::cout << logMessage;

    // 输出到文件
    if (logFile.is_open()) {
        logFile << logMessage;
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}