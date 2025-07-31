#include "hardware/include/sensor_interface.h"
#include "hardware/include/serial_interface.h"
#include "utils/include/logger.h"
#include <QStringList>
#include <QRegularExpression>

SensorInterface::SensorInterface(SerialInterface* serial, QObject* parent)
    : QObject(parent)
    , m_serial(serial)
    , m_pollTimer(new QTimer(this))
{
    if (m_serial) {
        connect(m_serial, &SerialInterface::lineReceived,
                this, &SensorInterface::onSerialDataReceived);
    }
    
    connect(m_pollTimer, &QTimer::timeout,
            this, &SensorInterface::onPollTimer);
}

SensorInterface::~SensorInterface() {
    stopPolling();
}

bool SensorInterface::requestAllSensorData() {
    if (!m_serial || !m_serial->isConnected()) {
        return false;
    }
    
    return m_serial->sendCommand("READ:ALL");
}

bool SensorInterface::requestDistanceSensors() {
    if (!m_serial || !m_serial->isConnected()) {
        return false;
    }
    
    return m_serial->sendCommand("READ:DIST");
}

bool SensorInterface::requestAngleSensor() {
    if (!m_serial || !m_serial->isConnected()) {
        return false;
    }
    
    return m_serial->sendCommand("READ:ANGLE");
}

bool SensorInterface::requestTemperature() {
    if (!m_serial || !m_serial->isConnected()) {
        return false;
    }
    
    return m_serial->sendCommand("READ:TEMP");
}

bool SensorInterface::requestCapacitance() {
    if (!m_serial || !m_serial->isConnected()) {
        return false;
    }
    
    return m_serial->sendCommand("READ:CAP");
}

void SensorInterface::startPolling(int intervalMs) {
    m_pollTimer->start(intervalMs);
    LOG_INFO(QString("Started sensor polling at %1ms interval").arg(intervalMs));
}

void SensorInterface::stopPolling() {
    m_pollTimer->stop();
    LOG_INFO("Stopped sensor polling");
}

bool SensorInterface::isPolling() const {
    return m_pollTimer->isActive();
}

void SensorInterface::processData(const QString& data) {
    // 使用之前的数据作为基础
    SensorData newData = m_latestData;
    bool hasUpdate = false;
    
    // 解析格式: "D:v1,v2,v3,v4,A:angle,T:temp,C:cap"
    QStringList parts = data.split(',');
    
    for (const QString& part : parts) {
        if (part.startsWith("D:")) {
            if (parseDistanceData(part)) {
                hasUpdate = true;
            }
        }
        else if (part.startsWith("A:")) {
            if (parseAngleData(part)) {
                hasUpdate = true;
            }
        }
        else if (part.startsWith("T:")) {
            if (parseTemperatureData(part)) {
                hasUpdate = true;
            }
        }
        else if (part.startsWith("C:")) {
            if (parseCapacitanceData(part)) {
                hasUpdate = true;
            }
        }
    }
    
    if (hasUpdate) {
        m_latestData.timestamp = QDateTime::currentDateTime();
        
        if (validateSensorData(m_latestData)) {
            emit sensorDataReceived(m_latestData);
        } else {
            emit dataError("Sensor data validation failed");
        }
    }
}

void SensorInterface::onSerialDataReceived(const QString& line) {
    processData(line);
}

void SensorInterface::onPollTimer() {
    requestAllSensorData();
}

bool SensorInterface::parseDistanceData(const QString& data) {
    // 格式: "D:25.5,26.1,155.8,156.2"
    QString values = data.mid(2); // 去掉 "D:"
    QStringList distances = values.split(',');
    
    if (distances.size() >= 4) {
        bool ok;
        float d1 = distances[0].toFloat(&ok);
        if (ok) {
            m_latestData.distanceUpper1 = d1;
            m_latestData.isValid.distanceUpper1 = true;
        }
        
        float d2 = distances[1].toFloat(&ok);
        if (ok) {
            m_latestData.distanceUpper2 = d2;
            m_latestData.isValid.distanceUpper2 = true;
        }
        
        float d3 = distances[2].toFloat(&ok);
        if (ok) {
            m_latestData.distanceLower1 = d3;
            m_latestData.isValid.distanceLower1 = true;
        }
        
        float d4 = distances[3].toFloat(&ok);
        if (ok) {
            m_latestData.distanceLower2 = d4;
            m_latestData.isValid.distanceLower2 = true;
        }
        
        return true;
    }
    
    return false;
}

bool SensorInterface::parseAngleData(const QString& data) {
    bool ok;
    float angle = data.mid(2).toFloat(&ok);
    if (ok) {
        m_latestData.angle = angle;
        m_latestData.isValid.angle = true;
        return true;
    }
    return false;
}

bool SensorInterface::parseTemperatureData(const QString& data) {
    bool ok;
    float temp = data.mid(2).toFloat(&ok);
    if (ok) {
        m_latestData.temperature = temp;
        m_latestData.isValid.temperature = true;
        return true;
    }
    return false;
}

bool SensorInterface::parseCapacitanceData(const QString& data) {
    bool ok;
    float cap = data.mid(2).toFloat(&ok);
    if (ok) {
        m_latestData.capacitance = cap;
        m_latestData.isValid.capacitance = true;
        return true;
    }
    return false;
}

bool SensorInterface::validateSensorData(const SensorData& data) {
    // 基本范围检查
    if (data.isValid.distanceUpper1 && 
        (data.distanceUpper1 < 0 || data.distanceUpper1 > 300)) {
        return false;
    }
    
    if (data.isValid.temperature && 
        (data.temperature < -40 || data.temperature > 100)) {
        return false;
    }
    
    // 可以添加更多验证
    return true;
}