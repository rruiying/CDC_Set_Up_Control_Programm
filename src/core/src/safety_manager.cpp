#include "../include/safety_manager.h"
#include <mutex>
#include "../../models/include/system_config.h"
#include "../../utils/include/logger.h"
#include "../../utils/include/time_utils.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <thread>

SafetyManager::SafetyManager() {
    // 从系统配置加载初始限位
    updateLimitsFromConfig();
    LOG_INFO("SafetyManager initialized");
}

bool SafetyManager::checkPosition(double height, double angle) {
    // 紧急停止状态下，任何位置都不安全
    if (emergencyStop) {
        recordViolation("Emergency stop active", height, angle);
        return false;
    }
    
    // 检查基本限位
    if (!checkLimits(height, angle)) {
        std::ostringstream oss;
        oss << "Position out of limits: height=" << height << "mm, angle=" << angle << "°";
        recordViolation(oss.str(), height, angle);
        return false;
    }
    
    // 检查禁止区域
    if (!checkForbiddenZones(height, angle)) {
        recordViolation("Position in forbidden zone", height, angle);
        return false;
    }
    
    return true;
}

bool SafetyManager::checkMovement(double fromHeight, double fromAngle,
                                  double toHeight, double toAngle) {
    // 检查起点和终点
    if (!checkPosition(toHeight, toAngle)) {
        return false;
    }
    
    // 检查移动距离
    if (!checkMovementDistance(fromHeight, fromAngle, toHeight, toAngle)) {
        std::ostringstream oss;
        oss << "Movement distance too large: from (" << fromHeight << "," << fromAngle 
            << ") to (" << toHeight << "," << toAngle << ")";
        recordViolation(oss.str(), toHeight, toAngle);
        return false;
    }
    
    return true;
}

bool SafetyManager::checkMoveSpeed(double fromHeight, double fromAngle,
                                   double toHeight, double toAngle, double timeSeconds) {
    if (timeSeconds <= 0) {
        return false;
    }
    
    double heightDiff = std::abs(toHeight - fromHeight);
    double angleDiff = std::abs(toAngle - fromAngle);
    
    double heightSpeed = heightDiff / timeSeconds;
    double angleSpeed = angleDiff / timeSeconds;
    
    if (heightSpeed > maxHeightSpeed || angleSpeed > maxAngleSpeed) {
        std::ostringstream oss;
        oss << "Move speed too high: " << heightSpeed << "mm/s, " << angleSpeed << "°/s";
        recordViolation(oss.str(), toHeight, toAngle);
        return false;
    }
    
    return true;
}

void SafetyManager::updateLimitsFromConfig() {
    auto& config = SystemConfig::getInstance();
    setCustomLimits(config.getMinHeight(), config.getMaxHeight(),
                   config.getMinAngle(), config.getMaxAngle());
}

void SafetyManager::setCustomLimits(double minHeight, double maxHeight,
                                    double minAngle, double maxAngle) {
    std::lock_guard<std::mutex> lock(mutex);
    
    minHeightLimit = minHeight;
    maxHeightLimit = maxHeight;
    minAngleLimit = minAngle;
    maxAngleLimit = maxAngle;
    
    LOG_INFO_F("Safety limits updated: height[%.1f-%.1f]mm, angle[%.1f-%.1f]°",
               minHeight, maxHeight, minAngle, maxAngle);
}

void SafetyManager::getEffectiveLimits(double& minHeight, double& maxHeight,
                                       double& minAngle, double& maxAngle) const {
    {
        std::lock_guard<std::mutex> lock(mutex);
        minHeight = minHeightLimit;
        maxHeight = maxHeightLimit;
        minAngle = minAngleLimit;
        maxAngle = maxAngleLimit;
    }
    applyModeModifiers(minHeight, maxHeight, minAngle, maxAngle);
}

void SafetyManager::setSpeedLimits(double maxHSpeed, double maxASpeed) {
    maxHeightSpeed = maxHSpeed;
    maxAngleSpeed = maxASpeed;
    LOG_INFO_F("Speed limits set: %.1fmm/s, %.1f°/s", maxHSpeed, maxASpeed);
}

void SafetyManager::setMaxSingleMove(double maxHeight, double maxAngle) {
    maxSingleMoveHeight = maxHeight;
    maxSingleMoveAngle = maxAngle;
}

double SafetyManager::calculateMinimumMoveTime(double fromHeight, double fromAngle,
                                               double toHeight, double toAngle) const {
    double heightDiff = std::abs(toHeight - fromHeight);
    double angleDiff = std::abs(toAngle - fromAngle);
    
    double heightTime = heightDiff / maxHeightSpeed;
    double angleTime = angleDiff / maxAngleSpeed;
    
    return std::max(heightTime, angleTime);
}

void SafetyManager::triggerEmergencyStop(const std::string& reason) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        emergencyStop = true;
        emergencyStopReason = reason;
    }
    
    LOG_ERROR("Emergency stop triggered: " + reason);
    notifyEmergencyStop(true);
}

void SafetyManager::clearEmergencyStop() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        emergencyStop = false;
        emergencyStopReason.clear();
    }
    
    LOG_INFO("Emergency stop cleared");
    notifyEmergencyStop(false);
}

std::string SafetyManager::getEmergencyStopReason() const {
    std::lock_guard<std::mutex> lock(mutex);
    return emergencyStopReason;
}

void SafetyManager::addForbiddenZone(double minHeight, double maxHeight,
                                     double minAngle, double maxAngle,
                                     const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex);
    
    ForbiddenZone zone{minHeight, maxHeight, minAngle, maxAngle, description};
    forbiddenZones.push_back(zone);
    
    LOG_INFO_F("Forbidden zone added: height[%.1f-%.1f]mm, angle[%.1f-%.1f]°",
               minHeight, maxHeight, minAngle, maxAngle);
}

