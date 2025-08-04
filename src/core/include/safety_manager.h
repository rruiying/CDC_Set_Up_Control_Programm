// src/core/include/safety_manager.h
#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <chrono>

/**
 * @brief 安全模式枚举
 */
enum class SafetyMode {
    NORMAL,      // 正常模式
    RESTRICTED,  // 限制模式（减小运动范围）
    MAINTENANCE  // 维护模式（允许更大范围）
};

/**
 * @brief 禁止区域结构
 */
struct ForbiddenZone {
    double minHeight;
    double maxHeight;
    double minAngle;
    double maxAngle;
    std::string description;
};

/**
 * @brief 安全违规记录
 */
struct SafetyViolation {
    int64_t timestamp;
    std::string reason;
    double attemptedHeight;
    double attemptedAngle;
};

/**
 * @brief 安全管理器类
 * 
 * 负责系统安全检查：
 * - 限位检查
 * - 速度检查
 * - 禁止区域检查
 * - 紧急停止管理
 * - 安全模式管理
 */
class SafetyManager {
public:
    /**
     * @brief 构造函数
     */
    SafetyManager();
    
    /**
     * @brief 析构函数
     */
    ~SafetyManager() = default;
    
    // 回调函数类型
    using ViolationCallback = std::function<void(const std::string&)>;
    using EmergencyStopCallback = std::function<void(bool)>;
    
    // 位置检查
    bool checkPosition(double height, double angle);
    bool checkMovement(double fromHeight, double fromAngle,
                      double toHeight, double toAngle);
    bool checkMoveSpeed(double fromHeight, double fromAngle,
                       double toHeight, double toAngle, double timeSeconds);
    
    // 限位管理
    void updateLimitsFromConfig();
    void setCustomLimits(double minHeight, double maxHeight,
                        double minAngle, double maxAngle);
    void getEffectiveLimits(double& minHeight, double& maxHeight,
                           double& minAngle, double& maxAngle) const;
    
    // 速度限制
    void setSpeedLimits(double maxHeightSpeed, double maxAngleSpeed);
    double getMaxHeightSpeed() const { return maxHeightSpeed; }
    double getMaxAngleSpeed() const { return maxAngleSpeed; }
    
    // 单次移动限制
    void setMaxSingleMove(double maxHeight, double maxAngle);
    double getMaxSingleMoveHeight() const { return maxSingleMoveHeight; }
    double getMaxSingleMoveAngle() const { return maxSingleMoveAngle; }
    
    // 计算最小移动时间
    double calculateMinimumMoveTime(double fromHeight, double fromAngle,
                                   double toHeight, double toAngle) const;
    
    // 紧急停止
    void triggerEmergencyStop(const std::string& reason);
    void clearEmergencyStop();
    bool isEmergencyStopped() const { return emergencyStop; }
    std::string getEmergencyStopReason() const;
    
    // 禁止区域管理
    void addForbiddenZone(double minHeight, double maxHeight,
                         double minAngle, double maxAngle,
                         const std::string& description = "");
    void removeForbiddenZone(size_t index);
    void clearForbiddenZones();
    std::vector<ForbiddenZone> getForbiddenZones() const;
    
    // 安全模式
    void setSafetyMode(SafetyMode mode);
    SafetyMode getSafetyMode() const { return safetyMode; }
    
    // 当前位置跟踪
    void setCurrentPosition(double height, double angle);
    void getCurrentPosition(double& height, double& angle) const;
    
    // 违规历史
    int getViolationCount() const;
    std::vector<SafetyViolation> getViolationHistory() const;
    void clearViolationHistory();
    
    // 回调设置
    void setViolationCallback(ViolationCallback callback);
    void setEmergencyStopCallback(EmergencyStopCallback callback);
    
    // 状态获取
    std::string getSafetyStatus() const;
    
private:
    // 内部检查方法
    bool checkLimits(double height, double angle) const;
    bool checkForbiddenZones(double height, double angle) const;
    bool checkMovementDistance(double fromHeight, double fromAngle,
                              double toHeight, double toAngle) const;
    void recordViolation(const std::string& reason, double height, double angle);
    void notifyViolation(const std::string& reason);
    void notifyEmergencyStop(bool stopped);
    void applyModeModifiers(double& minHeight, double& maxHeight,
                           double& minAngle, double& maxAngle) const;
    int64_t getCurrentTimestamp() const;
    
    // 成员变量
    mutable std::mutex mutex;
    
    // 限位参数
    double minHeightLimit = 0.0;
    double maxHeightLimit = 180.0;
    double minAngleLimit = -90.0;
    double maxAngleLimit = 90.0;
    
    // 速度限制 (单位/秒)
    std::atomic<double> maxHeightSpeed{50.0};  // mm/s
    std::atomic<double> maxAngleSpeed{30.0};   // degrees/s
    
    // 单次移动限制
    std::atomic<double> maxSingleMoveHeight{100.0};  // mm
    std::atomic<double> maxSingleMoveAngle{45.0};    // degrees
    
    // 当前位置
    std::atomic<double> currentHeight{0.0};
    std::atomic<double> currentAngle{0.0};
    
    // 紧急停止
    std::atomic<bool> emergencyStop{false};
    std::string emergencyStopReason;
    
    // 禁止区域
    std::vector<ForbiddenZone> forbiddenZones;
    
    // 安全模式
    std::atomic<SafetyMode> safetyMode{SafetyMode::NORMAL};
    
    // 违规历史
    std::vector<SafetyViolation> violations;
    static constexpr size_t maxViolationHistory = 100;
    
    // 回调函数
    ViolationCallback violationCallback;
    EmergencyStopCallback emergencyStopCallback;
    
    // 模式因子
    static constexpr double RESTRICTED_MODE_FACTOR = 0.7;  // 限制模式下减少30%范围
    static constexpr double MAINTENANCE_MODE_FACTOR = 1.2; // 维护模式下增加20%范围
};

#endif // SAFETY_MANAGER_H