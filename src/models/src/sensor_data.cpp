#include "models/include/sensor_data.h"
#include <QStringList>

SensorData SensorData::fromSerialData(const QString& data, 
                                     const SensorData& previous) {
    SensorData result = previous;
    result.isValid = {};  // 重置有效性标志
    
    // 简单解析，格式: "D:v1,v2,v3,v4,A:angle,T:temp,C:cap"
    QStringList segments = data.trimmed().split(',');
    
    for (const QString& segment : segments) {
        if (segment.startsWith("D:")) {
            // 处理距离数据
            QString values = segment.mid(2);
            bool ok;
            float val = values.toFloat(&ok);
            if (ok) {
                result.distanceUpper1 = val;
                result.isValid.distanceUpper1 = true;
            }
        }
        else if (segment.startsWith("T:")) {
            bool ok;
            float temp = segment.mid(2).toFloat(&ok);
            if (ok) {
                result.temperature = temp;
                result.isValid.temperature = true;
            }
        }
        else if (segment.startsWith("A:")) {
            bool ok;
            float angle = segment.mid(2).toFloat(&ok);
            if (ok) {
                result.angle = angle;
                result.isValid.angle = true;
            }
        }
        // 继续处理其他数据...
    }
    
    result.timestamp = QDateTime::currentDateTime();
    return result;
}

QString SensorData::toCSVLine() const {
    return QString("%1,%2,%3,%4,%5,%6,%7,%8,%9")
        .arg(timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(distanceUpper1).arg(distanceUpper2)
        .arg(distanceLower1).arg(distanceLower2)
        .arg(angle).arg(temperature).arg(capacitance)
        .arg(timestamp.toMSecsSinceEpoch());
}

QString SensorData::toCSVHeader() const {
    return "Timestamp,DistUpper1,DistUpper2,DistLower1,DistLower2,"
           "Angle,Temperature,Capacitance,TimestampMs";
}

bool SensorData::isTimeout(int seconds) const {
    return timestamp.secsTo(QDateTime::currentDateTime()) > seconds;
}