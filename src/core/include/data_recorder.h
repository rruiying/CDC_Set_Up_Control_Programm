#pragma once
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QQueue>
#include <memory>
#include "models/include/sensor_data.h"

class DataRecorder : public QObject {
    Q_OBJECT
    
public:
    enum class Format {
        CSV,
        JSON,
        Binary
    };
    
    struct RecordingInfo {
        QString fileName;
        QDateTime startTime;
        int recordCount;
        qint64 fileSize;
        bool isRecording;
    };
    
    explicit DataRecorder(QObject* parent = nullptr);
    ~DataRecorder();
    
    // 记录控制
    bool startRecording(const QString& fileName = QString());
    void stopRecording();
    bool isRecording() const;
    void pauseRecording();
    void resumeRecording();
    
    // 数据记录
    void recordData(const SensorData& data);
    void recordEvent(const QString& event);
    
    // 配置
    void setFormat(Format format);
    void setBufferSize(int size);
    void setMaxFileSize(qint64 bytes);
    void setMaxDuration(int seconds);
    void setDataPath(const QString& path);
    
    // 缓冲区管理
    void flushBuffer();
    int getBufferedCount() const;
    
    // 文件管理
    QString generateFileName() const;
    static QStringList getRecordedFiles(const QString& path);
    
    // 获取信息
    RecordingInfo getRecordingInfo() const;
    qint64 getMaxFileSize() const { return m_maxFileSize; }
    int getMaxDuration() const { return m_maxDuration; }
    
signals:
    void recordingStarted(const QString& fileName);
    void recordingStopped(const QString& fileName, int recordCount);
    void recordingError(const QString& error);
    void fileSizeLimit();
    void timeLimitReached();
    
private slots:
    void checkLimits();
    void autoFlush();
    
private:
    std::unique_ptr<QFile> m_file;
    std::unique_ptr<QTextStream> m_stream;
    QTimer* m_limitTimer;
    QTimer* m_flushTimer;
    
    Format m_format;
    QString m_dataPath;
    RecordingInfo m_info;
    bool m_isPaused;
    
    // 缓冲
    QQueue<SensorData> m_dataBuffer;
    int m_bufferSize;
    
    // 限制
    qint64 m_maxFileSize;
    int m_maxDuration;
    
    // 内部方法
    bool openFile(const QString& fileName);
    void closeFile();
    void writeHeader();
    void writeData(const SensorData& data);
    QString formatDataAsCSV(const SensorData& data) const;
    QString formatDataAsJSON(const SensorData& data) const;
};