#pragma once
#include <QObject>
#include <QTimer>
#include "models/include/sensor_data.h"

class SerialInterface;

class SensorInterface : public QObject {
    Q_OBJECT
    
public:
    explicit SensorInterface(SerialInterface* serial, QObject* parent = nullptr);
    ~SensorInterface();
    
    // 数据请求
    bool requestAllSensorData();
    bool requestDistanceSensors();
    bool requestAngleSensor();
    bool requestTemperature();
    bool requestCapacitance();
    
    // 自动轮询
    void startPolling(int intervalMs = 100);
    void stopPolling();
    bool isPolling() const;
    
    // 数据处理
    void processData(const QString& data);
    
    // 获取最新数据
    SensorData getLatestData() const { return m_latestData; }
    
signals:
    void sensorDataReceived(const SensorData& data);
    void dataError(const QString& error);
    void connectionLost();
    
private slots:
    void onSerialDataReceived(const QString& line);
    void onPollTimer();
    
private:
    SerialInterface* m_serial;
    QTimer* m_pollTimer;
    SensorData m_latestData;
    
    // 解析不同类型的数据
    bool parseDistanceData(const QString& data);
    bool parseAngleData(const QString& data);
    bool parseTemperatureData(const QString& data);
    bool parseCapacitanceData(const QString& data);
    
    // 验证数据
    bool validateSensorData(const SensorData& data);
};