#pragma once
#include <QObject>
#include "models/include/sensor_data.h"
#include "utils/include/config_manager.h"

class SafetyManager : public QObject {
    Q_OBJECT
    
public:
    enum class SpeedMode {
        Slow,
        Medium,
        Fast
    };
    
    struct MovementRequest {
        float targetHeight;
        float targetAngle;
        float speed;
    };
    
    explicit SafetyManager(QObject* parent = nullptr);
    ~SafetyManager();
    
    // 限制设置
    void setHeightLimits(float min, float max);
    void setAngleLimits(float min, float max);
    void setSpeedMode(SpeedMode mode);
    void setMaxTemperature(float maxTemp);
    
    // 安全检查
    bool isHeightSafe(float height) const;
    bool isAngleSafe(float angle) const;
    bool isSpeedSafe(float speed) const;
    bool isSensorDataValid(const SensorData& data) const;
    bool isMovementAllowed() const;
    bool validateMovementRequest(const MovementRequest& request) const;
    
    // 紧急停止
    void triggerEmergencyStop();
    void resetEmergencyStop();
    bool isEmergencyStopped() const;
    
    // 获取限制值
    float getMinHeight() const { return m_minHeight; }
    float getMaxHeight() const { return m_maxHeight; }
    float getMinAngle() const { return m_minAngle; }
    float getMaxAngle() const { return m_maxAngle; }
    float getMaxSpeed() const;
    
signals:
    void emergencyStopTriggered();
    void limitViolation(const QString& message);
    void safetyWarning(const QString& message);
    
private:
    float m_minHeight;
    float m_maxHeight;
    float m_minAngle;
    float m_maxAngle;
    float m_maxTemperature;
    SpeedMode m_speedMode;
    bool m_emergencyStopped;
    
    // 速度限制（mm/s）
    static constexpr float SPEED_SLOW = 10.0f;
    static constexpr float SPEED_MEDIUM = 30.0f;
    static constexpr float SPEED_FAST = 50.0f;
    
    // 传感器数据合理范围
    static constexpr float MIN_DISTANCE = 0.0f;
    static constexpr float MAX_DISTANCE = 300.0f;
    static constexpr float MIN_TEMPERATURE = -10.0f;
    static constexpr float MAX_TEMPERATURE_NORMAL = 60.0f;
};