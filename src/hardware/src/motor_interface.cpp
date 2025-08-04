#include "hardware/include/motor_interface.h"
#include "hardware/include/serial_interface.h"
#include "utils/include/logger.h"

MotorInterface::MotorInterface(SerialInterface* serial, QObject* parent)
    : QObject(parent)
    , m_serial(serial)
{
    if (!m_serial) {
        LOG_ERROR("MotorInterface created with null SerialInterface");
    }
}

MotorInterface::~MotorInterface() {
}

bool MotorInterface::setHeight(float heightMm) {
    QString cmd = formatHeightCommand(heightMm);
    return sendCommand(cmd);
}

bool MotorInterface::setAngle(float angleDeg) {
    QString cmd = formatAngleCommand(angleDeg);
    return sendCommand(cmd);
}

bool MotorInterface::setSpeed(float speedMmPerSec) {
    QString cmd = formatSpeedCommand(speedMmPerSec);
    return sendCommand(cmd);
}

bool MotorInterface::stop() {
    return sendCommand("STOP");
}

bool MotorInterface::home() {
    return sendCommand("HOME");
}

bool MotorInterface::moveToPosition(float height, float angle) {
    // 发送组合命令
    QString cmd = QString("M:%1,%2").arg(height, 0, 'f', 1)
                                    .arg(angle, 0, 'f', 1);
    return sendCommand(cmd);
}

bool MotorInterface::setAcceleration(float accel) {
    QString cmd = QString("ACC:%1").arg(accel, 0, 'f', 1);
    return sendCommand(cmd);
}

bool MotorInterface::enableMotors(bool enable) {
    return sendCommand(enable ? "ENABLE" : "DISABLE");
}

bool MotorInterface::requestStatus() {
    return sendCommand("STATUS");
}

QString MotorInterface::formatHeightCommand(float height) const {
    return QString("H:%1").arg(height, 0, 'f', 1);
}

QString MotorInterface::formatAngleCommand(float angle) const {
    return QString("A:%1").arg(angle, 0, 'f', 1);
}

QString MotorInterface::formatSpeedCommand(float speed) const {
    return QString("S:%1").arg(speed, 0, 'f', 1);
}

bool MotorInterface::sendCommand(const QString& cmd) {
    if (!m_serial || !m_serial->isOpen()) {
        LOG_WARNING("Cannot send motor command: Serial not connected");
        emit commandFailed("Serial port not connected");
        return false;
    }
    
    bool success = m_serial->sendCommand(cmd.toStdString());
    if (success) {
        LOG_INFO(QString("Motor command sent: %1").arg(cmd).toStdString());
    } else {
        LOG_ERROR(QString("Failed to send motor command: %1").arg(cmd).toStdString());
    }
    
    return success;
}