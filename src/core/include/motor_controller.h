#pragma once
#include <QObject>
#include <QTimer>
#include "models/include/motor_status.h"

class MotorInterface;
class SafetyManager;

class MotorController : public QObject {
    Q_OBJECT
    
public:
    enum class ControlMode {
        Manual,
        Automatic,
        Calibration
    };
    
    explicit MotorController(MotorInterface* motorInterface = nullptr,
                           SafetyManager* safetyManager = nullptr,
                           QObject* parent = nullptr);
    ~MotorController();
    
    // 运动控制
    bool moveToPosition(float height, float angle);
    bool setHeight(float height);
    bool setAngle(float angle);
    bool setSpeed(float speed);
    
    // 基本命令
    bool stop();
    bool emergencyStop();
    bool goHome();
    
    // 状态查询
    bool isMoving() const;
    bool isStopped() const;
    bool isAtTarget() const;
    MotorStatus getStatus() const;
    
    // 目标设置
    void setTargetHeight(float height);
    void setTargetAngle(float angle);
    float getTargetHeight() const { return m_targetHeight; }
    float getTargetAngle() const { return m_targetAngle; }
    
    // 控制模式
    void setControlMode(ControlMode mode);
    ControlMode getControlMode() const { return m_controlMode; }
    
signals:
    void movementStarted();
    void movementCompleted();
    void movementFailed(const QString& reason);
    void statusChanged(const MotorStatus& status);
    void targetReached();
    
private slots:
    void updateStatus();
    void onSafetyViolation(const QString& message);
    
private:
    MotorInterface* m_motorInterface;
    SafetyManager* m_safetyManager;
    QTimer* m_statusTimer;
    
    MotorStatus m_status;
    ControlMode m_controlMode;
    
    float m_targetHeight;
    float m_targetAngle;
    float m_currentHeight;
    float m_currentAngle;
    
    bool m_isMoving;
    bool m_isStopped;
    
    // 内部方法
    bool validateMovement(float height, float angle);
    void updateMotorStatus();
};