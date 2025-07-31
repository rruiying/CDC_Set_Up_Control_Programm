#include "utils/include/config_manager.h"
#include <QJsonDocument>
#include <QFile>

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::loadDefaults() {
    m_serialPort = "COM1";
    m_baudRate = 115200;
    m_maxHeight = 100.0f;
    m_minHeight = 0.0f;
    m_maxAngle = 45.0f;
    m_minAngle = -45.0f;
    m_sensorSpacing = 100.0f;
}

bool ConfigManager::loadFromFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();
    
    m_serialPort = obj["serialPort"].toString(m_serialPort);
    m_baudRate = obj["baudRate"].toInt(m_baudRate);
    m_maxHeight = obj["maxHeight"].toDouble(m_maxHeight);
    m_minHeight = obj["minHeight"].toDouble(m_minHeight);
    m_maxAngle = obj["maxAngle"].toDouble(m_maxAngle);
    m_minAngle = obj["minAngle"].toDouble(m_minAngle);
    m_sensorSpacing = obj["sensorSpacing"].toDouble(m_sensorSpacing);
    
    return true;
}

bool ConfigManager::saveToFile(const QString& filename) {
    QJsonObject obj;
    obj["serialPort"] = m_serialPort;
    obj["baudRate"] = m_baudRate;
    obj["maxHeight"] = m_maxHeight;
    obj["minHeight"] = m_minHeight;
    obj["maxAngle"] = m_maxAngle;
    obj["minAngle"] = m_minAngle;
    obj["sensorSpacing"] = m_sensorSpacing;
    
    QJsonDocument doc(obj);
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}