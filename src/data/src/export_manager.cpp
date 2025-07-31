#include "data/include/export_manager.h"
#include "utils/include/logger.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ExportManager::ExportManager(QObject* parent)
    : QObject(parent)
{
}

ExportManager::~ExportManager() {
}

bool ExportManager::exportData(const QList<SensorData>& data,
                              const QString& fileName,
                              const ExportOptions& options) {
    emit exportStarted();
    
    bool result = false;
    
    switch (options.format) {
        case ExportFormat::CSV:
            result = exportToCSV(data, fileName);
            break;
        case ExportFormat::JSON:
            result = exportToJSON(data, fileName);
            break;
        case ExportFormat::Excel:
            result = exportToExcel(data, fileName);
            break;
        case ExportFormat::PDF:
            result = exportToPDF(data, fileName);
            break;
    }
    
    if (result) {
        emit exportCompleted(fileName);
    } else {
        emit exportFailed("Export failed");
    }
    
    return result;
}

bool ExportManager::exportToCSV(const QList<SensorData>& data, 
                               const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open file for CSV export: %1").arg(fileName));
        return false;
    }
    
    QTextStream stream(&file);
    
    // 写入头部
    stream << "Timestamp,DistanceUpper1,DistanceUpper2,DistanceLower1,"
           << "DistanceLower2,Angle,Temperature,Capacitance,"
           << "CalculatedHeight,CalculatedAngle\n";
    
    // 写入数据
    int count = 0;
    for (const auto& d : data) {
        stream << d.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz") << ","
               << d.distanceUpper1 << ","
               << d.distanceUpper2 << ","
               << d.distanceLower1 << ","
               << d.distanceLower2 << ","
               << d.angle << ","
               << d.temperature << ","
               << d.capacitance << ","
               << d.calculatedHeight << ","
               << d.calculatedAngle << "\n";
        
        count++;
        emit exportProgress(count * 100 / data.size());
    }
    
    file.close();
    LOG_INFO(QString("Exported %1 records to CSV: %2").arg(count).arg(fileName));
    return true;
}

bool ExportManager::exportToJSON(const QList<SensorData>& data,
                                const QString& fileName) {
    QJsonArray dataArray;
    
    for (const auto& d : data) {
        QJsonObject obj;
        obj["timestamp"] = d.timestamp.toString(Qt::ISODate);
        obj["distanceUpper1"] = d.distanceUpper1;
        obj["distanceUpper2"] = d.distanceUpper2;
        obj["distanceLower1"] = d.distanceLower1;
        obj["distanceLower2"] = d.distanceLower2;
        obj["angle"] = d.angle;
        obj["temperature"] = d.temperature;
        obj["capacitance"] = d.capacitance;
        obj["calculatedHeight"] = d.calculatedHeight;
        obj["calculatedAngle"] = d.calculatedAngle;
        
        dataArray.append(obj);
    }
    
    QJsonObject root;
    root["data"] = dataArray;
    root["count"] = data.size();
    root["exportTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(root);
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file for JSON export: %1").arg(fileName));
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    LOG_INFO(QString("Exported %1 records to JSON: %2").arg(data.size()).arg(fileName));
    return true;
}

bool ExportManager::exportToExcel(const QList<SensorData>& data,
                                 const QString& fileName) {
    // Excel导出需要额外的库支持
    LOG_WARNING("Excel export not implemented yet");
    emit exportFailed("Excel export not available");
    return false;
}

bool ExportManager::exportToPDF(const QList<SensorData>& data,
                               const QString& fileName) {
    // PDF导出需要额外的库支持
    LOG_WARNING("PDF export not implemented yet");
    emit exportFailed("PDF export not available");
    return false;
}

bool ExportManager::exportReport(const QList<SensorData>& data,
                                const QString& fileName) {
    QString report = generateReport(data);
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    stream << report;
    file.close();
    
    return true;
}

QString ExportManager::generateReport(const QList<SensorData>& data) {
    QString report;
    QTextStream stream(&report);
    
    stream << "CDC Measurement Report\n";
    stream << "======================\n\n";
    stream << "Generated: " << QDateTime::currentDateTime().toString() << "\n";
    stream << "Total Records: " << data.size() << "\n\n";
    
    if (!data.isEmpty()) {
        stream << "Time Range: " << data.first().timestamp.toString()
               << " to " << data.last().timestamp.toString() << "\n\n";
        
        // 计算统计数据
        float sumTemp = 0, sumHeight = 0, sumAngle = 0, sumCap = 0;
        float minTemp = 999, maxTemp = -999;
        
        for (const auto& d : data) {
            sumTemp += d.temperature;
            sumHeight += d.calculatedHeight;
            sumAngle += d.angle;
            sumCap += d.capacitance;
            
            minTemp = std::min(minTemp, d.temperature);
            maxTemp = std::max(maxTemp, d.temperature);
        }
        
        stream << "Statistics:\n";
        stream << "-----------\n";
        stream << QString("Average Temperature: %1°C\n").arg(sumTemp / data.size(), 0, 'f', 2);
        stream << QString("Temperature Range: %1°C - %2°C\n").arg(minTemp, 0, 'f', 2).arg(maxTemp, 0, 'f', 2);
        stream << QString("Average Height: %1mm\n").arg(sumHeight / data.size(), 0, 'f', 2);
        stream << QString("Average Angle: %1°\n").arg(sumAngle / data.size(), 0, 'f', 2);
        stream << QString("Average Capacitance: %1pF\n").arg(sumCap / data.size(), 0, 'f', 2);
    }
    
    return report;
}