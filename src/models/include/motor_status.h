#pragma once
#include <QDateTime>

class MotorController;

struct MotorStatus {
    bool isConnected;
    bool isMoving;
    float currentHeight;
    float currentAngle;
    float targetHeight;
    float targetAngle;
    float currentSpeed;
    MotorController::ControlMode mode;
    QDateTime timestamp;
    QString errorMessage;
};

Q_DECLARE_METATYPE(MotorStatus)