#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>

// 日志级别枚举
enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// 日志类
class Logger {
public:
    // 单例模式
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // 禁止拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 初始化日志文件
    void init(const std::string& logFile = "server.log");

    // 记录日志
    void log(const std::string& message, LogLevel level);

    ~Logger();
private:
    // 私有构造函数
    Logger() {}

    // 日志文件流
    std::ofstream logFile;

    // 获取当前时间戳
    std::string getCurrentTime() const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        char buffer[80];
        std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
        return std::string(buffer);
    }
};

// 宏定义，方便使用
#define LOG_DEBUG(msg) Logger::getInstance().log(msg, DEBUG)
#define LOG_INFO(msg) Logger::getInstance().log(msg, INFO)
#define LOG_WARNING(msg) Logger::getInstance().log(msg, WARNING)
#define LOG_ERROR(msg) Logger::getInstance().log(msg, ERROR)

#endif // LOGGER_H