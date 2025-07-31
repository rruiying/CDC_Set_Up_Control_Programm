#include "core/include/motor_controller.h"
#include "core/include/safety_manager.h"
#include "hardware/include/motor_interface.h"
#include "utils/include/logger.h"
#include "utils/include/math_utils.h"

MotorController::MotorController(MotorInterface* motorInterface,
                               SafetyManager* safetyManager,
                               QObject* parent)
    : QObject(parent)
    , m_motorInterface(motorInterface)
    , m_safetyManager(safetyManager)
    , m_statusTimer(new QTimer(this))
    , m_controlMode(ControlMode::Manual)
    , m_targetHeight(0.0f)
    , m_targetAngle(0.0f)
    , m_currentHeight(0.0f)
    , m_currentAngle(0.0f)
    , m_isMoving(false)
    , m_isStopped(true)
{
    // 状态更新定时器
    connect(m_statusTimer, &QTimer::timeout,
            this, &MotorController::updateStatus);
    m_statusTimer->start(100); // 100ms更新一次
    
    // 连接安全管理器
    if (m_safetyManager) {
        connect(m_safetyManager, &SafetyManager::limitViolation,
                this, &MotorController::onSafetyViolation);
        connect(m_safetyManager, &SafetyManager::emergencyStopTriggered,
                this, &MotorController::emergencyStop);
    }
    
    // 初始化状态
    m_status.isConnected = false;
    m_status.isMoving = false;
    m_status.currentHeight = 0.0f;
    m_status.currentAngle = 0.0f;
    m_status.targetHeight = 0.0f;
    m_status.targetAngle = 0.0f;
}

MotorController::~MotorController() {
    stop();
}

bool MotorController::moveToPosition(float height, float angle) {
    if (!validateMovement(height, angle)) {
        return false;
    }
    
    m_targetHeight = height;
    m_targetAngle = angle;
    
    bool heightOk = true;
    bool angleOk = true;
    
    // 发送运动命令
    if (m_motorInterface) {
        if (std::abs(height - m_currentHeight) > 0.1f) {
            heightOk = m_motorInterface->setHeight(height);
        }
        
        if (std::abs(angle - m_currentAngle) > 0.1f) {
            angleOk = m_motorInterface->setAngle(angle);
        }
    }
    
    if (heightOk && angleOk) {
        m_isMoving = true;
        m_isStopped = false;
        emit movementStarted();
        LOG_INFO(QString("Movement started to H:%1mm, A:%2°")
                .arg(height).arg(angle));
        return true;
    } else {
        emit movementFailed("Failed to send motor commands");
        return false;
    }
}

bool MotorController::setHeight(float height) {
    return moveToPosition(height, m_currentAngle);
}

bool MotorController::setAngle(float angle) {
    return moveToPosition(m_currentHeight, angle);
}

bool MotorController::setSpeed(float speed) {
    if (!m_safetyManager || !m_safetyManager->isSpeedSafe(speed)) {
        LOG_WARNING(QString("Speed %1 is not safe").arg(speed));
        return false;
    }
    
    if (m_motorInterface) {
        return m_motorInterface->setSpeed(speed);
    }
    
    return false;
}

bool MotorController::stop() {
    m_isMoving = false;
    m_isStopped = true;
    
    if (m_motorInterface) {
        bool result = m_motorInterface->stop();
        if (result) {
            LOG_INFO("Motors stopped");
        }
        return result;
    }
    
    return false;
}

bool MotorController::emergencyStop() {
    LOG_ERROR("EMERGENCY STOP!");
    
    bool result = stop();
    
    if (m_safetyManager) {
        m_safetyManager->triggerEmergencyStop();
    }
    
    emit movementFailed("Emergency stop triggered");
    return result;
}

bool MotorController::goHome() {
    if (m_motorInterface) {
        bool result = m_motorInterface->home();
        if (result) {
            m_targetHeight = 0.0f;
            m_targetAngle = 0.0f;
            m_isMoving = true;
            m_isStopped = false;
            emit movementStarted();
            LOG_INFO("Homing started");
        }
        return result;
    }
    
    return false;
}

bool MotorController::isMoving() const {
    return m_isMoving;
}

bool MotorController::isStopped() const {
    return m_isStopped;
}

bool MotorController::isAtTarget() const {
    const float tolerance = 0.5f;
    
    bool heightOk = std::abs(m_currentHeight - m_targetHeight) < tolerance;
    bool angleOk = std::abs(m_currentAngle - m_targetAngle) < tolerance;
    
    return heightOk && angleOk;
}

MotorStatus MotorController::getStatus() const {
    return m_status;
}

void MotorController::setTargetHeight(float height) {
    if (m_safetyManager && m_safetyManager->isHeightSafe(height)) {
        m_targetHeight = height;
        updateMotorStatus();
        emit statusChanged(m_status);
    }
}

void MotorController::setTargetAngle(float angle) {
    if (m_safetyManager && m_safetyManager->isAngleSafe(angle)) {
        m_targetAngle = angle;
        updateMotorStatus();
        emit statusChanged(m_status);
    }
}

void MotorController::setControlMode(ControlMode mode) {
    m_controlMode = mode;
    LOG_INFO(QString("Control mode changed to: %1").arg(static_cast<int>(mode)));
}

void MotorController::updateStatus() {
    // 这里应该从MCU获取实际位置
    // 现在暂时模拟
    if (m_isMoving) {
        // 模拟向目标移动
        m_currentHeight = MathUtils::lerp(m_currentHeight, m_targetHeight, 0.1f);
        m_currentAngle = MathUtils::lerp(m_currentAngle, m_targetAngle, 0.1f);
        
        if (isAtTarget()) {
            m_isMoving = false;
            emit movementCompleted();
            emit targetReached();
            LOG_INFO("Target position reached");
        }
    }
    
    updateMotorStatus();
}

void MotorController::onSafetyViolation(const QString& message) {
    stop();
    emit movementFailed(QString("Safety violation: %1").arg(message));
}

bool MotorController::validateMovement(float height, float angle) {
    if (!m_safetyManager) {
        LOG_WARNING("No safety manager configured");
        return false;
    }
    
    if (!m_safetyManager->isMovementAllowed()) {
        emit movementFailed("Movement not allowed (emergency stop active)");
        return false;
    }
    
    SafetyManager::MovementRequest request;
    request.targetHeight = height;
    request.targetAngle = angle;
    request.speed = 30.0f; // 默认速度
    
    if (!m_safetyManager->validateMovementRequest(request)) {
        emit movementFailed("Movement request failed safety validation");
        return false;
    }
    
    return true;
}

void MotorController::updateMotorStatus() {
    m_status.isConnected = m_motorInterface != nullptr;
    m_status.isMoving = m_isMoving;
    m_status.currentHeight = m_currentHeight;
    m_status.currentAngle = m_currentAngle;
    m_status.targetHeight = m_targetHeight;
    m_status.targetAngle = m_targetAngle;
    m_status.mode = m_controlMode;
    m_status.timestamp = QDateTime::currentDateTime();
    
    emit statusChanged(m_status);
}