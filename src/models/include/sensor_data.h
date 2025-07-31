#pragma once
#include <QString>
#include <QDateTime>

struct SensorData {
    // 传感器数据
    float distanceUpper1 = 0.0f;
    float distanceUpper2 = 0.0f;
    float distanceLower1 = 0.0f;
    float distanceLower2 = 0.0f;
    float angle = 0.0f;
    float temperature = 20.0f;
    float capacitance = 0.0f;
    
    // 数据有效性
    struct ValidFlags {
        bool distanceUpper1 = false;
        bool distanceUpper2 = false;
        bool distanceLower1 = false;
        bool distanceLower2 = false;
        bool angle = false;
        bool temperature = false;
        bool capacitance = false;
    } isValid;
    
    // 时间戳
    QDateTime timestamp;
    
    // 方法
    static SensorData fromSerialData(const QString& data, 
                                   const SensorData& previous = SensorData());
    QString toCSVLine() const;
    QString toCSVHeader() const;
    bool isTimeout(int seconds) const;
};