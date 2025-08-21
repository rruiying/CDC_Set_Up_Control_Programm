#include "../include/logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstdio>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    startWorkerIfNeeded_();
}

Logger::~Logger() {
    close();
}

void Logger::startWorkerIfNeeded_() {
    bool expected = false;
    if (workerRunning.compare_exchange_strong(expected, true)) {
        stopping.store(false);
        worker_ = std::thread(&Logger::workerLoop_, this);
    }
}

void Logger::workerLoop_() {
    std::unique_lock<std::mutex> lk(q_mtx_);
    for (;;) {
        q_cv_.wait(lk, [this]{ return stopping.load() || !queue_.empty(); });
        if (stopping.load() && queue_.empty()) break;
        auto entry = std::move(queue_.front());
        queue_.pop_front();
        lk.unlock();
        writeLog_(entry); // 写I/O不持有队列锁
        lk.lock();
    }
}

void Logger::enqueue_(LogEntry&& entry) {
    {
        std::lock_guard<std::mutex> lk(q_mtx_);
        queue_.push_back(std::move(entry));
    }
    q_cv_.notify_one();
}


void Logger::close() {
    std::lock_guard<std::mutex> close_lock(close_mutex_);

    bool expected = false;
    if (!closed_.compare_exchange_strong(expected, true)) {
        return;
    }

    // 停止worker
    if (workerRunning.load()) {
        {
            std::lock_guard<std::mutex> lk(q_mtx_);
            stopping.store(true);
        }
        q_cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
        workerRunning.store(false);
    }

    // 关闭文件
    {
        std::lock_guard<std::mutex> lock(file_mtx_);
        if (logFile.is_open()) {
            logFile.close();
        }
        currentLogFile.clear();
    }
}

void Logger::setMinLevel(LogLevel level) {
    minLevel = level;
}

Logger::LogLevel Logger::getMinLevel() const {
    return minLevel;
}

void Logger::enableConsoleOutput(bool enable) {
    consoleOutput = enable;
}

void Logger::setLogFile(const std::string& filename, bool append) {
    std::lock_guard<std::mutex> lock(file_mtx_);
    if (logFile.is_open()) logFile.close();
    std::ios::openmode mode = std::ios::out | (append ? std::ios::app : std::ios::trunc);
    logFile.open(filename, mode);
    currentLogFile = filename;
}

void Logger::log(LogLevel level, const std::string& message, const std::string& category) {
    if (closed_.load()) return;
    if (level < minLevel) return;

    LogEntry entry(level, message, category);

    {
        std::lock_guard<std::mutex> lock(mem_mtx_);
        memoryBuffer.push_back(entry);
        while (memoryBuffer.size() > maxBufferSize) memoryBuffer.pop_front();
    }

    enqueue_(std::move(entry));
}

void Logger::info(const std::string& message, const std::string& category) {
    log(LogLevel::INFO, message, category);
}
void Logger::warning(const std::string& message, const std::string& category) {
    log(LogLevel::WARNING, message, category);
}
void Logger::error(const std::string& message, const std::string& category) {
    log(LogLevel::ERROR, message, category);
}

std::string Logger::vformat_(const char* fmt, va_list ap) {
    if (!fmt) return {};
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int size = std::vsnprintf(nullptr, 0, fmt, ap_copy);
    va_end(ap_copy);
    if (size <= 0) return {};
    std::string buf;
    buf.resize(static_cast<size_t>(size) + 1);
    std::vsnprintf(buf.data(), buf.size(), fmt, ap);
    buf.resize(static_cast<size_t>(size));
    return buf;
}
void Logger::infof(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string msg = vformat_(format, args);
    va_end(args);
    log(LogLevel::INFO, msg);
}

void Logger::warningf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string msg = vformat_(format, args);
    va_end(args);
    log(LogLevel::WARNING, msg);
}

void Logger::errorf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string msg = vformat_(format, args);
    va_end(args);
    log(LogLevel::ERROR, msg);
}

std::vector<Logger::LogEntry> Logger::getRecentLogs(size_t count) const {
    std::lock_guard<std::mutex> lock(mem_mtx_);
    std::vector<LogEntry> out;
    if (count >= memoryBuffer.size()) {
        out.assign(memoryBuffer.begin(), memoryBuffer.end());
    } else {
        out.reserve(count);
        auto it = memoryBuffer.end();
        std::advance(it, -static_cast<long>(count));
        out.assign(it, memoryBuffer.end());
    }
    return out;
}

std::vector<Logger::LogEntry> Logger::getLogsAtLeast(LogLevel minLvl) const {
    std::lock_guard<std::mutex> lock(mem_mtx_);
    std::vector<LogEntry> out;
    for (const auto& e : memoryBuffer) {
        if (e.level >= minLvl) out.push_back(e);
    }
    return out;
}

std::string Logger::toTimeString_(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::levelToString_(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        case LogLevel::OFF: return "OFF";
    }
    return "UNKNOWN";
}

void Logger::writeLog_(const LogEntry& entry) {
    std::ostringstream oss;
    oss << "[" << toTimeString_(entry.timestamp) << "]"
        << " [" << levelToString_(entry.level) << "]";
    if (!entry.category.empty()) oss << " [" << entry.category << "]";
    oss << " " << entry.message;

    const std::string line = oss.str();

    {
        std::lock_guard<std::mutex> lock(file_mtx_);
        if (logFile.is_open()) {
            logFile << line << '\n';
            if (entry.level >= LogLevel::ERROR) logFile.flush();
        }
    }

    if (consoleOutput) {
        if (entry.level >= LogLevel::WARNING) {
            std::cerr << line << std::endl;
        } else {
            std::cout << line << std::endl;
        }
    }
}

void Logger::clear() {
    {
        std::lock_guard<std::mutex> lock(mem_mtx_);
        memoryBuffer.clear();
    }
    {
        std::lock_guard<std::mutex> lock(q_mtx_);
        queue_.clear();
    }
}

std::vector<std::string> Logger::getAllLogs() const {
    std::lock_guard<std::mutex> lock(mem_mtx_);
    std::vector<std::string> result;
    result.reserve(memoryBuffer.size());
    
    for (const auto& entry : memoryBuffer) {
        result.push_back(formatLogEntry(entry));
    }
    
    return result;
}

std::string Logger::formatLogEntry(const LogEntry& entry) const {
    std::stringstream ss;
    ss << toTimeString_(entry.timestamp) 
       << " [" << levelToString_(entry.level) << "] "
       << "[" << entry.category << "] "
       << entry.message;
    return ss.str();
}