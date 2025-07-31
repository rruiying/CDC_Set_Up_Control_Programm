#include "core/include/safety_manager.h"
#include "utils/include/logger.h"

SafetyManager::SafetyManager(QObject* parent)
    : QObject(parent)
    , m_minHeight(0.0f)
    , m_maxHeight(100.0f)
    , m_minAngle(-45.0f)
    , m_maxAngle(45.0f)
    , m_maxTemperature(50.0f)
    , m_speedMode(SpeedMode::Medium)
    , m_emergencyStopped(false)
{
    // 从配置加载默认值
    ConfigManager& config = ConfigManager::instance();
    m_minHeight = config.getMinHeight();
    m_maxHeight = config.getMaxHeight();
    m_minAngle = config.getMinAngle();
    m_maxAngle = config.getMaxAngle();
}

SafetyManager::~SafetyManager() {
}

void SafetyManager::setHeightLimits(float min, float max) {
    if (min < max) {
        m_minHeight = min;
        m_maxHeight = max;
        LOG_INFO(QString("Height limits set: %1 - %2 mm")
                .arg(min).arg(max));
    } else {
        LOG_WARNING("Invalid height limits");
    }
}

void SafetyManager::setAngleLimits(float min, float max) {
    if (min < max) {
        m_minAngle = min;
        m_maxAngle = max;
        LOG_INFO(QString("Angle limits set: %1 - %2 degrees")
                .arg(min).arg(max));
    } else {
        LOG_WARNING("Invalid angle limits");
    }
}

void SafetyManager::setSpeedMode(SpeedMode mode) {
    m_speedMode = mode;
    LOG_INFO(QString("Speed mode set to: %1").arg(static_cast<int>(mode)));
}

bool SafetyManager::isHeightSafe(float height) const {
    bool safe = (height >= m_minHeight && height <= m_maxHeight);
    if (!safe) {
        const_cast<SafetyManager*>(this)->emit limitViolation(
            QString("Height %1mm is outside safe range [%2, %3]")
            .arg(height).arg(m_minHeight).arg(m_maxHeight));
    }
    return safe;
}

bool SafetyManager::isAngleSafe(float angle) const {
    bool safe = (angle >= m_minAngle && angle <= m_maxAngle);
    if (!safe) {
        const_cast<SafetyManager*>(this)->emit limitViolation(
            QString("Angle %1° is outside safe range [%2, %3]")
            .arg(angle).arg(m_minAngle).arg(m_maxAngle));
    }
    return safe;
}

bool SafetyManager::isSpeedSafe(float speed) const {
    return speed >= 0 && speed <= getMaxSpeed();
}

float SafetyManager::getMaxSpeed() const {
    switch (m_speedMode) {
        case SpeedMode::Slow:
            return SPEED_SLOW;
        case SpeedMode::Medium:
            return SPEED_MEDIUM;
        case SpeedMode::Fast:
            return SPEED_FAST;
        default:
            return SPEED_MEDIUM;
    }
}

bool SafetyManager::isSensorDataValid(const SensorData& data) const {
    // 检查距离传感器
    auto checkDistance = [this](float dist, const QString& name) {
        if (dist < MIN_DISTANCE || dist > MAX_DISTANCE) {
            const_cast<SafetyManager*>(this)->emit safetyWarning(
                QString("%1 value %2mm is out of range")
                .arg(name).arg(dist));
            return false;
        }
        return true;
    };
    
    if (!checkDistance(data.distanceUpper1, "Upper distance 1")) return false;
    if (!checkDistance(data.distanceUpper2, "Upper distance 2")) return false;
    if (!checkDistance(data.distanceLower1, "Lower distance 1")) return false;
    if (!checkDistance(data.distanceLower2, "Lower distance 2")) return false;
    
    // 检查温度
    if (data.temperature < MIN_TEMPERATURE || 
        data.temperature > MAX_TEMPERATURE_NORMAL) {
        emit safetyWarning(QString("Temperature %1°C is abnormal")
                          .arg(data.temperature));
        return false;
    }
    
    return true;
}

bool SafetyManager::isMovementAllowed() const {
    if (m_emergencyStopped) {
        LOG_WARNING("Movement blocked: Emergency stop active");
        return false;
    }
    return true;
}

bool SafetyManager::validateMovementRequest(const MovementRequest& request) const {
    if (!isMovementAllowed()) return false;
    if (!isHeightSafe(request.targetHeight)) return false;
    if (!isAngleSafe(request.targetAngle)) return false;
    if (!isSpeedSafe(request.speed)) return false;
    
    return true;
}

void SafetyManager::triggerEmergencyStop() {
    m_emergencyStopped = true;
    LOG_ERROR("EMERGENCY STOP TRIGGERED!");
    emit emergencyStopTriggered();
}

void SafetyManager::resetEmergencyStop() {
    m_emergencyStopped = false;
    LOG_INFO("Emergency stop reset");
}

bool SafetyManager::isEmergencyStopped() const {
    return m_emergencyStopped;
}