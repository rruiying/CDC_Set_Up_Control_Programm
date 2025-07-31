#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include "models/include/sensor_data.h"

class ExportManager : public QObject {
    Q_OBJECT
    
public:
    enum class ExportFormat {
        CSV,
        JSON,
        Excel,
        PDF
    };
    
    struct ExportOptions {
        ExportFormat format;
        bool includeHeader;
        bool includeTimestamp;
        bool includeStatistics;
        QString delimiter;  // for CSV
        int precision;      // decimal places
    };
    
    explicit ExportManager(QObject* parent = nullptr);
    ~ExportManager();
    
    // 导出方法
    bool exportData(const QList<SensorData>& data, 
                   const QString& fileName,
                   const ExportOptions& options);
    
    bool exportToCSV(const QList<SensorData>& data, const QString& fileName);
    bool exportToJSON(const QList<SensorData>& data, const QString& fileName);
    bool exportToExcel(const QList<SensorData>& data, const QString& fileName);
    bool exportToPDF(const QList<SensorData>& data, const QString& fileName);
    
    // 导出统计报告
    bool exportReport(const QList<SensorData>& data, 
                     const QString& fileName);
    
    // 批量导出
    bool exportBatch(const QStringList& sourceFiles, 
                    const QString& outputFile,
                    ExportFormat format);
    
signals:
    void exportStarted();
    void exportProgress(int percent);
    void exportCompleted(const QString& fileName);
    void exportFailed(const QString& error);
    
private:
    QString generateCSVContent(const QList<SensorData>& data,
                              const ExportOptions& options);
    QString generateJSONContent(const QList<SensorData>& data);
    QString generateReport(const QList<SensorData>& data);
};