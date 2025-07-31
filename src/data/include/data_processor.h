#pragma once
#include <QObject>
#include <QQueue>
#include "models/include/sensor_data.h"

class DataProcessor : public QObject {
    Q_OBJECT
    
public:
    enum class FilterType {
        None,
        MovingAverage,
        ExponentialSmoothing,
        MedianFilter,
        KalmanFilter
    };
    
    struct ProcessingConfig {
        FilterType filterType;
        int windowSize;
        float smoothingFactor;
        bool enableOutlierRemoval;
        float outlierThreshold;
    };
    
    explicit DataProcessor(QObject* parent = nullptr);
    ~DataProcessor();
    
    // 数据处理
    SensorData processData(const SensorData& rawData);
    QList<SensorData> processBatch(const QList<SensorData>& rawDataList);
    
    // 配置
    void setConfig(const ProcessingConfig& config);
    ProcessingConfig getConfig() const { return m_config; }
    
    // 滤波器设置
    void setFilterType(FilterType type);
    void setWindowSize(int size);
    void setSmoothingFactor(float factor);
    
    // 特征提取
    float calculateAverage(const QList<float>& values);
    float calculateStdDev(const QList<float>& values);
    QPair<float, float> calculateMinMax(const QList<float>& values);
    
    // 异常检测
    bool isOutlier(float value, const QList<float>& history);
    
signals:
    void dataProcessed(const SensorData& data);
    void outlierDetected(const QString& parameter, float value);
    
private:
    ProcessingConfig m_config;
    
    // 历史数据缓存
    QQueue<float> m_temperatureHistory;
    QQueue<float> m_heightHistory;
    QQueue<float> m_angleHistory;
    QQueue<float> m_capacitanceHistory;
    
    // 滤波实现
    float applyFilter(float value, QQueue<float>& history);
    float movingAverage(const QQueue<float>& data);
    float exponentialSmooth(float current, float previous);
    float medianFilter(QQueue<float> data);
};