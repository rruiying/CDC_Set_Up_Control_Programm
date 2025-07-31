#pragma once
#include <QString>
#include <QObject>
#include <QMutex>
#include <QFile>

class Logger : public QObject {
    Q_OBJECT
    
public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };
    
    static Logger& instance();
    
    void log(LogLevel level, const QString& message);
    void setLogFile(const QString& filename);
    void setLogLevel(LogLevel level);
    
signals:
    void newLogEntry(const QString& entry);
    
private:
    Logger();
    ~Logger();
    
    QString m_logFile;
    LogLevel m_minLevel = Info;
    QMutex m_mutex;
    
    QString levelToString(LogLevel level) const;
};

// 便捷宏
#define LOG_DEBUG(msg) Logger::instance().log(Logger::Debug, msg)
#define LOG_INFO(msg) Logger::instance().log(Logger::Info, msg)
#define LOG_WARNING(msg) Logger::instance().log(Logger::Warning, msg)
#define LOG_ERROR(msg) Logger::instance().log(Logger::Error, msg)