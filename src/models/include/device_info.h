#pragma once
#include <QString>
#include <QDateTime>

struct DeviceInfo {
    QString deviceName;
    QString portName;
    int baudRate;
    bool isConnected;
    QDateTime connectedTime;
    QString description;
    
    DeviceInfo() 
        : baudRate(115200)
        , isConnected(false) {}
};