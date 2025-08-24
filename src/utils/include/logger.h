#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <mutex>
#include <fstream>
#include <memory>
#include <chrono>
#include <cstdarg>
#include <condition_variable>
#include <thread>
#include <atomic>

class Logger {
public:
    enum class LogLevel : unsigned char {
        TRACE = 0,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL,
        OFF
    };

    struct LogEntry {
        std::chrono::system_clock::time_point timestamp;
        LogLevel level;
        std::string category;
        std::string message;

        LogEntry() = default;
        LogEntry(LogLevel lvl, std::string msg, std::string cat)
            : timestamp(std::chrono::system_clock::now()),
              level(lvl),
              category(std::move(cat)),
              message(std::move(msg)) {}
    };

    void clear();

    std::vector<std::string> getAllLogs() const;

    static Logger& getInstance();
    
    void setMinLevel(LogLevel level);
    LogLevel getMinLevel() const;
    void enableConsoleOutput(bool enable);
    void setLogFile(const std::string& filename, bool append = true);
    void close(); 

    // String versions with category
    void log(LogLevel level, const std::string& message, const std::string& category = "General");
    void info(const std::string& message, const std::string& category = "General");
    void warning(const std::string& message, const std::string& category = "General");
    void error(const std::string& message, const std::string& category = "General");

    // Printf-style versions - renamed to avoid ambiguity
    void infof(const char* format, ...);
    void warningf(const char* format, ...);
    void errorf(const char* format, ...);

    std::vector<LogEntry> getRecentLogs(size_t count = 100) const;

    std::vector<LogEntry> getLogsAtLeast(LogLevel minLevel) const;

    bool isConsoleOutputEnabled() const { return consoleOutput; }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void startWorkerIfNeeded_();
    void workerLoop_();
    void enqueue_(LogEntry&& entry);

    void writeLog_(const LogEntry& entry);
    static std::string toTimeString_(std::chrono::system_clock::time_point tp);
    static std::string levelToString_(LogLevel lvl);
    std::string formatLogEntry(const LogEntry& entry) const;
    static std::string vformat_(const char* fmt, va_list ap);    
    
private:
    LogLevel minLevel = LogLevel::INFO;
    bool consoleOutput = true;
    size_t maxBufferSize = 1000;

    mutable std::mutex mem_mtx_;
    std::deque<LogEntry> memoryBuffer;

    std::mutex file_mtx_;
    std::ofstream logFile;
    std::string currentLogFile;
    std::mutex close_mutex_;
    std::atomic<bool> closed_{false};

    std::mutex q_mtx_;
    std::condition_variable q_cv_;
    std::deque<LogEntry> queue_;
    std::atomic<bool> stopping{false};
    std::thread worker_;
    std::atomic<bool> workerRunning{false};
};

// Convenience macros
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

// Printf-style macros - now using infof, warningf, errorf
#if defined(_MSC_VER)
  #define LOG_INFO_F(fmt, ...)    Logger::getInstance().infof(fmt, ##__VA_ARGS__)
  #define LOG_WARNING_F(fmt, ...) Logger::getInstance().warningf(fmt, ##__VA_ARGS__)
  #define LOG_ERROR_F(fmt, ...)   Logger::getInstance().errorf(fmt, ##__VA_ARGS__)
#else
  #define LOG_INFO_F(fmt, ...)    Logger::getInstance().infof(fmt, ##__VA_ARGS__)
  #define LOG_WARNING_F(fmt, ...) Logger::getInstance().warningf(fmt, ##__VA_ARGS__)
  #define LOG_ERROR_F(fmt, ...)   Logger::getInstance().errorf(fmt, ##__VA_ARGS__)
#endif

#endif // LOGGER_H