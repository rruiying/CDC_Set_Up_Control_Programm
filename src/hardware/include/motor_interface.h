#pragma once
#include <QObject>
#include <QString>

class SerialInterface;

class MotorInterface : public QObject {
    Q_OBJECT
    
public:
    explicit MotorInterface(SerialInterface* serial, QObject* parent = nullptr);
    ~MotorInterface();
    
    // 基本运动命令
    bool setHeight(float heightMm);
    bool setAngle(float angleDeg);
    bool setSpeed(float speedMmPerSec);
    bool stop();
    bool home();
    
    // 高级命令
    bool moveToPosition(float height, float angle);
    bool setAcceleration(float accel);
    bool enableMotors(bool enable);
    
    // 状态查询
    bool requestStatus();
    
signals:
    void commandSent(const QString& command);
    void commandFailed(const QString& reason);
    
private:
    SerialInterface* m_serial;
    
    // 格式化命令
    QString formatHeightCommand(float height) const;
    QString formatAngleCommand(float angle) const;
    QString formatSpeedCommand(float speed) const;
    
    // 发送命令的通用方法
    bool sendCommand(const QString& cmd);
};