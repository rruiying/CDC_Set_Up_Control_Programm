#pragma once
#include <QString>
#include <QJsonObject>

class ConfigManager {
public:
    static ConfigManager& instance();
    
    // 加载/保存配置
    bool loadFromFile(const QString& filename);
    bool saveToFile(const QString& filename);
    void loadDefaults();
    
    // 串口配置
    QString getSerialPort() const { return m_serialPort; }
    int getSerialBaudRate() const { return m_baudRate; }
    void setSerialPort(const QString& port) { m_serialPort = port; }
    void setSerialBaudRate(int rate) { m_baudRate = rate; }
    
    // 系统限制
    float getMaxHeight() const { return m_maxHeight; }
    float getMinHeight() const { return m_minHeight; }
    float getMaxAngle() const { return m_maxAngle; }
    float getMinAngle() const { return m_minAngle; }
    
    void setMaxHeight(float height) { m_maxHeight = height; }
    void setMinHeight(float height) { m_minHeight = height; }
    void setMaxAngle(float angle) { m_maxAngle = angle; }
    void setMinAngle(float angle) { m_minAngle = angle; }
    
    // 传感器配置
    float getSensorSpacing() const { return m_sensorSpacing; }
    void setSensorSpacing(float spacing) { m_sensorSpacing = spacing; }
    
private:
    ConfigManager() { loadDefaults(); }
    
    // 串口设置
    QString m_serialPort = "COM1";
    int m_baudRate = 115200;
    
    // 系统限制
    float m_maxHeight = 100.0f;
    float m_minHeight = 0.0f;
    float m_maxAngle = 45.0f;
    float m_minAngle = -45.0f;
    
    // 传感器设置
    float m_sensorSpacing = 100.0f;  // 传感器间距mm
};