#include "core/include/data_recorder.h"
#include "utils/include/logger.h"
#include "utils/include/config_manager.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

DataRecorder::DataRecorder(QObject* parent)
    : QObject(parent)
    , m_limitTimer(new QTimer(this))
    , m_flushTimer(new QTimer(this))
    , m_format(Format::CSV)
    , m_dataPath("./runtime/data/exports/")
    , m_isPaused(false)
    , m_bufferSize(100)
    , m_maxFileSize(100 * 1024 * 1024) // 100MB
    , m_maxDuration(3600) // 1小时
{
    // 确保数据目录存在
    QDir().mkpath(m_dataPath);
    
    // 定时检查限制
    connect(m_limitTimer, &QTimer::timeout,
            this, &DataRecorder::checkLimits);
    
    // 定时刷新缓冲区
    connect(m_flushTimer, &QTimer::timeout,
            this, &DataRecorder::autoFlush);
    m_flushTimer->start(1000); // 每秒刷新一次
    
    // 从配置加载路径
    ConfigManager& config = ConfigManager::instance();
    // m_dataPath = config.getDataPath(); // 如果配置中有的话
}

DataRecorder::~DataRecorder() {
    if (isRecording()) {
        stopRecording();
    }
}

bool DataRecorder::startRecording(const QString& fileName) {
    if (isRecording()) {
        LOG_WARNING("Already recording");
        return false;
    }
    
    QString actualFileName = fileName.isEmpty() ? generateFileName() : fileName;
    
    if (!openFile(actualFileName)) {
        return false;
    }
    
    m_info.fileName = actualFileName;
    m_info.startTime = QDateTime::currentDateTime();
    m_info.recordCount = 0;
    m_info.fileSize = 0;
    m_info.isRecording = true;
    
    writeHeader();
    
    m_limitTimer->start(1000); // 每秒检查一次限制
    
    LOG_INFO(QString("Started recording to: %1").arg(actualFileName));
    emit recordingStarted(actualFileName);
    
    return true;
}

void DataRecorder::stopRecording() {
    if (!isRecording()) {
        return;
    }
    
    flushBuffer();
    closeFile();
    
    m_limitTimer->stop();
    m_info.isRecording = false;
    
    LOG_INFO(QString("Stopped recording. Records: %1").arg(m_info.recordCount));
    emit recordingStopped(m_info.fileName, m_info.recordCount);
}

bool DataRecorder::isRecording() const {
    return m_info.isRecording;
}

void DataRecorder::pauseRecording() {
    m_isPaused = true;
    LOG_INFO("Recording paused");
}

void DataRecorder::resumeRecording() {
    m_isPaused = false;
    LOG_INFO("Recording resumed");
}

void DataRecorder::recordData(const SensorData& data) {
    if (!isRecording() || m_isPaused) {
        return;
    }
    
    m_dataBuffer.enqueue(data);
    
    // 如果缓冲区满了，立即刷新
    if (m_dataBuffer.size() >= m_bufferSize) {
        flushBuffer();
    }
}

void DataRecorder::recordEvent(const QString& event) {
    if (!isRecording() || m_isPaused) {
        return;
    }
    
    if (m_stream) {
        QString timestamp = QDateTime::currentDateTime()
                          .toString("yyyy-MM-dd hh:mm:ss.zzz");
        *m_stream << QString("# Event at %1: %2").arg(timestamp).arg(event) 
                  << Qt::endl;
        m_stream->flush();
    }
}

void DataRecorder::setFormat(Format format) {
    if (isRecording()) {
        LOG_WARNING("Cannot change format while recording");
        return;
    }
    
    m_format = format;
}

void DataRecorder::setBufferSize(int size) {
    m_bufferSize = size;
}

void DataRecorder::setMaxFileSize(qint64 bytes) {
    m_maxFileSize = bytes;
}

void DataRecorder::setMaxDuration(int seconds) {
    m_maxDuration = seconds;
}

void DataRecorder::setDataPath(const QString& path) {
    m_dataPath = path;
    QDir().mkpath(m_dataPath);
}

void DataRecorder::flushBuffer() {
    if (!m_stream || m_dataBuffer.isEmpty()) {
        return;
    }
    
    while (!m_dataBuffer.isEmpty()) {
        writeData(m_dataBuffer.dequeue());
    }
    
    m_stream->flush();
}

int DataRecorder::getBufferedCount() const {
    return m_dataBuffer.size();
}

