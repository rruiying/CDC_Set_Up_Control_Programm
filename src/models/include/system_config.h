#pragma once
#include <QString>

struct SystemConfig {
    // 串口设置
    QString defaultPort;
    int defaultBaudRate;
    
    // 系统限制
    float maxHeight;
    float minHeight;
    float maxAngle;
    float minAngle;
    
    // 传感器配置
    float sensorSpacing;
    float temperatureOffset;
    
    // 数据记录
    QString dataPath;
    int maxFileSize;  // MB
    int recordingInterval;  // ms
    
    // UI设置
    QString theme;
    bool autoConnect;
    bool showTooltips;
    
    SystemConfig() 
        : defaultBaudRate(115200)
        , maxHeight(100.0f)
        , minHeight(0.0f)
        , maxAngle(45.0f)
        , minAngle(-45.0f)
        , sensorSpacing(100.0f)
        , temperatureOffset(0.0f)
        , dataPath("./data/")
        , maxFileSize(100)
        , recordingInterval(100)
        , theme("default")
        , autoConnect(false)
        , showTooltips(true) {}
};