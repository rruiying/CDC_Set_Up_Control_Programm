#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <chrono>

struct ForbiddenZone {
    double minHeight;
    double maxHeight;
    double minAngle;
    double maxAngle;
    std::string description;
};

struct SafetyViolation {
    int64_t timestamp;
    std::string reason;
    double attemptedHeight;
    double attemptedAngle;
};

class SafetyManager {
public:
    SafetyManager();
    
    ~SafetyManager() = default;
    
    using ViolationCallback = std::function<void(const std::string&)>;
    using EmergencyStopCallback = std::function<void(bool)>;
    
    bool checkPosition(double height, double angle);
    bool checkMovement(double fromHeight, double fromAngle,
                      double toHeight, double toAngle);
    bool checkMoveSpeed(double fromHeight, double fromAngle,
                       double toHeight, double toAngle, double timeSeconds);
    
    void updateLimitsFromConfig();
    void setCustomLimits(double minHeight, double maxHeight,
                        double minAngle, double maxAngle);
    void getEffectiveLimits(double& minHeight, double& maxHeight,
                           double& minAngle, double& maxAngle) const;
    
    void setSpeedLimits(double maxHeightSpeed, double maxAngleSpeed);
    double getMaxHeightSpeed() const { return maxHeightSpeed; }
    double getMaxAngleSpeed() const { return maxAngleSpeed; }
    

    void setMaxSingleMove(double maxHeight, double maxAngle);
    double getMaxSingleMoveHeight() const { return maxSingleMoveHeight; }
    double getMaxSingleMoveAngle() const { return maxSingleMoveAngle; }
    
    // 计算最小移动时间
    double calculateMinimumMoveTime(double fromHeight, double fromAngle,
                                   double toHeight, double toAngle) const;
    
    // 紧急停止
    void triggerEmergencyStop(const std::string& reason);
    bool isEmergencyStopped() const { return emergencyStop; }
    std::string getEmergencyStopReason() const;
    
    void addForbiddenZone(double minHeight, double maxHeight,
                         double minAngle, double maxAngle,
                         const std::string& description = "");
    void removeForbiddenZone(size_t index);
    void clearForbiddenZones();
    std::vector<ForbiddenZone> getForbiddenZones() const;
    
    // 当前位置跟踪
    void setCurrentPosition(double height, double angle);
    void getCurrentPosition(double& height, double& angle) const;
    
    // 违规历史
    int getViolationCount() const;
    std::vector<SafetyViolation> getViolationHistory() const;
    void clearViolationHistory();
    void reset();
    void clearEmergencyStop();
    
    // 回调设置
    void setViolationCallback(ViolationCallback callback);
    void setEmergencyStopCallback(EmergencyStopCallback callback);
    
private:
    // 内部检查方法
    bool checkLimits(double height, double angle) const;
    bool checkForbiddenZones(double height, double angle) const;
    bool checkMovementDistance(double fromHeight, double fromAngle,
                              double toHeight, double toAngle) const;
    void recordViolation(const std::string& reason, double height, double angle);
    void notifyViolation(const std::string& reason);
    void notifyEmergencyStop(bool stopped);

    mutable std::mutex mutex;
    
    double minHeightLimit = 0.0;
    double maxHeightLimit = 180.0;
    double minAngleLimit = -90.0;
    double maxAngleLimit = 90.0;

    bool emergencyStopActive = false;
    std::string lastError;
    int errorCount = 0;
    
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
    
    // 违规历史
    std::vector<SafetyViolation> violations;
    static constexpr size_t maxViolationHistory = 100;
    
    // 回调函数
    ViolationCallback violationCallback;
    EmergencyStopCallback emergencyStopCallback;
};

#endif // SAFETY_MANAGER_H