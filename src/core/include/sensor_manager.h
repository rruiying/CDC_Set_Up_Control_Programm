#pragma once
#include <QObject>
#include <QTimer>
#include <QQueue>
#include <memory>
#include "models/include/sensor_data.h"

class SensorInterface;

class SensorManager : public QObject {
    Q_OBJECT
    
public:
    struct Statistics {
        float avgTemperature;
        float avgHeight;
        float avgAngle;
        float avgCapacitance;
        int dataPoints;
        QDateTime startTime;
        QDateTime lastUpdate;
    };
    
    explicit SensorManager(SensorInterface* sensorInterface = nullptr, 
                          QObject* parent = nullptr);
    ~SensorManager();
    
    // 监控控制
    void startMonitoring(int intervalMs = 100);
    void stopMonitoring();
    bool isMonitoring() const;
    
    // 数据访问
    SensorData getCurrentData() const;
    QList<SensorData> getDataHistory(int maxPoints = 1000) const;
    Statistics getStatistics() const;
    
    // 数据处理
    void processSensorData(const SensorData& data);
    void clearHistory();
    
    // 设置
    void setHistorySize(int size);
    void setDataFilter(bool enable);
    
signals:
    void dataUpdated(const SensorData& data);
    void dataAnomalyDetected(const QString& message);
    void statisticsUpdated(const Statistics& stats);
    
private slots:
    void onSensorDataReceived(const SensorData& data);
    void updateStatistics();
    
private:
    SensorInterface* m_sensorInterface;
    SensorData m_currentData;
    QQueue<SensorData> m_dataHistory;
    Statistics m_statistics;
    QTimer* m_statsTimer;
    
    int m_maxHistorySize;
    bool m_filterEnabled;
    bool m_isMonitoring;
    
    // 数据处理
    SensorData filterData(const SensorData& data);
    void detectAnomalies(const SensorData& data);
    void calculateDerivedValues(SensorData& data);
};