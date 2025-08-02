// src/core/include/safety_manager.h
#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

/**
 * @brief 安全模式枚举
 */
enum class SafetyMode {
    NORMAL,       // 正常模式
    RESTRICTED,   // 限制模式（减少运动范围）
    MAINTENANCE   // 维护模式（允许扩展范围）
};

/**
 * @brief 安全违规记录
 */
struct SafetyViolation {
    int64_t timestamp;
    std::string reason;
    double height;
    double angle;
};

/**
 * @brief 禁止区域
 */
struct ForbiddenZone {
    double minHeight;
    double maxHeight;
    double minAngle;
    double maxAngle;
    std::string description;
};

/**
 * @brief 安全管理器类
 * 
 * 负责所有安全相关的检查：
 * - 位置限位检查
 * - 移动速度检查
 * - 碰撞区域检查
 * - 紧急停止管理
 * - 安全违规记录
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
    using ViolationCallback = std::function<void(const std::string& reason)>;
    using EmergencyStopCallback = std::function<void(bool stopped)>;
    
    // 基本检查方法
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
    void setMaxSingleMove(double maxHeight, double maxAngle);
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
    
    // 违规记录
    int getViolationCount() const;
    std::vector<SafetyViolation> getViolationHistory() const;
    void clearViolationHistory();
    
    // 回调设置
    void setViolationCallback(ViolationCallback callback);
    void setEmergencyStopCallback(EmergencyStopCallback callback);
    
    // 状态摘要
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
    std::atomic<double> minHeightLimit{0.0};
    std::atomic<double> maxHeightLimit{100.0};
    std::atomic<double> minAngleLimit{-90.0};
    std::atomic<double> maxAngleLimit{90.0};
    
    // 速度限制
    std::atomic<double> maxHeightSpeed{10.0};    // mm/s
    std::atomic<double> maxAngleSpeed{10.0};     // deg/s
    std::atomic<double> maxSingleMoveHeight{50.0}; // mm
    std::atomic<double> maxSingleMoveAngle{45.0};  // deg
    
    // 当前状态
    std::atomic<double> currentHeight{0.0};
    std::atomic<double> currentAngle{0.0};
    std::atomic<bool> emergencyStop{false};
    std::string emergencyStopReason;
    
    // 安全模式
    std::atomic<SafetyMode> safetyMode{SafetyMode::NORMAL};
    
    // 禁止区域
    std::vector<ForbiddenZone> forbiddenZones;
    
    // 违规记录
    std::vector<SafetyViolation> violations;
    const size_t maxViolationHistory = 100;
    
    // 回调函数
    ViolationCallback violationCallback;
    EmergencyStopCallback emergencyStopCallback;
    
    // 模式修饰符
    static constexpr double RESTRICTED_MODE_FACTOR = 0.8;  // 限制模式减少20%范围
    static constexpr double MAINTENANCE_MODE_FACTOR = 1.1; // 维护模式增加10%范围
};

#endif // SAFETY_MANAGER_H