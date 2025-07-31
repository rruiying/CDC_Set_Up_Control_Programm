#include "utils/include/logger.h"
#include <QDateTime>
#include <QTextStream>
#include <QMutexLocker>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : m_logFile("application.log") {
}

Logger::~Logger() {
}

void Logger::log(LogLevel level, const QString& message) {
    if (level < m_minLevel) return;
    
    QMutexLocker locker(&m_mutex);
    
    QString timestamp = QDateTime::currentDateTime()
                       .toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString entry = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);
    
    // 写入文件
    QFile file(m_logFile);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << entry << Qt::endl;
        file.close();
    }
    
    // 发送信号供UI使用
    emit newLogEntry(entry);
}

void Logger::setLogFile(const QString& filename) {
    QMutexLocker locker(&m_mutex);
    m_logFile = filename;
}

void Logger::setLogLevel(LogLevel level) {
    m_minLevel = level;
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case Debug: return "DEBUG";
        case Info: return "INFO";
        case Warning: return "WARNING";
        case Error: return "ERROR";
        default: return "UNKNOWN";
    }
}