void SafetyManager::removeForbiddenZone(size_t index) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (index < forbiddenZones.size()) {
        forbiddenZones.erase(forbiddenZones.begin() + index);
        LOG_INFO_F("Forbidden zone %zu removed", index);
    }
}

void SafetyManager::clearForbiddenZones() {
    std::lock_guard<std::mutex> lock(mutex);
    forbiddenZones.clear();
    LOG_INFO("All forbidden zones cleared");
}

std::vector<ForbiddenZone> SafetyManager::getForbiddenZones() const {
    std::lock_guard<std::mutex> lock(mutex);
    return forbiddenZones;
}

void SafetyManager::setSafetyMode(SafetyMode mode) {
    safetyMode = mode;
    LOG_INFO_F("Safety mode set to: %d", static_cast<int>(mode));
}

void SafetyManager::setCurrentPosition(double height, double angle) {
    currentHeight = height;
    currentAngle = angle;
}

void SafetyManager::getCurrentPosition(double& height, double& angle) const {
    height = currentHeight;
    angle = currentAngle;
}

int SafetyManager::getViolationCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return static_cast<int>(violations.size());
}

std::vector<SafetyViolation> SafetyManager::getViolationHistory() const {
    std::lock_guard<std::mutex> lock(mutex);
    return violations;
}

void SafetyManager::clearViolationHistory() {
    std::lock_guard<std::mutex> lock(mutex);
    violations.clear();
}

void SafetyManager::setViolationCallback(ViolationCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    violationCallback = callback;
}

void SafetyManager::setEmergencyStopCallback(EmergencyStopCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    emergencyStopCallback = callback;
}

std::string SafetyManager::getSafetyStatus() const {
    std::ostringstream oss;
    
    oss << "Safety Status:\n";
    oss << "  Mode: ";
    switch (safetyMode.load()) {
        case SafetyMode::NORMAL: oss << "NORMAL"; break;
        case SafetyMode::RESTRICTED: oss << "RESTRICTED"; break;
        case SafetyMode::MAINTENANCE: oss << "MAINTENANCE"; break;
    }
    oss << "\n";
    
    oss << "  Emergency Stop: " << (emergencyStop ? "ACTIVE" : "INACTIVE") << "\n";
    
    double minH, maxH, minA, maxA;
    getEffectiveLimits(minH, maxH, minA, maxA);
    oss << "  Effective Limits: Height[" << minH << "-" << maxH << "]mm, ";
    oss << "Angle[" << minA << "-" << maxA << "]°\n";
    
    oss << "  Forbidden Zones: " << forbiddenZones.size() << "\n";
    oss << "  Violations: " << violations.size();
    
    return oss.str();
}

// 私有方法实现

bool SafetyManager::checkLimits(double height, double angle) const {
    double minH, maxH, minA, maxA;
    getEffectiveLimits(minH, maxH, minA, maxA);
    
    return height >= minH && height <= maxH &&
           angle >= minA && angle <= maxA;
}

bool SafetyManager::checkForbiddenZones(double height, double angle) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (const auto& zone : forbiddenZones) {
        if (height >= zone.minHeight && height <= zone.maxHeight &&
            angle >= zone.minAngle && angle <= zone.maxAngle) {
            return false;
        }
    }
    
    return true;
}

bool SafetyManager::checkMovementDistance(double fromHeight, double fromAngle,
                                         double toHeight, double toAngle) const {
    double heightDiff = std::abs(toHeight - fromHeight);
    double angleDiff = std::abs(toAngle - fromAngle);
    
    return heightDiff <= maxSingleMoveHeight && angleDiff <= maxSingleMoveAngle;
}

void SafetyManager::recordViolation(const std::string& reason, double height, double angle) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        SafetyViolation violation{TimeUtils::getCurrentTimestamp(), reason, height, angle};
        violations.push_back(violation);
        
        // 限制历史大小
        while (violations.size() > maxViolationHistory) {
            violations.erase(violations.begin());
        }
    }
    
    LOG_WARNING("Safety violation: " + reason);
    notifyViolation(reason);
}

void SafetyManager::notifyViolation(const std::string& reason) {
    ViolationCallback cb;
    { std::lock_guard<std::mutex> lock(mutex); cb = violationCallback; }
    if (cb) {
        cb(reason);
    }
}

void SafetyManager::notifyEmergencyStop(bool stopped) {
    EmergencyStopCallback cb;
    { std::lock_guard<std::mutex> lock(mutex); cb = emergencyStopCallback; }
    if (cb) {
        cb(stopped);
    }
}

void SafetyManager::applyModeModifiers(double& minHeight, double& maxHeight,
                                       double& minAngle, double& maxAngle) const {
    double heightRange = maxHeight - minHeight;
    double angleRange = maxAngle - minAngle;
    double heightCenter = (maxHeight + minHeight) / 2.0;
    double angleCenter = (maxAngle + minAngle) / 2.0;
    
    switch (safetyMode.load()) {
        case SafetyMode::RESTRICTED:
            // 减少范围
            heightRange *= RESTRICTED_MODE_FACTOR;
            angleRange *= RESTRICTED_MODE_FACTOR;
            break;
            
        case SafetyMode::MAINTENANCE:
            // 增加范围
            heightRange *= MAINTENANCE_MODE_FACTOR;
            angleRange *= MAINTENANCE_MODE_FACTOR;
            break;
            
        case SafetyMode::NORMAL:
        default:
            // 保持不变
            break;
    }
    
    minHeight = heightCenter - heightRange / 2.0;
    maxHeight = heightCenter + heightRange / 2.0;
    minAngle = angleCenter - angleRange / 2.0;
    maxAngle = angleCenter + angleRange / 2.0;
}