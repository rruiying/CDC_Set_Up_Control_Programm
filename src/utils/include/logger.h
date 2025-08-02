// src/utils/include/logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <fstream>
#include <memory>
#include <chrono>
#include <cstdarg>

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    ALL = -1  // 用于过滤时表示所有级别
};

/**
 * @brief 日志条目结构
 */
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string message;
    std::string category;  // 日志类别（如 SYSTEM, COMM, OPERATION）
    
    LogEntry(LogLevel lvl, const std::string& msg, const std::string& cat = "SYSTEM")
        : timestamp(std::chrono::system_clock::now()), level(lvl), message(msg), category(cat) {}
};

/**
 * @brief 日志系统类（单例模式）
 * 
 * 提供线程安全的日志记录功能，支持：
 * - 文件和控制台输出
 * - 多级别日志过滤
 * - 内存缓冲区
 * - 格式化输出
 */
class Logger {
public:
    /**
     * @brief 获取Logger单例实例
     * @return Logger实例引用
     */
    static Logger& getInstance();
    
    // 删除拷贝构造和赋值操作
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    /**
     * @brief 设置日志文件路径
     * @param filePath 日志文件路径
     * @return true 如果成功打开文件
     */
    bool setLogFile(const std::string& filePath);
    
    /**
     * @brief 关闭日志文件
     */
    void close();
    
    /**
     * @brief 设置是否输出到控制台
     * @param enable true启用控制台输出
     */
    void setConsoleOutput(bool enable) { consoleOutput = enable; }
    
    /**
     * @brief 设置日志级别
     * @param level 最低记录级别
     */
    void setLogLevel(LogLevel level) { minLevel = level; }
    
    /**
     * @brief 设置内存缓冲区大小
     * @param size 缓冲区最大条目数
     */
    void setMemoryBufferSize(size_t size) { maxBufferSize = size; }
    
    // 基本日志方法
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
    // 格式化日志方法（类似printf）
    void info(const char* format, ...);
    void warning(const char* format, ...);
    void error(const char* format, ...);
    
    // 特殊类型日志
    void logOperation(const std::string& operation, const std::string& details);
    void logCommunication(const std::string& direction, const std::string& port, const std::string& data);
    
    /**
     * @brief 获取最近的日志条目
     * @param count 获取的条目数（0表示全部）
     * @return 日志条目向量
     */
    std::vector<LogEntry> getRecentLogs(size_t count = 0) const;
    
    /**
     * @brief 获取特定级别的日志
     * @param level 日志级别
     * @return 符合级别的日志条目
     */
    std::vector<LogEntry> getLogsByLevel(LogLevel level) const;
    
    /**
     * @brief 清空内存中的日志
     */
    void clearMemoryLogs();
    
    /**
     * @brief 重置Logger状态（主要用于测试）
     */
    void reset();
    
    /**
     * @brief 获取日志级别的字符串表示
     * @param level 日志级别
     * @return 级别字符串
     */
    static std::string levelToString(LogLevel level);
    
    /**
     * @brief 从字符串解析日志级别
     * @param levelStr 级别字符串
     * @return 日志级别
     */
    static LogLevel stringToLevel(const std::string& levelStr);
    
private:
    Logger();
    ~Logger();
    
    // 实际的日志记录方法
    void log(LogLevel level, const std::string& message, const std::string& category = "SYSTEM");
    
    // 格式化带可变参数的字符串
    std::string formatString(const char* format, va_list args);
    
    // 格式化时间戳
    std::string formatTimestamp(const std::chrono::system_clock::time_point& time) const;
    
    // 写入日志到文件和控制台
    void writeLog(const LogEntry& entry);
    
    // 成员变量
    mutable std::mutex mutex;           // 保护所有成员的互斥锁
    std::ofstream logFile;              // 日志文件流
    std::deque<LogEntry> memoryBuffer;  // 内存中的日志缓冲
    
    LogLevel minLevel = LogLevel::INFO; // 最低记录级别
    bool consoleOutput = true;          // 是否输出到控制台
    size_t maxBufferSize = 1000;        // 内存缓冲区最大大小
    std::string currentLogFile;         // 当前日志文件路径
    
    // 单例实例
    static std::unique_ptr<Logger> instance;
    static std::mutex instanceMutex;
};

// 便捷的全局日志宏
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

// 格式化日志宏
#define LOG_INFO_F(fmt, ...) Logger::getInstance().info(fmt, __VA_ARGS__)
#define LOG_WARNING_F(fmt, ...) Logger::getInstance().warning(fmt, __VA_ARGS__)
#define LOG_ERROR_F(fmt, ...) Logger::getInstance().error(fmt, __VA_ARGS__)

#endif // LOGGER_H