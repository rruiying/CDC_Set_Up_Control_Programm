#include "core/include/sensor_manager.h"
#include "hardware/include/sensor_interface.h"
#include "utils/include/logger.h"
#include "utils/include/math_utils.h"
#include "utils/include/config_manager.h"

SensorManager::SensorManager(SensorInterface* sensorInterface, QObject* parent)
    : QObject(parent)
    , m_sensorInterface(sensorInterface)
    , m_statsTimer(new QTimer(this))
    , m_maxHistorySize(10000)
    , m_filterEnabled(true)
    , m_isMonitoring(false)
{
    if (m_sensorInterface) {
        connect(m_sensorInterface, &SensorInterface::sensorDataReceived,
                this, &SensorManager::onSensorDataReceived);
    }
    
    // 统计更新定时器
    connect(m_statsTimer, &QTimer::timeout,
            this, &SensorManager::updateStatistics);
    m_statsTimer->start(1000); // 每秒更新统计
    
    // 初始化统计
    m_statistics = Statistics{};
    m_statistics.startTime = QDateTime::currentDateTime();
}

SensorManager::~SensorManager() {
    stopMonitoring();
}

void SensorManager::startMonitoring(int intervalMs) {
    if (m_sensorInterface) {
        m_sensorInterface->startPolling(intervalMs);
        m_isMonitoring = true;
        LOG_INFO(QString("Sensor monitoring started at %1ms interval").arg(intervalMs));
    }
}

void SensorManager::stopMonitoring() {
    if (m_sensorInterface) {
        m_sensorInterface->stopPolling();
        m_isMonitoring = false;
        LOG_INFO("Sensor monitoring stopped");
    }
}

bool SensorManager::isMonitoring() const {
    return m_isMonitoring;
}

SensorData SensorManager::getCurrentData() const {
    return m_currentData;
}

QList<SensorData> SensorManager::getDataHistory(int maxPoints) const {
    QList<SensorData> history;
    int count = std::min(maxPoints, m_dataHistory.size());
    
    // 从队列末尾开始取数据（最新的）
    for (int i = m_dataHistory.size() - count; i < m_dataHistory.size(); ++i) {
        history.append(m_dataHistory.at(i));
    }
    
    return history;
}

SensorManager::Statistics SensorManager::getStatistics() const {
    return m_statistics;
}

void SensorManager::processSensorData(const SensorData& data) {
    // 计算衍生值
    SensorData processedData = data;
    calculateDerivedValues(processedData);
    
    // 应用滤波
    if (m_filterEnabled) {
        processedData = filterData(processedData);
    }
    
    // 检测异常
    detectAnomalies(processedData);
    
    // 更新当前数据
    m_currentData = processedData;
    
    // 添加到历史
    m_dataHistory.enqueue(processedData);
    if (m_dataHistory.size() > m_maxHistorySize) {
        m_dataHistory.dequeue();
    }
    
    // 发送更新信号
    emit dataUpdated(processedData);
}

void SensorManager::clearHistory() {
    m_dataHistory.clear();
    m_statistics = Statistics{};
    m_statistics.startTime = QDateTime::currentDateTime();
    LOG_INFO("Sensor data history cleared");
}

void SensorManager::setHistorySize(int size) {
    m_maxHistorySize = size;
    
    // 如果当前历史超过新大小，裁剪
    while (m_dataHistory.size() > m_maxHistorySize) {
        m_dataHistory.dequeue();
    }
}

void SensorManager::setDataFilter(bool enable) {
    m_filterEnabled = enable;
    LOG_INFO(QString("Data filtering %1").arg(enable ? "enabled" : "disabled"));
}

void SensorManager::onSensorDataReceived(const SensorData& data) {
    processSensorData(data);
}

void SensorManager::updateStatistics() {
    if (m_dataHistory.isEmpty()) return;
    
    float sumTemp = 0, sumHeight = 0, sumAngle = 0, sumCap = 0;
    int validTemp = 0, validHeight = 0, validAngle = 0, validCap = 0;
    
    for (const auto& data : m_dataHistory) {
        if (data.isValid.temperature) {
            sumTemp += data.temperature;
            validTemp++;
        }
        if (data.isValid.distanceUpper1 && data.isValid.distanceUpper2) {
            sumHeight += (data.distanceUpper1 + data.distanceUpper2) / 2.0f;
            validHeight++;
        }
        if (data.isValid.angle) {
            sumAngle += data.angle;
            validAngle++;
        }
        if (data.isValid.capacitance) {
            sumCap += data.capacitance;
            validCap++;
        }
    }
    
    m_statistics.avgTemperature = validTemp > 0 ? sumTemp / validTemp : 0;
    m_statistics.avgHeight = validHeight > 0 ? sumHeight / validHeight : 0;
    m_statistics.avgAngle = validAngle > 0 ? sumAngle / validAngle : 0;
    m_statistics.avgCapacitance = validCap > 0 ? sumCap / validCap : 0;
    m_statistics.dataPoints = m_dataHistory.size();
    m_statistics.lastUpdate = QDateTime::currentDateTime();
    
    emit statisticsUpdated(m_statistics);
}

SensorData SensorManager::filterData(const SensorData& data) {
    SensorData filtered = data;
    
    // 简单的指数平滑滤波
    const float alpha = 0.3f;
    
    if (!m_dataHistory.isEmpty()) {
        const SensorData& prev = m_dataHistory.last();
        
        if (data.isValid.temperature && prev.isValid.temperature) {
            filtered.temperature = MathUtils::exponentialSmooth(
                prev.temperature, data.temperature, alpha);
        }
        
        if (data.isValid.distanceUpper1 && prev.isValid.distanceUpper1) {
            filtered.distanceUpper1 = MathUtils::exponentialSmooth(
                prev.distanceUpper1, data.distanceUpper1, alpha);
        }
        
        // 对其他传感器数据应用相同的滤波
    }
    
    return filtered;
}

void SensorManager::detectAnomalies(const SensorData& data) {
    // 温度异常检测
    if (data.isValid.temperature) {
        if (data.temperature > 60.0f) {
            emit dataAnomalyDetected(
                QString("High temperature detected: %1°C").arg(data.temperature));
        }
        else if (data.temperature < -10.0f) {
            emit dataAnomalyDetected(
                QString("Low temperature detected: %1°C").arg(data.temperature));
        }
    }
    
    // 距离传感器异常检测
    if (data.isValid.distanceUpper1 && data.isValid.distanceUpper2) {
        float diff = std::abs(data.distanceUpper1 - data.distanceUpper2);
        if (diff > 50.0f) {
            emit dataAnomalyDetected(
                QString("Large distance difference detected: %1mm").arg(diff));
        }
    }
    
    // 可以添加更多异常检测逻辑
}

void SensorManager::calculateDerivedValues(SensorData& data) {
    ConfigManager& config = ConfigManager::instance();
    
    // 计算平均高度
    if (data.isValid.distanceUpper1 && data.isValid.distanceUpper2) {
        data.calculatedHeight = (data.distanceUpper1 + data.distanceUpper2) / 2.0f;
    }
    
    // 通过两个距离传感器计算角度
    if (data.isValid.distanceUpper1 && data.isValid.distanceUpper2) {
        float sensorSpacing = config.getSensorSpacing();
        data.calculatedAngle = MathUtils::calculateAngle(
            data.distanceUpper1, data.distanceUpper2, sensorSpacing);
    }
}