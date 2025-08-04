// src/utils/src/logger.cpp
#include "../include/logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>

Logger& Logger::getInstance() {
    static Logger instance;  // C++11保证线程安全的静态局部变量初始化
    return instance;
}

Logger::Logger() {
    // 构造函数
}

Logger::~Logger() {
    close();
}

bool Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 如果已有文件打开，先关闭
    if (logFile.is_open()) {
        logFile.close();
    }
    
    // 打开新文件（追加模式）
    logFile.open(filePath, std::ios::app);
    if (logFile.is_open()) {
        currentLogFile = filePath;
        
        // 写入分隔符表示新的日志会话
        logFile << "\n===== New Log Session Started at " 
                << formatTimestamp(std::chrono::system_clock::now()) 
                << " =====\n" << std::endl;
        return true;
    }
    
    return false;
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(mutex);
    if (logFile.is_open()) {
        logFile.close();
    }
    currentLogFile.clear();
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string message = formatString(format, args);
    va_end(args);
    log(LogLevel::INFO, message);
}

void Logger::warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string message = formatString(format, args);
    va_end(args);
    log(LogLevel::WARNING, message);
}

void Logger::error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string message = formatString(format, args);
    va_end(args);
    log(LogLevel::ERROR, message);
}

void Logger::logOperation(const std::string& operation, const std::string& details) {
    std::string message = operation + " - " + details;
    log(LogLevel::INFO, message, "OPERATION");
}

void Logger::logCommunication(const std::string& direction, const std::string& port, const std::string& data) {
    std::string message = direction + " [" + port + "] " + data;
    log(LogLevel::INFO, message, "COMM");
}

std::vector<LogEntry> Logger::getRecentLogs(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (count == 0 || count >= memoryBuffer.size()) {
        return std::vector<LogEntry>(memoryBuffer.begin(), memoryBuffer.end());
    }
    
    // 返回最近的count条
    return std::vector<LogEntry>(memoryBuffer.end() - count, memoryBuffer.end());
}

std::vector<LogEntry> Logger::getLogsByLevel(LogLevel level) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<LogEntry> filtered;
    
    if (level == LogLevel::ALL) {
        return std::vector<LogEntry>(memoryBuffer.begin(), memoryBuffer.end());
    }
    
    std::copy_if(memoryBuffer.begin(), memoryBuffer.end(), 
                 std::back_inserter(filtered),
                 [level](const LogEntry& entry) {
                     return entry.level == level;
                 });
    
    return filtered;
}

void Logger::clearMemoryLogs() {
    std::lock_guard<std::mutex> lock(mutex);
    memoryBuffer.clear();
}

void Logger::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    close();
    memoryBuffer.clear();
    minLevel = LogLevel::INFO;
    consoleOutput = true;
    maxBufferSize = 1000;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::ALL:     return "ALL";
        default:                return "UNKNOWN";
    }
}

LogLevel Logger::stringToLevel(const std::string& levelStr) {
    if (levelStr == "INFO")    return LogLevel::INFO;
    if (levelStr == "WARNING") return LogLevel::WARNING;
    if (levelStr == "ERROR")   return LogLevel::ERROR;
    if (levelStr == "ALL")     return LogLevel::ALL;
    return LogLevel::INFO; // 默认
}

void Logger::log(LogLevel level, const std::string& message, const std::string& category) {
    // 检查日志级别
    if (level < minLevel) {
        return;
    }
    
    // 创建日志条目
    LogEntry entry(level, message, category);
    
    // 加锁保护共享资源
    std::lock_guard<std::mutex> lock(mutex);
    
    // 添加到内存缓冲
    memoryBuffer.push_back(entry);
    
    // 限制缓冲区大小
    while (memoryBuffer.size() > maxBufferSize) {
        memoryBuffer.pop_front();
    }
    
    // 写入文件和控制台
    writeLog(entry);
}

std::string Logger::formatString(const char* format, va_list args) {
    // 首先获取需要的缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    
    int size = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);
    
    if (size < 0) {
        return "Format error";
    }
    
    // 创建缓冲区并格式化
    std::vector<char> buffer(size + 1);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    
    return std::string(buffer.data());
}

std::string Logger::formatTimestamp(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch()).count() % 1000;
    
    std::tm* tm = std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms;
    
    return oss.str();
}

void Logger::writeLog(const LogEntry& entry) {
    // 格式化日志行
    std::ostringstream oss;
    oss << "[" << formatTimestamp(entry.timestamp) << "] ";
    oss << "[" << levelToString(entry.level) << "] ";
    
    if (entry.category != "SYSTEM") {
        oss << "[" << entry.category << "] ";
    }
    
    oss << entry.message;
    
    std::string logLine = oss.str();
    
    // 写入文件
    if (logFile.is_open()) {
        logFile << logLine << std::endl;
        logFile.flush(); // 确保立即写入
    }
    
    // 输出到控制台
    if (consoleOutput) {
        // 根据级别使用不同的输出流
        switch (entry.level) {
            case LogLevel::ERROR:
                std::cerr << logLine << std::endl;
                break;
            default:
                std::cout << logLine << std::endl;
                break;
        }
    }
}