#include "models/include/sensor_data.h"
#include <QStringList>

SensorData SensorData::fromSerialData(const QString& data, const SensorData& previous) {
    SensorData result = previous;
    result.isValid = {};  // 重置有效性标志
    
    // 解析格式: "D:v1,v2,v3,v4,A:angle,T:temp,C:cap"
    QStringList parts = data.trimmed().split(',');
    
    QString currentSection;
    QStringList distanceValues;
    
    for (const QString& part : parts) {
        QString trimmedPart = part.trimmed();
        
        // 检查是否是新的数据段
        if (trimmedPart.contains(':')) {
            // 处理之前收集的距离数据
            if (currentSection == "D" && distanceValues.size() >= 4) {
                bool ok;
                result.distanceUpper1 = distanceValues[0].toFloat(&ok);
                if (ok) result.isValid.distanceUpper1 = true;
                
                result.distanceUpper2 = distanceValues[1].toFloat(&ok);
                if (ok) result.isValid.distanceUpper2 = true;
                
                result.distanceLower1 = distanceValues[2].toFloat(&ok);
                if (ok) result.isValid.distanceLower1 = true;
                
                result.distanceLower2 = distanceValues[3].toFloat(&ok);
                if (ok) result.isValid.distanceLower2 = true;
            }
            
            // 解析新段
            QStringList sectionParts = trimmedPart.split(':');
            if (sectionParts.size() >= 2) {
                currentSection = sectionParts[0];
                QString value = sectionParts[1];
                
                if (currentSection == "A") {
                    bool ok;
                    result.angle = value.toFloat(&ok);
                    if (ok) result.isValid.angle = true;
                }
                else if (currentSection == "T") {
                    bool ok;
                    result.temperature = value.toFloat(&ok);
                    if (ok) result.isValid.temperature = true;
                }
                else if (currentSection == "C") {
                    bool ok;
                    result.capacitance = value.toFloat(&ok);
                    if (ok) result.isValid.capacitance = true;
                }
                else if (currentSection == "D") {
                    distanceValues.clear();
                    distanceValues.append(value);
                }
            }
        }
        else if (currentSection == "D") {
            // 收集距离值
            distanceValues.append(trimmedPart);
        }
    }
    
    // 处理最后的距离数据（如果有）
    if (currentSection == "D" && distanceValues.size() >= 4) {
        bool ok;
        result.distanceUpper1 = distanceValues[0].toFloat(&ok);
        if (ok) result.isValid.distanceUpper1 = true;
        
        result.distanceUpper2 = distanceValues[1].toFloat(&ok);
        if (ok) result.isValid.distanceUpper2 = true;
        
        result.distanceLower1 = distanceValues[2].toFloat(&ok);
        if (ok) result.isValid.distanceLower1 = true;
        
        result.distanceLower2 = distanceValues[3].toFloat(&ok);
        if (ok) result.isValid.distanceLower2 = true;
    }
    
    result.timestamp = QDateTime::currentDateTime();
    return result;
}

QString SensorData::toCSVLine() const {
    return QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10")
        .arg(timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(distanceUpper1, 0, 'f', 2)
        .arg(distanceUpper2, 0, 'f', 2)
        .arg(distanceLower1, 0, 'f', 2)
        .arg(distanceLower2, 0, 'f', 2)
        .arg(angle, 0, 'f', 2)
        .arg(temperature, 0, 'f', 2)
        .arg(capacitance, 0, 'f', 2)
        .arg(calculatedHeight, 0, 'f', 2)
        .arg(calculatedAngle, 0, 'f', 2);
}

QString SensorData::toCSVHeader() const {
    return "Timestamp,DistanceUpper1,DistanceUpper2,DistanceLower1,DistanceLower2,"
           "Angle,Temperature,Capacitance,CalculatedHeight,CalculatedAngle";
}

bool SensorData::isTimeout(int seconds) const {
    return timestamp.secsTo(QDateTime::currentDateTime()) > seconds;
}