QString DataRecorder::generateFileName() const {
    QString timestamp = QDateTime::currentDateTime()
                       .toString("yyyyMMdd_hhmmss");
    QString extension;
    
    switch (m_format) {
        case Format::CSV:
            extension = "csv";
            break;
        case Format::JSON:
            extension = "json";
            break;
        case Format::Binary:
            extension = "dat";
            break;
    }
    
    return QString("%1CDC_Data_%2.%3")
           .arg(m_dataPath)
           .arg(timestamp)
           .arg(extension);
}

QStringList DataRecorder::getRecordedFiles(const QString& path) {
    QDir dir(path);
    return dir.entryList(QStringList() << "CDC_Data_*.csv" 
                                      << "CDC_Data_*.json" 
                                      << "CDC_Data_*.dat",
                        QDir::Files, QDir::Time);
}

DataRecorder::RecordingInfo DataRecorder::getRecordingInfo() const {
    return m_info;
}

void DataRecorder::checkLimits() {
    if (!isRecording()) {
        return;
    }
    
    // 检查文件大小
    if (m_file && m_file->size() > m_maxFileSize) {
        LOG_WARNING("File size limit reached");
        emit fileSizeLimit();
        stopRecording();
        return;
    }
    
    // 检查时间限制
    int elapsed = m_info.startTime.secsTo(QDateTime::currentDateTime());
    if (elapsed > m_maxDuration) {
        LOG_WARNING("Time limit reached");
        emit timeLimitReached();
        stopRecording();
    }
}

void DataRecorder::autoFlush() {
    if (isRecording() && !m_dataBuffer.isEmpty()) {
        flushBuffer();
    }
}

bool DataRecorder::openFile(const QString& fileName) {
    m_file = std::make_unique<QFile>(fileName);
    
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString error = QString("Failed to open file: %1").arg(fileName);
        LOG_ERROR(error);
        emit recordingError(error);
        m_file.reset();
        return false;
    }
    
    m_stream = std::make_unique<QTextStream>(m_file.get());
    return true;
}

void DataRecorder::closeFile() {
    if (m_stream) {
        m_stream->flush();
        m_stream.reset();
    }
    
    if (m_file) {
        m_info.fileSize = m_file->size();
        m_file->close();
        m_file.reset();
    }
}

void DataRecorder::writeHeader() {
    if (!m_stream) return;
    
    switch (m_format) {
        case Format::CSV:
            *m_stream << "# CDC Measurement Data" << Qt::endl;
            *m_stream << "# Start Time: " 
                     << m_info.startTime.toString("yyyy-MM-dd hh:mm:ss") 
                     << Qt::endl;
            *m_stream << SensorData().toCSVHeader() << Qt::endl;
            break;
            
        case Format::JSON:
            *m_stream << "{" << Qt::endl;
            *m_stream << "  \"metadata\": {" << Qt::endl;
            *m_stream << "    \"startTime\": \"" 
                     << m_info.startTime.toString(Qt::ISODate) 
                     << "\"," << Qt::endl;
            *m_stream << "    \"format\": \"CDC Data Format v1.0\"" << Qt::endl;
            *m_stream << "  }," << Qt::endl;
            *m_stream << "  \"data\": [" << Qt::endl;
            break;
            
        case Format::Binary:
            // 二进制格式头部
            break;
    }
}

void DataRecorder::writeData(const SensorData& data) {
    if (!m_stream) return;
    
    switch (m_format) {
        case Format::CSV:
            *m_stream << formatDataAsCSV(data) << Qt::endl;
            break;
            
        case Format::JSON:
            if (m_info.recordCount > 0) {
                *m_stream << "," << Qt::endl;
            }
            *m_stream << formatDataAsJSON(data);
            break;
            
        case Format::Binary:
            // 实现二进制写入
            break;
    }
    
    m_info.recordCount++;
}

QString DataRecorder::formatDataAsCSV(const SensorData& data) const {
    return data.toCSVLine();
}

QString DataRecorder::formatDataAsJSON(const SensorData& data) const {
    QJsonObject obj;
    obj["timestamp"] = data.timestamp.toString(Qt::ISODate);
    obj["distanceUpper1"] = data.distanceUpper1;
    obj["distanceUpper2"] = data.distanceUpper2;
    obj["distanceLower1"] = data.distanceLower1;
    obj["distanceLower2"] = data.distanceLower2;
    obj["angle"] = data.angle;
    obj["temperature"] = data.temperature;
    obj["capacitance"] = data.capacitance;
    
    QJsonDocument doc(obj);
    return QString("    ") + doc.toJson(QJsonDocument::Compact);
}